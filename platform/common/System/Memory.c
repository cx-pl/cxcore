#include <limits.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../platform.h"
#include "../../../src/cxcore.h"
#include "../../../include/cxruntime.h"
#include "../cxarena.h"
#include "../cxgc.h"
#include "../../../src/cxthreading.h"

#ifdef CX_PLATFORM_WIN
#include <windows.h>
#ifdef CXCORE_ARENA_BENCHMARK
static uint64_t cx_gc_benchmark_monotonic_time_ns(void) {
    LARGE_INTEGER counter;
    LARGE_INTEGER frequency;
    uint64_t ticks;
    uint64_t rate;
    if (!QueryPerformanceCounter(&counter) ||
        !QueryPerformanceFrequency(&frequency) ||
        counter.QuadPart < 0 || frequency.QuadPart <= 0) {
        fputs("Could not read monotonic GC benchmark clock\n", stderr);
        exit(EXIT_FAILURE);
    }
    ticks = (uint64_t)counter.QuadPart;
    rate = (uint64_t)frequency.QuadPart;
    return (ticks / rate) * UINT64_C(1000000000) +
        ((ticks % rate) * UINT64_C(1000000000)) / rate;
}
#endif
static SRWLOCK cx_gc_mutex = SRWLOCK_INIT;
static CONDITION_VARIABLE cx_gc_condition = CONDITION_VARIABLE_INIT;
static void cx_gc_lock(void) { AcquireSRWLockExclusive(&cx_gc_mutex); }
static void cx_gc_unlock(void) { ReleaseSRWLockExclusive(&cx_gc_mutex); }
static void cx_gc_wake(void) { WakeAllConditionVariable(&cx_gc_condition); }
static void cx_gc_wait(void) {
    if (!SleepConditionVariableSRW(&cx_gc_condition, &cx_gc_mutex, INFINITE, 0)) {
        fputs("CX GC condition wait failed\n", stderr);
        exit(EXIT_FAILURE);
    }
}
#else
#include <pthread.h>
#ifdef CXCORE_ARENA_BENCHMARK
#include <time.h>
static uint64_t cx_gc_benchmark_monotonic_time_ns(void) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0 || now.tv_sec < 0) {
        fputs("Could not read monotonic GC benchmark clock\n", stderr);
        exit(EXIT_FAILURE);
    }
    return (uint64_t)now.tv_sec * UINT64_C(1000000000) +
        (uint64_t)now.tv_nsec;
}
#endif
static pthread_mutex_t cx_gc_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cx_gc_condition = PTHREAD_COND_INITIALIZER;
static void cx_gc_sync_check(int result) {
    if (result != 0) {
        fputs("CX GC synchronization failed\n", stderr);
        exit(EXIT_FAILURE);
    }
}
static void cx_gc_lock(void) { cx_gc_sync_check(pthread_mutex_lock(&cx_gc_mutex)); }
static void cx_gc_unlock(void) { cx_gc_sync_check(pthread_mutex_unlock(&cx_gc_mutex)); }
static void cx_gc_wake(void) { cx_gc_sync_check(pthread_cond_broadcast(&cx_gc_condition)); }
static void cx_gc_wait(void) { cx_gc_sync_check(pthread_cond_wait(&cx_gc_condition, &cx_gc_mutex)); }
#endif

#define CX_GC_INITIAL_THRESHOLD ((size_t)1024 * 1024)

struct cx_gc_allocation {
    void *address;
    size_t size;
    cx_bool marked;
    struct cx_gc_allocation *next;
};

struct cx_gc_registered_root {
    const void *address;
    size_t size;
    struct cx_gc_registered_root *next;
};

#define CX_GC_ROOT_INDEX_INITIAL_CAPACITY 32u

static struct cx_gc_registered_root **cx_gc_root_index;
static size_t cx_gc_root_index_capacity;
static size_t cx_gc_root_index_count;
static cx_bool cx_gc_root_index_failed;
#ifdef CXCORE_TESTING
static cx_bool cx_gc_test_fail_next_root_index_allocation;
#endif

struct cx_gc_root_handle {
    const void *address;
    size_t size;
    struct cx_gc_root_handle *next;
};

struct cx_gc_object_root {
    struct cx_gc_root_handle *handle;
    cx_ptr value;
};

static size_t cx_gc_heap_bytes;
static size_t cx_gc_arena_reserved_bytes;
static size_t cx_gc_process_memory_budget;
static void cx_memory_fail(const char *message);
#ifdef CXCORE_ARENA_BENCHMARK
static size_t cx_memory_benchmark_collection_count;
static size_t cx_memory_benchmark_collected_bytes;
static uint64_t cx_memory_benchmark_collection_pause_ns;

CX_API size_t cx_memory_benchmark_collection_count_get(void) {
    size_t result;
    cx_gc_lock();
    result = cx_memory_benchmark_collection_count;
    cx_gc_unlock();
    return result;
}

CX_API size_t cx_memory_benchmark_collected_bytes_get(void) {
    size_t result;
    cx_gc_lock();
    result = cx_memory_benchmark_collected_bytes;
    cx_gc_unlock();
    return result;
}

CX_API uint64_t cx_memory_benchmark_collection_pause_ns_get(void) {
    uint64_t result;
    cx_gc_lock();
    result = cx_memory_benchmark_collection_pause_ns;
    cx_gc_unlock();
    return result;
}

CX_API size_t cx_memory_benchmark_heap_bytes_get(void) {
    size_t result;
    cx_gc_lock();
    result = cx_gc_heap_bytes;
    cx_gc_unlock();
    return result;
}
#endif

CX_STRING_DEF(cx_memory_null_arena_action,
              "Memory.WithArena requires a non-null action");
CX_STRING_DEF(cx_memory_null_durable_action,
              "Memory.WithDurable requires a non-null action");
CX_STRING_DEF(cx_memory_invalid_arena_capacity,
              "Memory.WithArena capacity must be nonzero");
CX_STRING_DEF(cx_memory_arena_invalid_initial_capacity,
              "MemoryArena initial capacity must be nonzero");
CX_STRING_DEF(cx_memory_arena_invalid_capacity_range,
              "MemoryArena maximum capacity must be at least its nonzero initial capacity");
CX_STRING_DEF(cx_memory_arena_disposed,
              "MemoryArena has been disposed");
CX_STRING_DEF(cx_memory_arena_running,
              "MemoryArena is already running");
CX_STRING_DEF(cx_memory_null_instance, "MemoryArena instance is null");
CX_STRING_DEF(cx_memory_null_async_arena_action,
              "Memory.WithArenaAsync requires a non-null action");
CX_STRING_DEF(cx_memory_null_async_arena_task,
              "Memory.WithArenaAsync action returned a null task");
CX_STRING_DEF(cx_memory_async_arena_capture_failed,
              "Memory.WithArenaAsync could not retain its operation state");
CX_STRING_DEF(cx_memory_async_arena_multicast_action,
              "Memory.WithArenaAsync requires a single action");
CX_STRING_DEF(cx_memory_param_action, "action");
CX_STRING_DEF(cx_memory_param_capacity, "capacity");
CX_STRING_DEF(cx_memory_param_initial_capacity, "initialCapacity");
CX_STRING_DEF(cx_memory_param_maximum_capacity, "maximumCapacity");
CX_STRING_DEF(cx_memory_param_instance, "MemoryArena");

static void cx_memory_raise_invalid_operation(
    const struct cxcore_System_String *message) {
    cx_ptr exception = cx_exception_allocate_durable(
        cxcore_System_InvalidOperationException___typeinfo.Size);
    cxcore_System_InvalidOperationException___constructor__2(
        (struct cxcore_System_InvalidOperationException *)exception, message);
    cx_exception_throw(exception, __FILE__, __LINE__);
}

static void cx_memory_raise_argument_null(
    const struct cxcore_System_String *message,
    const struct cxcore_System_String *paramName) {
    cx_ptr exception = cx_exception_allocate_durable(
        cxcore_System_ArgumentNullException___typeinfo.Size);
    cxcore_System_ArgumentNullException___constructor__3(
        (struct cxcore_System_ArgumentNullException *)exception,
        message, paramName);
    cx_exception_throw(exception, __FILE__, __LINE__);
}

static void cx_memory_raise_argument_out_of_range(
    const struct cxcore_System_String *message,
    const struct cxcore_System_String *paramName) {
    cx_ptr exception = cx_exception_allocate_durable(
        cxcore_System_ArgumentOutOfRangeException___typeinfo.Size);
    cxcore_System_ArgumentOutOfRangeException___constructor__3(
        (struct cxcore_System_ArgumentOutOfRangeException *)exception,
        message, paramName);
    cx_exception_throw(exception, __FILE__, __LINE__);
}

static void cx_memory_raise_object_disposed(
    const struct cxcore_System_String *message) {
    cx_ptr exception = cx_exception_allocate_durable(
        cxcore_System_ObjectDisposedException___typeinfo.Size);
    cxcore_System_ObjectDisposedException___constructor__3(
        (struct cxcore_System_ObjectDisposedException *)exception,
        message, &cx_memory_param_instance);
    cx_exception_throw(exception, __FILE__, __LINE__);
}

static void cx_memory_raise_null_reference(
    const struct cxcore_System_String *message) {
    cx_ptr exception = cx_exception_allocate_durable(
        cxcore_System_NullReferenceException___typeinfo.Size);
    cxcore_System_NullReferenceException___constructor__2(
        (struct cxcore_System_NullReferenceException *)exception, message);
    cx_exception_throw(exception, __FILE__, __LINE__);
}

struct CX_FN_6FE0127D873E5D22_invocation {
    CX_FN_6FE0127D873E5D22 value;
    struct CX_FN_6FE0127D873E5D22_invocation *next;
};

struct cx_memory_async_arena_state {
    struct cxcore_System_Threading_Tasks_TaskHandle *operation;
    struct cxcore_System_Threading_Tasks_TaskHandle *completion;
    struct cx_arena *arena;
    struct cx_memory_async_arena_state *next;
    cx_bool completed;
};

static struct cx_memory_async_arena_state *cx_memory_async_arena_states;
static void cx_memory_async_arena_complete(void *environment);
static const struct cx_gc_root_region cx_memory_async_arena_roots[] = {
    {&cx_memory_async_arena_states,
     (cx_uint)sizeof(cx_memory_async_arena_states)},
};

static void cx_memory_async_arena_state_add(
    struct cx_memory_async_arena_state *state) {
    cx_gc_register_static_roots(cx_memory_async_arena_roots, 1u);
    cx_gc_runtime_lock();
    (void)cx_gc_store_reference_locked(
        (cx_ptr *)&state->next, (cx_ptr)cx_memory_async_arena_states);
    (void)cx_gc_store_reference_locked(
        (cx_ptr *)&cx_memory_async_arena_states, (cx_ptr)state);
    cx_gc_runtime_unlock();
}

static void cx_memory_async_arena_state_remove(
    struct cx_memory_async_arena_state *state) {
    struct cx_memory_async_arena_state **link;
    cx_gc_runtime_lock();
    link = &cx_memory_async_arena_states;
    while (*link != NULL && *link != state) link = &(*link)->next;
    if (*link != state) {
        cx_gc_runtime_unlock();
        fputs("CX async arena state is not rooted\n", stderr);
        exit(EXIT_FAILURE);
    }
    (void)cx_gc_store_reference_locked((cx_ptr *)link,
                                       (cx_ptr)state->next);
    (void)cx_gc_store_reference_locked((cx_ptr *)&state->next, NULL);
    (void)cx_gc_store_reference_locked((cx_ptr *)&state->operation, NULL);
    (void)cx_gc_store_reference_locked((cx_ptr *)&state->completion, NULL);
    (void)cx_gc_store_reference_locked((cx_ptr *)&state->arena, NULL);
    cx_gc_runtime_unlock();
}

static struct cxcore_System_Threading_Tasks_TaskHandle *
cx_memory_async_arena_invoke(CX_FN_6FE0127D873E5D22 action) {
    struct cxcore_System_Threading_Tasks_TaskHandle *operation = NULL;
    if (action.__invocation_list != NULL) {
        struct CX_FN_6FE0127D873E5D22_invocation *item;
        for (item = action.__invocation_list; item != NULL; item = item->next) {
            if (item->value.__function != NULL) {
                operation = item->value.__function(item->value.__environment);
            }
        }
    } else if (action.__function != NULL) {
        operation = action.__function(action.__environment);
    }
    if (operation == NULL) {
        cx_memory_raise_invalid_operation(&cx_memory_null_async_arena_task);
    }
    return operation;
}

struct cx_memory_async_arena_start_context {
    struct cx_memory_async_arena_state *state;
    CX_FN_6FE0127D873E5D22 action;
};

static void cx_memory_async_arena_start(void *context) {
    struct cx_memory_async_arena_start_context *start =
        (struct cx_memory_async_arena_start_context *)context;
    struct cxcore_System_Threading_Tasks_TaskHandle *operation;
    cx_action continuation = {0};
    cx_ptr *destinations[1];
    cx_ptr values[1];
    operation = cx_memory_async_arena_invoke(start->action);
    destinations[0] = (cx_ptr *)&start->state->operation;
    values[0] = (cx_ptr)operation;
    if (!cx_gc_store_references_retained_arena(
            destinations, values, 1u, start->state->arena)) {
        cx_memory_raise_invalid_operation(&cx_memory_async_arena_capture_failed);
    }
    continuation.__function = cx_memory_async_arena_complete;
    continuation.__environment = start->state;
    continuation.__target = (cx_ptr)start->state;
    continuation.__method = NULL;
    continuation.__invocation_list = NULL;
    cxcore_System_Threading_Tasks_TaskHandle_OnCompleted(
        operation, continuation);
}

static void cx_memory_async_arena_complete(void *environment) {
    struct cx_memory_async_arena_state *state =
        (struct cx_memory_async_arena_state *)environment;
    struct cx_exception_frame frame;
    struct cx_arena_durable_scope durable_scope;
    cx_ptr failure = NULL;
    if (state->completed) return;
    state->completed = CX_TRUE;
    cx_exception_push(&frame);
    if (setjmp(frame.environment) == 0) {
        if (cxcore_System_Threading_Tasks_TaskHandle_IsCanceled___const_get(
                state->operation)) {
            cx_arena_enter_durable(&durable_scope);
            cxcore_System_Threading_Tasks_TaskHandle_SetCanceled(
                state->completion);
            cx_arena_leave_durable(&durable_scope);
        } else {
            cxcore_System_Threading_Tasks_TaskHandle_GetResult(
                state->operation);
            cx_arena_enter_durable(&durable_scope);
            cxcore_System_Threading_Tasks_TaskHandle_SetResult(
                state->completion);
            cx_arena_leave_durable(&durable_scope);
        }
        cx_exception_pop(&frame);
    } else {
        failure = cx_exception_current();
        cx_exception_clear();
        cx_exception_pop(&frame);
        cx_arena_enter_durable(&durable_scope);
        cxcore_System_Threading_Tasks_TaskHandle_SetFault(
            state->completion,
            (struct cxcore_System_Exception *)failure);
        cx_arena_leave_durable(&durable_scope);
    }
    {
        struct cx_arena *arena = state->arena;
        cx_memory_async_arena_state_remove(state);
        cx_arena_close_logical_owner(arena);
    }
}

static struct cxcore_System_Threading_Tasks_TaskHandle *
cx_memory_run_async_arena(
    cx_uint initial_capacity, cx_uint maximum_capacity,
    cx_bool fixed_capacity, CX_FN_6FE0127D873E5D22 action) {
    struct cx_arena *arena;
    struct cx_arena_durable_scope durable_scope;
    struct cx_memory_async_arena_state *state;
    struct cx_memory_async_arena_start_context start;
    struct cxcore_System_Threading_Tasks_TaskHandle *completion;
    struct cx_exception_frame frame;
    cx_ptr failure;
    if (action.__function == NULL && action.__invocation_list == NULL) {
        cx_memory_raise_argument_null(&cx_memory_null_async_arena_action,
                                      &cx_memory_param_action);
    }
    if (action.__invocation_list != NULL &&
        action.__invocation_list->next != NULL) {
        cx_memory_raise_argument_out_of_range(
            &cx_memory_async_arena_multicast_action,
            &cx_memory_param_action);
    }
    if (fixed_capacity && maximum_capacity == 0) {
        cx_memory_raise_argument_out_of_range(
            &cx_memory_invalid_arena_capacity, &cx_memory_param_capacity);
    }
    arena = cx_arena_create((size_t)initial_capacity,
                            (size_t)maximum_capacity,
                            fixed_capacity != CX_FALSE);
    cx_arena_retain_logical(arena); // async scope owner
    cx_arena_enter_durable(&durable_scope);
    state = (struct cx_memory_async_arena_state *)cxcore_System_Memory_Alloc(
        (cx_uint)sizeof(*state));
    memset(state, 0, sizeof(*state));
    completion = (struct cxcore_System_Threading_Tasks_TaskHandle *)
        cxcore_System_Memory_Alloc((cx_uint)sizeof(*completion));
    cxcore_System_Threading_Tasks_TaskHandle___constructor__2(
        completion, CX_FALSE);
    (void)cx_gc_store_reference((cx_ptr *)&state->completion,
                                (cx_ptr)completion);
    (void)cx_gc_store_native_pointer(
        (cx_ptr *)&state->arena, (cx_ptr)arena);
    cx_arena_leave_durable(&durable_scope);
    cx_memory_async_arena_state_add(state);

    start.state = state;
    start.action = action;
    cx_exception_push(&frame);
    if (setjmp(frame.environment) == 0) {
        cx_arena_run_callback(arena, cx_memory_async_arena_start, &start);
        cx_exception_pop(&frame);
        return completion;
    }
    failure = cx_exception_current();
    cx_exception_clear();
    cx_exception_pop(&frame);
    cx_arena_enter_durable(&durable_scope);
    cxcore_System_Threading_Tasks_TaskHandle_SetFault(
        completion, (struct cxcore_System_Exception *)failure);
    cx_arena_leave_durable(&durable_scope);
    cx_memory_async_arena_state_remove(state);
    cx_arena_close_logical_owner(arena);
    return completion;
}

static void cx_memory_invoke_arena_action(void *context) {
    const cx_action *action = (const cx_action *)context;
    if (action->__invocation_list != NULL) {
        const struct CX_FN_136FFEE706DF35B2_invocation *item;
        for (item = action->__invocation_list; item != NULL; item = item->next) {
            item->value.__function(item->value.__environment);
        }
    } else {
        action->__function(action->__environment);
    }
}

static void cx_memory_run_arena(
    cx_uint initial_capacity, cx_uint maximum_capacity,
    cx_bool fixed_capacity, cx_action action) {
    struct cx_arena *arena;
    struct cx_exception_frame frame;
    if (action.__function == NULL && action.__invocation_list == NULL) {
        cx_memory_raise_argument_null(&cx_memory_null_arena_action,
                                      &cx_memory_param_action);
    }
    if (fixed_capacity && maximum_capacity == 0) {
        cx_memory_raise_argument_out_of_range(
            &cx_memory_invalid_arena_capacity, &cx_memory_param_capacity);
    }
    arena = cx_arena_create((size_t)initial_capacity,
                            (size_t)maximum_capacity,
                            fixed_capacity != CX_FALSE);
    cx_exception_push(&frame);
    if (setjmp(frame.environment) == 0) {
        cx_arena_run_callback(arena, cx_memory_invoke_arena_action, &action);
        cx_exception_pop(&frame);
        cx_arena_destroy(arena);
        return;
    }
    // The exception path has already unwound the callback scope. Keep its
    // durable exception root while releasing the arena, then continue with the
    // caller's handler and original throw location.
    cx_exception_pop(&frame);
    cx_arena_destroy(arena);
    cx_exception_rethrow();
}

void cxcore_System_Memory_WithArena(cx_action action) {
    cx_memory_run_arena(0u, 0u, CX_FALSE, action);
}

void cxcore_System_Memory_WithArena__2(cx_uint capacity, cx_action action) {
    cx_memory_run_arena(capacity, capacity, CX_TRUE, action);
}

void cxcore_System_MemoryArena___constructor(
    struct cxcore_System_MemoryArena *self, cx_uint initial_capacity) {
    struct cx_arena *arena;
    if (self == NULL) {
        cx_memory_raise_null_reference(&cx_memory_null_instance);
    }
    if (initial_capacity == 0u) {
        cx_memory_raise_argument_out_of_range(
            &cx_memory_arena_invalid_initial_capacity,
            &cx_memory_param_initial_capacity);
    }
    arena = cx_arena_create((size_t)initial_capacity, 0u, 0);
    (void)cx_gc_store_native_pointer((cx_ptr *)&self->_nativeArena,
                                     (cx_ptr)arena);
    self->_disposed = CX_FALSE;
    self->_running = CX_FALSE;
}

void cxcore_System_MemoryArena___constructor__2(
    struct cxcore_System_MemoryArena *self, cx_uint initial_capacity,
    cx_uint maximum_capacity) {
    struct cx_arena *arena;
    if (self == NULL) {
        cx_memory_raise_null_reference(&cx_memory_null_instance);
    }
    if (initial_capacity == 0u || maximum_capacity < initial_capacity) {
        cx_memory_raise_argument_out_of_range(
            &cx_memory_arena_invalid_capacity_range,
            initial_capacity == 0u ? &cx_memory_param_initial_capacity
                                   : &cx_memory_param_maximum_capacity);
    }
    arena = cx_arena_create((size_t)initial_capacity,
                            (size_t)maximum_capacity,
                            maximum_capacity == initial_capacity);
    (void)cx_gc_store_native_pointer((cx_ptr *)&self->_nativeArena,
                                     (cx_ptr)arena);
    self->_disposed = CX_FALSE;
    self->_running = CX_FALSE;
}

void cxcore_System_MemoryArena_Run(
    struct cxcore_System_MemoryArena *self, cx_action action) {
    struct cx_arena *arena;
    struct cx_exception_frame frame;
    if (action.__function == NULL && action.__invocation_list == NULL) {
        cx_memory_raise_argument_null(&cx_memory_null_arena_action,
                                      &cx_memory_param_action);
    }
    if (self == NULL) {
        cx_memory_raise_null_reference(&cx_memory_param_instance);
    }
    cx_gc_runtime_lock();
    if (self->_disposed || self->_nativeArena == NULL) {
        cx_gc_runtime_unlock();
        cx_memory_raise_object_disposed(&cx_memory_arena_disposed);
    }
    if (self->_running) {
        cx_gc_runtime_unlock();
        cx_memory_raise_invalid_operation(&cx_memory_arena_running);
    }
    self->_running = CX_TRUE;
    arena = (struct cx_arena *)self->_nativeArena;
    cx_gc_runtime_unlock();

    cx_exception_push(&frame);
    if (setjmp(frame.environment) == 0) {
        cx_arena_run_callback(arena, cx_memory_invoke_arena_action, &action);
        cx_arena_reset(arena);
        cx_gc_runtime_lock();
        self->_running = CX_FALSE;
        cx_gc_runtime_unlock();
        cx_exception_pop(&frame);
        return;
    }

    // The exception unwinder has already left the active arena scope. Retain
    // the original exception while resetting storage and releasing the run
    // lease, then propagate it to the caller.
    cx_exception_pop(&frame);
    cx_arena_reset(arena);
    cx_gc_runtime_lock();
    self->_running = CX_FALSE;
    cx_gc_runtime_unlock();
    cx_exception_rethrow();
}

void cxcore_System_MemoryArena_Dispose(
    struct cxcore_System_MemoryArena *self) {
    struct cx_arena *arena;
    if (self == NULL) return;
    cx_gc_runtime_lock();
    if (self->_running) {
        cx_gc_runtime_unlock();
        cx_memory_raise_invalid_operation(&cx_memory_arena_running);
    }
    if (self->_disposed) {
        cx_gc_runtime_unlock();
        return;
    }
    arena = (struct cx_arena *)self->_nativeArena;
    (void)cx_gc_store_native_pointer_locked(
        (cx_ptr *)&self->_nativeArena, NULL);
    self->_disposed = CX_TRUE;
    cx_gc_runtime_unlock();
    cx_arena_destroy(arena);
}

struct cxcore_System_Threading_Tasks_TaskHandle *
cxcore_System_Memory_WithArenaAsync(CX_FN_6FE0127D873E5D22 action) {
    return cx_memory_run_async_arena(0u, 0u, CX_FALSE, action);
}

struct cxcore_System_Threading_Tasks_TaskHandle *
cxcore_System_Memory_WithArenaAsync__2(
    cx_uint capacity, CX_FN_6FE0127D873E5D22 action) {
    return cx_memory_run_async_arena(capacity, capacity, CX_TRUE, action);
}

void cxcore_System_Memory_WithDurable(cx_action action) {
    struct cx_exception_frame frame;
    if (action.__function == NULL && action.__invocation_list == NULL) {
        cx_memory_raise_argument_null(&cx_memory_null_durable_action,
                                      &cx_memory_param_action);
    }
    cx_exception_push(&frame);
    if (setjmp(frame.environment) == 0) {
        struct cx_arena_durable_scope scope;
        cx_arena_enter_durable(&scope);
        cx_memory_invoke_arena_action(&action);
        cx_arena_leave_durable(&scope);
        cx_exception_pop(&frame);
        return;
    }
    // The throw path unwinds durable contexts to this frame's checkpoint.
    // Preserve the exception while the wrapper restores control to its caller.
    cx_exception_pop(&frame);
    cx_exception_rethrow();
}

struct cx_gc_scan_context {
    struct cx_gc_allocation **worklist;
    size_t worklist_capacity;
    size_t *worklist_count;
    struct cx_gc_allocation **allocation_index;
    size_t allocation_index_count;
};

struct cx_gc_native_frame {
    jmp_buf registers;
    uintptr_t stack_position;
    const void *root_address;
    size_t root_size;
    struct cx_gc_native_frame *previous;
};

struct cx_gc_thread {
    uintptr_t stack_low;
    uintptr_t stack_high;
    const volatile cx_byte *stack_marker;
    const void *registers;
    cx_ptr *exception_root;
    struct cx_runtime_once *waiting_on;
    cx_ulong id;
    cx_uint mutex_count;
    cx_bool parked;
    struct cx_gc_native_frame *native_frame;
    struct cx_gc_native_frame *pending_native_frame;
    struct cx_gc_thread *next;
};

struct cx_runtime_once_durable_context {
    struct cx_runtime_once_durable_context *previous;
    struct cx_runtime_once *once;
    struct cx_arena_durable_scope scope;
};

static struct cx_gc_thread *cx_gc_threads;
static CX_THREAD_LOCAL struct cx_gc_thread *cx_gc_current_thread;
static uintptr_t cx_gc_stack_range_low = UINTPTR_MAX;
static uintptr_t cx_gc_stack_range_high;
static CX_THREAD_LOCAL struct cx_runtime_once_durable_context *
    cx_runtime_once_durable_context_current;
static cx_bool cx_gc_started;
static cx_bool cx_gc_stop_requested;
static cx_ulong cx_gc_next_thread_id;
static void cx_gc_lock_heap(void);

static void cx_gc_recompute_stack_range_locked(void) {
    struct cx_gc_thread *thread;
    cx_gc_stack_range_low = UINTPTR_MAX;
    cx_gc_stack_range_high = 0;
    for (thread = cx_gc_threads; thread != NULL; thread = thread->next) {
        if (thread->stack_low < cx_gc_stack_range_low)
            cx_gc_stack_range_low = thread->stack_low;
        if (thread->stack_high > cx_gc_stack_range_high)
            cx_gc_stack_range_high = thread->stack_high;
    }
}

#ifdef CXCORE_TESTING
static cx_bool cx_memory_fail_next_allocation;
static cx_bool cx_memory_fail_next_record;
static size_t cx_memory_test_collection_count;
static size_t cx_memory_test_collected_bytes;
static size_t cx_memory_test_allocation_count;
static size_t cx_memory_test_allocation_bytes;
static cx_bool cx_memory_test_track_allocations;
#define CX_MEMORY_TEST_ALLOCATION_SIZE_LIMIT 128u
static size_t cx_memory_test_allocation_size_count;
static size_t cx_memory_test_allocation_sizes[CX_MEMORY_TEST_ALLOCATION_SIZE_LIMIT];
static size_t cx_memory_test_allocation_size_counts[CX_MEMORY_TEST_ALLOCATION_SIZE_LIMIT];
typedef void (*cx_memory_test_hook)(void *context);
static CX_THREAD_LOCAL cx_memory_test_hook cx_memory_test_native_pending_hook;
static CX_THREAD_LOCAL void *cx_memory_test_native_pending_context;
static cx_memory_test_hook cx_memory_test_collection_started_hook;
static void *cx_memory_test_collection_started_context;

CX_API void cx_memory_test_set_native_call_pending_hook(
    cx_memory_test_hook hook, void *context) {
    cx_memory_test_native_pending_context = context;
    cx_memory_test_native_pending_hook = hook;
}

CX_API void cx_memory_test_set_collection_started_hook(
    cx_memory_test_hook hook, void *context) {
    cx_gc_lock_heap();
    cx_memory_test_collection_started_context = context;
    cx_memory_test_collection_started_hook = hook;
    cx_gc_unlock();
}

CX_API void cx_memory_test_fail_next_allocation(void) {
    cx_gc_lock_heap();
    cx_memory_fail_next_allocation = CX_TRUE;
    cx_gc_unlock();
}

CX_API void cx_memory_test_fail_next_record_allocation(void) {
    cx_gc_lock_heap();
    cx_memory_fail_next_record = CX_TRUE;
    cx_gc_unlock();
}

CX_API void cx_memory_test_fail_next_static_root_index_allocation(void) {
    cx_gc_lock_heap();
    cx_gc_test_fail_next_root_index_allocation = CX_TRUE;
    cx_gc_unlock();
}

CX_API void cx_memory_test_reset_static_root_index(void) {
    cx_gc_lock_heap();
    free(cx_gc_root_index);
    cx_gc_root_index = NULL;
    cx_gc_root_index_capacity = 0;
    cx_gc_root_index_count = 0;
    cx_gc_root_index_failed = CX_FALSE;
    cx_gc_test_fail_next_root_index_allocation = CX_FALSE;
    cx_gc_unlock();
}

CX_API cx_bool cx_memory_test_static_root_index_is_disabled(void) {
    cx_bool result;
    cx_gc_lock_heap();
    result = cx_gc_root_index_failed;
    cx_gc_unlock();
    return result;
}

CX_API size_t cx_memory_test_collection_count_get(void) {
    size_t result;
    cx_gc_lock_heap();
    result = cx_memory_test_collection_count;
    cx_gc_unlock();
    return result;
}

CX_API size_t cx_memory_test_collected_bytes_get(void) {
    size_t result;
    cx_gc_lock_heap();
    result = cx_memory_test_collected_bytes;
    cx_gc_unlock();
    return result;
}

CX_API size_t cx_memory_test_heap_bytes_get(void) {
    size_t result;
    cx_gc_lock_heap();
    result = cx_gc_heap_bytes;
    cx_gc_unlock();
    return result;
}

CX_API size_t cx_memory_test_allocation_count_get(void) {
    size_t result;
    cx_gc_lock_heap();
    result = cx_memory_test_allocation_count;
    cx_gc_unlock();
    return result;
}

CX_API size_t cx_memory_test_allocation_bytes_get(void) {
    size_t result;
    cx_gc_lock_heap();
    result = cx_memory_test_allocation_bytes;
    cx_gc_unlock();
    return result;
}

CX_API void cx_memory_test_allocation_tracking_set(cx_bool enabled) {
    cx_gc_lock_heap();
    cx_memory_test_track_allocations = enabled;
    cx_gc_unlock();
}

CX_API void cx_memory_test_allocation_sizes_reset(void) {
    cx_gc_lock_heap();
    cx_memory_test_allocation_size_count = 0;
    memset(cx_memory_test_allocation_sizes, 0, sizeof(cx_memory_test_allocation_sizes));
    memset(cx_memory_test_allocation_size_counts, 0, sizeof(cx_memory_test_allocation_size_counts));
    cx_gc_unlock();
}

CX_API size_t cx_memory_test_allocation_size_entry_count_get(void) {
    size_t result;
    cx_gc_lock_heap();
    result = cx_memory_test_allocation_size_count;
    cx_gc_unlock();
    return result;
}

CX_API size_t cx_memory_test_allocation_size_entry_get(
    size_t index, size_t *size) {
    size_t result = 0;
    cx_gc_lock_heap();
    if (index < cx_memory_test_allocation_size_count) {
        *size = cx_memory_test_allocation_sizes[index];
        result = cx_memory_test_allocation_size_counts[index];
    }
    cx_gc_unlock();
    return result;
}

CX_API cx_uint cx_runtime_once_test_waiter_count(struct cx_runtime_once *once) {
    struct cx_gc_thread *thread;
    cx_uint count = 0;
    cx_gc_lock_heap();
    for (thread = cx_gc_threads; thread != NULL; thread = thread->next) {
        if (thread->waiting_on == once) ++count;
    }
    cx_gc_unlock();
    return count;
}
#endif

static struct cx_gc_allocation *cx_gc_allocations;
static struct cx_gc_registered_root *cx_gc_roots;
static struct cx_gc_root_handle *cx_gc_scoped_roots;
static size_t cx_gc_next_collection = CX_GC_INITIAL_THRESHOLD;

static size_t cx_gc_root_hash(const void *address, size_t size,
                              size_t capacity) {
    uintptr_t hash = (uintptr_t)address;
    hash ^= (uintptr_t)size + (uintptr_t)0x9e3779b9u +
        (hash << 6) + (hash >> 2);
    hash ^= hash >> (sizeof(hash) * 4u);
    hash *= (uintptr_t)0x85ebca6bu;
    hash ^= hash >> (sizeof(hash) * 4u);
    return (size_t)(hash & (uintptr_t)(capacity - 1u));
}

static struct cx_gc_registered_root **cx_gc_root_index_find_slot(
    struct cx_gc_registered_root **index, size_t capacity,
    const void *address, size_t size) {
    size_t hash = cx_gc_root_hash(address, size, capacity);
    size_t mask = capacity - 1u;
    size_t slot = hash & mask;
    size_t step = ((hash >> (sizeof(hash) * 4u)) ^ hash) & mask;
    step |= 1u;
    while (index[slot] != NULL &&
           (index[slot]->address != address || index[slot]->size != size)) {
        slot = (slot + step) & mask;
    }
    return &index[slot];
}

static cx_bool cx_gc_root_index_rebuild(size_t minimum_capacity) {
    struct cx_gc_registered_root **index;
    struct cx_gc_registered_root *root;
    size_t root_count = 0;
    size_t capacity = minimum_capacity < CX_GC_ROOT_INDEX_INITIAL_CAPACITY
        ? CX_GC_ROOT_INDEX_INITIAL_CAPACITY : minimum_capacity;
    for (root = cx_gc_roots; root != NULL; root = root->next) {
        if (root_count == SIZE_MAX) return CX_FALSE;
        ++root_count;
    }
    while (root_count > capacity - capacity / 4u) {
        if (capacity > SIZE_MAX / 2u) return CX_FALSE;
        capacity *= 2u;
    }
    if (capacity > SIZE_MAX / sizeof(*index)) return CX_FALSE;
#ifdef CXCORE_TESTING
    if (cx_gc_test_fail_next_root_index_allocation) {
        cx_gc_test_fail_next_root_index_allocation = CX_FALSE;
        return CX_FALSE;
    }
#endif
    index = (struct cx_gc_registered_root **)calloc(capacity, sizeof(*index));
    if (index == NULL) return CX_FALSE;
    for (root = cx_gc_roots; root != NULL; root = root->next) {
        struct cx_gc_registered_root **slot = cx_gc_root_index_find_slot(
            index, capacity, root->address, root->size);
        *slot = root;
    }
    free(cx_gc_root_index);
    cx_gc_root_index = index;
    cx_gc_root_index_capacity = capacity;
    cx_gc_root_index_count = root_count;
    return CX_TRUE;
}

static struct cx_gc_registered_root *cx_gc_find_registered_root(
    const void *address, size_t size) {
    struct cx_gc_registered_root *root;
    if (cx_gc_root_index != NULL) {
        struct cx_gc_registered_root *const *slot =
            (struct cx_gc_registered_root *const *)cx_gc_root_index_find_slot(
                cx_gc_root_index, cx_gc_root_index_capacity, address, size);
        return *slot;
    }
    for (root = cx_gc_roots; root != NULL; root = root->next) {
        if (root->address == address && root->size == size) return root;
    }
    return NULL;
}

static void cx_gc_index_registered_root(
    struct cx_gc_registered_root *root) {
    if (cx_gc_root_index_failed) return;
    if (cx_gc_root_index == NULL) {
        if (!cx_gc_root_index_rebuild(CX_GC_ROOT_INDEX_INITIAL_CAPACITY)) {
            cx_gc_root_index_failed = CX_TRUE;
        }
        return;
    }
    if (cx_gc_root_index_count + 1u >
        cx_gc_root_index_capacity - cx_gc_root_index_capacity / 4u) {
        if (cx_gc_root_index_capacity <= SIZE_MAX / 2u &&
            cx_gc_root_index_rebuild(cx_gc_root_index_capacity * 2u)) {
            return;
        }
        if (cx_gc_root_index_count >= cx_gc_root_index_capacity - 1u) {
            free(cx_gc_root_index);
            cx_gc_root_index = NULL;
            cx_gc_root_index_capacity = 0;
            cx_gc_root_index_count = 0;
            cx_gc_root_index_failed = CX_TRUE;
            return;
        }
    }
    {
        struct cx_gc_registered_root **slot = cx_gc_root_index_find_slot(
            cx_gc_root_index, cx_gc_root_index_capacity,
            root->address, root->size);
        *slot = root;
        ++cx_gc_root_index_count;
    }
}

static cx_bool cx_memory_should_fail_allocation(void) {
#ifdef CXCORE_TESTING
    if (cx_memory_fail_next_allocation) {
        cx_memory_fail_next_allocation = CX_FALSE;
        return CX_TRUE;
    }
#endif
    return CX_FALSE;
}

static void cx_memory_fail(const char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(EXIT_FAILURE);
}

static void cx_memory_out_of_memory(void) {
    cx_memory_fail("Out of memory");
}

// Called with the heap mutex held. Attachment cannot expose a partial snapshot.
static void cx_gc_attach_locked(void) {
    struct cx_gc_thread *thread;
    uintptr_t low;
    uintptr_t high;
    uintptr_t stack_position = cx_platform_current_stack_position();
    if (!cx_platform_current_stack_bounds(&low, &high) ||
        stack_position < low || stack_position >= high) {
        cx_memory_fail("CX GC could not establish thread stack bounds");
    }
    thread = (struct cx_gc_thread *)calloc(1, sizeof(*thread));
    if (thread == NULL) {
        cx_memory_out_of_memory();
    }
    thread->stack_low = low;
    thread->stack_high = high;
    thread->exception_root = cx_exception_thread_root();
    if (++cx_gc_next_thread_id == 0) {
        cx_memory_fail("CX thread identity space exhausted");
    }
    thread->id = cx_gc_next_thread_id;
    thread->next = cx_gc_threads;
    cx_gc_threads = thread;
    if (low < cx_gc_stack_range_low) cx_gc_stack_range_low = low;
    if (high > cx_gc_stack_range_high) cx_gc_stack_range_high = high;
    cx_gc_current_thread = thread;
    cx_gc_started = CX_TRUE;
}

// Capture below the complete active frame, including saved nonvolatile registers.
// A local-variable address can omit caller roots saved by an optimized prologue.
static void cx_gc_lock_heap(void) {
    uintptr_t stack_position = cx_platform_current_stack_position();
    jmp_buf registers;
    struct cx_gc_thread *thread;
    cx_gc_lock();
    if (cx_gc_current_thread == NULL) {
        if (cx_gc_started) {
            cx_memory_fail("CX managed entry requires cx_gc_attach_thread on this thread");
        }
        cx_gc_attach_locked();
    }
    thread = cx_gc_current_thread;
    if (thread->parked) {
        cx_memory_fail("CX managed entry is forbidden inside a blocking callback");
    }
    if (cx_gc_stop_requested) {
        (void)setjmp(registers);
        thread->registers = registers;
        thread->stack_marker = (const volatile cx_byte *)stack_position;
        thread->parked = CX_TRUE;
        cx_gc_wake();
        while (cx_gc_stop_requested) {
            cx_gc_wait();
        }
        thread->parked = CX_FALSE;
        thread->stack_marker = NULL;
        thread->registers = NULL;
    }
}

void cx_gc_attach_thread(void) {
    if (cx_gc_current_thread != NULL) {
        cx_gc_safepoint();
        return;
    }
    cx_gc_lock();
    while (cx_gc_stop_requested) {
        cx_gc_wait();
    }
    cx_gc_attach_locked();
    cx_gc_unlock();
}

void cx_gc_runtime_detach(void) {
    struct cx_gc_thread **link;
    cx_bool recompute_stack_range;
    if (cx_gc_current_thread == NULL) {
        cx_memory_fail("CX GC detach requires an attached thread");
    }
    if (cx_gc_current_thread->mutex_count != 0) {
        cx_memory_fail("CX thread cannot exit while owning a mutex");
    }
    if (!cx_exception_thread_detach()) {
        cx_memory_fail("CX GC cannot detach a thread with active exception handlers");
    }
    recompute_stack_range = cx_gc_current_thread->stack_low == cx_gc_stack_range_low ||
        cx_gc_current_thread->stack_high == cx_gc_stack_range_high;
    for (link = &cx_gc_threads; *link != cx_gc_current_thread; link = &(*link)->next) { }
    *link = cx_gc_current_thread->next;
    free(cx_gc_current_thread);
    cx_gc_current_thread = NULL;
    if (recompute_stack_range) cx_gc_recompute_stack_range_locked();
    cx_gc_wake();
}

void cx_gc_detach_thread(void) {
    if (cx_gc_current_thread == NULL) {
        cx_memory_fail("CX GC detach requires an attached thread");
    }
    cx_gc_lock_heap();
    cx_gc_runtime_detach();
    cx_gc_unlock();
}

cx_bool cx_gc_enter_thread(void) {
    if (cx_gc_current_thread == NULL) {
        cx_gc_attach_thread();
        return CX_TRUE;
    }
    cx_gc_safepoint();
    return CX_FALSE;
}

void cx_gc_leave_thread(cx_bool attachedHere) {
    if (attachedHere) cx_gc_detach_thread();
    else cx_gc_safepoint();
}

struct cx_gc_root_handle *cx_gc_add_root(const void *address, cx_uint size) {
    struct cx_gc_root_handle *handle;
    cx_gc_lock_heap();
    if (address == NULL || size == 0) {
        cx_memory_fail("CX root handle requires a valid stable region");
    }
    handle = (struct cx_gc_root_handle *)malloc(sizeof(*handle));
    if (handle == NULL) cx_memory_out_of_memory();
    handle->address = address;
    handle->size = size;
    handle->next = cx_gc_scoped_roots;
    cx_gc_scoped_roots = handle;
    cx_gc_unlock();
    return handle;
}

void cx_gc_remove_root(struct cx_gc_root_handle *handle) {
    struct cx_gc_root_handle **link;
    cx_gc_lock_heap();
    for (link = &cx_gc_scoped_roots; *link != NULL; link = &(*link)->next) {
        if (*link == handle) {
            *link = handle->next;
            free(handle);
            cx_gc_unlock();
            return;
        }
    }
    cx_memory_fail("CX root handle is not registered");
}

void cx_gc_safepoint(void) {
    cx_gc_lock_heap();
    cx_gc_unlock();
}

void cx_gc_check_thread(void) {
    cx_gc_safepoint();
}

void cx_gc_runtime_lock(void) { cx_gc_lock_heap(); }
void cx_gc_native_lock(void) { cx_gc_lock(); }
void cx_gc_native_unlock(void) { cx_gc_unlock(); }
void cx_gc_native_wake(void) { cx_gc_wake(); }
void cx_gc_runtime_unlock(void) { cx_gc_unlock(); }
static cx_bool cx_gc_memory_budget_fits(size_t heap_bytes,
                                       size_t arena_bytes) {
    size_t total;
    if (arena_bytes > SIZE_MAX - heap_bytes) return CX_FALSE;
    total = heap_bytes + arena_bytes;
    return cx_gc_process_memory_budget == 0 ||
        total <= cx_gc_process_memory_budget;
}

cx_bool cx_gc_arena_reserve_locked(size_t bytes) {
    if (bytes > SIZE_MAX - cx_gc_arena_reserved_bytes ||
        !cx_gc_memory_budget_fits(
            cx_gc_heap_bytes, cx_gc_arena_reserved_bytes + bytes)) {
        return CX_FALSE;
    }
    cx_gc_arena_reserved_bytes += bytes;
    return CX_TRUE;
}

void cx_gc_arena_release_locked(size_t bytes) {
    if (bytes > cx_gc_arena_reserved_bytes) {
        cx_memory_fail("CX arena reservation accounting underflow");
    }
    cx_gc_arena_reserved_bytes -= bytes;
}
void cx_gc_runtime_wake(void) { cx_gc_wake(); }
cx_ulong cx_gc_runtime_thread_id(void) { return cx_gc_current_thread->id; }
void cx_gc_runtime_mutex_acquired(void) {
    if (cx_gc_current_thread->mutex_count == UINT_MAX) {
        cx_memory_fail("CX mutex ownership count overflow");
    }
    ++cx_gc_current_thread->mutex_count;
}
void cx_gc_runtime_mutex_released(void) { --cx_gc_current_thread->mutex_count; }

void cx_gc_runtime_wait(void) {
    uintptr_t stack_position = cx_platform_current_stack_position();
    jmp_buf registers;
    struct cx_gc_thread *thread = cx_gc_current_thread;
    (void)setjmp(registers);
    thread->registers = registers;
    thread->stack_marker = (const volatile cx_byte *)stack_position;
    thread->parked = CX_TRUE;
    if (cx_gc_stop_requested) cx_gc_wake();
    do {
        cx_gc_wait();
    } while (cx_gc_stop_requested);
    thread->parked = CX_FALSE;
    thread->stack_marker = NULL;
    thread->registers = NULL;
}

void cx_gc_run_blocking(cx_gc_blocking_callback callback, void *context) {
    uintptr_t stack_position = cx_platform_current_stack_position();
    jmp_buf registers;
    struct cx_gc_thread *thread;
    if (callback == NULL) {
        cx_memory_fail("CX GC blocking callback must not be null");
    }
    cx_gc_lock_heap();
    thread = cx_gc_current_thread;
    (void)setjmp(registers);
    thread->registers = registers;
    thread->stack_marker = (const volatile cx_byte *)stack_position;
    thread->parked = CX_TRUE;
    cx_gc_wake();
    cx_gc_unlock();
    callback(context);
    cx_gc_lock();
    while (cx_gc_stop_requested) {
        cx_gc_wait();
    }
    thread->parked = CX_FALSE;
    thread->stack_marker = NULL;
    thread->registers = NULL;
    cx_gc_unlock();
}

static void cx_gc_run_native_call_core(cx_native_call_callback callback,
                                      void *context,
                                      const void *root_address,
                                      size_t root_size) {
    struct cx_gc_native_frame frame;
    struct cx_gc_thread *thread;
    if (callback == NULL) cx_memory_fail("Native call adapter must not be null");
    if ((root_address == NULL) != (root_size == 0) ||
        (root_address != NULL &&
         (uintptr_t)root_address > UINTPTR_MAX - root_size)) {
        cx_memory_fail("Native call root range is invalid");
    }
    if (cx_gc_current_thread == NULL) cx_gc_check_thread();
    thread = cx_gc_current_thread;
    frame.root_address = root_address;
    frame.root_size = root_size;
    // Publish the root range before entering the lock/safepoint path. If a
    // collection is already in progress, the collector scans this pending
    // frame after the thread parks, before the active native frame is linked.
    thread->pending_native_frame = &frame;
#ifdef CXCORE_TESTING
    if (cx_memory_test_native_pending_hook != NULL) {
        cx_memory_test_native_pending_hook(
            cx_memory_test_native_pending_context);
    }
#endif
    cx_gc_lock_heap();
    thread = cx_gc_current_thread;
    frame.previous = thread->native_frame;
    frame.stack_position = cx_platform_current_stack_position();
    (void)setjmp(frame.registers);
    thread->native_frame = &frame;
    thread->pending_native_frame = NULL;
    thread->registers = frame.registers;
    thread->stack_marker = (const volatile cx_byte *)frame.stack_position;
    thread->parked = CX_TRUE;
    cx_gc_wake();
    cx_gc_unlock();
    callback(context);
    cx_gc_lock();
    while (cx_gc_stop_requested) cx_gc_wait();
    thread->native_frame = frame.previous;
    thread->registers = NULL;
    thread->stack_marker = NULL;
    thread->parked = CX_FALSE;
    cx_gc_unlock();
}

void cx_gc_run_native_call(cx_native_call_callback callback, void *context) {
    cx_gc_run_native_call_core(callback, context, NULL, 0);
}

void cx_gc_run_native_call_with_roots(cx_native_call_callback callback,
                                      void *context,
                                      const void *root_address,
                                      size_t root_size) {
    cx_gc_run_native_call_core(callback, context, root_address, root_size);
}

cx_bool cx_gc_resume_native_callback(void) {
    struct cx_gc_thread *thread;
    cx_gc_lock();
    thread = cx_gc_current_thread;
    if (thread == NULL || !thread->parked || thread->native_frame == NULL) {
        cx_gc_unlock();
        return CX_FALSE;
    }
    while (cx_gc_stop_requested) cx_gc_wait();
    thread->parked = CX_FALSE;
    thread->registers = NULL;
    thread->stack_marker = NULL;
    cx_gc_unlock();
    return CX_TRUE;
}

void cx_gc_park_native_callback(void) {
    struct cx_gc_thread *thread;
    cx_gc_lock_heap();
    thread = cx_gc_current_thread;
    if (thread->native_frame == NULL) cx_memory_fail("Callback has no native frame to resume");
    thread->registers = thread->native_frame->registers;
    thread->stack_marker = (const volatile cx_byte *)thread->native_frame->stack_position;
    thread->parked = CX_TRUE;
    cx_gc_wake();
    cx_gc_unlock();
}

cx_bool cx_runtime_once_begin(struct cx_runtime_once *once) {
    uintptr_t stack_position = cx_platform_current_stack_position();
    jmp_buf registers;
    struct cx_gc_thread *thread;
    struct cx_runtime_once_durable_context *durable_context;
    cx_ptr failure;
    struct cx_gc_root_region root;
    if (once == NULL) {
        cx_memory_fail("CX initialization gate must not be null");
    }
    root.address = &once->failure;
    root.size = (cx_uint)sizeof(once->failure);
    cx_gc_register_static_roots(&root, 1);
    cx_gc_lock_heap();
    thread = cx_gc_current_thread;
    if (once->state == 1 && once->owner != thread) {
        (void)setjmp(registers);
        thread->registers = registers;
        thread->stack_marker = (const volatile cx_byte *)stack_position;
        thread->parked = CX_TRUE;
        thread->waiting_on = once;
        cx_gc_wake();
        while (once->state == 1 || cx_gc_stop_requested) {
            cx_gc_wait();
        }
        thread->parked = CX_FALSE;
        thread->waiting_on = NULL;
        thread->stack_marker = NULL;
        thread->registers = NULL;
    }
    if (once->state == 0) {
        once->state = 1;
        once->owner = thread;
        cx_gc_unlock();
        durable_context = (struct cx_runtime_once_durable_context *)
            malloc(sizeof(*durable_context));
        if (durable_context == NULL) cx_memory_out_of_memory();
        durable_context->previous = cx_runtime_once_durable_context_current;
        durable_context->once = once;
        cx_arena_enter_durable(&durable_context->scope);
        cx_runtime_once_durable_context_current = durable_context;
        return CX_TRUE;
    }
    failure = once->failure;
    cx_gc_unlock();
    if (failure != NULL) {
        cx_exception_throw(failure, "<module initialization>", 0);
    }
    return CX_FALSE;
}

static void cx_runtime_once_finish(struct cx_runtime_once *once, cx_ptr failure) {
    struct cx_runtime_once_durable_context *durable_context;
    cx_gc_lock_heap();
    if (once == NULL || once->state != 1 || once->owner != cx_gc_current_thread) {
        cx_memory_fail("CX initialization gate must be completed by its owner");
    }
    durable_context = cx_runtime_once_durable_context_current;
    if (durable_context == NULL || durable_context->once != once) {
        cx_memory_fail("CX initialization durable contexts must finish in LIFO order");
    }
    cx_gc_unlock();
    cx_arena_leave_durable(&durable_context->scope);
    cx_runtime_once_durable_context_current = durable_context->previous;
    free(durable_context);

    cx_gc_lock_heap();
    if (once->state != 1 || once->owner != cx_gc_current_thread) {
        cx_memory_fail("CX initialization gate changed while leaving its durable context");
    }
    (void)cx_gc_store_reference_locked(&once->failure, failure);
    once->state = failure == NULL ? 2 : 3;
    once->owner = NULL;
    cx_gc_wake();
    cx_gc_unlock();
}

void cx_runtime_once_complete(struct cx_runtime_once *once) {
    cx_runtime_once_finish(once, NULL);
}

void cx_runtime_once_fail(struct cx_runtime_once *once, cx_ptr failure) {
    if (failure == NULL) {
        cx_memory_fail("CX failed initialization requires an exception");
    }
    cx_runtime_once_finish(once, failure);
}

static struct cx_gc_allocation *cx_gc_find_allocation(const void *candidate) {
    uintptr_t address = (uintptr_t)candidate;
    struct cx_gc_allocation *allocation;
    for (allocation = cx_gc_allocations; allocation != NULL; allocation = allocation->next) {
        uintptr_t begin = (uintptr_t)allocation->address;
        if (address >= begin && address - begin < allocation->size) {
            return allocation;
        }
    }
    return NULL;
}

static int cx_gc_compare_allocation_address(const void *left, const void *right) {
    const struct cx_gc_allocation *left_allocation =
        *(const struct cx_gc_allocation *const *)left;
    const struct cx_gc_allocation *right_allocation =
        *(const struct cx_gc_allocation *const *)right;
    uintptr_t left_address = (uintptr_t)left_allocation->address;
    uintptr_t right_address = (uintptr_t)right_allocation->address;
    return left_address < right_address ? -1 :
        left_address > right_address ? 1 : 0;
}

static struct cx_gc_allocation *cx_gc_find_allocation_index(
    const void *candidate, struct cx_gc_allocation *const *index,
    size_t index_count) {
    uintptr_t address = (uintptr_t)candidate;
    size_t low = 0;
    size_t high = index_count;
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        struct cx_gc_allocation *allocation = index[middle];
        uintptr_t begin = (uintptr_t)allocation->address;
        if (address < begin) {
            high = middle;
        } else if (address - begin < allocation->size) {
            return allocation;
        } else {
            low = middle + 1;
        }
    }
    return NULL;
}

static cx_bool cx_gc_is_current_stack_slot(const void *address, size_t size);

static cx_bool cx_gc_reference_range_fits_managed_allocation(
    const void *destination, size_t range_size) {
    uintptr_t address = (uintptr_t)destination;
    uintptr_t end;
    struct cx_arena *destination_arena;
    struct cx_gc_allocation *allocation;
    if (range_size > UINTPTR_MAX - address) return CX_FALSE;
    if (range_size == 0) return CX_TRUE;
    // Current-thread stack ranges cannot overlap GC allocations. Most local
    // reference stores take this path; managed, static, and foreign-stack
    // destinations still receive the complete overlap check below.
    if (cx_gc_is_current_stack_slot(destination, range_size)) return CX_TRUE;
    // Arena allocations are tracked separately from GC allocations. Validate
    // their complete destination extent here, then avoid scanning the GC heap
    // list for an address that cannot overlap one of its allocations.
    if (cx_arena_find_store_range_owner_locked(
            destination, range_size, &destination_arena) &&
        destination_arena != NULL) {
        return CX_TRUE;
    }
    end = address + range_size;
    for (allocation = cx_gc_allocations; allocation != NULL;
         allocation = allocation->next) {
        uintptr_t allocation_begin = (uintptr_t)allocation->address;
        uintptr_t allocation_end;
        if (allocation->size > UINTPTR_MAX - allocation_begin) return CX_FALSE;
        allocation_end = allocation_begin + allocation->size;
        if (address < allocation_end && allocation_begin < end) {
            // Any part of a store that overlaps a managed block must remain
            // entirely inside that one allocation. Checking only the starting
            // byte misses forged destinations immediately before a block.
            return address >= allocation_begin && end <= allocation_end
                ? CX_TRUE : CX_FALSE;
        }
    }
    return CX_TRUE;
}

static cx_bool cx_gc_reference_range_is_managed_allocation(
    const void *destination, size_t range_size) {
    uintptr_t address = (uintptr_t)destination;
    uintptr_t end;
    struct cx_gc_allocation *allocation;
    if (range_size > UINTPTR_MAX - address) return CX_FALSE;
    end = address + range_size;
    for (allocation = cx_gc_allocations; allocation != NULL;
         allocation = allocation->next) {
        uintptr_t begin = (uintptr_t)allocation->address;
        uintptr_t allocation_end;
        if (allocation->size > UINTPTR_MAX - begin) return CX_FALSE;
        allocation_end = begin + allocation->size;
        if (address >= begin && end <= allocation_end) return CX_TRUE;
    }
    return CX_FALSE;
}

static cx_bool cx_gc_reference_range_is_registered_root(
    const void *destination, size_t range_size) {
    uintptr_t address = (uintptr_t)destination;
    uintptr_t end;
    struct cx_gc_root_handle *root;
    if (range_size > UINTPTR_MAX - address) return CX_FALSE;
    end = address + range_size;
    for (root = cx_gc_scoped_roots; root != NULL; root = root->next) {
        uintptr_t begin = (uintptr_t)root->address;
        uintptr_t root_end;
        if (root->size > UINTPTR_MAX - begin) continue;
        root_end = begin + root->size;
        if (address >= begin && end <= root_end) return CX_TRUE;
    }
    return CX_FALSE;
}

static cx_bool cx_gc_is_current_stack_slot(const void *address, size_t size) {
    uintptr_t slot = (uintptr_t)address;
    uintptr_t low = cx_gc_current_thread->stack_low;
    uintptr_t high = cx_gc_current_thread->stack_high;
    return slot >= low && slot < high && size <= high - slot
        ? CX_TRUE : CX_FALSE;
}

// Callers hold the GC mutex, so stack ranges cannot disappear while a store
// is checked. References into any attached thread's stack have frame lifetimes
// and must never be retained by managed storage.
static cx_bool cx_gc_is_any_thread_stack_slot_locked(const void *address,
                                                    size_t size) {
    uintptr_t slot = (uintptr_t)address;
    struct cx_gc_thread *thread;
    for (thread = cx_gc_threads; thread != NULL; thread = thread->next) {
        if (slot >= thread->stack_low && slot < thread->stack_high &&
            size <= thread->stack_high - slot) {
            return CX_TRUE;
        }
    }
    return CX_FALSE;
}

static cx_ptr cx_gc_store_value_locked(cx_ptr *destination, cx_ptr value) {
    cx_bool stack_destination;
    if (destination == NULL) {
        cx_memory_fail("CX reference store requires a destination");
    }
    stack_destination = cx_gc_is_current_stack_slot(
        destination, sizeof(*destination));
    if (!cx_gc_reference_range_fits_managed_allocation(
            destination, sizeof(*destination))) {
        cx_memory_fail("CX managed reference store exceeds its allocation");
    }
    if (!stack_destination &&
        cx_gc_is_any_thread_stack_slot_locked(value, 1u)) {
        cx_memory_fail("CX stack reference cannot be stored in managed storage");
    }
    if (!cx_arena_reference_store_allowed_locked(
            destination, sizeof(*destination), value, stack_destination)) {
        cx_memory_fail("CX scoped reference cannot escape its arena");
    }
    *destination = value;
    return value;
}

static cx_ptr cx_gc_store_value(cx_ptr *destination, cx_ptr value) {
    cx_ptr result;
    if (destination == NULL) {
        cx_memory_fail("CX reference store requires a destination");
    }
    cx_gc_check_thread();
    cx_gc_lock_heap();
    result = cx_gc_store_value_locked(destination, value);
    cx_gc_unlock();
    return result;
}

cx_ptr cx_gc_store_reference_locked(cx_ptr *destination, cx_ptr value) {
    return cx_gc_store_value_locked(destination, value);
}

cx_ptr cx_gc_store_native_pointer_locked(cx_ptr *destination, cx_ptr value) {
    return cx_gc_store_value_locked(destination, value);
}

#ifdef CXCORE_TESTING
CX_API cx_ptr cx_gc_test_store_reference_locked(
    cx_ptr *destination, cx_ptr value) {
    cx_ptr result;
    cx_gc_runtime_lock();
    result = cx_gc_store_reference_locked(destination, value);
    cx_gc_runtime_unlock();
    return result;
}
#endif

cx_ptr cx_gc_store_reference(cx_ptr *destination, cx_ptr value) {
    return cx_gc_store_value(destination, value);
}

cx_ptr cx_gc_copy_native_value(void *destination, const void *source,
                              size_t size, const size_t *pointer_offsets,
                              size_t pointer_count) {
    size_t index;
    cx_bool destination_is_stack;
    if (destination == NULL || source == NULL ||
        (pointer_count != 0 && pointer_offsets == NULL)) {
        cx_memory_fail("CX native value copy requires valid storage and pointer metadata");
    }
    if (size > UINTPTR_MAX - (uintptr_t)destination ||
        size > UINTPTR_MAX - (uintptr_t)source) {
        cx_memory_fail("CX native value copy range overflows address space");
    }
    cx_gc_check_thread();
    cx_gc_lock_heap();
    destination_is_stack = cx_gc_is_current_stack_slot(destination, size);
    if (!cx_gc_reference_range_fits_managed_allocation(destination, size)) {
        cx_gc_unlock();
        cx_memory_fail("CX native value copy exceeds its managed allocation");
    }
    for (index = 0; index < pointer_count; ++index) {
        cx_ptr value;
        cx_ptr *destination_field;
        if (pointer_offsets[index] > size ||
            sizeof(value) > size - pointer_offsets[index]) {
            cx_gc_unlock();
            cx_memory_fail("CX native value pointer metadata exceeds its layout");
        }
        memcpy(&value, (const cx_byte *)source + pointer_offsets[index],
               sizeof(value));
        destination_field = (cx_ptr *)((cx_byte *)destination +
                                        pointer_offsets[index]);
        if ((!destination_is_stack &&
             cx_gc_is_any_thread_stack_slot_locked(value, 1u)) ||
            !cx_arena_reference_store_allowed_locked(
                destination_field, sizeof(*destination_field), value,
                destination_is_stack)) {
            cx_gc_unlock();
            cx_memory_fail("CX scoped or stack reference cannot be copied into this native value destination");
        }
    }
    memmove(destination, source, size);
    cx_gc_unlock();
    return destination;
}

cx_ptr cx_gc_copy_native_value_unknown_layout(void *destination,
                                             const void *source,
                                             size_t size) {
    size_t offset;
    if (destination == NULL || source == NULL) {
        cx_memory_fail("CX native value copy requires valid storage");
    }
    if (size > UINTPTR_MAX - (uintptr_t)destination ||
        size > UINTPTR_MAX - (uintptr_t)source) {
        cx_memory_fail("CX native value copy range overflows address space");
    }
    cx_gc_check_thread();
    cx_gc_lock_heap();
    if (!cx_gc_is_current_stack_slot(destination, size)) {
        cx_gc_unlock();
        cx_memory_fail("CX native value with an unknown pointer layout cannot be copied outside the current stack");
    }
    if (size >= sizeof(cx_ptr)) {
        size_t candidate_count = size - sizeof(cx_ptr) + 1u;
        for (offset = 0; offset < candidate_count; ++offset) {
            cx_ptr value;
            const void *destination_slot =
                (const cx_byte *)destination + offset;
            memcpy(&value, (const cx_byte *)source + offset, sizeof(value));
            if (value == NULL) continue;
            if ((cx_gc_is_any_thread_stack_slot_locked(value, 1u) &&
                 !cx_gc_is_current_stack_slot(value, 1u)) ||
                !cx_arena_reference_store_allowed_locked(
                    destination_slot, sizeof(value), value, CX_TRUE)) {
                cx_gc_unlock();
                cx_memory_fail("CX native value with an unknown pointer layout contains a reference that cannot be stored in this stack frame");
            }
        }
    }
    memmove(destination, source, size);
    cx_gc_unlock();
    return destination;
}

cx_bool cx_gc_store_references_retained_arena(
    cx_ptr *const *destinations, const cx_ptr *values, size_t count,
    struct cx_arena *retained_arena) {
    size_t index;
    if ((count != 0 && (destinations == NULL || values == NULL)) ||
        retained_arena == NULL) {
        return CX_FALSE;
    }
    cx_gc_check_thread();
    cx_gc_lock_heap();
    for (index = 0; index < count; ++index) {
        cx_ptr *destination = destinations[index];
        if (destination == NULL ||
            !cx_gc_reference_range_fits_managed_allocation(
                destination, sizeof(*destination)) ||
            (!cx_gc_reference_range_is_managed_allocation(
                 destination, sizeof(*destination)) &&
             !cx_gc_reference_range_is_registered_root(
                 destination, sizeof(*destination))) ||
            cx_gc_is_any_thread_stack_slot_locked(destination,
                sizeof(*destination)) ||
            cx_arena_contains_storage_locked(destination) ||
            cx_gc_is_any_thread_stack_slot_locked(values[index], 1u) ||
            !cx_arena_retained_reference_source_allowed_locked(
                retained_arena, values[index])) {
            cx_gc_unlock();
            return CX_FALSE;
        }
    }
    for (index = 0; index < count; ++index) {
        *destinations[index] = values[index];
    }
    cx_gc_unlock();
    return CX_TRUE;
}

cx_ptr cx_gc_store_native_pointer(cx_ptr *destination, cx_ptr value) {
    return cx_gc_store_value(destination, value);
}

cx_bool cx_gc_is_durable_reference(cx_ptr value) {
    cx_bool is_durable;
    cx_gc_check_thread();
    cx_gc_lock_heap();
    is_durable = cx_arena_find_owner_locked(value) == NULL &&
        !cx_arena_contains_storage_locked(value) &&
        !cx_gc_is_any_thread_stack_slot_locked(value, 1u)
        ? CX_TRUE : CX_FALSE;
    cx_gc_unlock();
    return is_durable;
}

struct cx_iface_ref cx_gc_store_interface_reference(
    struct cx_iface_ref *destination, struct cx_iface_ref value) {
    cx_bool instance_is_stack;
    cx_bool owner_is_stack;
    cx_bool destination_is_stack;
    if (destination == NULL) {
        cx_memory_fail("CX interface store requires a destination");
    }
    cx_gc_check_thread();
    cx_gc_lock_heap();
    instance_is_stack = cx_gc_is_current_stack_slot(
        &destination->instance, sizeof(destination->instance));
    owner_is_stack = cx_gc_is_current_stack_slot(
        &destination->owner, sizeof(destination->owner));
    destination_is_stack = cx_gc_is_any_thread_stack_slot_locked(
        destination, sizeof(*destination));
    if (!cx_gc_reference_range_fits_managed_allocation(
            destination, sizeof(*destination))) {
        cx_memory_fail("CX managed interface store exceeds its allocation");
    }
    if (!destination_is_stack &&
        (cx_gc_is_any_thread_stack_slot_locked(value.instance, 1u) ||
         cx_gc_is_any_thread_stack_slot_locked(value.owner, 1u))) {
        cx_memory_fail("CX stack reference cannot be stored in managed storage");
    }
    if (!cx_arena_reference_store_allowed_locked(
            destination, sizeof(*destination), value.instance, instance_is_stack) ||
        !cx_arena_reference_store_allowed_locked(
            destination, sizeof(*destination), value.owner, owner_is_stack)) {
        cx_memory_fail("CX scoped interface reference cannot escape its arena");
    }
    destination->vtable = value.vtable;
    destination->instance = value.instance;
    destination->owner = value.owner;
    cx_gc_unlock();
    return value;
}

static void cx_gc_mark_candidate(const void *candidate,
                                 struct cx_gc_allocation **worklist,
                                 size_t worklist_capacity,
                                 size_t *worklist_count,
                                 struct cx_gc_allocation *const *allocation_index,
                                 size_t allocation_index_count) {
    struct cx_gc_allocation *allocation = allocation_index != NULL
        ? cx_gc_find_allocation_index(
            candidate, allocation_index, allocation_index_count)
        : cx_gc_find_allocation(candidate);
    if (allocation == NULL || allocation->marked) {
        return;
    }
    allocation->marked = CX_TRUE;
    if (*worklist_count < worklist_capacity) {
        worklist[(*worklist_count)++] = allocation;
    }
}

// Conservative collection deliberately reads arbitrary stack words, including
// padding bytes that AddressSanitizer marks as red zones.
#if defined(__clang__)
__attribute__((no_sanitize("address")))
#elif defined(__GNUC__)
__attribute__((no_sanitize_address))
#endif
static void cx_gc_scan_region(const void *address,
                              size_t size,
                              struct cx_gc_allocation **worklist,
                              size_t worklist_capacity,
                              size_t *worklist_count,
                              struct cx_gc_allocation *const *allocation_index,
                              size_t allocation_index_count) {
    uintptr_t begin = (uintptr_t)address;
    uintptr_t aligned;
    size_t offset;
    if (address == NULL || size < sizeof(cx_ptr) || begin > UINTPTR_MAX - (sizeof(cx_ptr) - 1)) {
        return;
    }
    aligned = (begin + sizeof(cx_ptr) - 1) & ~(uintptr_t)(sizeof(cx_ptr) - 1);
    offset = (size_t)(aligned - begin);
    while (offset <= size - sizeof(cx_ptr)) {
        cx_ptr candidate;
        memcpy(&candidate, (const cx_byte *)address + offset, sizeof(candidate));
        cx_gc_mark_candidate(candidate, worklist, worklist_capacity,
                             worklist_count, allocation_index,
                             allocation_index_count);
        if (size - sizeof(cx_ptr) - offset < sizeof(cx_ptr)) {
            break;
        }
        offset += sizeof(cx_ptr);
    }
}

static void cx_gc_scan_arena_region(const void *address, size_t size,
                                    void *context) {
    struct cx_gc_scan_context *scan =
        (struct cx_gc_scan_context *)context;
    cx_gc_scan_region(address, size, scan->worklist,
                      scan->worklist_capacity, scan->worklist_count,
                      scan->allocation_index, scan->allocation_index_count);
}

static size_t cx_gc_count_allocations(void) {
    size_t count = 0;
    struct cx_gc_allocation *allocation;
    for (allocation = cx_gc_allocations; allocation != NULL; allocation = allocation->next) {
        if (count == SIZE_MAX) {
            return SIZE_MAX;
        }
        ++count;
    }
    return count;
}

static void cx_gc_capture_registers(jmp_buf registers) {
    if (setjmp(registers) != 0) abort();
}

static void cx_gc_collect_locked(void) {
    struct cx_gc_allocation **worklist;
    struct cx_gc_allocation **allocation_index = NULL;
    struct cx_gc_allocation *allocation;
    struct cx_gc_allocation *previous;
    struct cx_gc_registered_root *root;
    struct cx_gc_root_handle *scoped_root;
    struct cx_gc_thread *thread;
    size_t allocation_count;
    size_t allocation_index_count = 0;
    size_t worklist_count = 0;
    size_t worklist_index = 0;
    size_t live_bytes = 0;
    size_t collected_bytes = 0;
    jmp_buf registers;
    uintptr_t stack_marker = cx_platform_current_stack_position();
    uintptr_t stack_low;
    uintptr_t stack_high;
#ifdef CXCORE_ARENA_BENCHMARK
    uint64_t collection_start_ns;
#endif

    allocation_count = cx_gc_count_allocations();
    if (allocation_count == 0) {
        return;
    }
    if (allocation_count > SIZE_MAX / sizeof(*worklist)) {
        return;
    }
    worklist = (struct cx_gc_allocation **)malloc(allocation_count * sizeof(*worklist));
    if (worklist == NULL) {
        return;
    }
    if (allocation_count <= SIZE_MAX / sizeof(*allocation_index)) {
        allocation_index = (struct cx_gc_allocation **)malloc(
            allocation_count * sizeof(*allocation_index));
    }
    if (!cx_platform_current_stack_bounds(&stack_low, &stack_high)) {
        free(allocation_index);
        free(worklist);
        return;
    }
    if (stack_marker < stack_low || stack_marker >= stack_high) {
        free(allocation_index);
        free(worklist);
        return;
    }

#ifdef CXCORE_ARENA_BENCHMARK
    collection_start_ns = cx_gc_benchmark_monotonic_time_ns();
#endif
    cx_gc_stop_requested = CX_TRUE;
#ifdef CXCORE_TESTING
    if (cx_memory_test_collection_started_hook != NULL) {
        cx_memory_test_collection_started_hook(
            cx_memory_test_collection_started_context);
    }
#endif
    for (;;) {
        for (thread = cx_gc_threads; thread != NULL; thread = thread->next) {
            if (thread != cx_gc_current_thread && !thread->parked) {
                break;
            }
        }
        if (thread == NULL) {
            break;
        }
        cx_gc_wait();
    }
    // Fail closed: never sweep if any registered stack cannot be scanned.
    for (thread = cx_gc_threads; thread != NULL; thread = thread->next) {
        if (thread != cx_gc_current_thread &&
            ((uintptr_t)thread->stack_marker < thread->stack_low ||
             (uintptr_t)thread->stack_marker >= thread->stack_high)) {
            free(allocation_index);
            free(worklist);
            cx_gc_stop_requested = CX_FALSE;
            cx_gc_wake();
            return;
        }
    }

    for (allocation = cx_gc_allocations; allocation != NULL; allocation = allocation->next) {
        allocation->marked = CX_FALSE;
        if (allocation_index != NULL)
            allocation_index[allocation_index_count++] = allocation;
    }
    if (allocation_index != NULL) {
        qsort(allocation_index, allocation_index_count,
              sizeof(*allocation_index), cx_gc_compare_allocation_address);
    }
    for (root = cx_gc_roots; root != NULL; root = root->next) {
        cx_gc_scan_region(root->address, root->size, worklist,
                          allocation_count, &worklist_count,
                          allocation_index, allocation_index_count);
    }
    for (scoped_root = cx_gc_scoped_roots; scoped_root != NULL; scoped_root = scoped_root->next) {
        cx_gc_scan_region(scoped_root->address, scoped_root->size,
                          worklist, allocation_count, &worklist_count,
                          allocation_index, allocation_index_count);
    }
    {
        struct cx_gc_scan_context arena_scan;
        arena_scan.worklist = worklist;
        arena_scan.worklist_capacity = allocation_count;
        arena_scan.worklist_count = &worklist_count;
        arena_scan.allocation_index = allocation_index;
        arena_scan.allocation_index_count = allocation_index_count;
        cx_arena_scan_active(cx_gc_scan_arena_region, &arena_scan);
    }
    for (thread = cx_gc_threads; thread != NULL; thread = thread->next) {
        cx_gc_scan_region(thread->exception_root, sizeof(cx_ptr),
                          worklist, allocation_count, &worklist_count,
                          allocation_index, allocation_index_count);
        {
            struct cx_gc_native_frame *frame;
            frame = thread->pending_native_frame;
            if (frame != NULL && frame->root_address != NULL) {
                cx_gc_scan_region(frame->root_address, frame->root_size,
                                  worklist, allocation_count, &worklist_count,
                                  allocation_index, allocation_index_count);
            }
            for (frame = thread->native_frame; frame != NULL; frame = frame->previous) {
                if (frame->root_address != NULL) {
                    cx_gc_scan_region(frame->root_address, frame->root_size,
                                      worklist, allocation_count, &worklist_count,
                                      allocation_index, allocation_index_count);
                }
            }
        }
    }
    cx_gc_capture_registers(registers);
    cx_gc_scan_region(registers, sizeof(registers), worklist,
                      allocation_count, &worklist_count,
                      allocation_index, allocation_index_count);
    cx_gc_scan_region((const void *)stack_marker,
                      (size_t)(stack_high - stack_marker),
                      worklist, allocation_count, &worklist_count,
                      allocation_index, allocation_index_count);
    for (thread = cx_gc_threads; thread != NULL; thread = thread->next) {
        if (thread == cx_gc_current_thread) {
            continue;
        }
        cx_gc_scan_region(thread->registers, sizeof(jmp_buf),
                          worklist, allocation_count, &worklist_count,
                          allocation_index, allocation_index_count);
        cx_gc_scan_region((const void *)thread->stack_marker,
                          (size_t)(thread->stack_high - (uintptr_t)thread->stack_marker),
                          worklist, allocation_count, &worklist_count,
                          allocation_index, allocation_index_count);
    }

    while (worklist_index < worklist_count) {
        allocation = worklist[worklist_index++];
        cx_gc_scan_region(allocation->address, allocation->size,
                          worklist, allocation_count, &worklist_count,
                          allocation_index, allocation_index_count);
    }

    previous = NULL;
    allocation = cx_gc_allocations;
    while (allocation != NULL) {
        struct cx_gc_allocation *next = allocation->next;
        if (allocation->marked) {
            live_bytes += allocation->size;
            previous = allocation;
        } else {
            if (previous == NULL) {
                cx_gc_allocations = next;
            } else {
                previous->next = next;
            }
            cx_gc_heap_bytes -= allocation->size;
            collected_bytes += allocation->size;
            free(allocation->address);
            free(allocation);
        }
        allocation = next;
    }
    free(allocation_index);
    free(worklist);

    if (live_bytes > SIZE_MAX / 2) {
        cx_gc_next_collection = SIZE_MAX;
    } else {
        size_t next_threshold = live_bytes * 2;
        cx_gc_next_collection = next_threshold < CX_GC_INITIAL_THRESHOLD
            ? CX_GC_INITIAL_THRESHOLD : next_threshold;
    }
#ifdef CXCORE_TESTING
    ++cx_memory_test_collection_count;
    cx_memory_test_collected_bytes += collected_bytes;
#else
    (void)collected_bytes;
#endif
#ifdef CXCORE_ARENA_BENCHMARK
    ++cx_memory_benchmark_collection_count;
    cx_memory_benchmark_collected_bytes += collected_bytes;
    cx_memory_benchmark_collection_pause_ns +=
        cx_gc_benchmark_monotonic_time_ns() - collection_start_ns;
#endif
    cx_gc_stop_requested = CX_FALSE;
    cx_gc_wake();
}

void cx_gc_collect(void) {
    cx_gc_lock_heap();
    cx_gc_collect_locked();
    cx_gc_unlock();
}

static cx_bool cx_gc_reserve_heap_capacity_locked(size_t additional_bytes) {
    if (additional_bytes <= SIZE_MAX - cx_gc_heap_bytes &&
        cx_gc_memory_budget_fits(cx_gc_heap_bytes + additional_bytes,
                                 cx_gc_arena_reserved_bytes)) {
        return CX_TRUE;
    }
    cx_gc_collect_locked();
    return additional_bytes <= SIZE_MAX - cx_gc_heap_bytes &&
        cx_gc_memory_budget_fits(cx_gc_heap_bytes + additional_bytes,
                                 cx_gc_arena_reserved_bytes);
}

cx_bool cx_gc_set_process_memory_budget(cx_ulong maximum_bytes) {
    size_t maximum;
    cx_bool accepted;
    if ((uint64_t)maximum_bytes > (uint64_t)SIZE_MAX) return CX_FALSE;
    maximum = (size_t)maximum_bytes;
    cx_gc_lock_heap();
    accepted = maximum == 0 ||
        (cx_gc_arena_reserved_bytes <= SIZE_MAX - cx_gc_heap_bytes &&
         cx_gc_heap_bytes + cx_gc_arena_reserved_bytes <= maximum);
    if (accepted) cx_gc_process_memory_budget = maximum;
    cx_gc_unlock();
    return accepted;
}

cx_bool cxcore_System_Memory_SetProcessMemoryBudget(cx_ulong maximum_bytes) {
    return cx_gc_set_process_memory_budget(maximum_bytes);
}

void cxcore_System_GC_Collect(void) {
    cx_gc_collect();
}

void cx_gc_register_static_roots(const struct cx_gc_root_region *roots, cx_uint count) {
    cx_uint index;
    cx_gc_lock_heap();
    if (count != 0 && roots == NULL) {
        cx_memory_fail("Invalid CX static root table");
    }
    for (index = 0; index < count; ++index) {
        struct cx_gc_registered_root *root;
        const void *address = roots[index].address;
        size_t size = roots[index].size;
        if (address == NULL || size == 0) {
            continue;
        }
        root = cx_gc_find_registered_root(address, size);
        if (root == NULL) {
            root = (struct cx_gc_registered_root *)malloc(sizeof(*root));
            if (root == NULL) {
                cx_memory_out_of_memory();
            }
            root->address = address;
            root->size = size;
            root->next = cx_gc_roots;
            cx_gc_roots = root;
            cx_gc_index_registered_root(root);
        }
    }
    cx_gc_unlock();
}

cx_ptr cxcore_System_Memory_Alloc(cx_uint size) {
    size_t allocation_size = size == 0 ? 1 : (size_t)size;
    struct cx_gc_allocation *record;
    void *block;

#ifdef CXCORE_TESTING
    // Keep the deterministic allocation-failure hook consistent when the
    // active destination is an arena instead of the GC heap.
    cx_gc_lock_heap();
    if (cx_memory_should_fail_allocation()) {
        cx_memory_out_of_memory();
    }
    cx_gc_unlock();
#endif

    block = cx_arena_allocate_current(
        allocation_size, cx_arena_maximum_alignment());
    if (block != NULL) return block;

    cx_gc_lock_heap();
#ifndef CXCORE_TESTING
    if (cx_memory_should_fail_allocation()) {
        cx_memory_out_of_memory();
    }
#endif
    if (allocation_size > SIZE_MAX - cx_gc_heap_bytes) {
        cx_memory_out_of_memory();
    }
    if (!cx_gc_reserve_heap_capacity_locked(allocation_size)) {
        cx_memory_out_of_memory();
    }
    if (allocation_size > cx_gc_next_collection -
            (cx_gc_heap_bytes < cx_gc_next_collection ? cx_gc_heap_bytes : cx_gc_next_collection)) {
        cx_gc_collect_locked();
    }
    block = calloc(1, allocation_size);
    if (block == NULL) {
        cx_gc_collect_locked();
        block = calloc(1, allocation_size);
    }
    if (block == NULL) {
        cx_memory_out_of_memory();
    }
#ifdef CXCORE_TESTING
    if (cx_memory_fail_next_record) {
        cx_memory_fail_next_record = CX_FALSE;
        record = NULL;
    } else
#endif
    {
        record = (struct cx_gc_allocation *)malloc(sizeof(*record));
    }
    if (record == NULL) {
        free(block);
        cx_gc_collect_locked();
        record = (struct cx_gc_allocation *)malloc(sizeof(*record));
        block = calloc(1, allocation_size);
    }
    if (record == NULL || block == NULL) {
        cx_memory_out_of_memory();
    }
    record->address = block;
    record->size = allocation_size;
    record->marked = CX_FALSE;
    record->next = cx_gc_allocations;
    cx_gc_allocations = record;
    cx_gc_heap_bytes += allocation_size;
#ifdef CXCORE_TESTING
    if (cx_memory_test_track_allocations) {
        size_t index;
        ++cx_memory_test_allocation_count;
        cx_memory_test_allocation_bytes += allocation_size;
        for (index = 0; index < cx_memory_test_allocation_size_count; ++index) {
            if (cx_memory_test_allocation_sizes[index] == allocation_size) {
                ++cx_memory_test_allocation_size_counts[index];
                break;
            }
        }
        if (index == cx_memory_test_allocation_size_count &&
            index < CX_MEMORY_TEST_ALLOCATION_SIZE_LIMIT) {
            cx_memory_test_allocation_sizes[index] = allocation_size;
            cx_memory_test_allocation_size_counts[index] = 1;
            ++cx_memory_test_allocation_size_count;
        }
    }
#endif
    if (cx_gc_heap_bytes > cx_gc_next_collection) {
        cx_gc_next_collection = cx_gc_heap_bytes > SIZE_MAX / 2
            ? SIZE_MAX : cx_gc_heap_bytes * 2;
    }
    cx_gc_unlock();
    return block;
}

CX_EXPORT cx_ptr cx_memory_alloc_for_owner(const void *owner, cx_uint size) {
    if (owner == NULL) {
        cx_memory_fail("Allocation owner is null");
    }
    return cx_gc_alloc_for_owner(
        owner, (size_t)size, cx_arena_maximum_alignment());
}

void *cx_gc_alloc_for_owner(const void *owner, size_t size, size_t alignment) {
    struct cx_arena_durable_scope durable_scope;
    void *block;
    if (owner == NULL || size > UINT_MAX) {
        cx_memory_fail("Allocation owner and size are invalid");
    }
    block = cx_arena_allocate_for_owner(owner, size, alignment);
    if (block != NULL) return block;
    cx_arena_enter_durable(&durable_scope);
    block = cxcore_System_Memory_Alloc((cx_uint)size);
    cx_arena_leave_durable(&durable_scope);
    return block;
}

cx_ptr cxcore_System_Memory_Realloc(cx_ptr block, cx_uint size) {
    struct cx_gc_allocation *record;
    void *resized;
    size_t new_size;
    if (block == NULL) {
        return cxcore_System_Memory_Alloc(size);
    }
    cx_gc_lock_heap();
    if (cx_arena_contains_storage_locked(block)) {
        cx_gc_unlock();
        return cx_arena_reallocate_current(block, (size_t)size);
    }
    if (size == 0) {
        cx_gc_unlock();
        cxcore_System_Memory_Free(block);
        return CX_NULL;
    }
    record = cx_gc_find_allocation(block);
    if (record == NULL || record->address != block) {
        cx_memory_fail("Memory.Realloc requires a pointer returned by Memory.Alloc");
    }
    if (cx_memory_should_fail_allocation()) {
        cx_memory_out_of_memory();
    }
    new_size = (size_t)size;
    if (new_size > record->size) {
        size_t growth = new_size - record->size;
        size_t available = cx_gc_next_collection -
            (cx_gc_heap_bytes < cx_gc_next_collection ? cx_gc_heap_bytes : cx_gc_next_collection);
        if (growth > available) {
            cx_gc_collect_locked();
            record = cx_gc_find_allocation(block);
            if (record == NULL || record->address != block) {
                cx_memory_fail("Memory.Realloc lost its managed allocation during collection");
            }
        }
        if (!cx_gc_reserve_heap_capacity_locked(growth)) {
            cx_memory_out_of_memory();
        }
        record = cx_gc_find_allocation(block);
        if (record == NULL || record->address != block) {
            cx_memory_fail("Memory.Realloc lost its managed allocation during collection");
        }
    }
    if (new_size > SIZE_MAX - (cx_gc_heap_bytes - record->size)) {
        cx_memory_out_of_memory();
    }
    resized = realloc(block, new_size);
    if (resized == NULL) {
        cx_gc_collect_locked();
        resized = realloc(block, new_size);
    }
    if (resized == NULL) {
        cx_memory_out_of_memory();
    }
    cx_gc_heap_bytes -= record->size;
    record->address = resized;
    record->size = new_size;
    cx_gc_heap_bytes += new_size;
    if (cx_gc_heap_bytes > cx_gc_next_collection) {
        cx_gc_next_collection = cx_gc_heap_bytes > SIZE_MAX / 2
            ? SIZE_MAX : cx_gc_heap_bytes * 2;
    }
    cx_gc_unlock();
    return resized;
}

void cxcore_System_Memory_Free(cx_ptr block) {
    struct cx_gc_allocation *record;
    struct cx_gc_allocation *previous = NULL;
    if (block == NULL) {
        return;
    }
    cx_gc_lock_heap();
    if (cx_arena_contains_storage_locked(block)) {
        cx_memory_fail("Memory.Free cannot release scoped arena storage");
    }
    record = cx_gc_allocations;
    while (record != NULL && record->address != block) {
        previous = record;
        record = record->next;
    }
    if (record != NULL) {
        if (previous == NULL) {
            cx_gc_allocations = record->next;
        } else {
            previous->next = record->next;
        }
        cx_gc_heap_bytes -= record->size;
        free(record);
    }
    free(block);
    cx_gc_unlock();
}

void cxcore_System_Memory_Copy(cx_ptr dest, cx_ptr src, cx_uint size) {
    cx_bool arenas_have_storage;
    cx_bool destination_is_stack;
    size_t candidate_count;
    size_t offset;
    cx_gc_lock_heap();
    if (size == 0) {
        cx_gc_unlock();
        return;
    }
    if (dest == NULL || src == NULL) {
        abort();
    }
    if (!cx_arena_memory_copy_allowed_locked(dest, src, (size_t)size)) {
        cx_memory_fail("Memory.Copy cannot access scoped arena storage");
    }
    destination_is_stack = cx_gc_is_current_stack_slot(dest, (size_t)size);
    // Memory.Copy has no type layout, so it cannot distinguish reference
    // fields from scalar bytes. Inspect every byte offset so packed or
    // deliberately unaligned pointer representations receive the same lifetime
    // barrier as naturally aligned references. Typed copies should use
    // cx_gc_copy_value, which avoids this conservative raw-byte check.
    candidate_count = (size_t)size < sizeof(cx_ptr)
        ? 0
        : (size_t)size - sizeof(cx_ptr) + 1u;
    // A pointer-sized copy has exactly one possible reference window. Handle
    // it directly to avoid the arena-list preflight needed by longer scans.
    if (candidate_count == 1u) {
        cx_ptr candidate;
        memcpy(&candidate, src, sizeof(candidate));
        if (candidate != CX_NULL) {
            uintptr_t candidate_address = (uintptr_t)candidate;
            if (candidate_address >= cx_gc_stack_range_low &&
                candidate_address < cx_gc_stack_range_high &&
                cx_gc_is_any_thread_stack_slot_locked(candidate, 1u) &&
                !destination_is_stack) {
                cx_memory_fail("Memory.Copy cannot publish a stack reference");
            }
            // Treat any pointer into reserved arena storage as a candidate.
            // The store validator distinguishes a live allocation from
            // padding, a superseded extent, or unused capacity and rejects
            // those invalid references instead of letting a raw byte copy
            // publish them without lifetime validation.
            if (cx_arena_contains_storage_locked(candidate) &&
                !cx_arena_reference_store_allowed_locked(
                    dest, sizeof(cx_ptr), candidate,
                    destination_is_stack)) {
                cx_memory_fail("Memory.Copy cannot publish a scoped reference");
            }
        }
        memcpy(dest, src, size);
        cx_gc_unlock();
        return;
    }
    arenas_have_storage = cx_arena_has_storage_locked()
        ? CX_TRUE : CX_FALSE;
    // Most byte windows are scalar data. Bound exact stack lookups with the
    // coarse thread-stack range maintained during attachment and detachment.
    for (offset = 0; offset < candidate_count; ++offset) {
        cx_ptr candidate;
        uintptr_t candidate_address;
        const void *destination_slot = (const cx_byte *)dest + offset;
        memcpy(&candidate, (const cx_byte *)src + offset, sizeof(candidate));
        if (candidate == CX_NULL) continue;
        candidate_address = (uintptr_t)candidate;
        if (candidate_address >= cx_gc_stack_range_low &&
            candidate_address < cx_gc_stack_range_high &&
            cx_gc_is_any_thread_stack_slot_locked(candidate, 1u) &&
            !destination_is_stack) {
            cx_memory_fail("Memory.Copy cannot publish a stack reference");
        }
        if (arenas_have_storage &&
            cx_arena_contains_storage_locked(candidate) &&
            !cx_arena_reference_store_allowed_locked(
                destination_slot, sizeof(cx_ptr), candidate,
                destination_is_stack)) {
            cx_memory_fail("Memory.Copy cannot publish a scoped reference");
        }
    }
    memcpy(dest, src, size);
    cx_gc_unlock();
}

cx_ptr cxcore_System_Memory_AddObjectRoot(cx_ptr value) {
    struct cx_gc_object_root *root;
    if (!cx_gc_is_durable_reference(value)) {
        cx_memory_fail("CX scoped reference cannot escape its arena");
    }
    root = (struct cx_gc_object_root *)malloc(sizeof(*root));
    if (root == NULL) cx_memory_out_of_memory();
    root->value = CX_NULL;
    root->handle = cx_gc_add_root(&root->value, (cx_uint)sizeof(root->value));
    (void)cx_gc_store_reference(&root->value, value);
    return root;
}

void cxcore_System_Memory_RemoveObjectRoot(cx_ptr handle) {
    struct cx_gc_object_root *root = (struct cx_gc_object_root *)handle;
    if (root != NULL) {
        cx_gc_remove_root(root->handle);
        free(root);
    }
}

#if defined(CXCORE_TESTING)
CX_EXPORT cx_ptr cxcore_test_System_Memory_AddObjectRoot(cx_ptr value) {
    return cxcore_System_Memory_AddObjectRoot(value);
}

CX_EXPORT void cxcore_test_System_Memory_RemoveObjectRoot(cx_ptr handle) {
    cxcore_System_Memory_RemoveObjectRoot(handle);
}
#endif

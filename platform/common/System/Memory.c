#include <limits.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../platform.h"
#include "../../../src/cxcore.h"
#include "../cxgc.h"

#ifdef CX_PLATFORM_WIN
#include <windows.h>
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

struct cx_gc_root_handle {
    const void *address;
    size_t size;
    struct cx_gc_root_handle *next;
};

static size_t cx_gc_heap_bytes;

struct cx_gc_native_frame {
    jmp_buf registers;
    volatile cx_byte marker;
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
    struct cx_gc_thread *next;
};

static struct cx_gc_thread *cx_gc_threads;
static CX_THREAD_LOCAL struct cx_gc_thread *cx_gc_current_thread;
static cx_bool cx_gc_started;
static cx_bool cx_gc_stop_requested;
static cx_ulong cx_gc_next_thread_id;
static void cx_gc_lock_heap(void);

#ifdef CXCORE_TESTING
static cx_bool cx_memory_fail_next_allocation;
static cx_bool cx_memory_fail_next_record;
static size_t cx_memory_test_collection_count;
static size_t cx_memory_test_collected_bytes;

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
    volatile cx_byte marker = 0;
    if (!cx_platform_current_stack_bounds(&low, &high) ||
        (uintptr_t)&marker < low || (uintptr_t)&marker >= high) {
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
    cx_gc_current_thread = thread;
    cx_gc_started = CX_TRUE;
}

// The marker belongs to this active frame for the entire parked interval.
static void cx_gc_lock_heap(void) {
    volatile cx_byte marker = 0;
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
        thread->stack_marker = &marker;
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
    if (cx_gc_current_thread == NULL) {
        cx_memory_fail("CX GC detach requires an attached thread");
    }
    if (cx_gc_current_thread->mutex_count != 0) {
        cx_memory_fail("CX thread cannot exit while owning a mutex");
    }
    if (!cx_exception_thread_detach()) {
        cx_memory_fail("CX GC cannot detach a thread with active exception handlers");
    }
    for (link = &cx_gc_threads; *link != cx_gc_current_thread; link = &(*link)->next) { }
    *link = cx_gc_current_thread->next;
    free(cx_gc_current_thread);
    cx_gc_current_thread = NULL;
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
    volatile cx_byte marker = 0;
    jmp_buf registers;
    struct cx_gc_thread *thread = cx_gc_current_thread;
    (void)setjmp(registers);
    thread->registers = registers;
    thread->stack_marker = &marker;
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
    volatile cx_byte marker = 0;
    jmp_buf registers;
    struct cx_gc_thread *thread;
    if (callback == NULL) {
        cx_memory_fail("CX GC blocking callback must not be null");
    }
    cx_gc_lock_heap();
    thread = cx_gc_current_thread;
    (void)setjmp(registers);
    thread->registers = registers;
    thread->stack_marker = &marker;
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

void cx_gc_run_native_call(cx_native_call_callback callback, void *context) {
    struct cx_gc_native_frame frame;
    struct cx_gc_thread *thread;
    if (callback == NULL) cx_memory_fail("Native call adapter must not be null");
    cx_gc_lock_heap();
    thread = cx_gc_current_thread;
    frame.previous = thread->native_frame;
    frame.marker = 0;
    (void)setjmp(frame.registers);
    thread->native_frame = &frame;
    thread->registers = frame.registers;
    thread->stack_marker = &frame.marker;
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
    thread->stack_marker = &thread->native_frame->marker;
    thread->parked = CX_TRUE;
    cx_gc_wake();
    cx_gc_unlock();
}

cx_bool cx_runtime_once_begin(struct cx_runtime_once *once) {
    volatile cx_byte marker = 0;
    jmp_buf registers;
    struct cx_gc_thread *thread;
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
        thread->stack_marker = &marker;
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
    cx_gc_lock_heap();
    if (once == NULL || once->state != 1 || once->owner != cx_gc_current_thread) {
        cx_memory_fail("CX initialization gate must be completed by its owner");
    }
    once->failure = failure;
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

static void cx_gc_mark_candidate(const void *candidate,
                                 struct cx_gc_allocation **worklist,
                                 size_t worklist_capacity,
                                 size_t *worklist_count) {
    struct cx_gc_allocation *allocation = cx_gc_find_allocation(candidate);
    if (allocation == NULL || allocation->marked) {
        return;
    }
    allocation->marked = CX_TRUE;
    if (*worklist_count < worklist_capacity) {
        worklist[(*worklist_count)++] = allocation;
    }
}

static void cx_gc_scan_region(const void *address,
                              size_t size,
                              struct cx_gc_allocation **worklist,
                              size_t worklist_capacity,
                              size_t *worklist_count) {
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
        cx_gc_mark_candidate(candidate, worklist, worklist_capacity, worklist_count);
        if (size - sizeof(cx_ptr) - offset < sizeof(cx_ptr)) {
            break;
        }
        offset += sizeof(cx_ptr);
    }
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

static void cx_gc_collect_locked(void) {
    struct cx_gc_allocation **worklist;
    struct cx_gc_allocation *allocation;
    struct cx_gc_allocation *previous;
    struct cx_gc_registered_root *root;
    struct cx_gc_root_handle *scoped_root;
    struct cx_gc_thread *thread;
    size_t allocation_count;
    size_t worklist_count = 0;
    size_t worklist_index = 0;
    size_t live_bytes = 0;
    size_t collected_bytes = 0;
    jmp_buf registers;
    volatile cx_byte stack_marker = 0;
    uintptr_t stack_low;
    uintptr_t stack_high;

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
    if (!cx_platform_current_stack_bounds(&stack_low, &stack_high)) {
        free(worklist);
        return;
    }
    if ((uintptr_t)&stack_marker < stack_low ||
        (uintptr_t)&stack_marker >= stack_high) {
        free(worklist);
        return;
    }

    cx_gc_stop_requested = CX_TRUE;
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
            free(worklist);
            cx_gc_stop_requested = CX_FALSE;
            cx_gc_wake();
            return;
        }
    }

    for (allocation = cx_gc_allocations; allocation != NULL; allocation = allocation->next) {
        allocation->marked = CX_FALSE;
    }
    for (root = cx_gc_roots; root != NULL; root = root->next) {
        cx_gc_scan_region(root->address, root->size, worklist, allocation_count, &worklist_count);
    }
    for (scoped_root = cx_gc_scoped_roots; scoped_root != NULL; scoped_root = scoped_root->next) {
        cx_gc_scan_region(scoped_root->address, scoped_root->size,
                          worklist, allocation_count, &worklist_count);
    }
    for (thread = cx_gc_threads; thread != NULL; thread = thread->next) {
        cx_gc_scan_region(thread->exception_root, sizeof(cx_ptr),
                          worklist, allocation_count, &worklist_count);
    }
    if (setjmp(registers) == 0) {
        cx_gc_scan_region(registers, sizeof(registers), worklist, allocation_count, &worklist_count);
    }
    cx_gc_scan_region((const void *)&stack_marker,
                      (size_t)(stack_high - (uintptr_t)&stack_marker),
                      worklist, allocation_count, &worklist_count);
    for (thread = cx_gc_threads; thread != NULL; thread = thread->next) {
        if (thread == cx_gc_current_thread) {
            continue;
        }
        cx_gc_scan_region(thread->registers, sizeof(jmp_buf),
                          worklist, allocation_count, &worklist_count);
        cx_gc_scan_region((const void *)thread->stack_marker,
                          (size_t)(thread->stack_high - (uintptr_t)thread->stack_marker),
                          worklist, allocation_count, &worklist_count);
    }

    while (worklist_index < worklist_count) {
        allocation = worklist[worklist_index++];
        cx_gc_scan_region(allocation->address, allocation->size,
                          worklist, allocation_count, &worklist_count);
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
    cx_gc_stop_requested = CX_FALSE;
    cx_gc_wake();
}

void cx_gc_collect(void) {
    cx_gc_lock_heap();
    cx_gc_collect_locked();
    cx_gc_unlock();
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
        for (root = cx_gc_roots; root != NULL; root = root->next) {
            if (root->address == address && root->size == size) {
                break;
            }
        }
        if (root == NULL) {
            root = (struct cx_gc_registered_root *)malloc(sizeof(*root));
            if (root == NULL) {
                cx_memory_out_of_memory();
            }
            root->address = address;
            root->size = size;
            root->next = cx_gc_roots;
            cx_gc_roots = root;
        }
    }
    cx_gc_unlock();
}

cx_ptr CX_ID_4(cxcore, System, Memory, Alloc)(cx_uint size) {
    size_t allocation_size = size == 0 ? 1 : (size_t)size;
    struct cx_gc_allocation *record;
    void *block;

    cx_gc_lock_heap();
    if (cx_memory_should_fail_allocation()) {
        cx_memory_out_of_memory();
    }
    if (allocation_size > SIZE_MAX - cx_gc_heap_bytes) {
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
    if (cx_gc_heap_bytes > cx_gc_next_collection) {
        cx_gc_next_collection = cx_gc_heap_bytes > SIZE_MAX / 2
            ? SIZE_MAX : cx_gc_heap_bytes * 2;
    }
    cx_gc_unlock();
    return block;
}

cx_ptr CX_ID_4(cxcore, System, Memory, Realloc)(cx_ptr block, cx_uint size) {
    struct cx_gc_allocation *record;
    void *resized;
    size_t new_size;
    if (block == NULL) {
        return CX_ID_4(cxcore, System, Memory, Alloc)(size);
    }
    if (size == 0) {
        CX_ID_4(cxcore, System, Memory, Free)(block);
        return CX_NULL;
    }
    cx_gc_lock_heap();
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

void CX_ID_4(cxcore, System, Memory, Free)(cx_ptr block) {
    struct cx_gc_allocation *record;
    struct cx_gc_allocation *previous = NULL;
    if (block == NULL) {
        return;
    }
    cx_gc_lock_heap();
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

void CX_ID_4(cxcore, System, Memory, Copy)(cx_ptr dest, cx_ptr src, cx_uint size) {
    cx_gc_lock_heap();
    if (size == 0) {
        cx_gc_unlock();
        return;
    }
    if (dest == NULL || src == NULL) {
        abort();
    }
    memcpy(dest, src, size);
    cx_gc_unlock();
}

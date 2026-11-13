#include "Threading.internal.h"
#include "Thread.internal.h"
#include "../../../platform.h"

typedef struct cxcore_System_Threading_Thread cx_thread;

struct cx_thread_state {
    cx_action action;
    struct cx_arena *logical_arena;
    struct cx_exception failure;
    cx_ulong id;
    cx_bool attached;
    cx_bool complete;
    struct cx_thread_state *next;
};

static struct cx_thread_state *cx_thread_states;
static size_t cx_active_threads;
static const struct cx_gc_root_region cx_thread_roots[] = {
    {&cx_thread_states, (cx_uint)sizeof(cx_thread_states)},
};

CX_STRING_DEF(cx_thread_invalid, "Invalid thread handle or self join");
CX_STRING_DEF(cx_thread_null_action, "Thread.Start requires a non-null action");
CX_STRING_DEF(cx_thread_start_failed, "Unable to start native thread");
CX_STRING_DEF(cx_thread_scoped_action, "A synchronous allocation arena cannot publish a thread action");
CX_STRING_DEF(cx_thread_join_active_arena, "Thread.Join cannot wait while its retained arena is active on the caller");
CX_STRING_DEF(cx_thread_invalid_sleep, "Sleep duration must be non-negative");

static void cx_thread_store_action(cx_action *destination, cx_action value) {
    destination->__function = value.__function;
    (void)cx_gc_store_reference(
        (cx_ptr *)&destination->__environment,
        (cx_ptr)value.__environment);
    (void)cx_gc_store_reference(
        (cx_ptr *)&destination->__target, value.__target);
    destination->__method = value.__method;
    (void)cx_gc_store_reference(
        (cx_ptr *)&destination->__invocation_list,
        (cx_ptr)value.__invocation_list);
}

static void cx_thread_store_exception(struct cx_exception *destination,
                                      const struct cx_exception *value) {
    (void)cx_gc_store_reference(
        (cx_ptr *)&destination->exceptionObject, value->exceptionObject);
    destination->thrownFileName = value->thrownFileName;
    destination->thrownLine = value->thrownLine;
}

static cx_bool cx_thread_store_action_with_arena(
    cx_action *destination,
    cx_action action,
    struct cx_arena *logical_arena) {
    if (logical_arena == NULL) {
        if (!cx_arena_async_use_allowed() ||
        !cx_gc_is_durable_reference((cx_ptr)action.__environment) ||
        !cx_gc_is_durable_reference(action.__target) ||
        !cx_gc_is_durable_reference((cx_ptr)action.__invocation_list)) {
            return CX_FALSE;
        }
        cx_thread_store_action(destination, action);
        return CX_TRUE;
    }
    {
        cx_ptr *destinations[3] = {
            (cx_ptr *)&destination->__environment,
            (cx_ptr *)&destination->__target,
            (cx_ptr *)&destination->__invocation_list,
        };
        cx_ptr values[3] = {
            (cx_ptr)action.__environment,
            action.__target,
            (cx_ptr)action.__invocation_list,
        };
        if (!cx_gc_store_references_retained_arena(
                destinations, values, 3u, logical_arena)) {
            return CX_FALSE;
        }
        destination->__function = action.__function;
        destination->__method = action.__method;
    }
    return CX_TRUE;
}

// Runtime-owned metadata is read/written only under the collector mutex. Waits
// park through cx_gc_runtime_wait; native handles are detached immediately.
static void cx_thread_unlink(struct cx_thread_state *state) {
    struct cx_thread_state **link;
    for (link = &cx_thread_states; *link != NULL; link = &(*link)->next) {
        if (*link == state) {
            (void)cx_gc_store_reference_locked(
                (cx_ptr *)link, (cx_ptr)state->next);
            (void)cx_gc_store_reference_locked(
                (cx_ptr *)&state->next, NULL);
            return;
        }
    }
}

void cx_thread_worker(void *context) {
    struct cx_thread_state *state = context;
    struct cx_exception_frame frame;
    struct cx_arena_scope arena_scope;
    volatile char callback_stack_boundary = 0;
    struct cx_arena *logical_arena;
    cx_gc_attach_thread();
    cx_gc_runtime_lock();
    state->id = cx_gc_runtime_thread_id();
    state->attached = CX_TRUE;
    cx_gc_runtime_wake();
    cx_gc_runtime_unlock();
    cx_exception_push(&frame);
    if (setjmp(frame.environment) == 0) {
        cx_bool entered_arena = CX_FALSE;
        if (state->logical_arena != NULL) {
            cx_arena_enter_logical(state->logical_arena, &arena_scope);
            cx_arena_allow_callback_stack_borrows(
                &arena_scope, &callback_stack_boundary);
            entered_arena = CX_TRUE;
        }
        if (state->action.__invocation_list != NULL) {
            struct CX_FN_136FFEE706DF35B2_invocation *item;
            for (item = state->action.__invocation_list; item != NULL; item = item->next) {
                cx_gc_safepoint();
                item->value.__function(item->value.__environment);
            }
        } else {
            state->action.__function(state->action.__environment);
        }
        if (entered_arena) {
            cx_arena_leave(&arena_scope);
            entered_arena = CX_FALSE;
        }
    } else {
        cx_thread_store_exception(&state->failure,
                                  cx_exception_thread_state());
    }
    cx_exception_pop(&frame);
    cx_exception_clear();
    cx_thread_store_action(&state->action, (cx_action){0});
    cx_gc_runtime_lock();
    logical_arena = state->logical_arena;
    (void)cx_gc_store_reference_locked(
        (cx_ptr *)&state->logical_arena, NULL);
    cx_gc_runtime_unlock();
    if (logical_arena != NULL) cx_arena_release_logical(logical_arena);
    cx_gc_runtime_lock();
    state->complete = CX_TRUE;
    --cx_active_threads;
    if (state->failure.exceptionObject == NULL) cx_thread_unlink(state);
    cx_gc_runtime_wake();
    // Unregister atomically with completion publication. No managed access may
    // occur after this point, including in native thread teardown.
    cx_gc_runtime_detach();
    cx_gc_runtime_unlock();
}

cx_thread cxcore_System_Threading_Thread_Start(cx_action action) {
    struct cx_thread_state *state;
    struct cx_arena_durable_scope durable_scope;
    struct cx_arena *logical_arena;
    cx_bool started;
    if (action.__function == NULL && action.__invocation_list == NULL) {
        cx_threading_raise(&cx_thread_null_action);
    }
    if (!cx_arena_scheduler_use_allowed()) {
        cx_threading_raise(&cx_thread_scoped_action);
    }
    cx_arena_enter_durable(&durable_scope);
    state = (struct cx_thread_state *)cxcore_System_Memory_Alloc((cx_uint)sizeof(*state));
    cx_arena_leave_durable(&durable_scope);
    logical_arena = cx_arena_capture_logical_context();
    (void)cx_gc_store_native_pointer(
        (cx_ptr *)&state->logical_arena, (cx_ptr)logical_arena);
    if (!cx_thread_store_action_with_arena(
            &state->action, action, logical_arena)) {
        (void)cx_gc_store_native_pointer(
            (cx_ptr *)&state->logical_arena, NULL);
        if (logical_arena != NULL) cx_arena_release_logical(logical_arena);
        cx_threading_raise(&cx_thread_scoped_action);
    }
    cx_gc_register_static_roots(cx_thread_roots, 1);
    cx_gc_runtime_lock();
    (void)cx_gc_store_reference_locked(
        (cx_ptr *)&state->next, (cx_ptr)cx_thread_states);
    (void)cx_gc_store_reference_locked(
        (cx_ptr *)&cx_thread_states, (cx_ptr)state);
    ++cx_active_threads;
    started = cx_platform_thread_start(state) != 0;
    if (!started) {
        --cx_active_threads;
        cx_thread_unlink(state);
        cx_gc_runtime_unlock();
        cx_thread_store_action(&state->action, (cx_action){0});
        logical_arena = state->logical_arena;
        (void)cx_gc_store_native_pointer(
            (cx_ptr *)&state->logical_arena, NULL);
        if (logical_arena != NULL) cx_arena_release_logical(logical_arena);
        cx_threading_raise(&cx_thread_start_failed);
    }
    while (!state->attached) cx_gc_runtime_wait();
    cx_gc_runtime_unlock();
    return (cx_thread){state};
}

void cxcore_System_Threading_Thread___constructor(cx_thread *self) {
    (void)cx_gc_store_native_pointer(&self->_state, NULL);
}

static struct cx_thread_state *cx_thread_require(const cx_thread *self) {
    if (self == NULL || self->_state == NULL) cx_threading_raise(&cx_thread_invalid);
    return (struct cx_thread_state *)self->_state;
}

void cxcore_System_Threading_Thread_Join(const cx_thread *self) {
    struct cx_thread_state *state = cx_thread_require(self);
    struct cx_exception failure;
    cx_gc_runtime_lock();
    if (state->id == cx_gc_runtime_thread_id()) {
        cx_gc_runtime_unlock();
        cx_threading_raise(&cx_thread_invalid);
        return;
    }
    if (!state->complete && state->logical_arena != NULL &&
        cx_arena_current_is(state->logical_arena)) {
        cx_gc_runtime_unlock();
        cx_threading_raise(&cx_thread_join_active_arena);
    }
    while (!state->complete) cx_gc_runtime_wait();
    failure = state->failure;
    cx_thread_unlink(state);
    cx_gc_runtime_unlock();
    if (failure.exceptionObject != NULL) {
        cx_exception_throw(failure.exceptionObject, failure.thrownFileName, failure.thrownLine);
    }
}

cx_bool cxcore_System_Threading_Thread_IsAlive___const_get(const cx_thread *self) {
    struct cx_thread_state *state = cx_thread_require(self);
    cx_bool result;
    cx_gc_runtime_lock();
    result = !state->complete;
    cx_gc_runtime_unlock();
    return result;
}

cx_ulong cxcore_System_Threading_Thread_Id___const_get(const cx_thread *self) {
    struct cx_thread_state *state = cx_thread_require(self);
    cx_ulong result;
    cx_gc_runtime_lock();
    result = state->id;
    cx_gc_runtime_unlock();
    return result;
}

cx_ulong cxcore_System_Threading_Thread_CurrentId___const_get(void) {
    cx_ulong result;
    cx_gc_runtime_lock();
    result = cx_gc_runtime_thread_id();
    cx_gc_runtime_unlock();
    return result;
}

void cx_thread_wait_all(void) {
    struct cx_exception failure = {0};
    cx_gc_runtime_lock();
    while (cx_active_threads != 0) cx_gc_runtime_wait();
    if (cx_thread_states != NULL) {
        struct cx_thread_state *state = cx_thread_states;
        failure = state->failure;
        cx_thread_unlink(state);
    }
    cx_gc_runtime_unlock();
    if (failure.exceptionObject != NULL) {
        cx_exception_throw(failure.exceptionObject, failure.thrownFileName, failure.thrownLine);
    }
}

struct cx_sleep_context { cx_uint milliseconds; };
static CX_THREAD_LOCAL struct cx_sleep_context cx_sleep_state;
static void cx_thread_native_sleep(void *context) {
    cx_uint milliseconds = ((struct cx_sleep_context *)context)->milliseconds;
    cx_platform_thread_sleep(milliseconds);
}
void cxcore_System_Threading_Thread_Sleep(cx_int milliseconds) {
    if (milliseconds < 0) cx_threading_raise(&cx_thread_invalid_sleep);
    cx_sleep_state.milliseconds = (cx_uint)milliseconds;
    cx_gc_run_blocking(cx_thread_native_sleep, &cx_sleep_state);
}
void cxcore_System_Threading_Thread_Yield(void) {
    cx_gc_safepoint();
    cx_platform_thread_yield();
}

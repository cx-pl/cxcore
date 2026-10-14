#include "Threading.internal.h"
#include "Thread.internal.h"
#include "../../../platform.h"

typedef struct CX_ID_4(cxcore, System, Threading, Thread) cx_thread;

struct cx_thread_state {
    cx_action action;
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
CX_STRING_DEF(cx_thread_invalid_sleep, "Sleep duration must be non-negative");

// Runtime-owned metadata is read/written only under the collector mutex. Waits
// park through cx_gc_runtime_wait; native handles are detached immediately.
static void cx_thread_unlink(struct cx_thread_state *state) {
    struct cx_thread_state **link;
    for (link = &cx_thread_states; *link != NULL; link = &(*link)->next) {
        if (*link == state) {
            *link = state->next;
            state->next = NULL;
            return;
        }
    }
}

void cx_thread_worker(void *context) {
    struct cx_thread_state *state = context;
    struct cx_exception_frame frame;
    cx_gc_attach_thread();
    cx_gc_runtime_lock();
    state->id = cx_gc_runtime_thread_id();
    state->attached = CX_TRUE;
    cx_gc_runtime_wake();
    cx_gc_runtime_unlock();
    cx_exception_push(&frame);
    if (setjmp(frame.environment) == 0) {
        if (state->action.__invocation_list != NULL) {
            struct CX_FN_136FFEE706DF35B2_invocation *item;
            for (item = state->action.__invocation_list; item != NULL; item = item->next) {
                cx_gc_safepoint();
                item->value.__function(item->value.__environment);
            }
        } else {
            state->action.__function(state->action.__environment);
        }
    } else {
        state->failure = *cx_exception_thread_state();
    }
    cx_exception_pop(&frame);
    cx_exception_clear();
    cx_gc_runtime_lock();
    state->action = (cx_action){0};
    state->complete = CX_TRUE;
    --cx_active_threads;
    if (state->failure.exceptionObject == NULL) cx_thread_unlink(state);
    cx_gc_runtime_wake();
    // Unregister atomically with completion publication. No managed access may
    // occur after this point, including in native thread teardown.
    cx_gc_runtime_detach();
    cx_gc_runtime_unlock();
}

cx_thread CX_ID_5(cxcore, System, Threading, Thread, Start)(cx_action action) {
    struct cx_thread_state *state;
    cx_bool started;
    if (action.__function == NULL && action.__invocation_list == NULL) {
        cx_threading_raise(&cx_thread_null_action);
    }
    state = (struct cx_thread_state *)CX_ID_4(cxcore, System, Memory, Alloc)((cx_uint)sizeof(*state));
    state->action = action;
    cx_gc_register_static_roots(cx_thread_roots, 1);
    cx_gc_runtime_lock();
    state->next = cx_thread_states;
    cx_thread_states = state;
    ++cx_active_threads;
    started = cx_platform_thread_start(state) != 0;
    if (!started) {
        --cx_active_threads;
        cx_thread_unlink(state);
        cx_gc_runtime_unlock();
        cx_threading_raise(&cx_thread_start_failed);
        return (cx_thread){NULL};
    }
    while (!state->attached) cx_gc_runtime_wait();
    cx_gc_runtime_unlock();
    return (cx_thread){state};
}

void CX_ID_5(cxcore, System, Threading, Thread, __constructor)(cx_thread *self) {
    self->_state = NULL;
}

static struct cx_thread_state *cx_thread_require(const cx_thread *self) {
    if (self == NULL || self->_state == NULL) cx_threading_raise(&cx_thread_invalid);
    return (struct cx_thread_state *)self->_state;
}

void CX_ID_5(cxcore, System, Threading, Thread, Join)(const cx_thread *self) {
    struct cx_thread_state *state = cx_thread_require(self);
    struct cx_exception failure;
    cx_gc_runtime_lock();
    if (state->id == cx_gc_runtime_thread_id()) {
        cx_gc_runtime_unlock();
        cx_threading_raise(&cx_thread_invalid);
        return;
    }
    while (!state->complete) cx_gc_runtime_wait();
    failure = state->failure;
    cx_thread_unlink(state);
    cx_gc_runtime_unlock();
    if (failure.exceptionObject != NULL) {
        cx_exception_throw(failure.exceptionObject, failure.thrownFileName, failure.thrownLine);
    }
}

cx_bool CX_ID_6(cxcore, System, Threading, Thread, IsAlive, __const_get)(const cx_thread *self) {
    struct cx_thread_state *state = cx_thread_require(self);
    cx_bool result;
    cx_gc_runtime_lock();
    result = !state->complete;
    cx_gc_runtime_unlock();
    return result;
}

cx_ulong CX_ID_6(cxcore, System, Threading, Thread, Id, __const_get)(const cx_thread *self) {
    struct cx_thread_state *state = cx_thread_require(self);
    cx_ulong result;
    cx_gc_runtime_lock();
    result = state->id;
    cx_gc_runtime_unlock();
    return result;
}

cx_ulong CX_ID_6(cxcore, System, Threading, Thread, CurrentId, __const_get)(void) {
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
void CX_ID_5(cxcore, System, Threading, Thread, Sleep)(cx_int milliseconds) {
    if (milliseconds < 0) cx_threading_raise(&cx_thread_invalid_sleep);
    cx_sleep_state.milliseconds = (cx_uint)milliseconds;
    cx_gc_run_blocking(cx_thread_native_sleep, &cx_sleep_state);
}
void CX_ID_5(cxcore, System, Threading, Thread, Yield)(void) {
    cx_gc_safepoint();
    cx_platform_thread_yield();
}

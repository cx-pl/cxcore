#include <limits.h>
#include "Threading.internal.h"

typedef struct cxcore_System_Threading_Mutex cx_mutex;
struct cx_mutex_state { cx_ulong owner; cx_uint recursion; };

CX_STRING_DEF(cx_mutex_invalid, "Mutex is uninitialized or is not owned by this thread");
CX_STRING_DEF(cx_mutex_overflow, "Mutex recursion count overflow");

void cxcore_System_Threading_Mutex___constructor(cx_mutex *self) {
    void *state = cx_gc_alloc_for_owner(
        self, sizeof(struct cx_mutex_state), cx_arena_maximum_alignment());
    (void)cx_gc_store_native_pointer(&self->_state, state);
}
static struct cx_mutex_state *cx_mutex_require(const cx_mutex *self) {
    if (self == NULL || self->_state == NULL) cx_threading_raise(&cx_mutex_invalid);
    return (struct cx_mutex_state *)self->_state;
}
void cxcore_System_Threading_Mutex_Lock(const cx_mutex *self) {
    struct cx_mutex_state *state = cx_mutex_require(self);
    cx_ulong id;
    cx_gc_runtime_lock();
    id = cx_gc_runtime_thread_id();
    while (state->owner != 0 && state->owner != id) cx_gc_runtime_wait();
    if (state->recursion == UINT_MAX) {
        cx_gc_runtime_unlock();
        cx_threading_raise(&cx_mutex_overflow);
        return;
    }
    state->owner = id;
    ++state->recursion;
    cx_gc_runtime_mutex_acquired();
    cx_gc_runtime_unlock();
}
void cxcore_System_Threading_Mutex_Unlock(const cx_mutex *self) {
    struct cx_mutex_state *state = cx_mutex_require(self);
    cx_gc_runtime_lock();
    if (state->owner != cx_gc_runtime_thread_id() || state->recursion == 0) {
        cx_gc_runtime_unlock();
        cx_threading_raise(&cx_mutex_invalid);
        return;
    }
    --state->recursion;
    cx_gc_runtime_mutex_released();
    if (state->recursion == 0) {
        state->owner = 0;
        cx_gc_runtime_wake();
    }
    cx_gc_runtime_unlock();
}

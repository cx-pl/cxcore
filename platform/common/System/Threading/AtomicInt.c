#include <limits.h>
#include "Threading.internal.h"

typedef struct cxcore_System_Threading_AtomicInt cx_atomic_int;
struct cx_atomic_int_state { cx_int value; };

CX_STRING_DEF(cx_atomic_invalid, "AtomicInt is uninitialized");

void cxcore_System_Threading_AtomicInt___constructor(cx_atomic_int *self, cx_int value) {
    struct cx_atomic_int_state *state = (struct cx_atomic_int_state *)cx_gc_alloc_for_owner(
        self, sizeof(*state), cx_arena_maximum_alignment());
    state->value = value;
    (void)cx_gc_store_native_pointer(&self->_state, state);
}
void cxcore_System_Threading_AtomicInt___constructor__2(cx_atomic_int *self) {
    cxcore_System_Threading_AtomicInt___constructor(self, 0);
}
static struct cx_atomic_int_state *cx_atomic_require(const cx_atomic_int *self) {
    if (self == NULL || self->_state == NULL) cx_threading_raise(&cx_atomic_invalid);
    return (struct cx_atomic_int_state *)self->_state;
}
cx_int cxcore_System_Threading_AtomicInt_Value___const_get(const cx_atomic_int *self) {
    struct cx_atomic_int_state *state = cx_atomic_require(self);
    cx_int result;
    cx_gc_runtime_lock();
    result = state->value;
    cx_gc_runtime_unlock();
    return result;
}
void cxcore_System_Threading_AtomicInt_Value___set(cx_atomic_int *self, cx_int value) {
    (void)cxcore_System_Threading_AtomicInt_Exchange(self, value);
}
static cx_int cx_atomic_adjust(const cx_atomic_int *self, cx_uint delta) {
    struct cx_atomic_int_state *state = cx_atomic_require(self);
    cx_int result;
    cx_gc_runtime_lock();
    state->value = (cx_int)((cx_uint)state->value + delta);
    result = state->value;
    cx_gc_runtime_unlock();
    return result;
}
cx_int cxcore_System_Threading_AtomicInt_Increment(const cx_atomic_int *self) {
    return cx_atomic_adjust(self, 1);
}
cx_int cxcore_System_Threading_AtomicInt_Decrement(const cx_atomic_int *self) {
    return cx_atomic_adjust(self, UINT_MAX);
}
cx_int cxcore_System_Threading_AtomicInt_Exchange(const cx_atomic_int *self, cx_int value) {
    struct cx_atomic_int_state *state = cx_atomic_require(self);
    cx_int result;
    cx_gc_runtime_lock();
    result = state->value;
    state->value = value;
    cx_gc_runtime_unlock();
    return result;
}
cx_int cxcore_System_Threading_AtomicInt_CompareExchange(
    const cx_atomic_int *self, cx_int value, cx_int comparand) {
    struct cx_atomic_int_state *state = cx_atomic_require(self);
    cx_int result;
    cx_gc_runtime_lock();
    result = state->value;
    if (result == comparand) state->value = value;
    cx_gc_runtime_unlock();
    return result;
}

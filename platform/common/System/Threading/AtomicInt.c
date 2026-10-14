#include <limits.h>
#include "Threading.internal.h"

typedef struct CX_ID_4(cxcore, System, Threading, AtomicInt) cx_atomic_int;
struct cx_atomic_int_state { cx_int value; };

CX_STRING_DEF(cx_atomic_invalid, "AtomicInt is uninitialized");

void CX_ID_5(cxcore, System, Threading, AtomicInt, __constructor)(cx_atomic_int *self, cx_int value) {
    struct cx_atomic_int_state *state = (struct cx_atomic_int_state *)CX_ID_4(
        cxcore, System, Memory, Alloc)((cx_uint)sizeof(*state));
    state->value = value;
    self->_state = state;
}
void CX_ID_6(cxcore, System, Threading, AtomicInt, __constructor, _2)(cx_atomic_int *self) {
    CX_ID_5(cxcore, System, Threading, AtomicInt, __constructor)(self, 0);
}
static struct cx_atomic_int_state *cx_atomic_require(const cx_atomic_int *self) {
    if (self == NULL || self->_state == NULL) cx_threading_raise(&cx_atomic_invalid);
    return (struct cx_atomic_int_state *)self->_state;
}
cx_int CX_ID_6(cxcore, System, Threading, AtomicInt, Value, __const_get)(const cx_atomic_int *self) {
    struct cx_atomic_int_state *state = cx_atomic_require(self);
    cx_int result;
    cx_gc_runtime_lock();
    result = state->value;
    cx_gc_runtime_unlock();
    return result;
}
void CX_ID_6(cxcore, System, Threading, AtomicInt, Value, __set)(cx_atomic_int *self, cx_int value) {
    (void)CX_ID_5(cxcore, System, Threading, AtomicInt, Exchange)(self, value);
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
cx_int CX_ID_5(cxcore, System, Threading, AtomicInt, Increment)(const cx_atomic_int *self) {
    return cx_atomic_adjust(self, 1);
}
cx_int CX_ID_5(cxcore, System, Threading, AtomicInt, Decrement)(const cx_atomic_int *self) {
    return cx_atomic_adjust(self, UINT_MAX);
}
cx_int CX_ID_5(cxcore, System, Threading, AtomicInt, Exchange)(const cx_atomic_int *self, cx_int value) {
    struct cx_atomic_int_state *state = cx_atomic_require(self);
    cx_int result;
    cx_gc_runtime_lock();
    result = state->value;
    state->value = value;
    cx_gc_runtime_unlock();
    return result;
}
cx_int CX_ID_5(cxcore, System, Threading, AtomicInt, CompareExchange)(
    const cx_atomic_int *self, cx_int value, cx_int comparand) {
    struct cx_atomic_int_state *state = cx_atomic_require(self);
    cx_int result;
    cx_gc_runtime_lock();
    result = state->value;
    if (result == comparand) state->value = value;
    cx_gc_runtime_unlock();
    return result;
}

#ifndef CX_THREADING_H
#define CX_THREADING_H

#include "cx.h"

CX_TYPE_DEF(CX_ID_4(cxcore, System, Threading, Thread)) { cx_ptr _state; };
CX_TYPE_DEF(CX_ID_4(cxcore, System, Threading, Mutex)) { cx_ptr _state; };
CX_TYPE_DEF(CX_ID_4(cxcore, System, Threading, AtomicInt)) { cx_ptr _state; };

CX_API struct CX_ID_4(cxcore, System, Threading, Thread)
CX_ID_5(cxcore, System, Threading, Thread, Start)(cx_action action);
CX_API void CX_ID_5(cxcore, System, Threading, Thread, __constructor)(
    struct CX_ID_4(cxcore, System, Threading, Thread) *self);
CX_API void CX_ID_5(cxcore, System, Threading, Thread, Join)(
    const struct CX_ID_4(cxcore, System, Threading, Thread) *self);
CX_API cx_bool CX_ID_6(cxcore, System, Threading, Thread, IsAlive, __const_get)(
    const struct CX_ID_4(cxcore, System, Threading, Thread) *self);
CX_API cx_ulong CX_ID_6(cxcore, System, Threading, Thread, Id, __const_get)(
    const struct CX_ID_4(cxcore, System, Threading, Thread) *self);
CX_API cx_ulong CX_ID_6(cxcore, System, Threading, Thread, CurrentId, __const_get)(void);
CX_API void CX_ID_5(cxcore, System, Threading, Thread, Sleep)(cx_int milliseconds);
CX_API void CX_ID_5(cxcore, System, Threading, Thread, Yield)(void);

CX_API void CX_ID_5(cxcore, System, Threading, Mutex, __constructor)(
    struct CX_ID_4(cxcore, System, Threading, Mutex) *self);
CX_API void CX_ID_5(cxcore, System, Threading, Mutex, Lock)(
    const struct CX_ID_4(cxcore, System, Threading, Mutex) *self);
CX_API void CX_ID_5(cxcore, System, Threading, Mutex, Unlock)(
    const struct CX_ID_4(cxcore, System, Threading, Mutex) *self);

CX_API void CX_ID_5(cxcore, System, Threading, AtomicInt, __constructor)(
    struct CX_ID_4(cxcore, System, Threading, AtomicInt) *self, cx_int value);
CX_API void CX_ID_6(cxcore, System, Threading, AtomicInt, __constructor, _2)(
    struct CX_ID_4(cxcore, System, Threading, AtomicInt) *self);
CX_API cx_int CX_ID_6(cxcore, System, Threading, AtomicInt, Value, __const_get)(
    const struct CX_ID_4(cxcore, System, Threading, AtomicInt) *self);
CX_API void CX_ID_6(cxcore, System, Threading, AtomicInt, Value, __set)(
    struct CX_ID_4(cxcore, System, Threading, AtomicInt) *self, cx_int value);
CX_API cx_int CX_ID_5(cxcore, System, Threading, AtomicInt, Increment)(
    const struct CX_ID_4(cxcore, System, Threading, AtomicInt) *self);
CX_API cx_int CX_ID_5(cxcore, System, Threading, AtomicInt, Decrement)(
    const struct CX_ID_4(cxcore, System, Threading, AtomicInt) *self);
CX_API cx_int CX_ID_5(cxcore, System, Threading, AtomicInt, Exchange)(
    const struct CX_ID_4(cxcore, System, Threading, AtomicInt) *self, cx_int value);
CX_API cx_int CX_ID_5(cxcore, System, Threading, AtomicInt, CompareExchange)(
    const struct CX_ID_4(cxcore, System, Threading, AtomicInt) *self, cx_int value, cx_int comparand);

#endif

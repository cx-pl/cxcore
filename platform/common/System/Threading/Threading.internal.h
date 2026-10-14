#ifndef CX_THREADING_INTERNAL_H
#define CX_THREADING_INTERNAL_H

#include "cxcore.h"
#include "cxgc.h"

static inline void cx_threading_raise(const struct CX_ID_3(cxcore, System, String) *message) {
    struct CX_ID_3(cxcore, System, Exception) *exception =
        (struct CX_ID_3(cxcore, System, Exception) *)CX_ID_4(cxcore, System, Memory, Alloc)(
            (cx_uint)sizeof(*exception));
    CX_ID_4(cxcore, System, Exception, __constructor)(exception, message);
    cx_exception_throw(exception, __FILE__, __LINE__);
}

#endif

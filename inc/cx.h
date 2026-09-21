#ifndef __CX_H__
#define __CX_H__

#include "cxdefs.h"
#include "cxtypes.h"

struct CX_ID_3(cxcore, System, Array);
struct CX_ID_4(cxcore, System, Reflection, TypeInfo);

CX_EXPORT struct CX_ID_3(cxcore, System, Array)* cx_array_new(
    cx_uint length,
    cx_uint elementSize);
CX_EXPORT cx_ptr cx_array_at(
    struct CX_ID_3(cxcore, System, Array)* array,
    cx_uint index,
    cx_uint elementSize);

CX_EXPORT cx_ptr cx_nullable_new(cx_ptr value, cx_uint size);

CX_EXPORT void cx_exception_push(struct cx_exception_frame* frame);
CX_EXPORT void cx_exception_pop(struct cx_exception_frame* frame);
CX_EXPORT void cx_exception_throw(cx_ptr exceptionObject, const char* fileName, int line);
CX_EXPORT void cx_exception_rethrow(void);
CX_EXPORT cx_ptr cx_exception_current(void);
CX_EXPORT cx_bool cx_exception_pending(void);
CX_EXPORT void cx_exception_clear(void);
CX_EXPORT cx_bool cx_exception_matches(
    const struct CX_ID_4(cxcore, System, Reflection, TypeInfo)* typeInfo);

#endif // __CX_H__

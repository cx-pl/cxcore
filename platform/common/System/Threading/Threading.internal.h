#ifndef __CX_THREADING_INTERNAL_H__
#define __CX_THREADING_INTERNAL_H__

#include "cxcore.h"
#include "cxgc.h"
#include "../../cxarena.h"

static inline void cx_threading_raise(const struct cxcore_System_String *message) {
    cx_ptr exception = cx_exception_allocate_durable(
        cxcore_System_InvalidOperationException___typeinfo.Size);
    cxcore_System_InvalidOperationException___constructor__2(
        (struct cxcore_System_InvalidOperationException *)exception, message);
    cx_exception_throw(exception, __FILE__, __LINE__);
}

static inline void cx_threading_raise_invalid_operation(
    const struct cxcore_System_String *message) {
    cx_threading_raise(message);
}

static inline void cx_threading_raise_argument_null(
    const struct cxcore_System_String *message,
    const struct cxcore_System_String *paramName) {
    cx_ptr exception = cx_exception_allocate_durable(
        cxcore_System_ArgumentNullException___typeinfo.Size);
    cxcore_System_ArgumentNullException___constructor__3(
        (struct cxcore_System_ArgumentNullException *)exception,
        message, paramName);
    cx_exception_throw(exception, __FILE__, __LINE__);
}

static inline void cx_threading_raise_argument_out_of_range(
    const struct cxcore_System_String *message,
    const struct cxcore_System_String *paramName) {
    cx_ptr exception = cx_exception_allocate_durable(
        cxcore_System_ArgumentOutOfRangeException___typeinfo.Size);
    cxcore_System_ArgumentOutOfRangeException___constructor__3(
        (struct cxcore_System_ArgumentOutOfRangeException *)exception,
        message, paramName);
    cx_exception_throw(exception, __FILE__, __LINE__);
}

void cx_task_report_unobserved_faults(void);

#endif // __CX_THREADING_INTERNAL_H__

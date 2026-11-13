#ifndef __CX_THREADING_INTERNAL_H__
#define __CX_THREADING_INTERNAL_H__

#include "cxcore.h"
#include "cxgc.h"
#include "../../cxarena.h"

static inline void cx_threading_raise(const struct cxcore_System_String *message) {
    cx_exception_raise_message(message, __FILE__, __LINE__);
}

void cx_task_report_unobserved_faults(void);

#endif // __CX_THREADING_INTERNAL_H__

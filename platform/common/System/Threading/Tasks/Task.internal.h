#ifndef __CX_TASK_INTERNAL_H__
#define __CX_TASK_INTERNAL_H__

#include "cxthreading.h"

CX_API void cx_task_state_on_completed_with_acknowledgement(
    struct cxcore_System_Threading_Tasks_TaskState *self,
    cx_action continuation,
    void (*completion)(void *environment),
    void *completion_environment);

#endif // __CX_TASK_INTERNAL_H__

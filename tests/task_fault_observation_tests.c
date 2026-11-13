#include <stdio.h>
#include <string.h>

#include "../src/cxcore.h"

static int expect_task_failure(
    struct cxcore_System_Threading_Tasks_TaskHandle *task,
    struct cxcore_System_Exception *expected) {
    struct cx_exception_frame frame;
    cx_exception_push(&frame);
    if (setjmp(frame.environment) == 0) {
        cxcore_System_Threading_Tasks_TaskHandle_GetResult(task);
        cx_exception_pop(&frame);
        return 0;
    }
    cx_exception_pop(&frame);
    if (cx_exception_current() != expected) {
        cx_exception_clear();
        return 0;
    }
    cx_exception_clear();
    return 1;
}

static int expect_shutdown_failure(
    struct cxcore_System_Exception *expected) {
    struct cx_exception_frame frame;
    cx_exception_push(&frame);
    if (setjmp(frame.environment) == 0) {
        cxcore_System_Threading_Tasks_TaskScheduler_Shutdown();
        cx_exception_pop(&frame);
        return 0;
    }
    cx_exception_pop(&frame);
    if (cx_exception_current() != expected) {
        cx_exception_clear();
        return 0;
    }
    cx_exception_clear();
    return 1;
}

int main(int argc, char **argv) {
    struct cxcore_System_Exception *exception;
    struct cxcore_System_Threading_Tasks_TaskCompletionSourceVoid *source;
    struct cxcore_System_Threading_Tasks_TaskHandle *task;
    struct cxcore_System_Threading_Tasks_TaskCompletionSourceVoid *destination_source;
    struct cxcore_System_Threading_Tasks_TaskHandle *destination_task;
    int observed_case;
    int forwarded_case;
    cx_gc_check_thread();
    if (argc != 2 ||
        (strcmp(argv[1], "observed") != 0 && strcmp(argv[1], "unobserved") != 0 &&
            strcmp(argv[1], "forwarded-observed") != 0 &&
            strcmp(argv[1], "forwarded-unobserved") != 0)) {
        fputs("expected observed, unobserved, forwarded-observed, or forwarded-unobserved mode\n", stderr);
        return 2;
    }
    observed_case = strcmp(argv[1], "observed") == 0 ||
        strcmp(argv[1], "forwarded-observed") == 0;
    forwarded_case = strncmp(argv[1], "forwarded-", 9) == 0;
    exception = (struct cxcore_System_Exception *)cxcore_System_Memory_Alloc((cx_uint)sizeof(*exception));
    cxcore_System_Exception___constructor(exception, NULL);
    source = (struct cxcore_System_Threading_Tasks_TaskCompletionSourceVoid *)cxcore_System_Memory_Alloc((cx_uint)sizeof(*source));
    cxcore_System_Threading_Tasks_TaskCompletionSourceVoid___constructor(source);
    task = cxcore_System_Threading_Tasks_TaskCompletionSourceVoid_Task___const_get(source);
    if (forwarded_case) {
        destination_source = (struct cxcore_System_Threading_Tasks_TaskCompletionSourceVoid *)cxcore_System_Memory_Alloc((cx_uint)sizeof(*destination_source));
        cxcore_System_Threading_Tasks_TaskCompletionSourceVoid___constructor(destination_source);
        destination_task = cxcore_System_Threading_Tasks_TaskCompletionSourceVoid_Task___const_get(destination_source);
        cxcore_System_Threading_Tasks_TaskRuntime_ForwardVoid(
            (struct cxcore_System_Object *)task,
            (struct cxcore_System_Object *)destination_source);
        cxcore_System_Threading_Tasks_TaskCompletionSourceVoid_SetException(source, exception);
        cxcore_System_Threading_Tasks_TaskScheduler_Run();
        task = destination_task;
    } else {
        cxcore_System_Threading_Tasks_TaskCompletionSourceVoid_SetException(
            source, exception);
    }

    if (observed_case) {
        if (!expect_task_failure(task, exception)) {
            fputs("GetResult did not throw the original task exception\n", stderr);
            return 1;
        }
        if (forwarded_case) {
            cxcore_System_Threading_Tasks_TaskScheduler_Shutdown();
            if (cx_exception_pending()) {
                fputs("observed forwarded task fault was reported during shutdown\n", stderr);
                return 1;
            }
            return 0;
        }
        cxcore_System_Threading_Tasks_TaskScheduler_Shutdown();
        return cx_exception_pending() ? 1 : 0;
    }

    if (!expect_shutdown_failure(exception)) {
        fputs("shutdown did not report the original unobserved task exception\n", stderr);
        return 1;
    }
    return 0;
}

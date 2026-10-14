#include "cxcore.h"
#include "cxgc.h"
#include "../platform.h"

#include <stdio.h>
#include <stdlib.h>

static CX_THREAD_LOCAL struct cx_exception_frame *cx_current_exception_frame;
static CX_THREAD_LOCAL struct cx_exception cx_current_exception;

cx_ptr *cx_exception_thread_root(void) {
    return &cx_current_exception.exceptionObject;
}

const struct cx_exception *cx_exception_thread_state(void) {
    return &cx_current_exception;
}

void cx_exception_restore(const struct cx_exception *state) {
    cx_current_exception = *state;
}

cx_bool cx_exception_thread_detach(void) {
    if (cx_current_exception_frame != NULL) {
        return CX_FALSE;
    }
    cx_current_exception = (struct cx_exception){0};
    return CX_TRUE;
}

static CX_NORETURN void cx_exception_dispatch(void) {
    if (cx_current_exception_frame != CX_NULL) {
        cx_platform_exception_jump(cx_current_exception_frame->environment);
    }

    fprintf(stderr, "Uncaught exception thrown at %s:%d\n",
            cx_current_exception.thrownFileName == CX_NULL ? "<unknown>"
                                                           : cx_current_exception.thrownFileName,
            cx_current_exception.thrownLine);
    exit(EXIT_FAILURE);
}
void cx_exception_push(struct cx_exception_frame *frame) {
    cx_gc_check_thread();
    frame->previous = cx_current_exception_frame;
    cx_current_exception_frame = frame;
}

void cx_exception_pop(struct cx_exception_frame *frame) {
    cx_gc_check_thread();
    if (cx_current_exception_frame != frame) {
        fputs("Invalid cx exception handler stack\n", stderr);
        abort();
    }
    cx_current_exception_frame = frame->previous;
}

void cx_exception_throw(cx_ptr exceptionObject, const char *fileName, int line) {
    cx_gc_check_thread();
    if (exceptionObject == CX_NULL) {
        fputs("Cannot throw a null exception\n", stderr);
        abort();
    }
    cx_current_exception.exceptionObject = exceptionObject;
    cx_current_exception.thrownFileName = fileName;
    cx_current_exception.thrownLine = line;
    cx_exception_dispatch();
}

void cx_exception_rethrow(void) {
    cx_gc_check_thread();
    if (cx_current_exception.exceptionObject == CX_NULL) {
        fputs("Cannot rethrow without an active exception\n", stderr);
        abort();
    }
    cx_exception_dispatch();
}

cx_ptr cx_exception_current(void) {
    cx_gc_check_thread();
    return cx_current_exception.exceptionObject;
}

cx_bool cx_exception_pending(void) {
    cx_gc_check_thread();
    return cx_current_exception.exceptionObject != CX_NULL;
}

void cx_exception_clear(void) {
    cx_gc_check_thread();
    cx_current_exception.exceptionObject = CX_NULL;
    cx_current_exception.thrownFileName = CX_NULL;
    cx_current_exception.thrownLine = 0;
}

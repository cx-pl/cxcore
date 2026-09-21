#include "cxcore.h"

#include <stdio.h>
#include <stdlib.h>

#if defined(_MSC_VER)
#define CX_THREAD_LOCAL __declspec(thread)
#else
#define CX_THREAD_LOCAL _Thread_local
#endif

static CX_THREAD_LOCAL struct cx_exception_frame* cx_current_exception_frame;
static CX_THREAD_LOCAL struct cx_exception cx_current_exception;

static void cx_exception_dispatch(void)
{
    if (cx_current_exception_frame != CX_NULL)
    {
        longjmp(cx_current_exception_frame->environment, 1);
    }

    fprintf(
        stderr,
        "Uncaught exception thrown at %s:%d\n",
        cx_current_exception.thrownFileName == CX_NULL
            ? "<unknown>"
            : cx_current_exception.thrownFileName,
        cx_current_exception.thrownLine);
    abort();
}

void cx_exception_push(struct cx_exception_frame* frame)
{
    frame->previous = cx_current_exception_frame;
    cx_current_exception_frame = frame;
}

void cx_exception_pop(struct cx_exception_frame* frame)
{
    if (cx_current_exception_frame != frame)
    {
        fputs("Invalid cx exception handler stack\n", stderr);
        abort();
    }
    cx_current_exception_frame = frame->previous;
}

void cx_exception_throw(cx_ptr exceptionObject, const char* fileName, int line)
{
    if (exceptionObject == CX_NULL)
    {
        fputs("Cannot throw a null exception\n", stderr);
        abort();
    }
    cx_current_exception.exceptionObject = exceptionObject;
    cx_current_exception.thrownFileName = fileName;
    cx_current_exception.thrownLine = line;
    cx_exception_dispatch();
}

void cx_exception_rethrow(void)
{
    if (cx_current_exception.exceptionObject == CX_NULL)
    {
        fputs("Cannot rethrow without an active exception\n", stderr);
        abort();
    }
    cx_exception_dispatch();
}

cx_ptr cx_exception_current(void)
{
    return cx_current_exception.exceptionObject;
}

cx_bool cx_exception_pending(void)
{
    return cx_current_exception.exceptionObject != CX_NULL;
}

void cx_exception_clear(void)
{
    cx_current_exception.exceptionObject = CX_NULL;
    cx_current_exception.thrownFileName = CX_NULL;
    cx_current_exception.thrownLine = 0;
}

cx_bool cx_exception_matches(
    const struct CX_ID_4(cxcore, System, Reflection, TypeInfo)* typeInfo)
{
    const struct CX_ID_4(cxcore, System, Reflection, TypeInfo)* currentType;
    if (cx_current_exception.exceptionObject == CX_NULL)
    {
        return CX_FALSE;
    }

    currentType = &CX_GET_TYPEINFO(cx_current_exception.exceptionObject);
    while (currentType != CX_NULL)
    {
        if (currentType == typeInfo)
        {
            return CX_TRUE;
        }
        currentType = currentType->BaseType._obj;
    }
    return CX_FALSE;
}

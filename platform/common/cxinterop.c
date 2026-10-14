#include "cxcore.h"
#include "cxgc.h"
#include "../platform.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static CX_THREAD_LOCAL cx_uint cx_saved_last_error;
static CX_THREAD_LOCAL cx_int cx_saved_errno;

void cx_interop_capture_last_error(void) {
    cx_saved_last_error = cx_platform_last_error();
}

void cx_interop_capture_errno(void) { cx_saved_errno = errno; }
cx_uint cx_interop_last_error(void) { return cx_saved_last_error; }
cx_int cx_interop_errno(void) { return cx_saved_errno; }

CX_STRING_DEF(cx_native_null_function, "Native function symbol is null");
CX_STRING_DEF(cx_native_context_failure, "Unable to allocate native call context");
CX_STRING_DEF(cx_native_stack_failure, "Native stack allocation count is invalid or exceeds 1 MiB");

void cx_interop_require_function(cx_bool nonnull) {
    if (!nonnull) {
        struct CX_ID_3(cxcore, System, Exception) *error = CX_ID_4(cxcore, System, Memory, Alloc)((cx_uint)sizeof(*error));
        CX_ID_4(cxcore, System, Exception, __constructor)(error, &cx_native_null_function);
        cx_exception_throw(error, __FILE__, __LINE__);
    }
}

void cx_interop_symbol_require(const void *symbol, size_t size) {
    uintptr_t address = 0;
    cx_interop_require_function(symbol != NULL && size == sizeof(address));
    memcpy(&address, symbol, size);
    // Supported Win32/Win64/Linux-x64 procedure representations; also reject
    // WGL's documented failure sentinel values before any typed call.
    cx_interop_require_function(address > 3 && address != UINTPTR_MAX);
}

void cx_interop_callback_enter(struct cx_native_callback_scope *scope) {
    scope->saved_errno = errno;
    scope->saved_last_error = cx_platform_last_error();
    scope->resumed = cx_gc_resume_native_callback();
    scope->attached = cx_gc_enter_thread();
    scope->previous_exception = *cx_exception_thread_state();
}

void cx_interop_callback_leave(struct cx_native_callback_scope *scope) {
    cx_exception_restore(&scope->previous_exception);
    if (scope->resumed) cx_gc_park_native_callback();
    else cx_gc_leave_thread(scope->attached);
    cx_platform_set_last_error(scope->saved_last_error);
    errno = scope->saved_errno;
}

void *cx_interop_context_alloc(size_t size) {
    void *context = malloc(size);
    if (context == NULL) {
        struct CX_ID_3(cxcore, System, Exception) *error = CX_ID_4(cxcore, System, Memory, Alloc)((cx_uint)sizeof(*error));
        CX_ID_4(cxcore, System, Exception, __constructor)(error, &cx_native_context_failure);
        cx_exception_throw(error, __FILE__, __LINE__);
    }
    return context;
}

cx_ulong cx_interop_stack_size(cx_long count, cx_ulong elementSize) {
    if (count < 0 || elementSize == 0 || (cx_ulong)count > 1024u * 1024u / elementSize) {
        struct CX_ID_3(cxcore, System, Exception) *error = CX_ID_4(cxcore, System, Memory, Alloc)((cx_uint)sizeof(*error));
        CX_ID_4(cxcore, System, Exception, __constructor)(error, &cx_native_stack_failure);
        cx_exception_throw(error, __FILE__, __LINE__);
    }
    return count == 0 ? 1 : (cx_ulong)count * elementSize;
}

void *cx_interop_context_alloc_aligned(size_t size, size_t alignment) {
    void *allocation;
    void *context;
    if (alignment < sizeof(void *)) alignment = sizeof(void *);
    cx_interop_require_function(size <= SIZE_MAX - alignment - sizeof(void *));
    allocation = cx_interop_context_alloc(size + alignment - 1 + sizeof(void *));
    context = cx_interop_align_pointer((char *)allocation + sizeof(void *), alignment);
    ((void **)context)[-1] = allocation;
    return context;
}
void cx_interop_context_free_aligned(void *context) { free(((void **)context)[-1]); }

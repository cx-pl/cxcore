#include "cxcore.h"
#include "../../../platform.h"
#include "../../cxarena.h"
#include <stdlib.h>
#include <string.h>

typedef struct cxcore_System_Interop_NativeLibrary cx_native_library;
typedef struct cxcore_System_Interop_NativeBuffer cx_native_buffer;
typedef struct cxcore_System_String cx_string;
CX_STRING_DEF(cx_library_failed, "Unable to load native library: ");
CX_STRING_DEF(cx_symbol_failed, "Unable to resolve native symbol: ");
CX_STRING_DEF(cx_library_invalid, "Native library/symbol names cannot be null or contain embedded zeros");

static CX_NORETURN void cx_library_error(const cx_string *prefix, const cx_string *name) {
    struct cx_arena_durable_scope scope;
    cx_string *message;
    struct cxcore_System_Exception *error;
    cx_arena_enter_durable(&scope);
    message = name == NULL ? (cx_string *)prefix
        : cxcore_System_String___cx_operator_plus(prefix, name);
    error = (struct cxcore_System_Exception *)cxcore_System_Memory_Alloc(
        (cx_uint)sizeof(*error));
    cxcore_System_Exception___constructor(error, message);
    cx_arena_leave_durable(&scope);
    cx_exception_throw(error, __FILE__, __LINE__);
}

static void cx_library_validate_name(const cx_string *name) {
    if (name == NULL || (name->_length != 0 && memchr(name->_data, 0, name->_length) != NULL))
        cx_library_error(&cx_library_invalid, NULL);
}

struct cx_library_open_context { const void *path; cx_ptr handle; };
static void cx_library_open(void *storage) {
    struct cx_library_open_context *context = storage;
    context->handle = cx_platform_native_library_open(context->path);
}

void cxcore_System_Interop_NativeLibrary___constructor(cx_native_library *self) {
    cxcore_System_Object___constructor(&self->__base);
    CX_INIT_VTABLE(self, cxcore_System_Interop_NativeLibrary);
    (void)cx_gc_store_native_pointer(&self->_handle, NULL);
}

cx_native_library *cxcore_System_Interop_NativeLibrary_Load(cx_string *path) {
    cx_native_buffer *buffer;
    cx_native_library *library;
    struct cx_library_open_context *context;
    struct cx_arena_durable_scope durable_scope;
    cx_ptr handle;
    cx_gc_check_thread(); cx_library_validate_name(path);
    if (cx_platform_native_library_uses_utf16()) {
        buffer = cxcore_System_Interop_NativeBuffer_FromUtf16(
            cxcore_System_String_ToWideString(path), CX_TRUE);
    } else {
        buffer = cxcore_System_Interop_NativeBuffer_FromUtf8(path, CX_TRUE);
    }
    context = cx_interop_context_alloc(sizeof(*context));
    context->path = buffer->_data; context->handle = NULL;
    cx_gc_run_native_call(cx_library_open, context);
    handle = context->handle; free(context);
    cxcore_System_Interop_NativeBuffer_Dispose(buffer);
    if (handle == NULL) cx_library_error(&cx_library_failed, path);
    cx_arena_enter_durable(&durable_scope);
    library = cxcore_System_Memory_Alloc((cx_uint)sizeof(*library));
    cxcore_System_Interop_NativeLibrary___constructor(library);
    (void)cx_gc_store_native_pointer(&library->_handle, handle);
    cx_arena_leave_durable(&durable_scope);
    // No FreeLibrary/dlclose: all resolved addresses remain valid until exit.
    return library;
}

void cx_interop_library_resolve(const cx_native_library *library, const cx_string *name,
    void *result, size_t resultSize) {
    cx_native_buffer *buffer;
    cx_gc_check_thread(); cx_library_validate_name(name);
    if (library == NULL || library->_handle == NULL || result == NULL)
        cx_library_error(&cx_symbol_failed, name);
    buffer = cxcore_System_Interop_NativeBuffer_FromUtf8((cx_string *)name, CX_TRUE);
    if (!cx_platform_native_library_resolve(library->_handle, buffer->_data, result, resultSize)) {
        cxcore_System_Interop_NativeBuffer_Dispose(buffer);
        cx_library_error(&cx_symbol_failed, name);
    }
    cxcore_System_Interop_NativeBuffer_Dispose(buffer);
}

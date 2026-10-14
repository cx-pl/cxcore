#include "cxcore.h"
#include "../../../platform.h"
#include <stdlib.h>
#include <string.h>

typedef struct CX_ID_4(cxcore, System, Interop, NativeLibrary) cx_native_library;
typedef struct CX_ID_4(cxcore, System, Interop, NativeBuffer) cx_native_buffer;
typedef struct CX_ID_3(cxcore, System, String) cx_string;
CX_STRING_DEF(cx_library_failed, "Unable to load native library: ");
CX_STRING_DEF(cx_symbol_failed, "Unable to resolve native symbol: ");
CX_STRING_DEF(cx_library_invalid, "Native library/symbol names cannot be null or contain embedded zeros");

static CX_NORETURN void cx_library_error(const cx_string *prefix, const cx_string *name) {
    cx_string *message = name == NULL ? (cx_string *)prefix
        : CX_ID_4(cxcore, System, String, __cx_operator_plus)(prefix, name);
    struct CX_ID_3(cxcore, System, Exception) *error = CX_ID_4(cxcore, System, Memory, Alloc)((cx_uint)sizeof(*error));
    CX_ID_4(cxcore, System, Exception, __constructor)(error, message);
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

void CX_ID_5(cxcore, System, Interop, NativeLibrary, __constructor)(cx_native_library *self) {
    CX_ID_4(cxcore, System, Object, __constructor)(&self->__base);
    CX_INIT_VTABLE(self, CX_ID_4(cxcore, System, Interop, NativeLibrary));
    self->_handle = NULL;
}

cx_native_library *CX_ID_5(cxcore, System, Interop, NativeLibrary, Load)(cx_string *path) {
    cx_native_buffer *buffer;
    cx_native_library *library;
    struct cx_library_open_context *context;
    cx_ptr handle;
    cx_gc_check_thread(); cx_library_validate_name(path);
    if (cx_platform_native_library_uses_utf16()) {
        buffer = CX_ID_5(cxcore, System, Interop, NativeBuffer, FromUtf16)(
            CX_ID_4(cxcore, System, String, ToWideString)(path), CX_TRUE);
    } else {
        buffer = CX_ID_5(cxcore, System, Interop, NativeBuffer, FromUtf8)(path, CX_TRUE);
    }
    context = cx_interop_context_alloc(sizeof(*context));
    context->path = buffer->_data; context->handle = NULL;
    cx_gc_run_native_call(cx_library_open, context);
    handle = context->handle; free(context);
    CX_ID_5(cxcore, System, Interop, NativeBuffer, Dispose)(buffer);
    if (handle == NULL) cx_library_error(&cx_library_failed, path);
    library = CX_ID_4(cxcore, System, Memory, Alloc)((cx_uint)sizeof(*library));
    CX_ID_5(cxcore, System, Interop, NativeLibrary, __constructor)(library);
    library->_handle = handle;
    // No FreeLibrary/dlclose: all resolved addresses remain valid until exit.
    return library;
}

void cx_interop_library_resolve(const cx_native_library *library, const cx_string *name,
    void *result, size_t resultSize) {
    cx_native_buffer *buffer;
    cx_gc_check_thread(); cx_library_validate_name(name);
    if (library == NULL || library->_handle == NULL || result == NULL)
        cx_library_error(&cx_symbol_failed, name);
    buffer = CX_ID_5(cxcore, System, Interop, NativeBuffer, FromUtf8)((cx_string *)name, CX_TRUE);
    if (!cx_platform_native_library_resolve(library->_handle, buffer->_data, result, resultSize)) {
        CX_ID_5(cxcore, System, Interop, NativeBuffer, Dispose)(buffer);
        cx_library_error(&cx_symbol_failed, name);
    }
    CX_ID_5(cxcore, System, Interop, NativeBuffer, Dispose)(buffer);
}

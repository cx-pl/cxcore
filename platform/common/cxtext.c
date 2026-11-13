#include "System/Text.internal.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern CX_API void cxcore_System_IO_IOException___constructor__5(
    struct cxcore_System_IO_IOException *self,
    const struct cxcore_System_String *message,
    const struct cxcore_System_String *operation,
    const struct cxcore_System_String *path,
    const struct cxcore_System_String *nativeErrorDomain,
    cx_int nativeErrorCode);

static struct cxcore_System_String *cx_text_string(const char *text) {
    size_t length;
    struct cxcore_System_String *value;
    cx_byte *data;
    if (text == NULL) return CX_NULL;
    length = strlen(text);
    if (length > UINT_MAX) abort();
    data = length == 0 ? CX_NULL : cx_exception_allocate_durable((cx_uint)length);
    if (length != 0) memcpy(data, text, length);
    value = cx_exception_allocate_durable((cx_uint)sizeof(*value));
    cxcore_System_String___constructor__2(value, (cx_uint)length, data);
    return value;
}

#define CX_TEXT_THROW_DEFAULT(name, type, typeInfo, constructor) \
    CX_NORETURN void name(const char *fileName, int line) { \
        type *error = cx_exception_allocate_durable( \
            typeInfo.Size); \
        constructor(error); \
        cx_exception_throw((struct cxcore_System_Exception *)error, fileName, line); \
    }

CX_TEXT_THROW_DEFAULT(cx_text_null_reference_at,
    struct cxcore_System_NullReferenceException,
    cxcore_System_NullReferenceException___typeinfo,
    cxcore_System_NullReferenceException___constructor)
CX_TEXT_THROW_DEFAULT(cx_text_index_range_at,
    struct cxcore_System_IndexOutOfRangeException,
    cxcore_System_IndexOutOfRangeException___typeinfo,
    cxcore_System_IndexOutOfRangeException___constructor)
CX_TEXT_THROW_DEFAULT(cx_text_overflow_at,
    struct cxcore_System_OverflowException,
    cxcore_System_OverflowException___typeinfo,
    cxcore_System_OverflowException___constructor)
CX_TEXT_THROW_DEFAULT(cx_text_format_at,
    struct cxcore_System_FormatException,
    cxcore_System_FormatException___typeinfo,
    cxcore_System_FormatException___constructor)
CX_NORETURN void cx_text_object_disposed_at(const char *fileName, int line) {
    struct cxcore_System_ObjectDisposedException *error =
        cx_exception_allocate_durable(
            cxcore_System_ObjectDisposedException___typeinfo.Size);
    cxcore_System_ObjectDisposedException___constructor__3(
        error, CX_NULL, CX_NULL);
    cx_exception_throw((struct cxcore_System_Exception *)error, fileName, line);
}

CX_NORETURN void cx_text_argument_null_at(
    const char *parameterName, const char *fileName, int line) {
    struct cxcore_System_ArgumentNullException *error =
        cx_exception_allocate_durable(
            cxcore_System_ArgumentNullException___typeinfo.Size);
    cxcore_System_ArgumentNullException___constructor__3(
        error, CX_NULL, cx_text_string(parameterName));
    cx_exception_throw((struct cxcore_System_Exception *)error, fileName, line);
}

CX_NORETURN void cx_text_argument_range_at(
    const char *parameterName, const char *fileName, int line) {
    struct cxcore_System_ArgumentOutOfRangeException *error =
        cx_exception_allocate_durable(
            cxcore_System_ArgumentOutOfRangeException___typeinfo.Size);
    cxcore_System_ArgumentOutOfRangeException___constructor__3(
        error, CX_NULL, cx_text_string(parameterName));
    cx_exception_throw((struct cxcore_System_Exception *)error, fileName, line);
}

CX_NORETURN void cx_text_io_at(
    const char *operation, const char *errorDomain, int errorCode,
    const char *fileName, int line) {
    struct cxcore_System_IO_IOException *error =
        cx_exception_allocate_durable(cxcore_System_IO_IOException___typeinfo.Size);
    cxcore_System_IO_IOException___constructor__5(
        error, CX_NULL, cx_text_string(operation), CX_NULL,
        cx_text_string(errorDomain), (cx_int)errorCode);
    cx_exception_throw((struct cxcore_System_Exception *)error, fileName, line);
}

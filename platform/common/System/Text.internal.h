#ifndef __CX_TEXT_INTERNAL_H__
#define __CX_TEXT_INTERNAL_H__

#include "cxcore.h"

CX_NORETURN void cx_text_null_reference_at(const char *fileName, int line);
CX_NORETURN void cx_text_object_disposed_at(const char *fileName, int line);
CX_NORETURN void cx_text_argument_null_at(
    const char *parameterName, const char *fileName, int line);
CX_NORETURN void cx_text_argument_range_at(
    const char *parameterName, const char *fileName, int line);
CX_NORETURN void cx_text_index_range_at(const char *fileName, int line);
CX_NORETURN void cx_text_overflow_at(const char *fileName, int line);
CX_NORETURN void cx_text_format_at(const char *fileName, int line);
CX_NORETURN void cx_text_io_at(
    const char *operation, const char *errorDomain, int errorCode,
    const char *fileName, int line);
CX_NORETURN void cx_encoding_error(cx_uint position);

#define cx_text_null_reference() \
    cx_text_null_reference_at(__FILE__, __LINE__)
#define cx_text_object_disposed() \
    cx_text_object_disposed_at(__FILE__, __LINE__)
#define cx_text_argument_null(parameterName) \
    cx_text_argument_null_at(parameterName, __FILE__, __LINE__)
#define cx_text_argument_range(parameterName) \
    cx_text_argument_range_at(parameterName, __FILE__, __LINE__)
#define cx_text_index_range() \
    cx_text_index_range_at(__FILE__, __LINE__)
#define cx_text_overflow() \
    cx_text_overflow_at(__FILE__, __LINE__)
#define cx_text_format() \
    cx_text_format_at(__FILE__, __LINE__)
#define cx_text_io(operation, errorDomain, errorCode) \
    cx_text_io_at(operation, errorDomain, errorCode, __FILE__, __LINE__)

#endif // __CX_TEXT_INTERNAL_H__

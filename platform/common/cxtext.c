#include "System/Text.internal.h"

CX_STRING_DEF(cx_text_invalid, "Invalid native text buffer or index");

CX_NORETURN void cx_text_error(void) {
    struct cxcore_System_Exception *error = cx_exception_allocate_durable((cx_uint)sizeof(*error));
    cxcore_System_Exception___constructor(error, &cx_text_invalid);
    cx_exception_throw(error, __FILE__, __LINE__);
}

#include "System/Text.internal.h"

CX_STRING_DEF(cx_text_invalid, "Invalid native text buffer or index");

CX_NORETURN void cx_text_error(void) {
    struct CX_ID_3(cxcore, System, Exception) *error = CX_ID_4(cxcore, System, Memory, Alloc)((cx_uint)sizeof(*error));
    CX_ID_4(cxcore, System, Exception, __constructor)(error, &cx_text_invalid);
    cx_exception_throw(error, __FILE__, __LINE__);
}

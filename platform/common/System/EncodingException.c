#include "Text.internal.h"

CX_STRING_DEF(cx_encoding_invalid, "Malformed Unicode encoding");

CX_NORETURN void cx_encoding_error(cx_uint position) {
    struct CX_ID_3(cxcore, System, EncodingException) *error = CX_ID_4(cxcore, System, Memory, Alloc)((cx_uint)sizeof(*error));
    CX_ID_4(cxcore, System, EncodingException, __constructor)(error, position);
    cx_exception_throw(error, __FILE__, __LINE__);
}

void CX_ID_4(cxcore, System, EncodingException, __constructor)(
    struct CX_ID_3(cxcore, System, EncodingException) *self, cx_uint position) {
    CX_ID_4(cxcore, System, Exception, __constructor)(&self->__base, &cx_encoding_invalid);
    CX_INIT_VTABLE(self, CX_ID_3(cxcore, System, EncodingException));
    self->_position = position;
}
cx_uint CX_ID_5(cxcore, System, EncodingException, Position, __const_get)(
    const struct CX_ID_3(cxcore, System, EncodingException) *self) { return self->_position; }

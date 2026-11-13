#include "Text.internal.h"

#if !defined(CX_USE_GENERATED_CORE)
extern void __cx_text_init_CXCORE(void);
#endif

CX_STRING_DEF(cx_encoding_invalid, "Malformed Unicode encoding");

CX_NORETURN void cx_encoding_error(cx_uint position) {
    struct cxcore_System_EncodingException *error =
        cx_exception_allocate_durable((cx_uint)sizeof(*error));
    cxcore_System_EncodingException___constructor(error, position);
    cx_exception_throw(error, __FILE__, __LINE__);
}

void cxcore_System_EncodingException___constructor(struct cxcore_System_EncodingException *self,
                                                   cx_uint position) {
#if !defined(CX_USE_GENERATED_CORE)
    // The fallback core keeps text type metadata in a separately initialized
    // generated module. This native constructor bypasses generated wrappers.
    __cx_text_init_CXCORE();
#endif
    cxcore_System_Exception___constructor(&self->__base, &cx_encoding_invalid);
    CX_INIT_VTABLE(self, cxcore_System_EncodingException);
    self->_position = position;
}

cx_uint cxcore_System_EncodingException_Position___const_get(
    const struct cxcore_System_EncodingException *self) {
    return self->_position;
}

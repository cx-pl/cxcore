#include "Text.internal.h"

#if !defined(CX_USE_GENERATED_CORE)
extern void __cx_text_init_CXCORE(void);
#endif

CX_STRING_DEF(cx_encoding_invalid, "Malformed Unicode encoding");
CX_STRING_DEF(cx_encoding_default, "Text encoding error");

CX_NORETURN void cx_encoding_error(cx_uint position) {
    struct cxcore_System_EncodingException *error =
        cx_exception_allocate_durable((cx_uint)sizeof(*error));
    cxcore_System_EncodingException___constructor__2(error, position);
    cx_exception_throw(error, __FILE__, __LINE__);
}

static void cx_encoding_exception_initialize(
    struct cxcore_System_EncodingException *self,
    const struct cxcore_System_String *message,
    const struct cxcore_System_Exception *innerException,
    cx_uint position) {
#if !defined(CX_USE_GENERATED_CORE)
    // The fallback core keeps text type metadata in a separately initialized
    // generated module. This native constructor bypasses generated wrappers.
    __cx_text_init_CXCORE();
#endif
    if (innerException == CX_NULL) {
        cxcore_System_Exception___constructor(
            &self->__base, message == CX_NULL ? &cx_encoding_default : message);
    } else {
        cxcore_System_Exception___constructor__2(
            &self->__base, message == CX_NULL ? &cx_encoding_default : message,
            innerException);
    }
    CX_INIT_VTABLE(self, cxcore_System_EncodingException);
    self->_position = position;
}

void cxcore_System_EncodingException___constructor(
    struct cxcore_System_EncodingException *self) {
    cx_encoding_exception_initialize(self, &cx_encoding_default, CX_NULL, 0u);
}

void cxcore_System_EncodingException___constructor__2(
    struct cxcore_System_EncodingException *self, cx_uint position) {
    cx_encoding_exception_initialize(self, &cx_encoding_invalid, CX_NULL, position);
}

void cxcore_System_EncodingException___constructor__3(
    struct cxcore_System_EncodingException *self,
    const struct cxcore_System_String *message) {
    cx_encoding_exception_initialize(self, message, CX_NULL, 0u);
}

void cxcore_System_EncodingException___constructor__4(
    struct cxcore_System_EncodingException *self,
    const struct cxcore_System_String *message,
    const struct cxcore_System_Exception *innerException) {
    cx_encoding_exception_initialize(self, message, innerException, 0u);
}

void cxcore_System_EncodingException___constructor__5(
    struct cxcore_System_EncodingException *self,
    const struct cxcore_System_String *message,
    cx_uint position) {
    cx_encoding_exception_initialize(self, message, CX_NULL, position);
}

cx_uint cxcore_System_EncodingException_Position___const_get(
    const struct cxcore_System_EncodingException *self) {
    return self->_position;
}

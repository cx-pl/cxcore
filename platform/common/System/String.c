#include <string.h>
#include "../../../src/cxcore.h"

CX_STRING_DEF(CX_ID_5(_, cxcore, System, String, __typeName), "String");
CX_STRING_DEF(CX_ID_5(_, cxcore, System, String, __namespace), "System");

CX_STRUCT_TYPEINFO_DEF(
    CX_ID_3(cxcore, System, String),
    CX_ID_5(_, cxcore, System, String, __typeName),
    CX_ID_5(_, cxcore, System, String, __namespace),
    2345, // TODO: Replace with real hash
    CX_REFLECTION_FLAG_FINAL |
    CX_REFLECTION_FLAG_VISIBILITY_PUBLIC |
    CX_REFLECTION_FLAG_TYPE_CLASS
);

CX_BEGIN_VTABLE_DEF(CX_ID_3(cxcore, System, String))
CX_END_VTABLE_DEF;

void CX_ID_4(cxcore, System, String, __constructor)(
    struct CX_ID_3(cxcore, System, String)* __this    
) {
    CX_ID_4(cxcore, System, Object, __constructor)(&__this->__base);

    __this->_length = 0;
    __this->_data = CX_NULL;
}

void CX_ID_5(cxcore, System, String, __constructor, _2)(
    struct CX_ID_3(cxcore, System, String)* __this,
    cx_uint length,
    cx_ptr data
) {
    CX_ID_4(cxcore, System, Object, __constructor)(&__this->__base);

    __this->_length = length;
    __this->_data = data;
}

void CX_ID_5(cxcore, System, String, __constructor, _3)(
    struct CX_ID_3(cxcore, System, String)* __this,
    cx_uint length,
    cx_char ch
) {
    CX_ID_4(cxcore, System, Object, __constructor)(&__this->__base);

    __this->_length = length;
    __this->_data = CX_ID_4(cxcore, System, Memory, Alloc)(length);
    memset(__this->_data, ch, length);
}

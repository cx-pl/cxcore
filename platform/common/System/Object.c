#include "../../../src/cxcore.h"

CX_STRING_DEF(CX_ID_5(_, cxcore, System, Object, __typeName), "Object");
CX_STRING_DEF(CX_ID_5(_, cxcore, System, Object, __namespace), "System");

CX_STRUCT_TYPEINFO_DEF(
    CX_ID_3(cxcore, System, Object), 
    CX_ID_5(_, cxcore, System, Object, __typeName),
    CX_ID_5(_, cxcore, System, Object, __namespace),
	1234, // TODO: Replace with real hash
    CX_REFLECTION_FLAG_VISIBILITY_PUBLIC |
    CX_REFLECTION_FLAG_TYPE_CLASS
);

CX_BEGIN_VTABLE_DEF(CX_ID_3(cxcore, System, Object))
CX_END_VTABLE_DEF;

struct CX_ID_4(cxcore, System, Reflection, TypeInfo) CX_ID_5(cxcore, System, Object, __get, Type)(
    struct CX_ID_3(cxcore, System, Object)* __this
) {
    return CX_GET_TYPEINFO(__this);
}

void CX_ID_4(cxcore, System, Object, __constructor)(
    struct CX_ID_3(cxcore, System, Object)* __this
) {
    __this->__vtable = CX_ID_4(cxcore, System, Object, __vtable);
    __this->_gc = CX_NULL;
}

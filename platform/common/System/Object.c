#include "src/cxcore.h"

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

cx_ulong CX_ID_4(cxcore, System, Object, GetHash)(
    struct CX_ID_3(cxcore, System, Object)* __this
) {
    struct CX_ID_4(cxcore, System, Reflection, TypeInfo)* typeInfo;

    // Return hash of type info
    typeInfo = &CX_GET_TYPEINFO(__this);
    return typeInfo->Hash;
}

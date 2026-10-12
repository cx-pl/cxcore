#include "../../../src/cxcore.h"

#include <stdint.h>

struct CX_ID_4(cxcore, System, Reflection, TypeInfo)
    CX_ID_5(cxcore, System, Object, Type,
            __const_get)(const struct CX_ID_3(cxcore, System, Object) * __this) {
    const struct CX_ID_4(cxcore, System, Reflection, TypeInfo) *boxedType =
        cx_boxed_value_type((cx_ptr)__this);
    if (boxedType != CX_NULL) {
        return *boxedType;
    }
    return CX_GET_TYPEINFO(__this);
}

struct CX_ID_3(cxcore, System, String) *CX_ID_4(
    cxcore, System, Object, ToString)(const struct CX_ID_3(cxcore, System, Object) * __this) {
    const struct CX_ID_4(cxcore, System, Reflection, TypeInfo) typeInfo =
        CX_ID_5(cxcore, System, Object, Type, __const_get)(__this);
    return (struct CX_ID_3(cxcore, System, String) *)typeInfo.Name;
}

cx_bool CX_ID_4(cxcore, System, Object, Equals)(
    const struct CX_ID_3(cxcore, System, Object) * __this,
    struct CX_ID_3(cxcore, System, Object) *other) {
    return __this == other;
}

cx_int CX_ID_4(cxcore, System, Object, GetHashCode)(
    const struct CX_ID_3(cxcore, System, Object) * __this) {
    const cx_ulong identity = (cx_ulong)(uintptr_t)__this;
    return (cx_int)(identity ^ (identity >> 32));
}

void CX_ID_4(cxcore, System, Object,
             __constructor)(struct CX_ID_3(cxcore, System, Object) * __this) {
    CX_INIT_VTABLE(__this, CX_ID_3(cxcore, System, Object));

    __this->_gc = CX_NULL;
}

#include "../../../src/cxcore.h"

struct CX_ID_4(cxcore, System, Reflection, TypeInfo) CX_ID_5(cxcore, System, Object, Type, __const_get)(
    const struct CX_ID_3(cxcore, System, Object)* __this
) {
    return CX_GET_TYPEINFO(__this);
}

void CX_ID_4(cxcore, System, Object, __constructor)(
    struct CX_ID_3(cxcore, System, Object)* __this
) {
    CX_INIT_VTABLE(__this, CX_ID_3(cxcore, System, Object));

    __this->_gc = CX_NULL;
}

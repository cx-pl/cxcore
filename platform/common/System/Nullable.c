#include "../../../src/cxcore.h"

struct CX_ID_3(cxcore, System, Nullable) _null = { CX_NULL };

void CX_ID_5(cxcore, System, Nullable, Null, __const_get)(
    void* __returnValue
) {
    *(struct CX_ID_3(cxcore, System, Nullable)*)__returnValue = _null;
}

cx_bool CX_ID_5(cxcore, System, Nullable, HasValue, __const_get)(
    const struct CX_ID_3(cxcore, System, Nullable)* __this    
) {
	return __this->_obj != CX_NULL;
}

void CX_ID_5(cxcore, System, Nullable, Value, __const_get)(
    const struct CX_ID_3(cxcore, System, Nullable)* __this,
    void* __returnValue    
) {
	*(struct CX_ID_3(cxcore, System, Nullable)*)__returnValue = *__this;
}

void CX_ID_5(cxcore, System, Nullable, Value, __set)(
    struct CX_ID_3(cxcore, System, Nullable)* __this,
    void* value    
) {
    __this->_obj = value;
}

void CX_ID_4(cxcore, System, Nullable, __constructor)(
    struct CX_ID_3(cxcore, System, Nullable)* __this,
    cx_ptr obj
) {
	__this->_obj = obj;
}

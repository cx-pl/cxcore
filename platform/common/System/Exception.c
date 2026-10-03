#include <stdlib.h>
#include "../../../src/cxcore.h"

struct CX_ID_4(cxcore, System, Reflection, TypeInfo) CX_ID_5(
    cxcore, System, Exception, RuntimeType, __const_get)(
        const struct CX_ID_3(cxcore, System, Exception)* __this)
{
    return CX_GET_TYPEINFO(__this);
}

cx_bool CX_ID_4(cxcore, System, Exception, Matches)(
    const struct CX_ID_3(cxcore, System, Exception)* exception,
    const struct CX_ID_4(cxcore, System, Reflection, TypeInfo) typeInfo)
{
    return CX_ID_5(cxcore, System, Reflection, TypeInfo, IsInBaseTypeChain)(
        &typeInfo,
        CX_ID_5(cxcore, System, Exception, RuntimeType, __const_get)(exception));
}


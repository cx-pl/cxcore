#include "../../platform.h"
#ifdef CX_PLATFORM_WIN32

#include <windows.h>
#include "../../../src/cxcore.h"

CX_STRING_DEF(SystemName, "Windows");
CX_STRING_DEF(NewLine, "\r\n");

const struct CX_ID_3(cxcore, System, String)* CX_ID_5(cxcore, System, Environment, SystemName, __const_get)(
) {
    return &SystemName;
}

const struct CX_ID_3(cxcore, System, String)* CX_ID_5(cxcore, System, Environment, NewLine, __const_get)(
) {
    return &NewLine;
}

#endif // CX_PLATFORM_WIN32

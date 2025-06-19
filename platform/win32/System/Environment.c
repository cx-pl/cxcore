#include "../../platform.h"
#ifdef CX_PLATFORM_WIN32

#include <windows.h>
#include "../../../src/cxcore.h"

CX_STRING_DEF(SystemName, "Windows");
CX_STRING_DEF(NewLine, "\r\n");

struct CX_ID_3(cxcore, System, String)* CX_ID_5(cxcore, System, Environment, __get, SystemName)(
) {
    return &SystemName;
}

struct CX_ID_3(cxcore, System, String)* CX_ID_5(cxcore, System, Environment, __get, NewLine)(
) {
    return &NewLine;
}

#endif // CX_PLATFORM_WIN32

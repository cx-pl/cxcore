#include "../../platform.h"

#ifdef CX_PLATFORM_LINUX

#include "../../../src/cxcore.h"

CX_STRING_DEF(SystemName, "Linux");
CX_STRING_DEF(NewLine, "\n");

const struct CX_ID_3(cxcore, System, String) *
    CX_ID_5(cxcore, System, Environment, SystemName, __const_get)() {
    return &SystemName;
}

const struct CX_ID_3(cxcore, System, String) *
    CX_ID_5(cxcore, System, Environment, NewLine, __const_get)() {
    return &NewLine;
}

#endif // CX_PLATFORM_LINUX

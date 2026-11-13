#include "../../platform.h"
#ifdef CX_PLATFORM_WIN

#include <windows.h>
#include "../../../src/cxcore.h"

CX_STRING_DEF(SystemName, "Windows");
CX_STRING_DEF(NewLine, "\r\n");

const struct cxcore_System_String *
    cxcore_System_Environment_SystemName___const_get() {
    return &SystemName;
}

const struct cxcore_System_String *
    cxcore_System_Environment_NewLine___const_get() {
    return &NewLine;
}

#endif // CX_PLATFORM_WIN

#include "../../../platform.h"
#ifdef CX_PLATFORM_WIN

#include <windows.h>

uint32_t cx_platform_last_error(void) {
    return (uint32_t)GetLastError();
}

void cx_platform_set_last_error(uint32_t error) {
    SetLastError((DWORD)error);
}

#endif // CX_PLATFORM_WIN

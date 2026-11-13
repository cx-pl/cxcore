#include "../../../platform.h"
#ifdef CX_PLATFORM_LINUX

uint32_t cx_platform_last_error(void) {
    return 0;
}

void cx_platform_set_last_error(uint32_t error) {
    (void)error;
}

#endif // CX_PLATFORM_LINUX

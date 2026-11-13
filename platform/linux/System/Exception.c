#include "../../platform.h"
#ifdef CX_PLATFORM_LINUX

CX_NORETURN void cx_platform_exception_jump(jmp_buf environment) {
    longjmp(environment, 1);
}

#endif // CX_PLATFORM_LINUX

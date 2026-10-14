#include "../platform.h"

CX_NORETURN void cx_platform_exception_jump(jmp_buf environment) {
    longjmp(environment, 1);
}

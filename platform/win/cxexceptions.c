#include "../platform.h"

CX_NORETURN void cx_platform_exception_jump(jmp_buf environment) {
#if defined(_MSC_VER) && defined(_M_X64)
    // CX emits its own catch/finally transfers. CRT SEH unwinding is neither
    // needed nor valid for MSVC frames realigned for native SDK records.
    // The target CRT's public jmp_buf layout carries the optional SEH frame.
    ((_JUMP_BUFFER *)environment)->Frame = 0;
#endif
    longjmp(environment, 1);
}

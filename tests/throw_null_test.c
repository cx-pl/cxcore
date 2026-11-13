#include "../src/cxcore.h"

#ifdef _WIN32
#include <windows.h>
#endif

int main(void) {
#ifdef _WIN32
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif
    cx_exception_throw(CX_NULL, __FILE__, __LINE__);
}

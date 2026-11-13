#include "cxcore.h"

cx_uint cxcore_System_Interop_NativeError_GetLastError(void) {
    return cx_interop_last_error();
}

cx_int cxcore_System_Interop_NativeError_GetErrno(void) {
    return cx_interop_errno();
}

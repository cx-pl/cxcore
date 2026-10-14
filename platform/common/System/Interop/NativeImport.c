#include "cxcore.h"

cx_uint CX_ID_5(cxcore, System, Interop, NativeError, GetLastError)(void) {
    return cx_interop_last_error();
}

cx_int CX_ID_5(cxcore, System, Interop, NativeError, GetErrno)(void) {
    return cx_interop_errno();
}

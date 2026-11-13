#include "cxnumeric.h"
#include <limits.h>
#include "../../../src/cxcore.h"

cx_uint cxcore_System_UInt_MinValue = 0;
cx_uint cxcore_System_UInt_MaxValue = UINT_MAX;

struct cxcore_System_Nullable cxcore_System_UInt_TryParse(
    const struct cxcore_System_String *text) {
    uint64_t magnitude;
    int negative;
    if (!cx_numeric_integer(text, UINT_MAX, 0, &magnitude, &negative))
        return (struct cxcore_System_Nullable){0};
    cx_uint value = (cx_uint)((cx_uint)magnitude);
    return cx_numeric_store(&value, sizeof(value));
}
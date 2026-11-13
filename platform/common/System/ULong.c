#include "cxnumeric.h"
#include <limits.h>
#include "../../../src/cxcore.h"

cx_ulong cxcore_System_ULong_MinValue = 0;
cx_ulong cxcore_System_ULong_MaxValue = ULONG_MAX;

struct cxcore_System_Nullable cxcore_System_ULong_TryParse(
    const struct cxcore_System_String *text) {
    uint64_t magnitude;
    int negative;
    if (!cx_numeric_integer(text, ULLONG_MAX, 0, &magnitude, &negative))
        return (struct cxcore_System_Nullable){0};
    cx_ulong value = (cx_ulong)((cx_ulong)magnitude);
    return cx_numeric_store(&value, sizeof(value));
}
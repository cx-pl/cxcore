#include "cxnumeric.h"
#include <limits.h>
#include "../../../src/cxcore.h"

cx_short cxcore_System_Short_MinValue = SHRT_MIN;
cx_short cxcore_System_Short_MaxValue = SHRT_MAX;

struct cxcore_System_Nullable cxcore_System_Short_TryParse(
    const struct cxcore_System_String *text) {
    uint64_t magnitude;
    int negative;
    if (!cx_numeric_integer(text, SHRT_MAX, (uint64_t)SHRT_MAX + 1, &magnitude, &negative))
        return (struct cxcore_System_Nullable){0};
    cx_short value = (cx_short)(negative ? -(int64_t)(magnitude - (magnitude != 0)) - (magnitude != 0) : (int64_t)magnitude);
    return cx_numeric_store(&value, sizeof(value));
}
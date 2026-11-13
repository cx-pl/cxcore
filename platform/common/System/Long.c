#include "cxnumeric.h"
#include <limits.h>
#include "../../../src/cxcore.h"

cx_long cxcore_System_Long_MinValue = LONG_MIN;
cx_long cxcore_System_Long_MaxValue = LONG_MAX;

struct cxcore_System_Nullable cxcore_System_Long_TryParse(
    const struct cxcore_System_String *text) {
    uint64_t magnitude;
    int negative;
    if (!cx_numeric_integer(text, LLONG_MAX, (uint64_t)LLONG_MAX + 1, &magnitude, &negative))
        return (struct cxcore_System_Nullable){0};
    cx_long value = (cx_long)(negative ? -(int64_t)(magnitude - (magnitude != 0)) - (magnitude != 0) : (int64_t)magnitude);
    return cx_numeric_store(&value, sizeof(value));
}
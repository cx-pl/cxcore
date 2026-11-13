#include "cxnumeric.h"
#include <limits.h>
#include "../../../src/cxcore.h"

cx_sbyte cxcore_System_SByte_MinValue = SCHAR_MIN;
cx_sbyte cxcore_System_SByte_MaxValue = SCHAR_MAX;

struct cxcore_System_Nullable cxcore_System_SByte_TryParse(
    const struct cxcore_System_String *text) {
    uint64_t magnitude;
    int negative;
    if (!cx_numeric_integer(text, SCHAR_MAX, (uint64_t)SCHAR_MAX + 1, &magnitude, &negative))
        return (struct cxcore_System_Nullable){0};
    cx_sbyte value = (cx_sbyte)(negative ? -(int64_t)(magnitude - (magnitude != 0)) - (magnitude != 0) : (int64_t)magnitude);
    return cx_numeric_store(&value, sizeof(value));
}
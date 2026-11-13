#include "cxnumeric.h"
#include <limits.h>
#include "../../../src/cxcore.h"

cx_int cxcore_System_Int_MinValue = INT_MIN;
cx_int cxcore_System_Int_MaxValue = INT_MAX;

struct cxcore_System_Nullable cxcore_System_Int_TryParse(
    const struct cxcore_System_String *text) {
    uint64_t magnitude;
    int negative;
    if (!cx_numeric_integer(text, INT_MAX, (uint64_t)INT_MAX + 1, &magnitude, &negative))
        return (struct cxcore_System_Nullable){0};
    cx_int value = (cx_int)(negative ? -(int64_t)(magnitude - (magnitude != 0)) - (magnitude != 0) : (int64_t)magnitude);
    return cx_numeric_store(&value, sizeof(value));
}
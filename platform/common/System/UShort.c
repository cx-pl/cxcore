#include "cxnumeric.h"
#include <limits.h>
#include "../../../src/cxcore.h"

cx_ushort cxcore_System_UShort_MinValue = 0;
cx_ushort cxcore_System_UShort_MaxValue = USHRT_MAX;

struct cxcore_System_Nullable cxcore_System_UShort_TryParse(
    const struct cxcore_System_String *text) {
    uint64_t magnitude;
    int negative;
    if (!cx_numeric_integer(text, USHRT_MAX, 0, &magnitude, &negative))
        return (struct cxcore_System_Nullable){0};
    cx_ushort value = (cx_ushort)((cx_ushort)magnitude);
    return cx_numeric_store(&value, sizeof(value));
}
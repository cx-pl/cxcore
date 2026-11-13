#include "cxnumeric.h"
#include <limits.h>
#include "../../../src/cxcore.h"

cx_byte cxcore_System_Byte_MinValue = 0;
cx_byte cxcore_System_Byte_MaxValue = UCHAR_MAX;

struct cxcore_System_Nullable cxcore_System_Byte_TryParse(
    const struct cxcore_System_String *text) {
    uint64_t magnitude;
    int negative;
    if (!cx_numeric_integer(text, UCHAR_MAX, 0, &magnitude, &negative))
        return (struct cxcore_System_Nullable){0};
    cx_byte value = (cx_byte)((cx_byte)magnitude);
    return cx_numeric_store(&value, sizeof(value));
}
#include "cxnumeric.h"
#include <float.h>
#include "../../../src/cxcore.h"

cx_float cxcore_System_Float_MinValue = FLT_MIN;
cx_float cxcore_System_Float_MaxValue = FLT_MAX;
struct cxcore_System_Nullable cxcore_System_Float_TryParse(
    const struct cxcore_System_String *text) {
    return cx_numeric_float(text);
}
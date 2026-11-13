#include "cxnumeric.h"
#include <float.h>
#include "../../../src/cxcore.h"

cx_double cxcore_System_Double_MinValue = DBL_MIN;
cx_double cxcore_System_Double_MaxValue = DBL_MAX;
struct cxcore_System_Nullable cxcore_System_Double_TryParse(
    const struct cxcore_System_String *text) {
    return cx_numeric_double(text);
}
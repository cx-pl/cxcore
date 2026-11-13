#ifndef __CX_NUMERIC_INTERNAL_H__
#define __CX_NUMERIC_INTERNAL_H__

#include "cxcore.h"
#include <stdint.h>

#if defined(CXCORE_TESTING)
#define CX_NUMERIC_TEST_API CX_API
#else
#define CX_NUMERIC_TEST_API
#endif

int cx_numeric_integer(const struct cxcore_System_String *text,
    uint64_t positiveMax, uint64_t negativeMax, uint64_t *value, int *negative);
struct cxcore_System_Nullable cx_numeric_store(const void *value, cx_uint size);
CX_NUMERIC_TEST_API struct cxcore_System_Nullable
cx_numeric_double(const struct cxcore_System_String *text);
struct cxcore_System_Nullable cx_numeric_float(const struct cxcore_System_String *text);
struct cxcore_System_Nullable cx_numeric_decimal(const struct cxcore_System_String *text);

#endif // __CX_NUMERIC_INTERNAL_H__

#include "cxcore.h"

#include <string.h>

cx_ptr cx_nullable_new(cx_ptr value, cx_uint size)
{
    cx_ptr storage = CX_ID_4(cxcore, System, Memory, Alloc)(size);
    memcpy(storage, value, size);
    return storage;
}

#include <stdlib.h>
#include <string.h>
#include "../../../src/cxcore.h"

// Note: This is the simplest implementation. We should have GC implemented here.

cx_ptr CX_ID_4(cxcore, System, Memory, Alloc)(
    cx_uint size
) {
    return malloc(size);
}

cx_ptr CX_ID_4(cxcore, System, Memory, Realloc)(
    cx_ptr block,
    cx_uint size
) {
    return realloc(block, size);
}

void CX_ID_4(cxcore, System, Memory, Free)(
    cx_ptr block
) {
    free(block);
}

void CX_ID_4(cxcore, System, Memory, Copy)(
    cx_ptr dest,
    cx_ptr src,
    cx_uint size
) {
    memcpy(dest, src, size);
}

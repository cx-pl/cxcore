#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../../src/cxcore.h"

// Note: This is the naive implementation. There should be some form of GC implemented here.

cx_ptr CX_ID_4(cxcore, System, Memory, Alloc)(
    cx_uint size
) {
    cx_ptr object = calloc(1, size == 0 ? 1 : size);
    if (object == NULL) {
        fputs("Out of memory\n", stderr);
        abort();
    }
    return object;
}

cx_ptr CX_ID_4(cxcore, System, Memory, Realloc)(
    cx_ptr block,
    cx_uint size
) {
    cx_ptr resized;
    if (size == 0) {
        free(block);
        return CX_NULL;
    }
    resized = realloc(block, size);
    if (resized == NULL) {
        fputs("Out of memory\n", stderr);
        abort();
    }
    return resized;
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
    if (size == 0) {
        return;
    }
    if (dest == NULL || src == NULL) {
        abort();
    }
    memcpy(dest, src, size);
}

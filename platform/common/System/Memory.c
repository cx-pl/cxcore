#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../../src/cxcore.h"

// Note: This is the naive implementation. There should be some form of GC implemented here.

#ifdef CXCORE_TESTING
static cx_bool cx_memory_fail_next_allocation;

CX_API void cx_memory_test_fail_next_allocation(void)
{
    cx_memory_fail_next_allocation = CX_TRUE;
}
#endif

static cx_bool cx_memory_should_fail_allocation(void)
{
#ifdef CXCORE_TESTING
    if (cx_memory_fail_next_allocation)
    {
        cx_memory_fail_next_allocation = CX_FALSE;
        return CX_TRUE;
    }
#endif
    return CX_FALSE;
}

static void cx_memory_out_of_memory(void)
{
    fputs("Out of memory\n", stderr);
    exit(EXIT_FAILURE);
}

cx_ptr CX_ID_4(cxcore, System, Memory, Alloc)(
    cx_uint size
) {
    cx_ptr object = cx_memory_should_fail_allocation()
        ? NULL
        : calloc(1, size == 0 ? 1 : size);
    if (object == NULL) {
        cx_memory_out_of_memory();
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
    resized = cx_memory_should_fail_allocation() ? NULL : realloc(block, size);
    if (resized == NULL) {
        cx_memory_out_of_memory();
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

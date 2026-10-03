#include <string.h>
#include "../src/cxcore.h"

CX_API void cx_memory_test_fail_next_allocation(void);

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }

    if (strcmp(argv[1], "alloc") == 0)
    {
        cx_memory_test_fail_next_allocation();
        CX_ID_4(cxcore, System, Memory, Alloc)(1);
    }
    else if (strcmp(argv[1], "realloc") == 0)
    {
        cx_ptr block = CX_ID_4(cxcore, System, Memory, Alloc)(1);
        cx_memory_test_fail_next_allocation();
        CX_ID_4(cxcore, System, Memory, Realloc)(block, 2);
    }
    else
    {
        return 2;
    }
    return 0;
}

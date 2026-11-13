#include <string.h>
#include "../src/cxcore.h"
#include "../platform/common/cxarena.h"

CX_API void cx_memory_test_fail_next_allocation(void);

int main(int argc, char **argv) {
    if (argc != 2) {
        return 2;
    }

    if (strcmp(argv[1], "alloc") == 0) {
        cx_memory_test_fail_next_allocation();
        cxcore_System_Memory_Alloc(1);
    } else if (strcmp(argv[1], "arena-alloc") == 0) {
        struct cx_arena *arena = cx_arena_create(64, 64, 1);
        struct cx_arena_scope scope;
        if (arena == NULL) return 3;
        cx_arena_enter(arena, &scope);
        cx_memory_test_fail_next_allocation();
        cxcore_System_Memory_Alloc(1);
    } else if (strcmp(argv[1], "realloc") == 0) {
        cx_ptr block = cxcore_System_Memory_Alloc(1);
        cx_memory_test_fail_next_allocation();
        cxcore_System_Memory_Realloc(block, 2);
    } else {
        return 2;
    }
    return 0;
}

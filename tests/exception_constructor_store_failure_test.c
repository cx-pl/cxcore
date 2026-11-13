#include <stddef.h>

#include "../src/cxcore.h"
#include "../platform/common/cxarena.h"

int main(int argc, char **argv) {
    struct cx_arena *arena;
    struct cx_arena_scope scope;
    struct cxcore_System_Exception *outer;
    struct cxcore_System_Exception *inner;

    (void)argc;
    (void)argv;
    outer = cxcore_System_Memory_Alloc((cx_uint)sizeof(*outer));
    cxcore_System_Exception___constructor(outer, CX_NULL);

    arena = cx_arena_create(4096u, 4096u, CX_TRUE);
    if (arena == NULL) return 2;
    cx_arena_enter(arena, &scope);
    inner = (struct cxcore_System_Exception *)cx_arena_allocate_current(
        sizeof(*inner), _Alignof(struct cxcore_System_Exception));
    if (inner == NULL) return 3;
    cxcore_System_Exception___constructor(inner, CX_NULL);

    // A durable Exception must not retain an exception from a temporary arena.
    cxcore_System_Exception___constructor__2(outer, CX_NULL, inner);
    return 4;
}

#include <stddef.h>

#include "../src/cxcore.h"
#include "../src/cxthreading.h"
#include "../platform/common/cxarena.h"

int main(int argc, char **argv) {
    struct cx_arena *arena;
    struct cx_arena_scope scope;
    struct cxcore_System_Threading_Tasks_Task *task;
    cx_ptr scoped_result;

    (void)argc;
    (void)argv;
    task = cxcore_System_Memory_Alloc((cx_uint)sizeof(*task));
    arena = cx_arena_create(4096u, 4096u, CX_TRUE);
    if (arena == NULL) return 2;
    cx_arena_enter(arena, &scope);
    scoped_result = cx_arena_allocate_current(sizeof(cx_uint), sizeof(cx_uint));
    if (scoped_result == NULL) return 3;

    // The erased Task<T> result slot must reject an arena pointer in a durable task.
    cxcore_System_Threading_Tasks_Task___constructor(task, scoped_result);
    return 4;
}

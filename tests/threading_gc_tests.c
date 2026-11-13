#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
typedef HANDLE test_thread;
typedef volatile LONG test_flag;
static void flag_set(test_flag *flag) { InterlockedExchange(flag, 1); }
static int flag_get(test_flag *flag) { return InterlockedCompareExchange(flag, 0, 0) != 0; }
static void native_yield(void) { Sleep(0); }
#else
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
typedef pthread_t test_thread;
typedef _Atomic int test_flag;
static void flag_set(test_flag *flag) { atomic_store_explicit(flag, 1, memory_order_release); }
static int flag_get(test_flag *flag) { return atomic_load_explicit(flag, memory_order_acquire); }
static void native_yield(void) { sched_yield(); }
#endif

#include "../src/cxcore.h"
#include "../platform/common/cxarena.h"
#include "../platform/common/cxgc.h"

#ifdef _WIN32
#define CX_TEST_NOINLINE __declspec(noinline)
#else
#define CX_TEST_NOINLINE __attribute__((noinline))
#endif

CX_API size_t cx_memory_test_collection_count_get(void);
CX_API size_t cx_memory_test_collected_bytes_get(void);
CX_API void cx_memory_test_fail_next_record_allocation(void);

struct test_context {
    test_flag ready;
    test_flag release;
    test_flag polling;
    test_flag resume;
    int failed;
    int slot;
};

struct arena_run_attempt_context {
    test_flag complete;
    int result;
    struct cxcore_System_MemoryArena *arena;
};

struct arena_gc_stress_node {
    cx_ptr next;
    cx_uint marker;
};

static cx_ptr worker_roots[4];
static const struct cx_gc_root_region roots[] = {
    {worker_roots, (cx_uint)sizeof(worker_roots)},
};

static void wait_for_release(void *argument) {
    struct test_context *context = (struct test_context *)argument;
    flag_set(&context->ready);
    while (!flag_get(&context->release)) {
        native_yield();
    }
}

static void worker_body(struct test_context *context) {
    volatile cx_ptr stack_root;
    volatile cx_byte *interior_root;
    cx_ptr interior_allocation;
    cx_uint index;
    cx_gc_attach_thread();
    cx_gc_attach_thread(); // Idempotent attachment must not duplicate registry entries.
    cx_gc_register_static_roots(roots, 1); // Concurrent deduplication.
    stack_root = cxcore_System_Memory_Alloc(64);
    *(cx_uint *)stack_root = 0x1234abcd;
    interior_allocation = cxcore_System_Memory_Alloc(64);
    interior_root = (cx_byte *)interior_allocation + 8;
    *interior_root = 0x5a;
    interior_allocation = NULL;
    worker_roots[context->slot] = cxcore_System_Memory_Alloc(sizeof(cx_uint));
    *(cx_uint *)worker_roots[context->slot] = 0x9876;
    cx_gc_run_blocking(wait_for_release, context);
    if (*(cx_uint *)stack_root != 0x1234abcd || *interior_root != 0x5a ||
        *(cx_uint *)worker_roots[context->slot] != 0x9876) {
        context->failed = 1;
    }
    flag_set(&context->polling);
    while (!flag_get(&context->resume)) {
        cx_gc_safepoint(); // A running allocation-free loop must cooperate too.
        native_yield();
    }
    // Allocations, explicit collections, reallocations and frees race across workers.
    for (index = 0; index < 300; ++index) {
        cx_ptr block = cxcore_System_Memory_Alloc(4096);
        memset(block, 0x6b, 4096);
        block = cxcore_System_Memory_Realloc(block, 8192);
        if (((cx_byte *)block)[4095] != 0x6b) {
            context->failed = 2;
        }
        if (index % 30 == 0) {
            cx_gc_collect();
        }
        cxcore_System_Memory_Free(block);
        if (index % 50 == 0) {
            (void)cxcore_System_Memory_Alloc(32768);
        }
        cx_gc_safepoint();
    }
    if (*(cx_uint *)stack_root != 0x1234abcd || *interior_root != 0x5a) {
        context->failed = 3;
    }
    worker_roots[context->slot] = NULL;
    cx_gc_detach_thread();
    // Reattach on the same OS thread; the removed record must not remain in the list.
    cx_gc_attach_thread();
    cx_gc_collect();
    cx_gc_detach_thread();
}

static CX_TEST_NOINLINE void retain_only_in_active_arena(cx_ptr *slot) {
    struct cx_arena_durable_scope durable_scope;
    volatile cx_ptr value;
    cx_arena_enter_durable(&durable_scope);
    value = cxcore_System_Memory_Alloc(64u);
    cx_arena_leave_durable(&durable_scope);
    *(cx_uint *)value = 0x6d22a11u;
    (void)cx_gc_store_reference(slot, value);
    value = NULL;
}

#ifdef _WIN32
static DWORD WINAPI worker_entry(LPVOID argument) {
    worker_body((struct test_context *)argument);
    return 0;
}
static int start_worker(test_thread *thread, struct test_context *context) {
    *thread = CreateThread(NULL, 0, worker_entry, context, 0, NULL);
    return *thread != NULL;
}
static void join_worker(void *argument) {
    test_thread *thread = (test_thread *)argument;
    if (WaitForSingleObject(*thread, INFINITE) != WAIT_OBJECT_0) {
        ExitProcess(10);
    }
    CloseHandle(*thread);
}
#else
static void *worker_entry(void *argument) {
    worker_body((struct test_context *)argument);
    return NULL;
}
static int start_worker(test_thread *thread, struct test_context *context) {
    return pthread_create(thread, NULL, worker_entry, context) == 0;
}
static void join_worker(void *argument) {
    test_thread *thread = (test_thread *)argument;
    if (pthread_join(*thread, NULL) != 0) {
        _Exit(10);
    }
}
#endif

static void invalid_managed_callback(void *context) {
    (void)context;
    cx_gc_collect();
}

static int arena_run_attempt_callback_count;

static void arena_run_attempt_callback(void *environment) {
    (void)environment;
    ++arena_run_attempt_callback_count;
}

static void arena_run_attempt_body(struct arena_run_attempt_context *context) {
    struct cx_exception_frame frame;
    cx_action action = {0};
    cx_gc_attach_thread();
    action.__function = arena_run_attempt_callback;
    cx_exception_push(&frame);
    if (setjmp(frame.environment) == 0) {
        cxcore_System_MemoryArena_Run(context->arena, action);
        context->result = 1; // An overlapping run must be rejected.
    } else {
        context->result = 0;
        cx_exception_pop(&frame);
        cx_exception_clear();
    }
    cx_gc_detach_thread();
    flag_set(&context->complete);
}

#ifdef _WIN32
static DWORD WINAPI arena_run_attempt_entry(LPVOID argument) {
    arena_run_attempt_body((struct arena_run_attempt_context *)argument);
    return 0;
}
static int start_arena_run_attempt(
    test_thread *thread, struct arena_run_attempt_context *context) {
    *thread = CreateThread(NULL, 0, arena_run_attempt_entry, context, 0, NULL);
    return *thread != NULL;
}
#else
static void *arena_run_attempt_entry(void *argument) {
    arena_run_attempt_body((struct arena_run_attempt_context *)argument);
    return NULL;
}
static int start_arena_run_attempt(
    test_thread *thread, struct arena_run_attempt_context *context) {
    return pthread_create(thread, NULL, arena_run_attempt_entry, context) == 0;
}
#endif

static int arena_stress_result;

static void run_memory_arena_gc_stress(void *environment) {
    test_thread threads[4];
    // Native callbacks update these flags while parent stacks are parked.
    // Keep native synchronization state outside the scanned stack regions.
    static struct test_context contexts[4];
    volatile cx_ptr parent_root;
    cx_ptr *arena_root_slot;
    size_t collections_before;
    size_t collected_before;
    int index;
    struct cxcore_System_MemoryArena *arena =
        (struct cxcore_System_MemoryArena *)environment;

    // A second OS thread must observe the active run lease and fail before
    // invoking its callback. This exercises the state check under the runtime
    // lock rather than only same-thread reentrancy.
    {
        static struct arena_run_attempt_context attempt;
        test_thread thread;
        attempt.complete = 0;
        attempt.result = -1;
        attempt.arena = arena;
        arena_run_attempt_callback_count = 0;
        if (!start_arena_run_attempt(&thread, &attempt)) {
            arena_stress_result = 7;
            return;
        }
        while (!flag_get(&attempt.complete)) {
            cx_gc_safepoint();
            native_yield();
        }
        cx_gc_run_blocking(join_worker, &thread);
        if (attempt.result != 0 || arena_run_attempt_callback_count != 0) {
            fputs("concurrent MemoryArena.Run was not rejected cleanly\n", stderr);
            arena_stress_result = 8;
            return;
        }
    }

    // The object-oriented API owns the active scope for the full callback.
    // Keep a managed object reachable only through a slot in that arena while
    // worker threads allocate and collect concurrently.
    parent_root = cxcore_System_Memory_Alloc(sizeof(cx_uint));
    *(cx_uint *)parent_root = 0xaced;
    arena_root_slot = (cx_ptr *)cx_arena_allocate_current(
        sizeof(cx_ptr), sizeof(cx_ptr));
    retain_only_in_active_arena(arena_root_slot);
    collections_before = cx_memory_test_collection_count_get();
    collected_before = cx_memory_test_collected_bytes_get();
    for (index = 0; index < 4; ++index) {
        contexts[index].slot = index;
        if (!start_worker(&threads[index], &contexts[index])) {
            arena_stress_result = 4;
            return;
        }
    }
    for (index = 0; index < 4; ++index) {
        while (!flag_get(&contexts[index].ready)) {
            cx_gc_safepoint();
            native_yield();
        }
    }
    // All worker roots exist only on parked stacks or registered static storage.
    cx_gc_collect();
    for (index = 0; index < 4; ++index) {
        flag_set(&contexts[index].release);
    }
    for (index = 0; index < 4; ++index) {
        while (!flag_get(&contexts[index].polling)) {
            cx_gc_safepoint();
            native_yield();
        }
    }
    cx_gc_collect();
    for (index = 0; index < 4; ++index) {
        flag_set(&contexts[index].resume);
    }
    // Keep extending an arena-owned object graph while workers allocate and
    // collect. The active arena's root slot is the only durable path to these
    // nodes after each local pointer is overwritten.
    for (index = 0; index < 128; ++index) {
        struct arena_gc_stress_node *node =
            (struct arena_gc_stress_node *)cx_arena_allocate_current(
                sizeof(*node), sizeof(cx_ptr));
        node->marker = 0xc0000000u + (cx_uint)index;
        (void)cx_gc_store_reference(&node->next, *arena_root_slot);
        (void)cx_gc_store_reference(arena_root_slot, node);
        if (index % 8 == 0) {
            cx_gc_safepoint();
            native_yield();
        }
    }
    for (index = 0; index < 4; ++index) {
        cx_gc_run_blocking(join_worker, &threads[index]);
        if (contexts[index].failed != 0) {
            fprintf(stderr, "worker %d lost a managed root: %d\n",
                    index, contexts[index].failed);
            arena_stress_result = 1;
            return;
        }
    }
    if (*(cx_uint *)parent_root != 0xaced ||
        cx_memory_test_collection_count_get() <= collections_before) {
        fputs("parent root lost or worker collections missing\n", stderr);
        arena_stress_result = 2;
        return;
    }
    if (arena_root_slot == NULL || *arena_root_slot == NULL) {
        fputs("MemoryArena.Run root was lost during worker collection\n", stderr);
        arena_stress_result = 6;
        return;
    }
    {
        cx_ptr current = *arena_root_slot;
        for (index = 127; index >= 0; --index) {
            const struct arena_gc_stress_node *node =
                (const struct arena_gc_stress_node *)current;
            if (node == NULL || node->marker != 0xc0000000u + (cx_uint)index) {
                fputs("arena graph lost a node during concurrent allocation and collection\n",
                      stderr);
                arena_stress_result = 9;
                return;
            }
            current = node->next;
        }
        if (current == NULL || *(cx_uint *)current != 0x6d22a11u) {
            fputs("arena graph lost its durable tail during worker collection\n",
                  stderr);
            arena_stress_result = 10;
            return;
        }
    }
    (void)cx_gc_store_reference((cx_ptr *)arena_root_slot, NULL);
    if (cx_memory_test_collected_bytes_get() <= collected_before) {
        fputs("unreachable worker allocations were not reclaimed\n", stderr);
        arena_stress_result = 3;
    }
}

int main(int argc, char **argv) {
    struct cxcore_System_MemoryArena arena;
    cx_action action = {0};
    if (argc == 2 && strcmp(argv[1], "invalid-detach") == 0) {
        cx_gc_detach_thread();
        return 20;
    }
    cx_gc_check_thread(); // Startup thread remains implicitly attached.
    if (argc == 2 && strcmp(argv[1], "invalid-blocking-entry") == 0) {
        cx_gc_run_blocking(invalid_managed_callback, NULL);
        return 21;
    }
    if (argc == 2 && strcmp(argv[1], "entry-after-detach") == 0) {
        cx_gc_detach_thread();
        cx_gc_check_thread();
        return 22;
    }
    if (argc != 1) {
        return 23;
    }
    // Metadata retry must allocate a fresh block after freeing its first attempt.
    cx_memory_test_fail_next_record_allocation();
    {
        cx_ptr allocation = cxcore_System_Memory_Alloc(sizeof(cx_uint));
        if (*(cx_uint *)allocation != 0) {
            fputs("metadata allocation retry did not return zeroed storage\n", stderr);
            return 5;
        }
        allocation = cxcore_System_Memory_Realloc(allocation, sizeof(cx_uint) * 2);
        *(cx_uint *)allocation = 0xaced;
    }
    cx_gc_collect(); // Detached records must not leave stale stack snapshots.
    cxcore_System_MemoryArena___constructor(&arena, 256u);
    action.__function = run_memory_arena_gc_stress;
    action.__environment = &arena;
    cxcore_System_MemoryArena_Run(&arena, action);
    cxcore_System_MemoryArena_Dispose(&arena);
    cx_gc_detach_thread();
    return arena_stress_result;
}

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
    stack_root = CX_ID_4(cxcore, System, Memory, Alloc)(64);
    *(cx_uint *)stack_root = 0x1234abcd;
    interior_allocation = CX_ID_4(cxcore, System, Memory, Alloc)(64);
    interior_root = (cx_byte *)interior_allocation + 8;
    *interior_root = 0x5a;
    interior_allocation = NULL;
    worker_roots[context->slot] = CX_ID_4(cxcore, System, Memory, Alloc)(sizeof(cx_uint));
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
        cx_ptr block = CX_ID_4(cxcore, System, Memory, Alloc)(4096);
        memset(block, 0x6b, 4096);
        block = CX_ID_4(cxcore, System, Memory, Realloc)(block, 8192);
        if (((cx_byte *)block)[4095] != 0x6b) {
            context->failed = 2;
        }
        if (index % 30 == 0) {
            cx_gc_collect();
        }
        CX_ID_4(cxcore, System, Memory, Free)(block);
        if (index % 50 == 0) {
            (void)CX_ID_4(cxcore, System, Memory, Alloc)(32768);
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

int main(int argc, char **argv) {
    test_thread threads[4];
    // Native callbacks update these flags while parent stacks are parked.
    // Keep native synchronization state outside the scanned stack regions.
    static struct test_context contexts[4];
    volatile cx_ptr parent_root;
    size_t collections_before;
    size_t collected_before;
    int index;
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
    parent_root = CX_ID_4(cxcore, System, Memory, Alloc)(sizeof(cx_uint));
    if (*(cx_uint *)parent_root != 0) {
        fputs("metadata allocation retry did not return zeroed storage\n", stderr);
        return 5;
    }
    parent_root = CX_ID_4(cxcore, System, Memory, Realloc)((cx_ptr)parent_root, sizeof(cx_uint) * 2);
    *(cx_uint *)parent_root = 0xaced;
    collections_before = cx_memory_test_collection_count_get();
    collected_before = cx_memory_test_collected_bytes_get();
    for (index = 0; index < 4; ++index) {
        contexts[index].slot = index;
        if (!start_worker(&threads[index], &contexts[index])) {
            return 4;
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
    for (index = 0; index < 4; ++index) {
        cx_gc_run_blocking(join_worker, &threads[index]);
        if (contexts[index].failed != 0) {
            fprintf(stderr, "worker %d lost a managed root: %d\n", index, contexts[index].failed);
            return 1;
        }
    }
    if (*(cx_uint *)parent_root != 0xaced ||
        cx_memory_test_collection_count_get() <= collections_before) {
        fputs("parent root lost or worker collections missing\n", stderr);
        return 2;
    }
    cx_gc_collect(); // Detached records must not leave stale stack snapshots.
    if (cx_memory_test_collected_bytes_get() <= collected_before) {
        fputs("unreachable worker allocations were not reclaimed\n", stderr);
        return 3;
    }
    cx_gc_detach_thread();
    return 0;
}

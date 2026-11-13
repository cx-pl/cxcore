#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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
static int flag_get(test_flag *flag) { return atomic_load_explicit(flag, memory_order_acquire) != 0; }
static void native_yield(void) { sched_yield(); }
#endif

#include "../src/cxcore.h"

#ifdef _WIN32
#define CX_TEST_NOINLINE __declspec(noinline)
#else
#define CX_TEST_NOINLINE __attribute__((noinline))
#endif

CX_API size_t cx_memory_test_collection_count_get(void);
typedef void (*test_hook)(void *context);
CX_API void cx_memory_test_set_native_call_pending_hook(test_hook hook, void *context);
CX_API void cx_memory_test_set_collection_started_hook(test_hook hook, void *context);

struct native_call_context {
    test_flag entered;
    test_flag collected;
    test_flag pending_published;
    test_flag collection_started;
    cx_ptr managed_value;
    cx_uint callback_result;
};

static void native_call_callback(void *argument) {
    struct native_call_context *context = (struct native_call_context *)argument;
    flag_set(&context->entered);
    while (!flag_get(&context->collected)) {
        native_yield();
    }
    if (context->managed_value == NULL) {
        context->callback_result = 1;
    }
}

#ifdef _WIN32
static DWORD WINAPI collector_entry(LPVOID argument) {
#else
static void *collector_entry(void *argument) {
#endif
    struct native_call_context *context = (struct native_call_context *)argument;
    cx_gc_attach_thread();
    while (!flag_get(&context->entered)) {
        native_yield();
    }
    cx_gc_collect();
    flag_set(&context->collected);
    cx_gc_detach_thread();
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

static int start_collector(test_thread *thread, struct native_call_context *context) {
#ifdef _WIN32
    *thread = CreateThread(NULL, 0, collector_entry, context, 0, NULL);
    return *thread != NULL;
#else
    return pthread_create(thread, NULL, collector_entry, context) == 0;
#endif
}

static void native_pending_hook(void *argument) {
    struct native_call_context *context = (struct native_call_context *)argument;
    flag_set(&context->pending_published);
    while (!flag_get(&context->collection_started)) {
        native_yield();
    }
}

static void collection_started_hook(void *argument) {
    struct native_call_context *context = (struct native_call_context *)argument;
    flag_set(&context->collection_started);
}

#ifdef _WIN32
static DWORD WINAPI pending_collector_entry(LPVOID argument) {
#else
static void *pending_collector_entry(void *argument) {
#endif
    struct native_call_context *context = (struct native_call_context *)argument;
    cx_gc_attach_thread();
    while (!flag_get(&context->pending_published)) {
        native_yield();
    }
    cx_gc_collect();
    flag_set(&context->collected);
    cx_gc_detach_thread();
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

static int start_pending_collector(test_thread *thread,
                                   struct native_call_context *context) {
#ifdef _WIN32
    *thread = CreateThread(NULL, 0, pending_collector_entry, context, 0, NULL);
    return *thread != NULL;
#else
    return pthread_create(thread, NULL, pending_collector_entry, context) == 0;
#endif
}

static int join_collector(test_thread thread) {
#ifdef _WIN32
    DWORD result = WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    return result == WAIT_OBJECT_0;
#else
    return pthread_join(thread, NULL) == 0;
#endif
}

static CX_TEST_NOINLINE void publish_context_value(struct native_call_context *context) {
    volatile cx_ptr value = cxcore_System_Memory_Alloc(sizeof(cx_uint));
    *(cx_uint *)value = 0x22a11u;
    context->managed_value = value;
    value = NULL;
}

static CX_TEST_NOINLINE int run_context_root_test(void) {
    struct native_call_context *context =
        (struct native_call_context *)calloc(1, sizeof(*context));
    test_thread collector;
    size_t collections_before;
    int passed;
    if (context == NULL) return 1;
    publish_context_value(context);
    collections_before = cx_memory_test_collection_count_get();
    if (!start_collector(&collector, context)) {
        free(context);
        return 2;
    }
    cx_gc_run_native_call_with_roots(
        native_call_callback, context, context, sizeof(*context));
    if (!join_collector(collector)) {
        free(context);
        return 3;
    }
    passed = context->callback_result == 0 &&
        cx_memory_test_collection_count_get() > collections_before &&
        cx_gc_is_durable_reference(context->managed_value) &&
        *(const cx_uint *)context->managed_value == 0x22a11u;
    context->managed_value = NULL;
    free(context);
    cx_gc_collect();
    return passed ? 0 : 4;
}

static CX_TEST_NOINLINE int run_pending_context_root_test(void) {
    struct native_call_context *context =
        (struct native_call_context *)calloc(1, sizeof(*context));
    test_thread collector;
    size_t collections_before;
    int passed;
    if (context == NULL) return 1;
    publish_context_value(context);
    collections_before = cx_memory_test_collection_count_get();
    cx_memory_test_set_collection_started_hook(
        collection_started_hook, context);
    if (!start_pending_collector(&collector, context)) {
        cx_memory_test_set_collection_started_hook(NULL, NULL);
        free(context);
        return 2;
    }
    cx_memory_test_set_native_call_pending_hook(native_pending_hook, context);
    cx_gc_run_native_call_with_roots(
        native_call_callback, context, context, sizeof(*context));
    cx_memory_test_set_native_call_pending_hook(NULL, NULL);
    if (!join_collector(collector)) {
        cx_memory_test_set_collection_started_hook(NULL, NULL);
        free(context);
        return 3;
    }
    cx_memory_test_set_collection_started_hook(NULL, NULL);
    passed = context->callback_result == 0 &&
        flag_get(&context->collection_started) &&
        cx_memory_test_collection_count_get() > collections_before &&
        cx_gc_is_durable_reference(context->managed_value) &&
        *(const cx_uint *)context->managed_value == 0x22a11u;
    context->managed_value = NULL;
    free(context);
    cx_gc_collect();
    return passed ? 0 : 4;
}

int main(void) {
    cx_gc_check_thread();
    if (run_context_root_test() != 0) {
        fputs("managed references in native call contexts must survive collection\n", stderr);
        return 1;
    }
    if (run_pending_context_root_test() != 0) {
        fputs("pending native call context roots must survive collection\n", stderr);
        return 2;
    }
    cx_gc_detach_thread();
    return 0;
}

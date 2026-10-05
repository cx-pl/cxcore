#include <stdint.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#define CX_TEST_NOINLINE __declspec(noinline)
#else
#include <pthread.h>
#define CX_TEST_NOINLINE __attribute__((noinline))
#endif

#include "../src/cxcore.h"

CX_API void cx_memory_test_fail_next_allocation(void);
CX_API size_t cx_memory_test_collection_count_get(void);
CX_API size_t cx_memory_test_collected_bytes_get(void);
CX_API size_t cx_memory_test_heap_bytes_get(void);

static cx_ptr static_root;
static cx_ptr aggregate_root;
static cx_ptr string_root;
static const struct cx_gc_root_region static_roots[] = {
    {&static_root, (cx_uint)sizeof(static_root)},
    {&aggregate_root, (cx_uint)sizeof(aggregate_root)},
    {&string_root, (cx_uint)sizeof(string_root)},
};

static void create_rooted_graph(void) {
    cx_ptr first = CX_ID_4(cxcore, System, Memory, Alloc)(sizeof(cx_ptr) * 2);
    cx_ptr second = CX_ID_4(cxcore, System, Memory, Alloc)(sizeof(cx_ptr));
    ((cx_ptr *)first)[0] = second;
    ((cx_ptr *)first)[1] = first;
    ((cx_uint *)second)[0] = 0x51a7u;
    static_root = first;
}

static void create_runtime_layout_graph(void) {
    struct CX_ID_3(cxcore, System, Array) *array = cx_array_new(1, (cx_uint)sizeof(cx_ptr));
    struct CX_ID_3(cxcore, System, Nullable) *nullable =
        (struct CX_ID_3(cxcore, System, Nullable) *)CX_ID_4(cxcore, System, Memory, Alloc)(
            (cx_uint)sizeof(*nullable));
    struct cx_iface_ref *interface_value =
        (struct cx_iface_ref *)CX_ID_4(cxcore, System, Memory, Alloc)(
            (cx_uint)sizeof(*interface_value));
    cx_ptr target = CX_ID_4(cxcore, System, Memory, Alloc)(sizeof(cx_uint));
    char *string_data =
        (char *)CX_ID_4(cxcore, System, Memory, Alloc)(5);
    struct CX_ID_3(cxcore, System, String) *string =
        (struct CX_ID_3(cxcore, System, String) *)CX_ID_4(cxcore, System, Memory, Alloc)(
            (cx_uint)sizeof(*string));

    ((cx_uint *)target)[0] = 0xacedu;
    interface_value->vtable = CX_NULL;
    interface_value->instance = target;
    nullable->_obj = interface_value;
    *(cx_ptr *)array->_data = nullable;
    memcpy(string_data, "root!", 5);
    CX_ID_5(cxcore, System, String, __constructor, _2)(string, 5, string_data);
    aggregate_root = array;
    string_root = string;
}

static CX_TEST_NOINLINE cx_bool retain_stack_root(void) {
    volatile cx_ptr stack_root = CX_ID_4(cxcore, System, Memory, Alloc)(64);
    ((cx_uint *)stack_root)[0] = 0x12345678u;
    cx_gc_collect();
    return stack_root != NULL && ((cx_uint *)stack_root)[0] == 0x12345678u;
}

static CX_TEST_NOINLINE void create_unrooted_cycle(void) {
    volatile cx_ptr first = CX_ID_4(cxcore, System, Memory, Alloc)(sizeof(cx_ptr));
    volatile cx_ptr second = CX_ID_4(cxcore, System, Memory, Alloc)(sizeof(cx_ptr));
    ((cx_ptr *)first)[0] = (cx_ptr)second;
    ((cx_ptr *)second)[0] = (cx_ptr)first;
    first = NULL;
    second = NULL;
}

static CX_TEST_NOINLINE void allocate_pressure(void) {
    cx_uint index;
    for (index = 0; index < 32; ++index) {
        (void)CX_ID_4(cxcore, System, Memory, Alloc)(65536);
    }
}

#ifdef _WIN32
static DWORD WINAPI allocate_from_second_thread(LPVOID context) {
    (void)context;
    cx_gc_check_thread();
    return 0;
}
#else
static void *allocate_from_second_thread(void *context) {
    (void)context;
    cx_gc_check_thread();
    return NULL;
}
#endif

int main(int argc, char **argv) {
    size_t collections_before;
    size_t collected_before;
    size_t heap_before;
    struct CX_ID_3(cxcore, System, Array) *aggregate_array;
    struct CX_ID_3(cxcore, System, Nullable) *aggregate_nullable;
    struct cx_iface_ref *aggregate_interface;

    if (argc == 2 && strcmp(argv[1], "second-thread") == 0) {
#ifdef _WIN32
        HANDLE thread;
        (void)CX_ID_4(cxcore, System, Memory, Alloc)(1);
        thread = CreateThread(NULL, 0, allocate_from_second_thread, NULL, 0, NULL);
        if (thread == NULL) {
            return 5;
        }
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
#else
        pthread_t thread;
        (void)CX_ID_4(cxcore, System, Memory, Alloc)(1);
        if (pthread_create(&thread, NULL, allocate_from_second_thread, NULL) != 0) {
            return 5;
        }
        pthread_join(thread, NULL);
#endif
        return 0;
    }
    if (argc != 1) {
        return 6;
    }

    cx_gc_register_static_roots(static_roots, (cx_uint)(sizeof(static_roots) / sizeof(static_roots[0])));
    if (!retain_stack_root()) {
        fputs("stack root was not retained\n", stderr);
        return 1;
    }

    create_rooted_graph();
    cx_gc_collect();
    if (static_root == NULL || ((cx_uint *)((cx_ptr *)static_root)[0])[0] != 0x51a7u ||
        ((cx_ptr *)static_root)[1] != static_root) {
        fputs("registered static root or object graph was not retained\n", stderr);
        return 2;
    }
    create_runtime_layout_graph();
    cx_gc_collect();
    aggregate_array = (struct CX_ID_3(cxcore, System, Array) *)aggregate_root;
    aggregate_nullable = aggregate_array == NULL
        ? NULL
        : (struct CX_ID_3(cxcore, System, Nullable) *)*(cx_ptr *)aggregate_array->_data;
    aggregate_interface = aggregate_nullable == NULL
        ? NULL : (struct cx_iface_ref *)aggregate_nullable->_obj;
    if (aggregate_interface == NULL || string_root == NULL ||
        ((cx_uint *)aggregate_interface->instance)[0] != 0xacedu ||
        memcmp(((struct CX_ID_3(cxcore, System, String) *)string_root)->_data, "root!", 5) != 0) {
        fputs("array, nullable, interface, or string references were not retained\n", stderr);
        return 7;
    }

    heap_before = cx_memory_test_heap_bytes_get();
    collections_before = cx_memory_test_collection_count_get();
    collected_before = cx_memory_test_collected_bytes_get();
    static_root = NULL;
    aggregate_root = NULL;
    string_root = NULL;
    aggregate_array = NULL;
    aggregate_nullable = NULL;
    aggregate_interface = NULL;
    create_unrooted_cycle();
    allocate_pressure();
    cx_gc_collect();
    if (cx_memory_test_collection_count_get() <= collections_before) {
        fputs("allocation pressure did not trigger collection\n", stderr);
        return 3;
    }
    if (cx_memory_test_collected_bytes_get() <= collected_before ||
        cx_memory_test_heap_bytes_get() >= heap_before + 32u * 65536u) {
        fputs("unreachable allocations were not reclaimed\n", stderr);
        return 4;
    }

    return 0;
}

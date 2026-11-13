#if !defined(_WIN32)
#define _GNU_SOURCE
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <windows.h>
#else
#include <sched.h>
#include <time.h>
#endif

#include "cxcore.h"
#include "../platform/common/cxarena.h"
#include "../platform/common/cxgc.h"
#include "../src/cxtext.api.h"
#include "../src/cxstringbuilder.api.h"

typedef struct cxcore_System_Reflection_TypeInfo cx_type_info;
typedef struct cxcore_System_Collections_Array cx_array;
typedef struct cxcore_System_String cx_string;
typedef struct cxcore_System_WideString cx_wide_string;
typedef struct cxcore_System_StringBuilder cx_string_builder;

struct bench_object {
    void *vtable;
    cx_int payload;
};

struct bench_field {
    cx_uint flags;
    cx_uint offset;
    cx_ptr type_info;
    const char *name;
};

struct bench_function {
    cx_uint flags;
    cx_uint slot;
    cx_ptr return_type_info;
    const char *name;
    const struct cx_reflection_parameter *parameters;
    cx_uint parameter_count;
};

static cx_int bench_iterations = 250000;
static cx_int bench_samples = 7;
static uint64_t bench_sink;
static volatile cx_int bench_interface_calls;

static void bench_interface_target(void) {
    ++bench_interface_calls;
}

static cx_type_info bench_base_type = {
    .Hash = 0x1001,
};
static cx_type_info bench_interface_type = {
    .Hash = 0x1003,
};
static union cx_vtable_entry bench_interface_vtable[] = {
    {.data = &bench_interface_type},
    {.function = bench_interface_target},
};
static const struct cx_interface_impl bench_interfaces[] = {
    {&bench_interface_type, bench_interface_vtable},
};
static const struct bench_field bench_fields[] = {
    {0, 0, &bench_base_type, "field0"},
    {0, 4, &bench_base_type, "field1"},
    {0, 8, &bench_base_type, "field2"},
    {0, 12, &bench_base_type, "field3"},
    {0, 16, &bench_base_type, "field4"},
    {0, 20, &bench_base_type, "field5"},
    {0, 24, &bench_base_type, "field6"},
    {0, 28, &bench_base_type, "field7"},
};
static const struct bench_function bench_functions[] = {
    {0, 0, &bench_base_type, "Function0", NULL, 0},
    {0, 1, &bench_base_type, "Function1", NULL, 0},
    {0, 2, &bench_base_type, "Function2", NULL, 0},
    {0, 3, &bench_base_type, "Function3", NULL, 0},
    {0, 4, &bench_base_type, "Function4", NULL, 0},
    {0, 5, &bench_base_type, "Function5", NULL, 0},
    {0, 6, &bench_base_type, "Function6", NULL, 0},
    {0, 7, &bench_base_type, "Function7", NULL, 0},
};
static const struct cx_runtime_type_info bench_runtime_type = {
    .interfaces = bench_interfaces,
    .interfaceCount = (cx_uint)(sizeof(bench_interfaces) / sizeof(bench_interfaces[0])),
    .fields = (const struct cx_reflection_field *)bench_fields,
    .fieldCount = (cx_uint)(sizeof(bench_fields) / sizeof(bench_fields[0])),
    .functions = (const struct cx_reflection_function *)bench_functions,
    .functionCount = (cx_uint)(sizeof(bench_functions) / sizeof(bench_functions[0])),
};
static cx_type_info bench_derived_type = {
    .Hash = 0x1002,
    .BaseType = {._obj = &bench_base_type},
    .RuntimeTypeInfo = (cx_ptr)&bench_runtime_type,
};
static union cx_vtable_entry bench_object_vtable[] = {
    {.data = &bench_derived_type},
};
static const char bench_text[] = "CX runtime benchmark: UTF-8 text payload";
static cx_string bench_string = {
    ._length = (cx_uint)(sizeof(bench_text) - 1),
    ._data = (cx_ptr)bench_text,
};
static struct bench_object bench_fixture_object = {
    .vtable = bench_object_vtable,
    .payload = 37,
};
static cx_int bench_exception_object;
static cx_ptr *bench_blocks;
static struct cx_arena *bench_arena;
static double bench_fixed_arena_destroy_elapsed_seconds;
static size_t bench_fixed_arena_peak_used_bytes;
static cx_ptr bench_request_result;
static size_t bench_durable_copy_bytes;
static double *bench_request_latency_ns;
static size_t bench_request_latency_count;
static cx_byte bench_copy_source[256];
static cx_byte bench_copy_destination[256];

#define BENCH_REQUEST_FIELD_COUNT 32u
static const cx_uint bench_request_input[BENCH_REQUEST_FIELD_COUNT] = {
    11, 23, 37, 41, 53, 67, 71, 83, 97, 101, 113, 127, 131, 149, 157, 163,
    179, 181, 193, 211, 223, 227, 239, 251, 257, 271, 277, 281, 293, 307, 311, 313
};
static const cx_byte bench_request_text_data[] = "values=7,11,13";
static const cx_byte bench_response_count_prefix_data[] = "count=";
static const cx_byte bench_response_sum_separator_data[] = ";sum=";
static const cx_byte bench_response_expected_data[] = "count=3;sum=62";
static cx_string bench_request_text = {
    ._length = (cx_uint)(sizeof(bench_request_text_data) - 1u),
    ._data = (cx_ptr)bench_request_text_data,
};
static cx_string bench_response_count_prefix = {
    ._length = (cx_uint)(sizeof(bench_response_count_prefix_data) - 1u),
    ._data = (cx_ptr)bench_response_count_prefix_data,
};
static cx_string bench_response_sum_separator = {
    ._length = (cx_uint)(sizeof(bench_response_sum_separator_data) - 1u),
    ._data = (cx_ptr)bench_response_sum_separator_data,
};

struct bench_durable_response {
    cx_uint request_id;
    cx_uint selected_count;
    uint64_t transformed_sum;
    uint64_t selected_sum;
};

struct bench_scoped_reference_context {
    cx_int iterations;
    uint64_t checksum;
};

#if defined(_MSC_VER)
#define BENCH_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define BENCH_NOINLINE __attribute__((noinline))
#else
#define BENCH_NOINLINE
#endif

static BENCH_NOINLINE void bench_unchecked_reference_store(
    cx_ptr *destination, cx_ptr source) {
    *destination = source;
}

struct bench_scoped_reference_value {
    cx_ptr reference;
    cx_uint scalar;
};

struct bench_public_witharena_context {
    cx_bool allocate_payload;
    uint64_t checksum;
};

typedef void (*bench_operation)(cx_int iterations);

struct bench_case {
    const char *name;
    bench_operation operation;
};

static double bench_now_seconds(void) {
#if defined(_WIN32)
    LARGE_INTEGER counter;
    static double frequency_value;
    LARGE_INTEGER frequency;
    if (frequency_value == 0.0) {
        QueryPerformanceFrequency(&frequency);
        frequency_value = (double)frequency.QuadPart;
    }
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / frequency_value;
#else
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        fputs("Could not read monotonic benchmark clock\n", stderr);
        exit(3);
    }
    return (double)now.tv_sec + (double)now.tv_nsec / 1000000000.0;
#endif
}

static cx_ptr bench_alloc(cx_uint size) {
    return cxcore_System_Memory_Alloc(size);
}

static void bench_free(cx_ptr block) {
    cxcore_System_Memory_Free(block);
}

static void bench_memory_alloc_free(cx_int iterations) {
    cx_int index;
    uint64_t checksum = 0;
    for (index = 0; index < iterations; ++index) {
        cx_ptr block = bench_alloc(64);
        checksum += (uint64_t)(uintptr_t)block & 0xffu;
        bench_free(block);
    }
    bench_sink += checksum;
}

static void bench_memory_bulk_alloc_free(cx_int iterations) {
    cx_int index;
    uint64_t checksum = 0;
    for (index = 0; index < iterations; ++index) {
        cx_uint *block = (cx_uint *)bench_alloc(64);
        *block = (cx_uint)index;
        bench_blocks[index] = block;
        checksum += *block;
    }
    for (index = 0; index < iterations; ++index) {
        bench_free(bench_blocks[index]);
    }
    bench_sink += checksum;
}

static void bench_arena_bulk_alloc_reset(cx_int iterations) {
    struct cx_arena_scope scope;
    cx_int index;
    uint64_t checksum = 0;
    cx_arena_enter(bench_arena, &scope);
    for (index = 0; index < iterations; ++index) {
        cx_uint *block = (cx_uint *)cx_arena_allocate_current(
            64, cx_arena_maximum_alignment());
        *block = (cx_uint)index;
        bench_blocks[index] = block;
        checksum += *block;
    }
    cx_arena_leave(&scope);
    cx_arena_reset(bench_arena);
    bench_sink += checksum;
}

#define BENCH_FIXED_ARENA_CAPACITY 4096u
#define BENCH_FIXED_ARENA_BATCH 64

static void bench_arena_fixed_capacity_destroy(cx_int iterations) {
    cx_int offset;
    uint64_t checksum = 0;
    double destroy_elapsed = 0.0;
    bench_fixed_arena_peak_used_bytes = 0;
    for (offset = 0; offset < iterations;) {
        cx_int count = iterations - offset;
        cx_int index;
        double destroy_start;
        if (count > BENCH_FIXED_ARENA_BATCH) count = BENCH_FIXED_ARENA_BATCH;
        for (index = 0; index < count; ++index) {
            struct cx_arena *arena = cx_arena_create(
                0, BENCH_FIXED_ARENA_CAPACITY, 1);
            struct cx_arena_scope scope;
            cx_uint *payload;
            struct cx_arena_statistics statistics = {0};
            if (arena == NULL) {
                fputs("Could not create fixed-capacity benchmark arena\n", stderr);
                exit(4);
            }
            cx_arena_enter(arena, &scope);
            payload = (cx_uint *)cx_arena_allocate_current(
                sizeof(*payload), cx_arena_maximum_alignment());
            *payload = (cx_uint)(offset + index);
            checksum += *payload;
            cx_arena_leave(&scope);
            cx_arena_get_statistics(arena, &statistics);
            if (statistics.peak_used_bytes > bench_fixed_arena_peak_used_bytes)
                bench_fixed_arena_peak_used_bytes = statistics.peak_used_bytes;
            bench_blocks[index] = arena;
        }
        destroy_start = bench_now_seconds();
        for (index = 0; index < count; ++index)
            cx_arena_destroy((struct cx_arena *)bench_blocks[index]);
        destroy_elapsed += bench_now_seconds() - destroy_start;
        offset += count;
    }
    bench_fixed_arena_destroy_elapsed_seconds = destroy_elapsed;
    bench_sink += checksum;
}

static void bench_public_witharena_callback(void *raw_context) {
    struct bench_public_witharena_context *context =
        (struct bench_public_witharena_context *)raw_context;
    if (context->allocate_payload) {
        cx_uint *payload = (cx_uint *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(*payload));
        if (payload == NULL) {
            fputs("Public WithArena allocation failed\n", stderr);
            exit(5);
        }
        *payload = 0x51a7u;
        context->checksum += *payload;
    }
}

static cx_action bench_public_witharena_action(
    struct bench_public_witharena_context *context) {
    cx_action action = {0};
    action.__function = bench_public_witharena_callback;
    action.__environment = context;
    return action;
}

static void bench_public_witharena_growable_empty(cx_int iterations) {
    cx_int index;
    for (index = 0; index < iterations; ++index) {
        struct bench_public_witharena_context context = {CX_FALSE, 0};
        cxcore_System_Memory_WithArena(
            bench_public_witharena_action(&context));
        bench_sink += context.checksum;
    }
}

static void bench_public_witharena_fixed_empty(cx_int iterations) {
    cx_int index;
    for (index = 0; index < iterations; ++index) {
        struct bench_public_witharena_context context = {CX_FALSE, 0};
        cxcore_System_Memory_WithArena__2(
            BENCH_FIXED_ARENA_CAPACITY,
            bench_public_witharena_action(&context));
        bench_sink += context.checksum;
    }
}

static void bench_public_witharena_fixed_alloc4(cx_int iterations) {
    cx_int index;
    for (index = 0; index < iterations; ++index) {
        struct bench_public_witharena_context context = {CX_TRUE, 0};
        cxcore_System_Memory_WithArena__2(
            BENCH_FIXED_ARENA_CAPACITY,
            bench_public_witharena_action(&context));
        bench_sink += context.checksum;
    }
}

static void bench_memory_copy_pointer_free(cx_int iterations) {
    cx_int index;
    uint64_t checksum = 0;
    for (index = 0; index < iterations; ++index) {
        cxcore_System_Memory_Copy(bench_copy_destination, bench_copy_source,
                                  (cx_uint)sizeof(bench_copy_source));
        checksum += bench_copy_destination[(cx_uint)index %
                                            (cx_uint)sizeof(bench_copy_destination)];
    }
    bench_sink += checksum;
}

static void bench_memory_copy_reference_sized(cx_int iterations) {
    cx_int index;
    uint64_t checksum = 0;
    const cx_uint size = (cx_uint)sizeof(struct bench_scoped_reference_value);
    for (index = 0; index < iterations; ++index) {
        cxcore_System_Memory_Copy(bench_copy_destination, bench_copy_source, size);
        checksum += bench_copy_destination[(cx_uint)index % size];
    }
    bench_sink += checksum;
}

static void bench_scoped_reference_store_callback(void *raw_context) {
    struct bench_scoped_reference_context *context =
        (struct bench_scoped_reference_context *)raw_context;
    cx_ptr sources[2];
    cx_ptr destination = CX_NULL;
    cx_int index;
    sources[0] = cx_arena_allocate_current(1, 1);
    sources[1] = cx_arena_allocate_current(1, 1);
    for (index = 0; index < context->iterations; ++index) {
        cx_ptr source = sources[index & 1];
        cx_gc_store_reference(&destination, source);
        context->checksum += destination == source;
    }
}

static void bench_unchecked_scoped_reference_store_callback(void *raw_context) {
    struct bench_scoped_reference_context *context =
        (struct bench_scoped_reference_context *)raw_context;
    cx_ptr sources[2];
    cx_ptr destination = CX_NULL;
    cx_int index;
    sources[0] = cx_arena_allocate_current(1, 1);
    sources[1] = cx_arena_allocate_current(1, 1);
    for (index = 0; index < context->iterations; ++index) {
        cx_ptr source = sources[index & 1];
        bench_unchecked_reference_store(&destination, source);
        context->checksum += destination == source;
    }
}

static void bench_scoped_reference_copy_callback(void *raw_context) {
    struct bench_scoped_reference_context *context =
        (struct bench_scoped_reference_context *)raw_context;
    struct bench_scoped_reference_value source = {0};
    struct bench_scoped_reference_value destination = {0};
    cx_int index;
    source.reference = cx_arena_allocate_current(1, 1);
    source.scalar = 42u;
    for (index = 0; index < context->iterations; ++index) {
        cxcore_System_Memory_Copy(&destination, &source,
                                  (cx_uint)sizeof(source));
        context->checksum += (uint64_t)(destination.reference != CX_NULL) +
            destination.scalar;
    }
}

static void bench_arena_scoped_reference_store(cx_int iterations) {
    struct bench_scoped_reference_context context = {iterations, 0};
    cx_arena_run_callback(bench_arena,
                          bench_scoped_reference_store_callback, &context);
    cx_arena_reset(bench_arena);
    bench_sink += context.checksum;
}

static void bench_arena_unchecked_scoped_reference_store(cx_int iterations) {
    struct bench_scoped_reference_context context = {iterations, 0};
    cx_arena_run_callback(bench_arena,
                          bench_unchecked_scoped_reference_store_callback,
                          &context);
    cx_arena_reset(bench_arena);
    bench_sink += context.checksum;
}

static void bench_arena_scoped_reference_copy(cx_int iterations) {
    struct bench_scoped_reference_context context = {iterations, 0};
    cx_arena_run_callback(bench_arena,
                          bench_scoped_reference_copy_callback, &context);
    cx_arena_reset(bench_arena);
    bench_sink += context.checksum;
}

static uint64_t bench_request_process(const cx_uint *parsed,
                                     cx_uint *validated,
                                     cx_uint *selected,
                                     cx_uint request_id) {
    cx_uint index;
    uint64_t transformed_sum = 0;
    uint64_t selected_sum = 0;
    for (index = 0; index < BENCH_REQUEST_FIELD_COUNT; ++index) {
        validated[index] = parsed[index] * 3u + (index & 7u);
        transformed_sum += validated[index];
        if ((index & 1u) != 0) {
            selected[index / 2u] = validated[index] ^ request_id;
            selected_sum += selected[index / 2u];
        }
    }
    return (transformed_sum << 1) ^ selected_sum;
}

static void bench_request_heap(cx_int iterations) {
    cx_int iteration;
    uint64_t checksum = 0;
    for (iteration = 0; iteration < iterations; ++iteration) {
        double request_start = bench_now_seconds();
        cx_uint *parsed = (cx_uint *)bench_alloc(
            BENCH_REQUEST_FIELD_COUNT * (cx_uint)sizeof(cx_uint));
        cx_uint *validated = (cx_uint *)bench_alloc(
            BENCH_REQUEST_FIELD_COUNT * (cx_uint)sizeof(cx_uint));
        cx_uint *selected = (cx_uint *)bench_alloc(
            (BENCH_REQUEST_FIELD_COUNT / 2u) * (cx_uint)sizeof(cx_uint));
        struct bench_durable_response *response;
        struct bench_durable_response response_value = {0};
        struct cx_arena_durable_scope durable_scope;
        cx_uint index;
        uint64_t selected_sum = 0;
        uint64_t transformed_digest;
        memcpy(parsed, bench_request_input, sizeof(bench_request_input));
        transformed_digest = bench_request_process(
            parsed, validated, selected, (cx_uint)iteration);
        for (index = 0; index < BENCH_REQUEST_FIELD_COUNT / 2u; ++index)
            selected_sum += selected[index];
        cx_arena_enter_durable(&durable_scope);
        response = (struct bench_durable_response *)bench_alloc(
            (cx_uint)sizeof(*response));
        response_value.request_id = (cx_uint)iteration;
        response_value.selected_count = BENCH_REQUEST_FIELD_COUNT / 2u;
        response_value.transformed_sum = transformed_digest;
        response_value.selected_sum = selected_sum;
        memcpy(response, &response_value, sizeof(response_value));
        cx_gc_store_reference(&bench_request_result, response);
        cx_arena_leave_durable(&durable_scope);
        bench_durable_copy_bytes += sizeof(*response);
        checksum += selected_sum;
        checksum += response->request_id + response->transformed_sum;
        bench_request_latency_ns[iteration] =
            (bench_now_seconds() - request_start) * 1000000000.0;
    }
    bench_request_latency_count = (size_t)iterations;
    bench_sink += checksum;
}

struct bench_request_arena_context {
    cx_uint request_id;
    uint64_t checksum;
};

static void bench_request_arena_once(void *raw_context) {
    struct bench_request_arena_context *context =
        (struct bench_request_arena_context *)raw_context;
    cx_uint *parsed = (cx_uint *)cx_arena_allocate_current(
        BENCH_REQUEST_FIELD_COUNT * sizeof(cx_uint), cx_arena_maximum_alignment());
    cx_uint *validated = (cx_uint *)cx_arena_allocate_current(
        BENCH_REQUEST_FIELD_COUNT * sizeof(cx_uint), cx_arena_maximum_alignment());
    cx_uint *selected = (cx_uint *)cx_arena_allocate_current(
        (BENCH_REQUEST_FIELD_COUNT / 2u) * sizeof(cx_uint), cx_arena_maximum_alignment());
    struct bench_durable_response *response;
    struct bench_durable_response response_value = {0};
    cx_uint index;
    uint64_t selected_sum = 0;
    uint64_t transformed_digest;
    memcpy(parsed, bench_request_input, sizeof(bench_request_input));
    transformed_digest = bench_request_process(
        parsed, validated, selected, context->request_id);
    for (index = 0; index < BENCH_REQUEST_FIELD_COUNT / 2u; ++index)
        selected_sum += selected[index];
    {
        struct cx_arena_durable_scope durable_scope;
        cx_arena_enter_durable(&durable_scope);
        response = (struct bench_durable_response *)bench_alloc(
            (cx_uint)sizeof(*response));
        response_value.request_id = context->request_id;
        response_value.selected_count = BENCH_REQUEST_FIELD_COUNT / 2u;
        response_value.transformed_sum = transformed_digest;
        response_value.selected_sum = selected_sum;
        memcpy(response, &response_value, sizeof(response_value));
        cx_gc_store_reference(&bench_request_result, response);
        cx_arena_leave_durable(&durable_scope);
    }
    bench_durable_copy_bytes += sizeof(*response);
    context->checksum += response->request_id + response->transformed_sum;
}

static void bench_request_arena(cx_int iterations) {
    cx_int iteration;
    uint64_t checksum = 0;
    for (iteration = 0; iteration < iterations; ++iteration) {
        double request_start = bench_now_seconds();
        struct bench_request_arena_context context = {
            (cx_uint)iteration, 0
        };
        struct cx_arena_scope scope;
        cx_arena_enter(bench_arena, &scope);
        bench_request_arena_once(&context);
        cx_arena_leave(&scope);
        cx_arena_reset(bench_arena);
        checksum += context.checksum;
        bench_request_latency_ns[iteration] =
            (bench_now_seconds() - request_start) * 1000000000.0;
    }
    bench_request_latency_count = (size_t)iterations;
    bench_sink += checksum;
}

static uint64_t bench_request_runtime_process(void) {
    cx_uint maximum_fields = bench_request_text._length;
    cx_int *parsed = (cx_int *)bench_alloc(
        maximum_fields * (cx_uint)sizeof(*parsed));
    cx_int *validated = (cx_int *)bench_alloc(
        maximum_fields * (cx_uint)sizeof(*validated));
    cx_int *selected = (cx_int *)bench_alloc(
        maximum_fields * (cx_uint)sizeof(*selected));
    cx_wide_string *wide_request =
        cxcore_System_String_ToWideString(&bench_request_text);
    cx_uint parsed_count = 0;
    cx_uint selected_count = 0;
    cx_uint index;
    cx_int current_value = 0;
    cx_int selected_sum = 0;
    cx_bool has_digits = CX_FALSE;
    struct cx_arena_durable_scope durable_scope;
    cx_string_builder *response_builder;
    cx_string *response;

    for (index = 0; index < maximum_fields; ++index) {
        cx_char character = cxcore_System_WideString_ScalarAt(
            wide_request, index);
        if (character >= '0' && character <= '9') {
            current_value = current_value * 10 + character - '0';
            has_digits = CX_TRUE;
        } else if (has_digits) {
            parsed[parsed_count++] = current_value;
            current_value = 0;
            has_digits = CX_FALSE;
        }
    }
    if (has_digits) parsed[parsed_count++] = current_value;

    for (index = 0; index < parsed_count; ++index) {
        if (parsed[index] > 0) validated[index] = parsed[index] * 2;
    }
    for (index = 0; index < parsed_count; ++index) {
        if (validated[index] > 0) {
            selected[selected_count++] = validated[index];
            selected_sum += validated[index];
        }
    }

    cx_arena_enter_durable(&durable_scope);
    response_builder = (cx_string_builder *)bench_alloc(
        (cx_uint)sizeof(*response_builder));
    cxcore_System_StringBuilder___constructor__2(response_builder, 32u);
    cxcore_System_StringBuilder_Append(
        response_builder, &bench_response_count_prefix);
    cxcore_System_StringBuilder_Append__5(response_builder, selected_count);
    cxcore_System_StringBuilder_Append(
        response_builder, &bench_response_sum_separator);
    cxcore_System_StringBuilder_Append__4(response_builder, selected_sum);
    response = cxcore_System_StringBuilder_ToString(response_builder);
    cx_gc_store_reference(&bench_request_result, response);
    cx_arena_leave_durable(&durable_scope);

    if (selected_count != 3u || selected_sum != 62 || response == NULL ||
        response->_length != sizeof(bench_response_expected_data) - 1u ||
        memcmp(response->_data, bench_response_expected_data,
               sizeof(bench_response_expected_data) - 1u) != 0) {
        fputs("Synthetic runtime request parse/serialize mismatch\n", stderr);
        exit(5);
    }
    bench_durable_copy_bytes += response->_length;
    return (uint64_t)selected_sum + selected_count + response->_length;
}

static void bench_request_runtime_heap(cx_int iterations) {
    cx_int iteration;
    uint64_t checksum = 0;
    for (iteration = 0; iteration < iterations; ++iteration) {
        double request_start = bench_now_seconds();
        checksum += bench_request_runtime_process();
        bench_request_latency_ns[iteration] =
            (bench_now_seconds() - request_start) * 1000000000.0;
    }
    bench_request_latency_count = (size_t)iterations;
    bench_sink += checksum;
}

static void bench_request_runtime_arena(cx_int iterations) {
    cx_int iteration;
    uint64_t checksum = 0;
    for (iteration = 0; iteration < iterations; ++iteration) {
        double request_start = bench_now_seconds();
        struct cx_arena_scope scope;
        cx_arena_enter(bench_arena, &scope);
        checksum += bench_request_runtime_process();
        cx_arena_leave(&scope);
        cx_arena_reset(bench_arena);
        bench_request_latency_ns[iteration] =
            (bench_now_seconds() - request_start) * 1000000000.0;
    }
    bench_request_latency_count = (size_t)iterations;
    bench_sink += checksum;
}

static int bench_compare_latency(const void *left, const void *right) {
    double left_value = *(const double *)left;
    double right_value = *(const double *)right;
    return left_value < right_value ? -1 : left_value > right_value ? 1 : 0;
}

static double bench_latency_percentile(double percentile) {
    size_t index;
    if (bench_request_latency_count == 0) return 0.0;
    index = (size_t)(percentile * (double)bench_request_latency_count);
    if (index == 0) index = 1;
    if (index > bench_request_latency_count)
        index = bench_request_latency_count;
    return bench_request_latency_ns[index - 1];
}

static void bench_object_alloc_free(cx_int iterations) {
    cx_int index;
    uint64_t checksum = 0;
    for (index = 0; index < iterations; ++index) {
        struct bench_object *object = (struct bench_object *)bench_alloc(
            (cx_uint)sizeof(*object));
        object->vtable = bench_object_vtable;
        object->payload = index;
        checksum += (uint64_t)object->payload;
        bench_free(object);
    }
    bench_sink += checksum;
}

static void bench_array_create_access_free(cx_int iterations) {
    const cx_uint count = 32;
    cx_int iteration;
    uint64_t checksum = 0;
    for (iteration = 0; iteration < iterations; ++iteration) {
        cx_uint index;
        cx_array *array = cx_array_new(count, (cx_uint)sizeof(cx_int));
        for (index = 0; index < count; ++index) {
            cx_int value = iteration + (cx_int)index;
            memcpy(cx_array_at(array, index, (cx_uint)sizeof(value)), &value, sizeof(value));
            checksum += value;
        }
        bench_free(array->_data);
        bench_free(array);
    }
    bench_sink += checksum;
}

static void bench_string_length(cx_int iterations) {
    cx_int index;
    uint64_t checksum = 0;
    for (index = 0; index < iterations; ++index) {
        checksum += cxcore_System_String_Length___const_get(&bench_string);
    }
    bench_sink += checksum;
}

static void bench_type_checks(cx_int iterations) {
    cx_int index;
    uint64_t checksum = 0;
    for (index = 0; index < iterations; ++index) {
        checksum += (uint64_t)cx_is_object(&bench_fixture_object, &bench_base_type);
        checksum += (uint64_t)cx_is_object(&bench_fixture_object, &bench_interface_type);
    }
    bench_sink += checksum;
}

static void bench_typeinfo_common_fields(cx_int iterations) {
    cx_int index;
    uint64_t checksum = 0;
    const volatile cx_type_info *type_info =
        &cxcore_System_Reflection_TypeInfo___typeinfo;
    for (index = 0; index < iterations; ++index) {
        checksum += type_info->Hash;
        checksum += type_info->Flags;
        checksum += type_info->Size;
        checksum += type_info->GenericArity;
        checksum += (uint64_t)(type_info->RuntimeTypeInfo != NULL);
    }
    bench_sink += checksum;
}

static void bench_typeinfo_assignability(cx_int iterations) {
    cx_int index;
    uint64_t checksum = 0;
    for (index = 0; index < iterations; ++index) {
        checksum += cxcore_System_Reflection_TypeInfo_IsAssignableFrom(&bench_base_type, bench_derived_type);
        checksum += cxcore_System_Reflection_TypeInfo_HasRuntimeInterface(&bench_derived_type,
                                                 bench_interface_type.Hash);
        checksum += cxcore_System_Reflection_TypeInfo_IsInBaseTypeChain(&bench_base_type, bench_derived_type);
    }
    bench_sink += checksum;
}

static void bench_interface_dispatch(cx_int iterations) {
    cx_int index;
    uint64_t checksum = 0;
    for (index = 0; index < iterations; ++index) {
        struct cx_iface_ref interface_reference = cx_checked_cast_object_to_interface(
            &bench_fixture_object, &bench_interface_type);
        union cx_vtable_entry *vtable = (union cx_vtable_entry *)interface_reference.vtable;
        if (interface_reference.instance != NULL && vtable[1].function != NULL) {
            vtable[1].function();
        }
        checksum += (uint64_t)bench_interface_calls;
    }
    bench_sink += checksum;
}

static void bench_reflection(cx_int iterations) {
    cx_int index;
    uint64_t checksum = 0;
    for (index = 0; index < iterations; ++index) {
        cx_uint field_count;
        cx_uint function_count;
        const struct cx_reflection_field *fields =
            cx_reflection_fields(&bench_derived_type, &field_count);
        const struct cx_reflection_function *functions =
            cx_reflection_functions(&bench_derived_type, &function_count);
        checksum += field_count + function_count;
        checksum += (uint64_t)fields[index % field_count].offset;
        checksum += functions[index % function_count].slot;
    }
    bench_sink += checksum;
}

static void bench_typeinfo_reflection(cx_int iterations) {
    cx_int index;
    uint64_t checksum = 0;
    const cx_type_info *type_info =
        &cxcore_System_Reflection_TypeInfo___typeinfo;
    for (index = 0; index < iterations; ++index) {
        cx_uint field_count;
        const struct cx_reflection_field *fields =
            cx_reflection_fields(type_info, &field_count);
        checksum += field_count;
        checksum += fields[index % field_count].offset;
        checksum += (uint64_t)fields[index % field_count].name[0];
    }
    bench_sink += checksum;
}

static void bench_exception_throw_catch(cx_int iterations) {
    cx_int index;
    uint64_t checksum = 0;
    for (index = 0; index < iterations; ++index) {
        struct cx_exception_frame frame;
        cx_exception_push(&frame);
        if (setjmp(frame.environment) == 0) {
            cx_exception_throw(&bench_exception_object, "cxcore benchmark", 1);
        }
        checksum += (uint64_t)(cx_exception_current() == &bench_exception_object);
        cx_exception_pop(&frame);
        cx_exception_clear();
    }
    bench_sink += checksum;
}

static void bench_mixed_runtime_path(cx_int iterations) {
    cx_int index;
    uint64_t checksum = 0;
    for (index = 0; index < iterations; ++index) {
        struct bench_object *object = (struct bench_object *)bench_alloc(
            (cx_uint)sizeof(*object));
        struct cx_iface_ref interface_reference;
        cx_array *array;
        cx_uint field_count;
        const struct cx_reflection_field *fields;
        object->vtable = bench_object_vtable;
        object->payload = index;
        interface_reference = cx_checked_cast_object_to_interface(
            object, &bench_interface_type);
        array = cx_array_new(4, (cx_uint)sizeof(cx_int));
        memcpy(cx_array_at(array, 0, (cx_uint)sizeof(cx_int)), &object->payload,
               sizeof(object->payload));
        checksum += (uint64_t)cx_is_object(object, &bench_base_type);
        checksum += cxcore_System_String_Length___const_get(&bench_string);
        fields = cx_reflection_fields(&bench_derived_type, &field_count);
        checksum += fields[index % field_count].offset;
        if (interface_reference.instance != NULL) {
            ((union cx_vtable_entry *)interface_reference.vtable)[1].function();
        }
        bench_free(array->_data);
        bench_free(array);
        bench_free(object);
    }
    bench_sink += checksum;
}

static const struct bench_case bench_cases[] = {
    {"memory_alloc_free", bench_memory_alloc_free},
    {"memory_bulk_alloc_free", bench_memory_bulk_alloc_free},
    {"arena_bulk_alloc_growth_reset", bench_arena_bulk_alloc_reset},
    {"arena_bulk_alloc_reset", bench_arena_bulk_alloc_reset},
    {"arena_fixed_capacity_destroy_4k", bench_arena_fixed_capacity_destroy},
    {"public_witharena_growable_empty", bench_public_witharena_growable_empty},
    {"public_witharena_fixed_4k_empty", bench_public_witharena_fixed_empty},
    {"public_witharena_fixed_4k_alloc4", bench_public_witharena_fixed_alloc4},
    {"memory_copy_256_pointer_free", bench_memory_copy_pointer_free},
    {"memory_copy_reference_sized_pointer_free",
     bench_memory_copy_reference_sized},
    {"arena_scoped_reference_store", bench_arena_scoped_reference_store},
    {"arena_scoped_reference_store_unchecked",
     bench_arena_unchecked_scoped_reference_store},
    {"arena_scoped_reference_copy", bench_arena_scoped_reference_copy},
    {"synthetic_request_heap", bench_request_heap},
    {"synthetic_request_arena", bench_request_arena},
    {"cx_runtime_request_heap", bench_request_runtime_heap},
    {"cx_runtime_request_arena", bench_request_runtime_arena},
    {"object_alloc_free", bench_object_alloc_free},
    {"array_create_access_free_32", bench_array_create_access_free},
    {"string_utf8_length", bench_string_length},
    {"object_and_interface_type_checks", bench_type_checks},
    {"typeinfo_common_field_access", bench_typeinfo_common_fields},
    {"typeinfo_assignability", bench_typeinfo_assignability},
    {"typeinfo_reflection_fields", bench_typeinfo_reflection},
    {"interface_cast_and_dispatch", bench_interface_dispatch},
    {"reflection_fields_and_functions", bench_reflection},
    {"exception_throw_catch", bench_exception_throw_catch},
    {"mixed_runtime_path", bench_mixed_runtime_path},
};

static cx_int bench_parse_positive(const char *text, cx_int *value) {
    char *end = NULL;
    unsigned long parsed = strtoul(text, &end, 10);
    if (text[0] == '\0' || end == text || *end != '\0' || parsed == 0 ||
        parsed > 100000000ul) {
        return 0;
    }
    *value = (cx_int)parsed;
    return 1;
}

int main(int argc, char **argv) {
    cx_int index;
    cx_int warmup;
    struct cx_gc_root_region benchmark_roots;
    struct cx_gc_root_region request_result_root;
    for (index = 1; index < argc; ++index) {
        if (strcmp(argv[index], "--iterations") == 0 && index + 1 < argc) {
            if (!bench_parse_positive(argv[++index], &bench_iterations)) {
                fputs("Invalid --iterations value\n", stderr);
                return 2;
            }
        } else if (strcmp(argv[index], "--samples") == 0 && index + 1 < argc) {
            if (!bench_parse_positive(argv[++index], &bench_samples) || bench_samples < 3) {
                fputs("--samples must be at least 3\n", stderr);
                return 2;
            }
        } else {
            fputs("Usage: cxcore_benchmarks [--iterations N] [--samples N]\n", stderr);
            return 2;
        }
    }

#if defined(_WIN32)
    if (SetThreadAffinityMask(GetCurrentThread(), (DWORD_PTR)1) == 0) {
        fputs("Could not pin benchmark thread to logical CPU 0\n", stderr);
        return 3;
    }
#else
    {
        cpu_set_t allowed_cpus;
        cpu_set_t selected_cpu;
        int cpu;
        if (sched_getaffinity(0, sizeof(allowed_cpus), &allowed_cpus) != 0) {
            fputs("Could not query benchmark CPU affinity\n", stderr);
            return 3;
        }
        for (cpu = 0; cpu < CPU_SETSIZE; ++cpu) {
            if (CPU_ISSET(cpu, &allowed_cpus)) break;
        }
        if (cpu == CPU_SETSIZE) {
            fputs("No allowed CPU is available for the benchmark\n", stderr);
            return 3;
        }
        CPU_ZERO(&selected_cpu);
        CPU_SET(cpu, &selected_cpu);
        if (sched_setaffinity(0, sizeof(selected_cpu), &selected_cpu) != 0) {
            fputs("Could not pin benchmark thread to an allowed logical CPU\n", stderr);
            return 3;
        }
    }
#endif

    bench_blocks = (cx_ptr *)malloc((size_t)bench_iterations * sizeof(*bench_blocks));
    if ((size_t)bench_iterations > SIZE_MAX / sizeof(*bench_request_latency_ns)) {
        fputs("Benchmark latency sample size overflow\n", stderr);
        return 4;
    }
    bench_request_latency_ns = (double *)malloc(
        (size_t)bench_iterations * sizeof(*bench_request_latency_ns));
    if (bench_blocks != NULL) {
        memset(bench_blocks, 0,
               (size_t)bench_iterations * sizeof(*bench_blocks));
    }
    bench_arena = cx_arena_create(0, 0, 0);
    if (bench_blocks == NULL || bench_request_latency_ns == NULL ||
        bench_arena == NULL) {
        fputs("Could not initialize allocation benchmark state\n", stderr);
        return 4;
    }
    for (index = 0; index < (cx_int)sizeof(bench_copy_source); ++index)
        bench_copy_source[index] = (cx_byte)(index & 0xff);
    benchmark_roots.address = bench_blocks;
    benchmark_roots.size = (cx_uint)((size_t)bench_iterations * sizeof(*bench_blocks));
    cx_gc_register_static_roots(&benchmark_roots, 1);
    request_result_root.address = &bench_request_result;
    request_result_root.size = (cx_uint)sizeof(bench_request_result);
    cx_gc_register_static_roots(&request_result_root, 1);

    puts("arch,benchmark,sample,iterations,elapsed_ms,ns_per_operation,checksum,arena_peak_used_bytes,arena_capacity_bytes,arena_growth_count,arena_reset_count,durable_copy_bytes,gc_collection_count_delta,gc_collected_bytes_delta,gc_pause_ns_delta,heap_live_bytes_after,request_latency_p50_ns,request_latency_p95_ns,request_latency_p99_ns,request_latency_max_ns");
    for (index = 0; index < (cx_int)(sizeof(bench_cases) / sizeof(bench_cases[0])); ++index) {
        cx_int warmup_iterations = bench_iterations / 10 + 1;
        if (index != 0) {
            memset(bench_blocks, 0,
                   (size_t)bench_iterations * sizeof(*bench_blocks));
            cx_arena_destroy(bench_arena);
            bench_arena = cx_arena_create(0, 0, 0);
            if (bench_arena == NULL) {
                fputs("Could not isolate arena benchmark case state\n", stderr);
                return 4;
            }
        }
        if (strcmp(bench_cases[index].name,
                   "arena_bulk_alloc_reset") == 0) {
            // Measure this case with capacity retained from a full untimed
            // batch so the timed rows exclude chunk growth.
            warmup_iterations = bench_iterations;
        }
        for (warmup = 0; warmup < 2; ++warmup) {
            bench_cases[index].operation(warmup_iterations);
        }
        for (warmup = 0; warmup < bench_samples; ++warmup) {
            double start;
            double elapsed;
            struct cx_arena_statistics arena_statistics = {0};
            struct cx_arena_statistics arena_statistics_before = {0};
            size_t collection_count_before = cx_memory_benchmark_collection_count_get();
            size_t collected_bytes_before = cx_memory_benchmark_collected_bytes_get();
            uint64_t collection_pause_ns_before = cx_memory_benchmark_collection_pause_ns_get();
            size_t collection_count_delta;
            size_t collected_bytes_delta;
            uint64_t collection_pause_ns_delta;
            size_t heap_live_bytes_after;
            double latency_p50;
            double latency_p95;
            double latency_p99;
            double latency_max;
            if (strcmp(bench_cases[index].name,
                       "arena_bulk_alloc_growth_reset") == 0) {
                // Start each growth sample from an empty arena; keep setup
                // outside the timed batch so the row measures allocator growth.
                memset(bench_blocks, 0,
                       (size_t)bench_iterations * sizeof(*bench_blocks));
                cx_arena_destroy(bench_arena);
                bench_arena = cx_arena_create(0, 0, 0);
                if (bench_arena == NULL) {
                    fputs("Could not prepare cold arena growth sample\n", stderr);
                    return 4;
                }
            }
            if (strcmp(bench_cases[index].name, "arena_bulk_alloc_growth_reset") == 0 ||
                strcmp(bench_cases[index].name, "arena_bulk_alloc_reset") == 0 ||
                strcmp(bench_cases[index].name, "synthetic_request_arena") == 0 ||
                strcmp(bench_cases[index].name, "cx_runtime_request_arena") == 0) {
                cx_arena_get_statistics(bench_arena, &arena_statistics_before);
            }
            bench_durable_copy_bytes = 0;
            bench_request_latency_count = 0;
            start = bench_now_seconds();
            bench_cases[index].operation(bench_iterations);
            elapsed = bench_now_seconds() - start;
            if (strcmp(bench_cases[index].name,
                       "arena_fixed_capacity_destroy_4k") == 0) {
                elapsed = bench_fixed_arena_destroy_elapsed_seconds;
                arena_statistics.peak_used_bytes =
                    bench_fixed_arena_peak_used_bytes;
                arena_statistics.capacity_bytes = BENCH_FIXED_ARENA_CAPACITY;
            }
            if (strcmp(bench_cases[index].name,
                       "public_witharena_fixed_4k_empty") == 0 ||
                strcmp(bench_cases[index].name,
                       "public_witharena_fixed_4k_alloc4") == 0) {
                arena_statistics.capacity_bytes = BENCH_FIXED_ARENA_CAPACITY;
            }
            if (strcmp(bench_cases[index].name, "arena_bulk_alloc_growth_reset") == 0 ||
                strcmp(bench_cases[index].name, "arena_bulk_alloc_reset") == 0 ||
                strcmp(bench_cases[index].name, "synthetic_request_arena") == 0 ||
                strcmp(bench_cases[index].name, "cx_runtime_request_arena") == 0) {
                cx_arena_get_statistics(bench_arena, &arena_statistics);
                arena_statistics.growth_count -= arena_statistics_before.growth_count;
                arena_statistics.reset_count -= arena_statistics_before.reset_count;
            }
            collection_count_delta = cx_memory_benchmark_collection_count_get() - collection_count_before;
            collected_bytes_delta = cx_memory_benchmark_collected_bytes_get() - collected_bytes_before;
            collection_pause_ns_delta = cx_memory_benchmark_collection_pause_ns_get() - collection_pause_ns_before;
            heap_live_bytes_after = cx_memory_benchmark_heap_bytes_get();
            if (bench_request_latency_count != 0) {
                qsort(bench_request_latency_ns, bench_request_latency_count,
                      sizeof(*bench_request_latency_ns), bench_compare_latency);
            }
            latency_p50 = bench_latency_percentile(0.50);
            latency_p95 = bench_latency_percentile(0.95);
            latency_p99 = bench_latency_percentile(0.99);
            latency_max = bench_request_latency_count == 0
                ? 0.0
                : bench_request_latency_ns[bench_request_latency_count - 1];
            printf("%s,%s,%d,%d,%.6f,%.3f,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%.3f,%.3f,%.3f,%.3f\n",
                   sizeof(void *) == 8 ? "x64" : "x86",
                   bench_cases[index].name,
                   (int)warmup + 1,
                   (int)bench_iterations,
                   elapsed * 1000.0,
                   elapsed * 1000000000.0 / (double)bench_iterations,
                   (unsigned long long)bench_sink,
                   (unsigned long long)arena_statistics.peak_used_bytes,
                   (unsigned long long)arena_statistics.capacity_bytes,
                   (unsigned long long)arena_statistics.growth_count,
                   (unsigned long long)arena_statistics.reset_count,
                   (unsigned long long)bench_durable_copy_bytes,
                   (unsigned long long)collection_count_delta,
                   (unsigned long long)collected_bytes_delta,
                   (unsigned long long)collection_pause_ns_delta,
                   (unsigned long long)heap_live_bytes_after,
                   latency_p50,
                   latency_p95,
                   latency_p99,
                   latency_max);
        }
    }
    cx_arena_destroy(bench_arena);
    free(bench_blocks);
    free(bench_request_latency_ns);
    return 0;
}

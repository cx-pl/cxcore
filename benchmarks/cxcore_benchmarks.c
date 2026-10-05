#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "cxcore.h"

typedef struct CX_ID_4(cxcore, System, Reflection, TypeInfo) cx_type_info;
typedef struct CX_ID_3(cxcore, System, Array) cx_array;
typedef struct CX_ID_3(cxcore, System, String) cx_string;

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

typedef void (*bench_operation)(cx_int iterations);

struct bench_case {
    const char *name;
    bench_operation operation;
};

static double bench_now_seconds(void) {
    LARGE_INTEGER counter;
    LARGE_INTEGER frequency;
    QueryPerformanceCounter(&counter);
    QueryPerformanceFrequency(&frequency);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
}

static cx_ptr bench_alloc(cx_uint size) {
    return CX_ID_4(cxcore, System, Memory, Alloc)(size);
}

static void bench_free(cx_ptr block) {
    CX_ID_4(cxcore, System, Memory, Free)(block);
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
        checksum += CX_ID_5(cxcore, System, String, Length, __const_get)(&bench_string);
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
        &CX_ID_5(cxcore, System, Reflection, TypeInfo, __typeinfo);
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
        checksum += CX_ID_5(cxcore, System, Reflection, TypeInfo,
                            IsAssignableFrom)(&bench_base_type, bench_derived_type);
        checksum += CX_ID_5(cxcore, System, Reflection, TypeInfo,
                            HasRuntimeInterface)(&bench_derived_type,
                                                 bench_interface_type.Hash);
        checksum += CX_ID_5(cxcore, System, Reflection, TypeInfo,
                            IsInBaseTypeChain)(&bench_base_type, bench_derived_type);
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
        &CX_ID_5(cxcore, System, Reflection, TypeInfo, __typeinfo);
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
        checksum += CX_ID_5(cxcore, System, String, Length, __const_get)(&bench_string);
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

    if (SetThreadAffinityMask(GetCurrentThread(), (DWORD_PTR)1) == 0) {
        fputs("Could not pin benchmark thread to logical CPU 0\n", stderr);
        return 3;
    }

    puts("arch,benchmark,sample,iterations,elapsed_ms,ns_per_operation,checksum");
    for (index = 0; index < (cx_int)(sizeof(bench_cases) / sizeof(bench_cases[0])); ++index) {
        for (warmup = 0; warmup < 2; ++warmup) {
            bench_cases[index].operation(bench_iterations / 10 + 1);
        }
        for (warmup = 0; warmup < bench_samples; ++warmup) {
            double start = bench_now_seconds();
            double elapsed;
            bench_cases[index].operation(bench_iterations);
            elapsed = bench_now_seconds() - start;
            printf("%s,%s,%d,%d,%.6f,%.3f,%llu\n",
                   sizeof(void *) == 8 ? "x64" : "x86",
                   bench_cases[index].name,
                   (int)warmup + 1,
                   (int)bench_iterations,
                   elapsed * 1000.0,
                   elapsed * 1000000000.0 / (double)bench_iterations,
                   (unsigned long long)bench_sink);
        }
    }
    return 0;
}

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#define CX_TEST_NOINLINE __declspec(noinline)
#else
#include <pthread.h>
#define CX_TEST_NOINLINE __attribute__((noinline))
#endif

#include "../src/cxcore.h"
#include "../include/cxruntime.h"
#include "../src/cxthreading.h"
#include "../platform/common/cxarena.h"
#include "../platform/common/cxgc.h"
#include "../platform/common/cxnumeric.h"

extern cx_ptr cxcore_test_System_Memory_AddObjectRoot(cx_ptr value);
extern void cxcore_test_System_Memory_RemoveObjectRoot(cx_ptr handle);

struct cx_test_object_root {
    struct cx_gc_root_handle *handle;
    cx_ptr value;
};

struct typed_copy_test_value {
    cx_ptr reference;
    cx_uint scalar;
};

typedef cx_int (*native_function_value_test_pointer)(cx_int);
struct native_function_value_test {
    native_function_value_test_pointer function;
    cx_ptr reference;
};

static cx_int native_function_value_test_callback(cx_int value) {
    return value + 9;
}

static CX_TEST_NOINLINE cx_ptr create_durable_test_object_root(void) {
    struct cxcore_System_Object *value =
        (struct cxcore_System_Object *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(*value));
    cxcore_System_Object___constructor(value);
    cx_ptr root = cxcore_test_System_Memory_AddObjectRoot((cx_ptr)value);
    value = NULL;
    return root;
}

struct native_layout_pointer_test_value {
    cx_ptr value;
    cx_uint scalar;
};

struct native_layout_nested_test_value {
    cx_uint scalar;
    struct {
        cx_ptr value;
        cx_uint marker;
    } items[2];
};

struct reflected_native_embedded_value {
    struct native_layout_pointer_test_value native;
    cx_uint scalar;
};

struct native_unknown_embedded_value {
    struct native_layout_pointer_test_value native;
    cx_uint scalar;
};

struct native_unknown_layout_copy_context {
    cx_bool copied_scoped_reference;
};

struct durable_reference_test_context {
    cx_ptr value;
    cx_bool is_durable;
    cx_bool attached_here;
};

struct logical_arena_reentry_context {
    struct cx_arena *arena;
    cx_bool allocation_succeeded;
};

struct active_arena_teardown_context {
    struct cx_arena *arena;
    cx_bool destroy;
};

struct arena_callback_exception_context {
    cx_bool invoked;
};

struct arena_public_callback_context {
    cx_bool observed_arena_ownership;
};

struct durable_override_test_context {
    cx_bool observed_durable_allocation;
};

static const struct cxcore_System_Reflection_TypeInfo typed_copy_reference_type = {
    .Flags = CX_REFLECTION_FLAG_TYPE_CLASS,
    .Size = (cx_uint)sizeof(cx_ptr),
};
static const struct cxcore_System_Reflection_TypeInfo typed_copy_scalar_type = {
    .Flags = CX_REFLECTION_FLAG_TYPE_ENUM,
    .Size = (cx_uint)sizeof(cx_uint),
};
static const struct cx_reflection_field typed_copy_fields[] = {
    {0u, (cx_uint)offsetof(struct typed_copy_test_value, reference),
     (cx_ptr)&typed_copy_reference_type, "reference",
     (cx_uint)sizeof(((struct typed_copy_test_value *)0)->reference)},
    {0u, (cx_uint)offsetof(struct typed_copy_test_value, scalar),
     (cx_ptr)&typed_copy_scalar_type, "scalar",
     (cx_uint)sizeof(((struct typed_copy_test_value *)0)->scalar)},
};
static const struct cxcore_System_Reflection_TypeInfo
    native_function_value_test_type = {
        .Flags = CX_REFLECTION_FLAG_TYPE_DELEGATE,
        .Size = (cx_uint)sizeof(native_function_value_test_pointer),
    };
static const struct cx_reflection_field native_function_value_test_fields[] = {
    {CX_REFLECTION_FLAG_FIELD_RAW_BYTES,
     (cx_uint)offsetof(struct native_function_value_test, function),
     (cx_ptr)&native_function_value_test_type, "function",
     (cx_uint)sizeof(((struct native_function_value_test *)0)->function)},
    {0u,
     (cx_uint)offsetof(struct native_function_value_test, reference),
     (cx_ptr)&typed_copy_reference_type, "reference",
     (cx_uint)sizeof(((struct native_function_value_test *)0)->reference)},
};
static const struct cx_runtime_type_info native_function_value_test_runtime_type = {
    .fields = native_function_value_test_fields,
    .fieldCount = 2u,
};
static const struct cxcore_System_Reflection_TypeInfo
    native_function_value_test_value_type = {
        .Hash = 0xC0DEC0D3u,
        .Flags = CX_REFLECTION_FLAG_TYPE_STRUCT,
        .Size = (cx_uint)sizeof(struct native_function_value_test),
        .RuntimeTypeInfo = (cx_ptr)&native_function_value_test_runtime_type,
    };
static const struct cx_runtime_type_info typed_copy_runtime_type = {
    .fields = typed_copy_fields,
    .fieldCount = 2u,
};

static const size_t reflected_native_pointer_offsets[] = {
    offsetof(struct native_layout_pointer_test_value, value),
};
static const struct cx_native_value_layout reflected_native_layout = {
    sizeof(struct native_layout_pointer_test_value),
    reflected_native_pointer_offsets,
    sizeof(reflected_native_pointer_offsets) / sizeof(reflected_native_pointer_offsets[0]),
    CX_TRUE,
};
static const struct cx_native_value_layout incomplete_native_layout = {
    sizeof(struct native_layout_pointer_test_value), CX_NULL, 0u, CX_FALSE,
};
static const struct cx_reflection_field reflected_native_embedded_fields[] = {
    {(cx_uint)CX_REFLECTION_FLAG_FIELD_NATIVE_LAYOUT,
     (cx_uint)offsetof(struct reflected_native_embedded_value, native),
     (cx_ptr)&reflected_native_layout, "native",
     (cx_uint)sizeof(((struct reflected_native_embedded_value *)0)->native)},
    {0u,
     (cx_uint)offsetof(struct reflected_native_embedded_value, scalar),
     (cx_ptr)&typed_copy_scalar_type, "scalar",
     (cx_uint)sizeof(((struct reflected_native_embedded_value *)0)->scalar)},
};
static const struct cx_runtime_type_info reflected_native_embedded_runtime_type = {
    .fields = reflected_native_embedded_fields,
    .fieldCount = 2u,
};
static const struct cxcore_System_Reflection_TypeInfo reflected_native_embedded_type = {
    .Hash = 0xC0DEC0D1u,
    .Flags = CX_REFLECTION_FLAG_TYPE_STRUCT,
    .Size = (cx_uint)sizeof(struct reflected_native_embedded_value),
    .RuntimeTypeInfo = (cx_ptr)&reflected_native_embedded_runtime_type,
};
static const struct cx_reflection_field native_unknown_embedded_fields[] = {
    {(cx_uint)CX_REFLECTION_FLAG_FIELD_NATIVE_LAYOUT,
     (cx_uint)offsetof(struct native_unknown_embedded_value, native),
     (cx_ptr)&incomplete_native_layout, "native",
     (cx_uint)sizeof(((struct native_unknown_embedded_value *)0)->native)},
    {0u,
     (cx_uint)offsetof(struct native_unknown_embedded_value, scalar),
     (cx_ptr)&typed_copy_scalar_type, "scalar",
     (cx_uint)sizeof(((struct native_unknown_embedded_value *)0)->scalar)},
};
static const struct cx_runtime_type_info native_unknown_embedded_runtime_type = {
    .fields = native_unknown_embedded_fields,
    .fieldCount = 2u,
};
static const struct cxcore_System_Reflection_TypeInfo native_unknown_embedded_type = {
    .Hash = 0xC0DEC0D2u,
    .Flags = CX_REFLECTION_FLAG_TYPE_STRUCT,
    .Size = (cx_uint)sizeof(struct native_unknown_embedded_value),
    .RuntimeTypeInfo = (cx_ptr)&native_unknown_embedded_runtime_type,
};
static const struct cxcore_System_Reflection_TypeInfo typed_copy_value_type = {
    .Hash = 0xC0DEC0DEu,
    .Flags = CX_REFLECTION_FLAG_TYPE_STRUCT,
    .Size = (cx_uint)sizeof(struct typed_copy_test_value),
    .RuntimeTypeInfo = (cx_ptr)&typed_copy_runtime_type,
};

CX_API void cx_memory_test_fail_next_allocation(void);
CX_API void cx_memory_test_fail_next_record_allocation(void);
CX_API void cx_memory_test_fail_next_static_root_index_allocation(void);
CX_API void cx_memory_test_reset_static_root_index(void);
CX_API cx_bool cx_memory_test_static_root_index_is_disabled(void);
CX_API size_t cx_memory_test_collection_count_get(void);
CX_API size_t cx_memory_test_collected_bytes_get(void);
CX_API size_t cx_memory_test_allocation_count_get(void);
CX_API size_t cx_memory_test_allocation_bytes_get(void);

static void store_scoped_reference_in_arena_callback(void *context) {
    cx_ptr local_slot = NULL;
    cx_ptr value = cx_arena_allocate_current(sizeof(cx_uint), sizeof(cx_uint));
    cx_gc_store_reference(&local_slot, value);
    *(int *)context = local_slot == value;
}

static void store_scoped_reference_into_outer_slot(void *context) {
    cx_ptr value = cx_arena_allocate_current(sizeof(cx_uint), sizeof(cx_uint));
    cx_gc_store_reference((cx_ptr *)context, value);
}

static void copy_unknown_native_value_to_stack(void *context) {
    struct native_unknown_layout_copy_context *result =
        (struct native_unknown_layout_copy_context *)context;
    struct native_layout_pointer_test_value source = {0};
    struct native_layout_pointer_test_value destination = {0};
    source.value = cx_arena_allocate_current(
        sizeof(cx_uint), sizeof(cx_uint));
    source.scalar = 0x4321u;
    (void)cx_gc_copy_native_value_unknown_layout(
        &destination, &source, sizeof(source));
    result->copied_scoped_reference =
        destination.value == source.value &&
        destination.scalar == source.scalar;
}

static void copy_unknown_reflected_value_to_stack(void *context) {
    struct native_unknown_layout_copy_context *result =
        (struct native_unknown_layout_copy_context *)context;
    struct native_unknown_embedded_value source = {0};
    struct native_unknown_embedded_value destination = {0};
    source.native.value = cx_arena_allocate_current(
        sizeof(cx_uint), sizeof(cx_uint));
    source.native.scalar = 0x4321u;
    source.scalar = 0x1234u;
    (void)cx_gc_copy_value(&destination, &source,
                           &native_unknown_embedded_type);
    result->copied_scoped_reference =
        destination.native.value == source.native.value &&
        destination.native.scalar == source.native.scalar &&
        destination.scalar == source.scalar;
}

static void throw_from_arena_callback(void *context) {
    struct cxcore_System_String *message;
    cx_ptr message_data;
    ((struct arena_callback_exception_context *)context)->invoked = CX_TRUE;
    (void)cx_arena_allocate_current(sizeof(cx_uint), sizeof(cx_uint));
    message = cxcore_System_Memory_Alloc((cx_uint)sizeof(*message));
    message_data = cxcore_System_Memory_Alloc(2u);
    memcpy(message_data, "x", 2u);
    cxcore_System_String___constructor__2(message, 2u, message_data);
    cx_exception_raise_message(message, __FILE__, __LINE__);
}

static void overflow_arena_callback(void *context) {
    (void)context;
    (void)cx_arena_allocate_current(33u, 1u);
}

static void record_public_arena_callback(void *context) {
    struct arena_public_callback_context *callback = context;
    void *allocation = cx_arena_allocate_current(sizeof(cx_uint), sizeof(cx_uint));
    callback->observed_arena_ownership =
        allocation != NULL && cx_arena_current_owns(allocation);
}

static void record_durable_override_callback(void *context) {
    struct durable_override_test_context *callback = context;
    cx_ptr allocation = cxcore_System_Memory_Alloc(16u);
    callback->observed_durable_allocation =
        allocation != NULL && cx_gc_is_durable_reference(allocation);
}

static void throw_from_durable_override_callback(void *context) {
    struct cxcore_System_String *message;
    cx_ptr message_data = (cx_ptr)"durable scope exception";
    record_durable_override_callback(context);
    message = cxcore_System_Memory_Alloc(
        (cx_uint)sizeof(*message));
    cxcore_System_String___constructor__2(
        message, 23u, message_data);
    cx_exception_raise_message(message, __FILE__, __LINE__);
}
CX_API size_t cx_memory_test_heap_bytes_get(void);

static int test_memory_budget(void) {
    size_t heap = cx_memory_test_heap_bytes_get();
    struct cx_arena *arena;
    if (heap > SIZE_MAX - 64u ||
        !cxcore_System_Memory_SetProcessMemoryBudget((cx_ulong)(heap + 64u))) return 91;
    arena = cx_arena_create(64u, 0u, 0);
    if (cxcore_System_Memory_SetProcessMemoryBudget((cx_ulong)(heap + 63u))) return 92;
    cx_arena_destroy(arena);
    if (!cxcore_System_Memory_SetProcessMemoryBudget(0u)) return 93;
    return 0;
}

static cx_ptr static_root;
static cx_ptr aggregate_root;
static cx_ptr string_root;
static cx_uint native_pointer_store_value;
#define CX_TEST_STATIC_ROOT_INDEX_COUNT 64u
static cx_ptr static_root_index_values[CX_TEST_STATIC_ROOT_INDEX_COUNT];
static struct cx_runtime_once arena_outer_initialization;
static struct cx_runtime_once arena_inner_initialization;
static struct cx_runtime_once arena_failed_initialization;
static struct cx_runtime_once arena_published_initialization;
static cx_ptr arena_published_root;
static uintptr_t arena_published_expected_address;
static const struct cx_gc_root_region static_roots[] = {
    {&static_root, (cx_uint)sizeof(static_root)},
    {&aggregate_root, (cx_uint)sizeof(aggregate_root)},
    {&string_root, (cx_uint)sizeof(string_root)},
    {&arena_published_root, (cx_uint)sizeof(arena_published_root)},
};

static void test_noop_action(void *environment) { (void)environment; }

struct arena_type_info_array_context {
    cx_bool type_info_survived_collection;
};

struct arena_module_info_context {
    cx_bool module_info_survived_collection;
};

struct arena_function_info_context {
    cx_bool function_info_survived_collection;
};

static void create_type_info_array_in_arena(void *environment) {
    struct arena_type_info_array_context *context =
        (struct arena_type_info_array_context *)environment;
    struct cxcore_System_Reflection_TypeInfo typeInfo =
        cx_typeof_array(cxcore_System_Int___typeinfo);
    struct cxcore_System_Reflection_TypeInfo *elementType;
    cx_gc_collect();
    if (typeInfo.GenericParams == NULL ||
        typeInfo.GenericParams->_length != 1u ||
        typeInfo.GenericParams->_data == NULL ||
        typeInfo.Name == NULL) return;
    elementType = (struct cxcore_System_Reflection_TypeInfo *)
        typeInfo.GenericParams->_data;
    context->type_info_survived_collection =
        elementType->Hash == cxcore_System_Int___typeinfo.Hash;
}

static cx_ptr create_arena_annotation_value(void) {
    return cxcore_System_Memory_Alloc((cx_uint)sizeof(cx_uint));
}

static struct cxcore_System_String *create_arena_test_string(const char *text) {
    size_t length = strlen(text);
    struct cxcore_System_String *result =
        (struct cxcore_System_String *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(*result));
    cx_ptr data = length == 0u ? CX_NULL :
        cxcore_System_Memory_Alloc((cx_uint)length);
    if (length != 0u) memcpy(data, text, length);
    cxcore_System_String___constructor__2(result, (cx_uint)length, data);
    return result;
}

static void create_module_info_in_arena(void *environment) {
    struct arena_module_info_context *context =
        (struct arena_module_info_context *)environment;
    const struct cxcore_System_Reflection_TypeInfo *types[] = {
        &cxcore_System_Int___typeinfo,
    };
    const struct cx_annotation_argument arguments[] = {
        {"argument", "42", create_arena_annotation_value},
    };
    const struct cx_annotation_info annotations[] = {
        {
            &cxcore_System_Int___typeinfo,
            3,
            1,
            "Function",
            "member",
            "type",
            arguments,
            (cx_uint)(sizeof(arguments) / sizeof(arguments[0])),
        },
    };
    const struct cx_source_location sources[] = {
        {"arena-module.cx", 17u, 5u},
    };
    const struct cx_module_info descriptor = {
        .formatVersion = CX_MODULE_METADATA_FORMAT_VERSION,
        .name = "arena-module",
        .version = "1.2.3",
        .description = "arena metadata",
        .author = "CX",
        .types = types,
        .typeCount = (cx_uint)(sizeof(types) / sizeof(types[0])),
        .annotations = annotations,
        .annotationCount = (cx_uint)(sizeof(annotations) / sizeof(annotations[0])),
        .annotationSources = sources,
    };
    struct cxcore_System_Reflection_ModuleInfo module =
        cx_module_info_wrap(&descriptor);
    struct cxcore_System_Reflection_AnnotationInfo annotation =
        cxcore_System_Reflection_ModuleInfo_GetAnnotation(&module, 0u);
    struct cxcore_System_Reflection_TypeInfo *type;
    struct cxcore_System_String *argument_name;
    struct cxcore_System_String *argument_expression;
    cx_ptr argument_value;

    cx_gc_collect();
    if (module._descriptor != &descriptor ||
        module.Name == NULL || module.Description == NULL ||
        module.Author == NULL || module.Types == NULL ||
        module.Types->_length != 1u || module.Types->_data == NULL ||
        module.Name->_length != 12u ||
        memcmp(module.Name->_data, "arena-module", 12u) != 0 ||
        module.Description->_length != 14u ||
        memcmp(module.Description->_data, "arena metadata", 14u) != 0 ||
        module.Author->_length != 2u ||
        memcmp(module.Author->_data, "CX", 2u) != 0) return;

    type = (struct cxcore_System_Reflection_TypeInfo *)module.Types->_data;
    if (type->Hash != cxcore_System_Int___typeinfo.Hash ||
        type->Name == NULL || annotation.Target == NULL ||
        annotation.MemberName == NULL || annotation.TypeName == NULL ||
        annotation.SourceFile == NULL || annotation.ArgumentNames == NULL ||
        annotation.ArgumentExpressions == NULL ||
        annotation.ArgumentValues == NULL ||
        annotation.ArgumentNames->_length != 1u ||
        annotation.ArgumentExpressions->_length != 1u ||
        annotation.ArgumentValues->_length != 1u) return;
    argument_name = (struct cxcore_System_String *)
        ((cx_ptr *)annotation.ArgumentNames->_data)[0];
    argument_expression = (struct cxcore_System_String *)
        ((cx_ptr *)annotation.ArgumentExpressions->_data)[0];
    argument_value = ((cx_ptr *)annotation.ArgumentValues->_data)[0];
    context->module_info_survived_collection =
        annotation.Target->_length == 8u &&
        memcmp(annotation.Target->_data, "Function", 8u) == 0 &&
        annotation.MemberName->_length == 6u &&
        memcmp(annotation.MemberName->_data, "member", 6u) == 0 &&
        annotation.TypeName->_length == 4u &&
        memcmp(annotation.TypeName->_data, "type", 4u) == 0 &&
        annotation.SourceFile->_length == 15u &&
        memcmp(annotation.SourceFile->_data, "arena-module.cx", 15u) == 0 &&
        annotation.SourceLine == 17u && annotation.SourceColumn == 5u &&
        argument_name != NULL && argument_name->_length == 8u &&
        memcmp(argument_name->_data, "argument", 8u) == 0 &&
        argument_expression != NULL && argument_expression->_length == 2u &&
        memcmp(argument_expression->_data, "42", 2u) == 0 &&
        argument_value != NULL && cx_arena_current_owns(argument_value);
}

static void create_function_info_in_arena(void *environment) {
    struct arena_function_info_context *context =
        (struct arena_function_info_context *)environment;
    struct cxcore_System_Reflection_TypeInfo *arenaType =
        (struct cxcore_System_Reflection_TypeInfo *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*arenaType));
    struct cx_reflection_parameter parameter = {0};
    struct cx_reflection_function function = {0};
    struct cx_runtime_type_info functionRuntime = {0};
    struct cx_runtime_type_info enumerableRuntime = {0};
    struct cx_runtime_type_info enumeratorRuntime = {0};
    struct cx_reflection_function getEnumerator = {0};
    struct cx_reflection_function current = {0};
    struct cxcore_System_Reflection_TypeInfo owner = {0};
    struct cxcore_System_Reflection_TypeInfo enumerableType = {0};
    struct cxcore_System_Reflection_TypeInfo enumeratorType = {0};
    struct cxcore_System_Reflection_TypeInfo parameterType;
    struct cxcore_System_Reflection_TypeInfo enumerableElementType;
    struct cxcore_System_Reflection_TypeInfo boxedType;
    struct cxcore_System_Object *boxedObject;
    struct cxcore_System_Reflection_FunctionParamInfo *parameterInfo;
    cx_ptr defaultValue;
    cx_int boxedScalar = 17;

    cx_reflection_copy_type_info(arenaType, &cxcore_System_Int___typeinfo);
    cx_gc_store_reference((cx_ptr *)&arenaType->Name,
                          (cx_ptr)create_arena_test_string("scoped-type"));
    parameter.flags = 7u;
    parameter.typeInfo = (cx_ptr)arenaType;
    parameter.name = "arena-parameter";
    parameter.defaultValue =
        cxcore_System_Memory_Alloc((cx_uint)sizeof(cx_uint));
    function.flags = 3u;
    function.slot = 11u;
    function.returnTypeInfo = (cx_ptr)&cxcore_System_Int___typeinfo;
    function.name = "arena-function";
    function.parameters = &parameter;
    function.parameterCount = 1u;
    struct cxcore_System_Reflection_FunctionInfo info =
        cx_reflection_get_function_info(&function);

    functionRuntime.functions = &function;
    functionRuntime.functionCount = 1u;
    owner.RuntimeTypeInfo = (cx_ptr)&functionRuntime;
    parameterType = cxcore_System_Reflection_TypeInfo_GetFunctionParameterType(
        &owner, 0u, 0u);

    current.flags = CX_REFLECTION_FLAG_FUNCTION_PROPERTY_GET;
    current.returnTypeInfo = (cx_ptr)arenaType;
    current.name = "Current";
    enumeratorRuntime.functions = &current;
    enumeratorRuntime.functionCount = 1u;
    enumeratorType.RuntimeTypeInfo = (cx_ptr)&enumeratorRuntime;
    getEnumerator.returnTypeInfo = (cx_ptr)&enumeratorType;
    getEnumerator.name = "GetEnumerator";
    enumerableRuntime.functions = &getEnumerator;
    enumerableRuntime.functionCount = 1u;
    enumerableType.Name = create_arena_test_string("IEnumerable");
    enumerableType.Namespace = create_arena_test_string("System.Collections");
    enumerableType.GenericArity = 1u;
    enumerableType.RuntimeTypeInfo = (cx_ptr)&enumerableRuntime;
    enumerableElementType =
        cxcore_System_Reflection_TypeInfo_GetEnumerableElementType(
            &enumerableType);
    boxedObject = (struct cxcore_System_Object *)cx_box_value(
        &boxedScalar, (cx_uint)sizeof(boxedScalar), arenaType);
    boxedType = cxcore_System_Object_Type___const_get(boxedObject);

    cx_gc_collect();
    if (info.Name == NULL || info.Params == NULL ||
        info.Params->_length != 1u || info.Params->_data == NULL) return;
    parameterInfo = (struct cxcore_System_Reflection_FunctionParamInfo *)
        info.Params->_data;
    defaultValue = parameterInfo->DefaultValue;
    context->function_info_survived_collection =
        info.Flags == 3u && info.Offset == 11u && info.Name->_length == 14u &&
        memcmp(info.Name->_data, "arena-function", 14u) == 0 &&
        info.ReturnType.Hash == cxcore_System_Int___typeinfo.Hash &&
        parameterInfo->Flags == 7u && parameterInfo->Name != NULL &&
        parameterInfo->Name->_length == 15u &&
        memcmp(parameterInfo->Name->_data, "arena-parameter", 15u) == 0 &&
        parameterInfo->Type.Hash == cxcore_System_Int___typeinfo.Hash &&
        parameterInfo->Type.Name != NULL &&
        parameterInfo->Type.Name->_length == 11u &&
        memcmp(parameterInfo->Type.Name->_data, "scoped-type", 11u) == 0 &&
        parameterType.Hash == cxcore_System_Int___typeinfo.Hash &&
        parameterType.Name != NULL &&
        parameterType.Name->_length == 11u &&
        memcmp(parameterType.Name->_data, "scoped-type", 11u) == 0 &&
        cx_arena_current_owns((cx_ptr)parameterType.Name) &&
        enumerableElementType.Hash == cxcore_System_Int___typeinfo.Hash &&
        enumerableElementType.Name != NULL &&
        enumerableElementType.Name->_length == 11u &&
        memcmp(enumerableElementType.Name->_data, "scoped-type", 11u) == 0 &&
        cx_arena_current_owns((cx_ptr)enumerableElementType.Name) &&
        boxedType.Hash == cxcore_System_Int___typeinfo.Hash &&
        boxedType.Name != NULL && boxedType.Name->_length == 11u &&
        memcmp(boxedType.Name->_data, "scoped-type", 11u) == 0 &&
        cx_arena_current_owns((cx_ptr)boxedType.Name) &&
        defaultValue != NULL && cx_arena_current_owns(defaultValue);
}

static void create_rooted_graph(void) {
    cx_ptr first = cxcore_System_Memory_Alloc(sizeof(cx_ptr) * 2);
    cx_ptr second = cxcore_System_Memory_Alloc(sizeof(cx_ptr));
    ((cx_ptr *)first)[0] = second;
    ((cx_ptr *)first)[1] = first;
    ((cx_uint *)second)[0] = 0x51a7u;
    static_root = first;
}

static void create_runtime_layout_graph(void) {
    struct cxcore_System_Collections_Array *array = cx_array_new(1, (cx_uint)sizeof(cx_ptr));
    struct cxcore_System_Nullable *nullable =
        (struct cxcore_System_Nullable *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(*nullable));
    struct cx_iface_ref *interface_value =
        (struct cx_iface_ref *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(*interface_value));
    cx_ptr target = cxcore_System_Memory_Alloc(sizeof(cx_uint));
    char *string_data =
        (char *)cxcore_System_Memory_Alloc(5);
    struct cxcore_System_String *string =
        (struct cxcore_System_String *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(*string));

    ((cx_uint *)target)[0] = 0xacedu;
    interface_value->vtable = CX_NULL;
    interface_value->instance = target;
    nullable->_obj = interface_value;
    *(cx_ptr *)array->_data = nullable;
    memcpy(string_data, "root!", 5);
    cxcore_System_String___constructor__2(string, 5, string_data);
    aggregate_root = array;
    string_root = string;
}

static CX_TEST_NOINLINE cx_bool retain_stack_root(void) {
    volatile cx_ptr stack_root = cxcore_System_Memory_Alloc(64);
    ((cx_uint *)stack_root)[0] = 0x12345678u;
    cx_gc_collect();
    return stack_root != NULL && ((cx_uint *)stack_root)[0] == 0x12345678u;
}

// Keep several live values across calls so optimized builds exercise both
// register roots and caller registers spilled by the collector's prologue.
static CX_TEST_NOINLINE cx_bool retain_optimized_roots(void) {
    cx_uint *first = (cx_uint *)cxcore_System_Memory_Alloc(64);
    cx_uint *second = (cx_uint *)cxcore_System_Memory_Alloc(80);
    cx_uint *third = (cx_uint *)cxcore_System_Memory_Alloc(96);
    cx_uint iteration;
    *first = 0x12345678u;
    *second = 0xabcdef01u;
    *third = 0x98765432u;
    for (iteration = 0; iteration < 16; ++iteration) {
        cx_gc_collect();
        if (*first != 0x12345678u + iteration ||
            *second != 0xabcdef01u - iteration ||
            *third != (0x98765432u ^ iteration)) return CX_FALSE;
        ++*first;
        --*second;
        *third = 0x98765432u ^ (iteration + 1);
    }
    return CX_TRUE;
}

static CX_TEST_NOINLINE void create_unrooted_cycle(void) {
    volatile cx_ptr first = cxcore_System_Memory_Alloc(sizeof(cx_ptr));
    volatile cx_ptr second = cxcore_System_Memory_Alloc(sizeof(cx_ptr));
    ((cx_ptr *)first)[0] = (cx_ptr)second;
    ((cx_ptr *)second)[0] = (cx_ptr)first;
    first = NULL;
    second = NULL;
}

static CX_TEST_NOINLINE void allocate_pressure(void) {
    cx_uint index;
    for (index = 0; index < 32; ++index) {
        (void)cxcore_System_Memory_Alloc(65536);
    }
}

static CX_TEST_NOINLINE void retain_only_in_arena(struct cx_arena *arena,
                                                  cx_ptr *slot) {
    struct cx_arena_durable_scope durable_scope;
    volatile cx_ptr value;
    (void)arena;
    cx_arena_enter_durable(&durable_scope);
    value = cxcore_System_Memory_Alloc((cx_uint)sizeof(cx_uint));
    cx_arena_leave_durable(&durable_scope);
    *(cx_uint *)value = 0x22a4u;
    (void)cx_gc_store_reference(slot, value);
    value = NULL;
}

struct arena_gc_cycle_node {
    cx_ptr next;
    cx_uint marker;
};

static CX_TEST_NOINLINE void retain_cycle_only_in_arena(cx_ptr *slot) {
    struct cx_arena_durable_scope durable_scope;
    struct arena_gc_cycle_node * volatile first;
    struct arena_gc_cycle_node * volatile second;
    cx_arena_enter_durable(&durable_scope);
    first = (struct arena_gc_cycle_node *)cxcore_System_Memory_Alloc(
        (cx_uint)sizeof(*first));
    second = (struct arena_gc_cycle_node *)cxcore_System_Memory_Alloc(
        (cx_uint)sizeof(*second));
    cx_arena_leave_durable(&durable_scope);
    if (first == NULL || second == NULL) {
        fputs("could not allocate durable arena-root cycle\n", stderr);
        exit(1);
    }
    first->marker = 0x31a7u;
    second->marker = 0x42b8u;
    (void)cx_gc_store_reference(&first->next, (cx_ptr)second);
    (void)cx_gc_store_reference(&second->next, (cx_ptr)first);
    (void)cx_gc_store_reference(slot, (cx_ptr)first);
    first = NULL;
    second = NULL;
}

static CX_TEST_NOINLINE void publish_durable_runtime_graph_into_arena(
    cx_ptr *array_slot, cx_ptr *string_slot) {
    struct cx_arena_durable_scope durable_scope;
    cx_arena_enter_durable(&durable_scope);
    create_runtime_layout_graph();
    cx_arena_leave_durable(&durable_scope);
    (void)cx_gc_store_reference(array_slot, aggregate_root);
    (void)cx_gc_store_reference(string_slot, string_root);
    (void)cx_gc_store_reference(&aggregate_root, NULL);
    (void)cx_gc_store_reference(&string_root, NULL);
}

static CX_TEST_NOINLINE void publish_durable_box_into_arena(cx_ptr *slot) {
    struct cx_arena_durable_scope durable_scope;
    cx_int value = 0x5a31;
    volatile cx_ptr boxed;
    cx_arena_enter_durable(&durable_scope);
    boxed = cx_box_value(&value, (cx_uint)sizeof(value),
                         &cxcore_System_Int___typeinfo);
    cx_arena_leave_durable(&durable_scope);
    if (boxed == NULL) {
        fputs("could not allocate durable boxed arena root\n", stderr);
        exit(1);
    }
    (void)cx_gc_store_reference(slot, boxed);
    boxed = NULL;
}

static CX_TEST_NOINLINE void retain_only_in_arena_interior_reference(
    cx_ptr *slot, cx_uint byte_offset) {
    struct cx_arena_durable_scope durable_scope;
    volatile cx_ptr value;
    cx_arena_enter_durable(&durable_scope);
    value = cxcore_System_Memory_Alloc(64u);
    cx_arena_leave_durable(&durable_scope);
    ((cx_byte *)value)[byte_offset] = 0x6du;
    (void)cx_gc_store_reference(
        slot, (cx_ptr)((cx_byte *)value + byte_offset));
    value = NULL;
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

static void check_foreign_thread_stack_reference(
    struct durable_reference_test_context *context) {
    context->attached_here = cx_gc_enter_thread();
    context->is_durable = cx_gc_is_durable_reference(context->value);
    cx_gc_leave_thread(context->attached_here);
}

static void reenter_logical_arena_on_worker(
    struct logical_arena_reentry_context *context) {
    struct cx_arena_scope scope;
    cx_bool attached_here = cx_gc_enter_thread();
    cx_arena_enter(context->arena, &scope);
    context->allocation_succeeded =
        cx_arena_allocate_current(sizeof(cx_uint), sizeof(cx_uint)) != NULL;
    cx_arena_leave(&scope);
    cx_gc_leave_thread(attached_here);
}

static void attempt_active_arena_teardown_on_worker(
    struct active_arena_teardown_context *context) {
    cx_bool attached_here = cx_gc_enter_thread();
    if (context->destroy) cx_arena_destroy(context->arena);
    else cx_arena_reset(context->arena);
    cx_gc_leave_thread(attached_here);
}

#ifdef _WIN32
static DWORD WINAPI check_foreign_thread_stack_reference_entry(LPVOID argument) {
    check_foreign_thread_stack_reference(
        (struct durable_reference_test_context *)argument);
    return 0;
}
static DWORD WINAPI reenter_logical_arena_on_worker_entry(LPVOID argument) {
    reenter_logical_arena_on_worker(
        (struct logical_arena_reentry_context *)argument);
    return 0;
}
static DWORD WINAPI attempt_active_arena_teardown_on_worker_entry(LPVOID argument) {
    attempt_active_arena_teardown_on_worker(
        (struct active_arena_teardown_context *)argument);
    return 0;
}
#else
static void *check_foreign_thread_stack_reference_entry(void *argument) {
    check_foreign_thread_stack_reference(
        (struct durable_reference_test_context *)argument);
    return NULL;
}
static void *reenter_logical_arena_on_worker_entry(void *argument) {
    reenter_logical_arena_on_worker(
        (struct logical_arena_reentry_context *)argument);
    return NULL;
}
static void *attempt_active_arena_teardown_on_worker_entry(void *argument) {
    attempt_active_arena_teardown_on_worker(
        (struct active_arena_teardown_context *)argument);
    return NULL;
}
#endif

int main(int argc, char **argv) {
    size_t collections_before;
    size_t collected_before;
    if (argc == 2 && strcmp(argv[1], "arena-cycle-root") == 0) {
        struct cx_arena *arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena_scope scope;
        cx_ptr *slot;
        cx_arena_enter(arena, &scope);
        slot = (cx_ptr *)cx_arena_allocate_current(sizeof(cx_ptr), sizeof(cx_ptr));
        if (slot == NULL) return 139;
        *slot = NULL;
        retain_cycle_only_in_arena(slot);
        cx_gc_collect();
        if (*slot == NULL ||
            ((struct arena_gc_cycle_node *)*slot)->marker != 0x31a7u ||
            ((struct arena_gc_cycle_node *)*slot)->next == NULL ||
            ((struct arena_gc_cycle_node *)
                ((struct arena_gc_cycle_node *)*slot)->next)->marker != 0x42b8u ||
            ((struct arena_gc_cycle_node *)
                ((struct arena_gc_cycle_node *)*slot)->next)->next != *slot) {
            fputs("active arena did not retain a durable object cycle\n", stderr);
            return 140;
        }
        cx_arena_leave(&scope);
        cx_arena_destroy(arena);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-destroy-removes-gc-registration") == 0) {
        struct cx_arena *arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena_scope scope;
        uintptr_t retired_address;
        cx_arena_enter(arena, &scope);
        void *retired_storage = cx_arena_allocate_current(32u, 16u);
        if (retired_storage == NULL || !cx_arena_current_owns(retired_storage))
            return 141;
        retired_address = (uintptr_t)retired_storage;
        cx_arena_leave(&scope);
        cx_arena_destroy(arena);
        if (!cx_gc_is_durable_reference((cx_ptr)retired_address)) {
            fputs("destroyed arena storage remained registered as scoped\n", stderr);
            return 142;
        }
        cx_gc_collect();
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-runtime-object-roots") == 0) {
        struct cx_arena *arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena_scope scope;
        cx_ptr *slots;
        struct cxcore_System_Collections_Array *array;
        struct cxcore_System_Nullable *nullable;
        struct cx_iface_ref *interface_value;
        struct cxcore_System_String *string;
        cx_ptr *boxed_value;
        cx_arena_enter(arena, &scope);
        slots = (cx_ptr *)cx_arena_allocate_current(
            3u * (cx_uint)sizeof(cx_ptr), (cx_uint)sizeof(cx_ptr));
        if (slots == NULL) return 141;
        slots[0] = NULL;
        slots[1] = NULL;
        slots[2] = NULL;
        publish_durable_runtime_graph_into_arena(&slots[0], &slots[1]);
        publish_durable_box_into_arena(&slots[2]);
        cx_gc_collect();
        array = (struct cxcore_System_Collections_Array *)slots[0];
        string = (struct cxcore_System_String *)slots[1];
        boxed_value = (cx_ptr *)cx_unbox_value(
            (struct cxcore_System_Object *)slots[2],
            &cxcore_System_Int___typeinfo);
        if (array == NULL || array->_length != 1u || array->_data == NULL ||
            string == NULL || string->_length != 5u || string->_data == NULL ||
            memcmp(string->_data, "root!", 5u) != 0 || boxed_value == NULL ||
            *(cx_int *)boxed_value != 0x5a31) {
            fputs("active arena failed to retain string, array, or boxed roots\n",
                  stderr);
            return 142;
        }
        nullable = (struct cxcore_System_Nullable *)
            ((cx_ptr *)array->_data)[0];
        if (nullable == NULL || nullable->_obj == NULL) return 143;
        interface_value = (struct cx_iface_ref *)nullable->_obj;
        if (interface_value->instance == NULL ||
            *(cx_uint *)interface_value->instance != 0xacedu) return 144;
        cx_arena_leave(&scope);
        cx_arena_destroy(arena);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-logical-reset-rejects-live-owner") == 0) {
        struct cx_arena *arena = cx_arena_create(64u, 0u, 0);
        cx_arena_retain_logical(arena);
        cx_arena_reset(arena);
        return 137;
    }
    if (argc == 2 && strcmp(argv[1], "arena-logical-destroy-rejects-live-owner") == 0) {
        struct cx_arena *arena = cx_arena_create(64u, 0u, 0);
        cx_arena_retain_logical(arena);
        cx_arena_destroy(arena);
        return 138;
    }
    if (argc == 2 &&
        (strcmp(argv[1], "arena-concurrent-reset-rejects-active-scope") == 0 ||
         strcmp(argv[1], "arena-concurrent-destroy-rejects-active-scope") == 0)) {
        struct cx_arena *arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena_scope scope;
        struct active_arena_teardown_context context = {
            arena,
            strcmp(argv[1], "arena-concurrent-destroy-rejects-active-scope") == 0
                ? CX_TRUE : CX_FALSE,
        };
        cx_arena_enter(arena, &scope);
#ifdef _WIN32
        {
            HANDLE thread = CreateThread(
                NULL, 0, attempt_active_arena_teardown_on_worker_entry,
                &context, 0, NULL);
            if (thread == NULL) return 139;
            WaitForSingleObject(thread, INFINITE);
            CloseHandle(thread);
        }
#else
        {
            pthread_t thread;
            if (pthread_create(&thread, NULL,
                    attempt_active_arena_teardown_on_worker_entry,
                    &context) != 0) return 139;
            if (pthread_join(thread, NULL) != 0) return 140;
        }
#endif
        cx_arena_leave(&scope);
        cx_arena_destroy(arena);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-logical-retention") == 0) {
        struct cx_arena *arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena_scope scope;
        struct cx_arena_statistics statistics;
        struct logical_arena_reentry_context reentry = {0};
        cx_ptr *slot;
        cx_arena_enter(arena, &scope);
        slot = (cx_ptr *)cx_arena_allocate_current(sizeof(cx_ptr), sizeof(cx_ptr));
        *slot = NULL;
        retain_only_in_arena(arena, slot);
        cx_arena_leave(&scope);
        cx_arena_retain_logical(arena);
        cx_arena_get_statistics(arena, &statistics);
        if (statistics.active_depth != 0u ||
            statistics.logical_reference_count != 1u) return 131;
        cx_gc_collect();
        if (*slot == NULL || *(cx_uint *)*slot != 0x22a4u) {
            fputs("logically retained arena did not retain its managed graph\n", stderr);
            return 132;
        }
        reentry.arena = arena;
#ifdef _WIN32
        {
            HANDLE thread = CreateThread(
                NULL, 0, reenter_logical_arena_on_worker_entry, &reentry, 0, NULL);
            if (thread == NULL) return 134;
            WaitForSingleObject(thread, INFINITE);
            CloseHandle(thread);
        }
#else
        {
            pthread_t thread;
            if (pthread_create(&thread, NULL,
                    reenter_logical_arena_on_worker_entry, &reentry) != 0) return 134;
            if (pthread_join(thread, NULL) != 0) return 135;
        }
#endif
        if (!reentry.allocation_succeeded) return 136;
        cx_arena_release_logical(arena);
        cx_arena_get_statistics(arena, &statistics);
        if (statistics.logical_reference_count != 0u) return 133;
        cx_arena_reset(arena);
        cx_arena_destroy(arena);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-fixed-metadata-bounded") == 0) {
        struct cx_arena *arena = cx_arena_create(0u, 256u, 1);
        struct cx_arena_scope scope;
        struct cx_arena_statistics statistics;
        void *block;
        cx_arena_enter(arena, &scope);
        cx_memory_test_fail_next_record_allocation();
        block = cxcore_System_Memory_Alloc(32u);
        if (block == NULL || !cx_arena_current_owns(block)) return 114;
        cx_arena_get_statistics(arena, &statistics);
        if (!statistics.fixed_capacity || statistics.capacity_bytes != 256u ||
            statistics.chunk_count != 1u || statistics.growth_count != 0u ||
            statistics.requested_bytes != 32u ||
            statistics.used_bytes <= statistics.requested_bytes) return 115;
        cx_arena_leave(&scope);
        cx_arena_destroy(arena);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-bounded-no-fallback") == 0) {
        struct cx_arena *arena = cx_arena_create(0u, 16384u, 1);
        struct cx_arena_scope scope;
        struct cx_arena_statistics statistics;
        size_t heap_before = cx_memory_test_heap_bytes_get();
        size_t allocations_before = cx_memory_test_allocation_count_get();
        size_t allocation_bytes_before = cx_memory_test_allocation_bytes_get();
        size_t bounded_collections_before = cx_memory_test_collection_count_get();
        size_t index;
        cx_arena_enter(arena, &scope);
        for (index = 0; index < 64u; ++index) {
            void *block = cx_arena_allocate(arena, 32u, 16u);
            if (!cx_arena_owns(arena, block)) return 137;
        }
        cx_arena_get_statistics(arena, &statistics);
        if (!statistics.fixed_capacity || statistics.capacity_bytes != 16384u ||
            statistics.chunk_count != 1u || statistics.growth_count != 0u ||
            statistics.allocation_count != 64u ||
            cx_memory_test_heap_bytes_get() != heap_before ||
            cx_memory_test_allocation_count_get() != allocations_before ||
            cx_memory_test_allocation_bytes_get() != allocation_bytes_before ||
            cx_memory_test_collection_count_get() != bounded_collections_before) {
            return 138;
        }
        cx_arena_leave(&scope);
        cx_arena_destroy(arena);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-numeric-scratch") == 0) {
        struct cx_arena *arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena_scope scope;
        struct cx_arena_statistics before;
        struct cx_arena_statistics after;
        const char text_bytes[] = "1.5";
        struct cxcore_System_String text = {0};
        struct cxcore_System_Nullable parsed;
        cx_arena_enter(arena, &scope);
        text._length = 3u;
        text._data = (cx_ptr)text_bytes;
        cx_arena_get_statistics(arena, &before);
        parsed = cx_numeric_double(&text);
        cx_arena_get_statistics(arena, &after);
        if (parsed._obj == NULL || !cx_arena_owns(arena, parsed._obj) ||
            *(double *)parsed._obj != 1.5 ||
            after.allocation_count != before.allocation_count + 1u) return 113;
        cx_arena_leave(&scope);
        cx_arena_destroy(arena);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "durable-rejects-foreign-stack") == 0) {
        cx_uint stack_value = 0x51a7u;
        struct durable_reference_test_context context = {
            (cx_ptr)&stack_value, CX_TRUE, CX_FALSE,
        };
        cx_gc_check_thread();
#ifdef _WIN32
        HANDLE thread = CreateThread(
            NULL, 0, check_foreign_thread_stack_reference_entry,
            &context, 0, NULL);
        if (thread == NULL) return 110;
        if (WaitForSingleObject(thread, INFINITE) != WAIT_OBJECT_0) return 111;
        CloseHandle(thread);
#else
        pthread_t thread;
        if (pthread_create(&thread, NULL,
                           check_foreign_thread_stack_reference_entry,
                           &context) != 0) return 110;
        if (pthread_join(thread, NULL) != 0) return 111;
#endif
        return context.attached_here && !context.is_durable ? 0 : 112;
    }
    if (argc == 2 && strcmp(argv[1], "arena-exception-unwinds-scopes") == 0) {
        struct cx_arena *outer = cx_arena_create(64, 0, 0);
        struct cx_arena *inner = cx_arena_create(64, 0, 0);
        struct cx_arena_scope outer_scope;
        struct cx_arena_scope inner_scope;
        struct cx_exception_frame frame;
        cx_ptr caught_exception;
        struct cxcore_System_String *caught_message;
        cx_arena_enter(outer, &outer_scope);
        cx_exception_push(&frame);
        if (setjmp(frame.environment) == 0) {
            struct cxcore_System_String *message;
            cx_ptr message_data;
            cx_arena_enter(inner, &inner_scope);
            message = cxcore_System_Memory_Alloc(
                (cx_uint)sizeof(*message));
            message_data = cxcore_System_Memory_Alloc(21u);
            memcpy(message_data, "test scoped exception", 21u);
            cxcore_System_String___constructor__2(message, 21u, message_data);
            cx_exception_raise_message(message,
                                       __FILE__, __LINE__);
        }
        caught_exception = cx_exception_current();
        if (!cx_gc_is_durable_reference(caught_exception)) return 71;
        caught_message = cxcore_System_Exception_Message___const_get(
            (const struct cxcore_System_Exception *)caught_exception);
        if (caught_message == NULL ||
            !cx_gc_is_durable_reference((cx_ptr)caught_message) ||
            !cx_gc_is_durable_reference(caught_message->_data) ||
            caught_message->_length != 21u ||
            memcmp(caught_message->_data, "test scoped exception", 21u) != 0) {
            return 75;
        }
        cx_gc_collect();
        if (cx_exception_current() != caught_exception) return 74;
        if (cxcore_System_Exception_Message___const_get(
                (const struct cxcore_System_Exception *)caught_exception) !=
            caught_message) return 76;
        if (cx_arena_async_use_allowed()) return 72;
        if (!cx_arena_current_owns(cxcore_System_Memory_Alloc(8))) return 73;
        cx_exception_pop(&frame);
        cx_exception_clear();
        cx_arena_leave(&outer_scope);
        cx_arena_destroy(inner);
        cx_arena_destroy(outer);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-callback-exception-unwinds") == 0) {
        struct cx_exception_frame frame;
        struct arena_callback_exception_context callback_context = { CX_FALSE };
        cx_action action = {0};
        cx_ptr exception;
        struct cxcore_System_String *message;
        size_t heap_bytes;
        action.__function = throw_from_arena_callback;
        action.__environment = &callback_context;
        cx_exception_push(&frame);
        if (setjmp(frame.environment) == 0) {
            cxcore_System_Memory_WithArena(action);
            return 77;
        }
        if (!callback_context.invoked) return 78;
        if (!cx_arena_async_use_allowed()) return 79;
        exception = cx_exception_current();
        if (!cx_gc_is_durable_reference(exception)) return 80;
        message = cxcore_System_Exception_Message___const_get(
            (const struct cxcore_System_Exception *)exception);
        if (message == NULL || message->_length != 2u ||
            memcmp(message->_data, "x", 2u) != 0) return 81;
        heap_bytes = cx_memory_test_heap_bytes_get();
        if (!cxcore_System_Memory_SetProcessMemoryBudget((cx_ulong)heap_bytes))
            return 82;
        cx_exception_pop(&frame);
        cx_exception_clear();
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-zero-initial-growth") == 0) {
        struct cx_arena *arena = cx_arena_create(0, 0, 0);
        struct cx_arena_scope scope;
        struct cx_arena_statistics statistics;
        void *block;
        cx_arena_enter(arena, &scope);
        block = cx_arena_allocate(arena, 32, 16);
        if (block == NULL || !cx_arena_owns(arena, block)) return 75;
        cx_arena_leave(&scope);
        cx_arena_get_statistics(arena, &statistics);
        if (statistics.chunk_count != 1 || statistics.growth_count != 0 ||
            statistics.requested_bytes != 32) return 76;
        cx_arena_destroy(arena);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-reset-zeroes-reused-allocation") == 0) {
        struct cx_arena *arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena_scope scope;
        struct cx_arena_statistics before;
        struct cx_arena_statistics after;
        unsigned char *first;
        unsigned char *reused;
        size_t index;

        cx_arena_enter(arena, &scope);
        first = (unsigned char *)cx_arena_allocate(arena, 64u, 32u);
        if (first == NULL || ((uintptr_t)first & 31u) != 0u) return 138;
        memset(first, 0xA5, 64u);
        cx_arena_leave(&scope);
        cx_arena_get_statistics(arena, &before);
        if (before.allocation_count != 1u || before.requested_bytes != 64u ||
            before.chunk_count != 1u || !cx_arena_owns(arena, first)) return 139;

        cx_arena_reset(arena);
        cx_arena_get_statistics(arena, &after);
        if (after.allocation_count != 0u || after.requested_bytes != 0u ||
            after.chunk_count != before.chunk_count ||
            after.capacity_bytes != before.capacity_bytes ||
            after.generation != before.generation + 1u ||
            after.reset_count != before.reset_count + 1u ||
            cx_arena_owns(arena, first)) return 140;

        cx_arena_enter(arena, &scope);
        reused = (unsigned char *)cx_arena_allocate(arena, 64u, 32u);
        if (reused != first || ((uintptr_t)reused & 31u) != 0u) return 141;
        for (index = 0; index < 64u; ++index)
            if (reused[index] != 0u) return 142;
        cx_arena_leave(&scope);
        cx_arena_reset(arena);
        cx_arena_destroy(arena);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-array-allocation-follows-owner") == 0) {
        struct cx_arena *outer = cx_arena_create(256, 0, 0);
        struct cx_arena *inner = cx_arena_create(256, 0, 0);
        struct cx_arena_scope outer_scope;
        struct cx_arena_scope inner_scope;
        cx_ptr durable_owner = cxcore_System_Memory_Alloc((cx_uint)sizeof(cx_ptr));
        cx_ptr outer_owner;
        cx_ptr inner_owner;
        struct cxcore_System_Collections_Array *durable_array;
        struct cxcore_System_Collections_Array *outer_array;
        struct cxcore_System_Collections_Array *inner_array;

        cx_arena_enter(outer, &outer_scope);
        outer_owner = cx_arena_allocate_current(sizeof(cx_ptr), sizeof(cx_ptr));
        cx_arena_enter(inner, &inner_scope);
        inner_owner = cx_arena_allocate_current(sizeof(cx_ptr), sizeof(cx_ptr));

        durable_array = cx_array_new_for_owner(durable_owner, 3, sizeof(cx_uint));
        if (cx_arena_owns(outer, durable_array) ||
            cx_arena_owns(inner, durable_array) ||
            cx_arena_owns(outer, durable_array->_data) ||
            cx_arena_owns(inner, durable_array->_data)) return 77;

        outer_array = cx_array_new_for_owner(outer_owner, 3, sizeof(cx_uint));
        if (!cx_arena_owns(outer, outer_array) ||
            !cx_arena_owns(outer, outer_array->_data) ||
            cx_arena_owns(inner, outer_array) ||
            cx_arena_owns(inner, outer_array->_data)) return 78;

        inner_array = cx_array_new_for_owner(inner_owner, 3, sizeof(cx_uint));
        if (!cx_arena_owns(inner, inner_array) ||
            !cx_arena_owns(inner, inner_array->_data)) return 79;

        cx_arena_leave(&inner_scope);
        cx_arena_leave(&outer_scope);
        cx_gc_store_reference((cx_ptr *)durable_owner, durable_array);
        durable_array = NULL;
        cx_gc_collect();
        durable_array = (struct cxcore_System_Collections_Array *)
            *(cx_ptr *)durable_owner;
        if (durable_array == NULL || durable_array->_length != 3 ||
            durable_array->_data == NULL) return 80;
        cx_arena_destroy(inner);
        cx_arena_destroy(outer);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-object-allocation-follows-owner") == 0) {
        struct cx_arena *outer = cx_arena_create(256, 0, 0);
        struct cx_arena *inner = cx_arena_create(256, 0, 0);
        struct cx_arena_scope outer_scope;
        struct cx_arena_scope inner_scope;
        struct cx_arena_durable_scope durable_scope;
        cx_ptr durable_owner = cxcore_System_Memory_Alloc((cx_uint)sizeof(cx_ptr));
        cx_ptr outer_owner;
        cx_ptr inner_owner;
        cx_ptr durable_block;
        cx_ptr outer_block;
        cx_ptr inner_block;
        cx_ptr durable_block_during_override;
        cx_ptr inner_block_during_override;

        cx_arena_enter(outer, &outer_scope);
        outer_owner = cx_arena_allocate_current(sizeof(cx_ptr), sizeof(cx_ptr));
        cx_arena_enter(inner, &inner_scope);
        inner_owner = cx_arena_allocate_current(sizeof(cx_ptr), sizeof(cx_ptr));
        durable_block = cx_memory_alloc_for_owner(durable_owner, 24u);
        outer_block = cx_memory_alloc_for_owner(outer_owner, 24u);
        inner_block = cx_memory_alloc_for_owner(inner_owner, 24u);
        cx_arena_enter_durable(&durable_scope);
        durable_block_during_override = cx_memory_alloc_for_owner(durable_owner, 24u);
        inner_block_during_override = cx_memory_alloc_for_owner(inner_owner, 24u);
        cx_arena_leave_durable(&durable_scope);
        if (cx_arena_owns(outer, durable_block) || cx_arena_owns(inner, durable_block) ||
            !cx_arena_owns(outer, outer_block) || cx_arena_owns(inner, outer_block) ||
            !cx_arena_owns(inner, inner_block) ||
            cx_arena_owns(outer, durable_block_during_override) ||
            cx_arena_owns(inner, durable_block_during_override) ||
            !cx_arena_owns(inner, inner_block_during_override)) return 82;
        cx_arena_leave(&inner_scope);
        cx_arena_leave(&outer_scope);
        cx_arena_destroy(inner);
        cx_arena_destroy(outer);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-array-rejects-padding-owner") == 0) {
        struct cx_arena *arena = cx_arena_create(256, 0, 0);
        struct cx_arena_scope scope;
        cx_ptr owner;
        cx_arena_enter(arena, &scope);
        owner = cx_arena_allocate_current(sizeof(cx_ptr), sizeof(cx_ptr));
        (void)cx_array_new_for_owner((const unsigned char *)owner + sizeof(cx_ptr),
                                     1, sizeof(cx_uint));
        return 81;
    }
    if (argc == 2 && strcmp(argv[1], "arena-string-builder-growth-follows-owner") == 0) {
        struct cx_arena *outer = cx_arena_create(256, 0, 0);
        struct cx_arena *inner = cx_arena_create(256, 0, 0);
        struct cx_arena_scope outer_scope;
        struct cx_arena_scope inner_scope;
        struct cxcore_System_StringBuilder *builder;
        cx_ptr old_data;

        cx_arena_enter(outer, &outer_scope);
        builder = (struct cxcore_System_StringBuilder *)cx_arena_allocate_current(
            sizeof(*builder), cx_arena_maximum_alignment());
        cxcore_System_StringBuilder___constructor__2(builder, 8u);
        old_data = builder->_data;
        ((cx_byte *)builder->_data)[0] = 'x';
        builder->_length = 1u;

        cx_arena_enter(inner, &inner_scope);
        cxcore_System_StringBuilder_EnsureCapacity(builder, 128u);
        if (builder->_data == old_data || !cx_arena_owns(outer, builder->_data) ||
            cx_arena_owns(inner, builder->_data) ||
            ((cx_byte *)builder->_data)[0] != 'x') return 82;
        cx_arena_leave(&inner_scope);
        cx_arena_leave(&outer_scope);
        cx_arena_destroy(inner);
        cx_arena_destroy(outer);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-string-builder-to-string-follows-context") == 0) {
        struct cx_arena *arena = cx_arena_create(512u, 0u, 0);
        struct cx_arena_scope arena_scope;
        struct cx_arena_durable_scope durable_scope;
        struct cxcore_System_StringBuilder *durable_builder;
        struct cxcore_System_StringBuilder *arena_builder;
        struct cxcore_System_String *durable_result;
        struct cxcore_System_String *arena_result;

        durable_builder = (struct cxcore_System_StringBuilder *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*durable_builder));
        cxcore_System_StringBuilder___constructor__2(durable_builder, 8u);
        cxcore_System_StringBuilder_Append__4(durable_builder, 22);

        cx_arena_enter(arena, &arena_scope);
        durable_result = cxcore_System_StringBuilder_ToString(durable_builder);
        if (!cx_arena_owns(arena, durable_result) ||
            !cx_arena_owns(arena, durable_result->_data) ||
            durable_result->_length != 2u ||
            memcmp(durable_result->_data, "22", 2u) != 0) return 84;

        arena_builder = (struct cxcore_System_StringBuilder *)cx_arena_allocate_current(
            sizeof(*arena_builder), cx_arena_maximum_alignment());
        cxcore_System_StringBuilder___constructor__2(arena_builder, 8u);
        cxcore_System_StringBuilder_Append__4(arena_builder, 37);
        cx_arena_enter_durable(&durable_scope);
        arena_result = cxcore_System_StringBuilder_ToString(arena_builder);
        if (cx_arena_owns(arena, arena_result) ||
            cx_arena_owns(arena, arena_result->_data) ||
            !cx_gc_is_durable_reference((cx_ptr)arena_result) ||
            !cx_gc_is_durable_reference(arena_result->_data) ||
            arena_result->_length != 2u ||
            memcmp(arena_result->_data, "37", 2u) != 0) return 85;
        cx_arena_leave_durable(&durable_scope);
        cx_arena_leave(&arena_scope);
        cx_arena_destroy(arena);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-native-library-wrapper-is-durable") == 0) {
        struct cx_arena *arena = cx_arena_create(1024, 0, 0);
        struct cx_arena_scope scope;
        struct cxcore_System_String path = {0};
        struct cxcore_System_Interop_NativeLibrary *library;
#ifdef _WIN32
        static const cx_byte path_bytes[] = "kernel32.dll";
#else
        static const cx_byte path_bytes[] = "libc.so.6";
#endif

        path._length = (cx_uint)(sizeof(path_bytes) - 1u);
        path._data = (cx_ptr)path_bytes;
        cx_arena_enter(arena, &scope);
        library = cxcore_System_Interop_NativeLibrary_Load(&path);
        if (library == NULL || cx_arena_current_owns(library) ||
            !cx_gc_is_durable_reference((cx_ptr)library)) return 83;
        cx_arena_leave(&scope);
        cx_arena_destroy(arena);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-string-buffers-follow-owner") == 0) {
        struct cx_arena *outer = cx_arena_create(512, 0, 0);
        struct cx_arena *inner = cx_arena_create(512, 0, 0);
        struct cx_arena_scope outer_scope;
        struct cx_arena_scope inner_scope;
        struct cxcore_System_String *durable_string =
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*durable_string));
        struct cxcore_System_WideString *outer_string;
        static const cx_wchar wide_input[] = {'A', 0x20AC};

        cx_arena_enter(outer, &outer_scope);
        outer_string = (struct cxcore_System_WideString *)cx_arena_allocate_current(
            sizeof(*outer_string), cx_arena_maximum_alignment());
        cx_arena_enter(inner, &inner_scope);
        cxcore_System_String___constructor__3(durable_string, 3u, 'x');
        cxcore_System_WideString___constructor__2(
            outer_string, wide_input, (cx_uint)(sizeof(wide_input) / sizeof(wide_input[0])));
        if (cx_arena_current_owns(durable_string->_data) ||
            !cx_gc_is_durable_reference(durable_string->_data) ||
            !cx_arena_owns(outer, outer_string->_data) ||
            cx_arena_owns(inner, outer_string->_data) ||
            ((cx_byte *)durable_string->_data)[0] != 'x' ||
            ((cx_wchar *)outer_string->_data)[1] != 0x20AC) return 84;
        cx_arena_leave(&inner_scope);
        cx_arena_leave(&outer_scope);
        cx_arena_destroy(inner);
        cx_arena_destroy(outer);
        return 0;
    }
    size_t heap_before;
    struct cxcore_System_Collections_Array *aggregate_array;
    struct cxcore_System_Nullable *aggregate_nullable;
    struct cx_iface_ref *aggregate_interface;

    if (argc == 2 && strcmp(argv[1], "arena-overflow") == 0) {
        struct cx_exception_frame frame;
        cx_action action = {0};
        cx_ptr exception;
        struct cxcore_System_String *message;
        size_t overflow_collections_before = cx_memory_test_collection_count_get();
        action.__function = overflow_arena_callback;
        cx_exception_push(&frame);
        if (setjmp(frame.environment) == 0) {
            cxcore_System_Memory_WithArena__2(32u, action);
            return 18;
        }
        exception = cx_exception_current();
        message = cxcore_System_Exception_Message___const_get(
            (const struct cxcore_System_Exception *)exception);
        if (message == NULL || message->_length != 26u ||
            memcmp(message->_data, "CX arena capacity exceeded", 26u) != 0 ||
            !cx_arena_async_use_allowed() ||
            cx_memory_test_collection_count_get() != overflow_collections_before) return 19;
        cx_exception_pop(&frame);
        cx_exception_clear();
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-public-callback") == 0) {
        struct arena_public_callback_context callback_context = {0};
        cx_action action = {0};
        action.__function = record_public_arena_callback;
        action.__environment = &callback_context;
        cxcore_System_Memory_WithArena(action);
        if (!callback_context.observed_arena_ownership ||
            !cx_arena_async_use_allowed()) return 20;
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-type-info-array") == 0) {
        struct cx_arena *arena = cx_arena_create(2048u, 0u, 0);
        struct arena_type_info_array_context context = {CX_FALSE};
        if (arena == NULL) return 24;
        cx_arena_run_callback(arena, create_type_info_array_in_arena, &context);
        cx_arena_destroy(arena);
        return context.type_info_survived_collection ? 0 : 25;
    }
    if (argc == 2 && strcmp(argv[1], "arena-type-info-native-store-escape") == 0) {
        struct cx_arena *arena = cx_arena_create(1024u, 0u, 0);
        struct cx_arena_scope scope;
        struct cx_arena_durable_scope durable_scope;
        struct cxcore_System_Reflection_TypeInfo source =
            cxcore_System_Int___typeinfo;
        struct cxcore_System_Reflection_TypeInfo *destination;
        if (arena == NULL) return 30;
        cx_arena_enter(arena, &scope);
        source.RuntimeTypeInfo = cx_arena_allocate_current(
            (cx_uint)sizeof(struct cx_runtime_type_info),
            (cx_uint)sizeof(cx_ptr));
        cx_arena_enter_durable(&durable_scope);
        destination = (struct cxcore_System_Reflection_TypeInfo *)
            cxcore_System_Memory_Alloc((cx_uint)sizeof(*destination));
        cx_reflection_copy_type_info(destination, &source);
        cx_arena_leave_durable(&durable_scope);
        cx_arena_leave(&scope);
        cx_arena_destroy(arena);
        return 31;
    }
    if (argc == 2 && strcmp(argv[1], "arena-module-info-result") == 0) {
        struct cx_arena *arena = cx_arena_create(4096u, 0u, 0);
        struct arena_module_info_context context = {CX_FALSE};
        if (arena == NULL) return 26;
        cx_arena_run_callback(arena, create_module_info_in_arena, &context);
        cx_arena_destroy(arena);
        return context.module_info_survived_collection ? 0 : 27;
    }
    if (argc == 2 && strcmp(argv[1], "arena-function-info-result") == 0) {
        struct cx_arena *arena = cx_arena_create(4096u, 0u, 0);
        struct arena_function_info_context context = {CX_FALSE};
        if (arena == NULL) return 28;
        cx_arena_run_callback(arena, create_function_info_in_arena, &context);
        cx_arena_destroy(arena);
        return context.function_info_survived_collection ? 0 : 29;
    }
    if (argc == 2 && strcmp(argv[1], "arena-api-reflection") == 0) {
        const struct cxcore_System_Reflection_TypeInfo *memory_type = NULL;
        const struct cx_reflection_function *functions;
        cx_uint type_index;
        cx_uint count = 0;
        cx_uint with_arena_one_argument = 0;
        cx_uint with_arena_two_arguments = 0;
        cx_uint with_arena_async_one_argument = 0;
        cx_uint with_arena_async_two_arguments = 0;
        cx_uint with_durable_one_argument = 0;
        cx_uint copy_value_two_arguments = 0;
        cx_uint index;
        for (type_index = 0; type_index < CXCORE___module_info.typeCount;
             ++type_index) {
            const struct cxcore_System_Reflection_TypeInfo *candidate =
                CXCORE___module_info.types[type_index];
            if (candidate != NULL && candidate->Name != NULL &&
                candidate->Namespace != NULL &&
                candidate->Name->_length == 6u &&
                memcmp(candidate->Name->_data, "Memory", 6u) == 0 &&
                candidate->Namespace->_length == 6u &&
                memcmp(candidate->Namespace->_data, "System", 6u) == 0) {
                memory_type = candidate;
                break;
            }
        }
        if (memory_type == NULL) return 22;
        functions = cx_reflection_functions(memory_type, &count);
        for (index = 0; index < count; ++index) {
            if (strcmp(functions[index].name, "WithArena") == 0) {
                if (functions[index].parameterCount == 1u) {
                    ++with_arena_one_argument;
                } else if (functions[index].parameterCount == 2u) {
                    ++with_arena_two_arguments;
                }
            } else if (strcmp(functions[index].name, "WithArenaAsync") == 0) {
                if (functions[index].parameterCount == 1u) {
                    ++with_arena_async_one_argument;
                } else if (functions[index].parameterCount == 2u) {
                    ++with_arena_async_two_arguments;
                }
            } else if (strcmp(functions[index].name, "WithDurable") == 0 &&
                       functions[index].parameterCount == 1u) {
                ++with_durable_one_argument;
            } else if (strcmp(functions[index].name, "CopyValue") == 0 &&
                       functions[index].parameterCount == 2u) {
                ++copy_value_two_arguments;
            }
        }
        if (with_arena_one_argument != 1u ||
            with_arena_two_arguments != 1u ||
            with_arena_async_one_argument != 2u ||
            with_arena_async_two_arguments != 2u ||
            with_durable_one_argument != 1u) {
            return 21;
        }
        return copy_value_two_arguments == 1u ? 0 : 22;
    }
    if (argc == 2 && strcmp(argv[1], "arena-durable-context") == 0) {
        struct cx_arena *arena = cx_arena_create(4096u, 0u, 0);
        struct cx_arena_scope scope;
        struct cx_exception_frame frame;
        struct durable_override_test_context context = { CX_FALSE };
        cx_action action = {0};
        cx_ptr allocation;
        cx_arena_enter(arena, &scope);
        action.__function = record_durable_override_callback;
        action.__environment = &context;
        cxcore_System_Memory_WithDurable(action);
        if (!context.observed_durable_allocation) return 23;
        allocation = cxcore_System_Memory_Alloc(16u);
        if (allocation == NULL || !cx_arena_current_owns(allocation)) return 24;

        context.observed_durable_allocation = CX_FALSE;
        action.__function = throw_from_durable_override_callback;
        cx_exception_push(&frame);
        if (setjmp(frame.environment) == 0) {
            cxcore_System_Memory_WithDurable(action);
            return 25;
        }
        if (!cx_gc_is_durable_reference(cx_exception_current()) ||
            !cx_arena_current_owns(cxcore_System_Memory_Alloc(16u))) return 26;
        cx_exception_pop(&frame);
        cx_exception_clear();
        cx_arena_leave(&scope);
        cx_arena_destroy(arena);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "memory-budget-arena-overflow") == 0) {
        size_t heap = cx_memory_test_heap_bytes_get();
        if (heap > SIZE_MAX - 63u ||
            !cxcore_System_Memory_SetProcessMemoryBudget((cx_ulong)(heap + 63u))) return 94;
        (void)cx_arena_create(64u, 0u, 0);
        return 95;
    }
    if (argc == 2 && strcmp(argv[1], "memory-budget-heap-overflow") == 0) {
        cx_gc_register_static_roots(
            static_roots,
            (cx_uint)(sizeof(static_roots) / sizeof(static_roots[0])));
        cx_ptr rooted = cxcore_System_Memory_Alloc(8u);
        size_t heap;
        static_root = rooted;
        heap = cx_memory_test_heap_bytes_get();
        if (!cxcore_System_Memory_SetProcessMemoryBudget((cx_ulong)heap)) return 96;
        (void)cxcore_System_Memory_Alloc(1u);
        return 97;
    }
    if (argc == 2 && strcmp(argv[1], "memory-budget") == 0) {
        return test_memory_budget();
    }
    if (argc == 2 && strcmp(argv[1], "arena-escape") == 0) {
        struct cx_arena *arena = cx_arena_create(64, 0, 0);
        struct cx_arena_scope scope;
        struct cx_arena_durable_scope durable_scope;
        cx_ptr *durable_slot;
        cx_ptr scoped_value;
        cx_arena_enter(arena, &scope);
        scoped_value = cx_arena_allocate_current(sizeof(cx_uint), sizeof(cx_uint));
        cx_arena_enter_durable(&durable_scope);
        durable_slot = (cx_ptr *)cxcore_System_Memory_Alloc((cx_uint)sizeof(cx_ptr));
        cx_arena_leave_durable(&durable_scope);
        (void)cx_gc_store_reference(durable_slot, scoped_value);
        return 22;
    }
    if (argc == 2 && strcmp(argv[1], "arena-managed-reference-store-escape") == 0) {
        struct cx_arena *arena = cx_arena_create(128u, 0u, 0);
        struct cx_arena_scope scope;
        struct cx_arena_durable_scope durable_scope;
        cx_ptr node;
        cx_ptr scoped_value;
        cx_arena_enter_durable(&durable_scope);
        node = cxcore_System_Memory_Alloc(256u);
        cx_arena_leave_durable(&durable_scope);
        cx_arena_enter(arena, &scope);
        scoped_value = cx_arena_allocate_current(
            (cx_uint)sizeof(cx_uint), (cx_uint)sizeof(cx_uint));
        (void)cx_gc_store_reference((cx_ptr *)node, scoped_value);
        return 141;
    }
    if (argc == 2 && strcmp(argv[1], "arena-managed-reference-store-allows-same-arena") == 0) {
        struct cx_arena *arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena_scope scope;
        cx_ptr node;
        cx_ptr value;
        cx_arena_enter(arena, &scope);
        node = cx_arena_allocate_current(256u, (cx_uint)sizeof(cx_ptr));
        value = cx_arena_allocate_current(
            (cx_uint)sizeof(cx_uint), (cx_uint)sizeof(cx_uint));
        (void)cx_gc_store_reference((cx_ptr *)node, value);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-boxed-type-info-store-escape") == 0) {
        struct cx_arena *arena = cx_arena_create(1024u, 0u, 0);
        struct cx_arena_scope scope;
        struct cx_arena_durable_scope durable_scope;
        struct cxcore_System_Reflection_TypeInfo *scoped_type;
        const cx_int value = 42;
        cx_arena_enter(arena, &scope);
        scoped_type = (struct cxcore_System_Reflection_TypeInfo *)
            cx_arena_allocate_current(
                (cx_uint)sizeof(*scoped_type), (cx_uint)sizeof(cx_ptr));
        cx_reflection_copy_type_info(
            scoped_type, &cxcore_System_Int___typeinfo);
        cx_arena_enter_durable(&durable_scope);
        (void)cx_box_value(&value, (cx_uint)sizeof(value), scoped_type);
        cx_arena_leave_durable(&durable_scope);
        return 142;
    }
    if (argc == 2 && strcmp(argv[1], "arena-boxed-type-info-store-allows-same-arena") == 0) {
        struct cx_arena *arena = cx_arena_create(1024u, 0u, 0);
        struct cx_arena_scope scope;
        struct cxcore_System_Reflection_TypeInfo *scoped_type;
        const cx_int value = 42;
        cx_ptr boxed;
        cx_arena_enter(arena, &scope);
        scoped_type = (struct cxcore_System_Reflection_TypeInfo *)
            cx_arena_allocate_current(
                (cx_uint)sizeof(*scoped_type), (cx_uint)sizeof(cx_ptr));
        cx_reflection_copy_type_info(
            scoped_type, &cxcore_System_Int___typeinfo);
        boxed = cx_box_value(&value, (cx_uint)sizeof(value), scoped_type);
        if (boxed == CX_NULL || cx_boxed_value_type(boxed)->Hash !=
            cxcore_System_Int___typeinfo.Hash) return 143;
        cx_arena_leave(&scope);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-locked-store-allows-same-arena") == 0) {
        struct cx_arena *arena = cx_arena_create(128u, 0u, 0);
        struct cx_arena_scope scope;
        cx_ptr *slot;
        cx_uint *value;
        cx_arena_enter(arena, &scope);
        slot = (cx_ptr *)cx_arena_allocate_current(
            (cx_uint)sizeof(*slot), (cx_uint)sizeof(*slot));
        value = (cx_uint *)cx_arena_allocate_current(
            (cx_uint)sizeof(*value), (cx_uint)sizeof(*value));
        (void)cx_gc_test_store_reference_locked(
            (cx_ptr *)slot, (cx_ptr)value);
        if (*slot != (cx_ptr)value) return 139;
        cx_arena_leave(&scope);
        cx_arena_reset(arena);
        cx_arena_destroy(arena);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-locked-store-rejects-durable-escape") == 0) {
        struct cx_arena *arena = cx_arena_create(128u, 0u, 0);
        struct cx_arena_scope scope;
        struct cx_arena_durable_scope durable_scope;
        cx_ptr *durable_slot;
        cx_ptr scoped_value;
        cx_arena_enter_durable(&durable_scope);
        durable_slot = (cx_ptr *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(*durable_slot));
        cx_arena_leave_durable(&durable_scope);
        cx_arena_enter(arena, &scope);
        scoped_value = cx_arena_allocate_current(
            (cx_uint)sizeof(cx_uint), (cx_uint)sizeof(cx_uint));
        (void)cx_gc_test_store_reference_locked(durable_slot, scoped_value);
        return 140;
    }
    if (argc == 2 && strcmp(argv[1], "arena-stack-escape") == 0) {
        struct cx_arena *arena = cx_arena_create(64, 0, 0);
        struct cx_arena_scope scope;
        cx_ptr stack_slot = NULL;
        cx_ptr scoped_value;
        cx_arena_enter(arena, &scope);
        scoped_value = cx_arena_allocate_current(sizeof(cx_uint), sizeof(cx_uint));
        (void)cx_gc_store_reference(&stack_slot, scoped_value);
        return 28;
    }
    if (argc == 2 && strcmp(argv[1], "durable-object-root") == 0) {
        cx_ptr handle = create_durable_test_object_root();
        struct cx_test_object_root *root = (struct cx_test_object_root *)handle;
        struct cxcore_System_Object *value;
        if (root == NULL) return 142;
        cx_gc_collect();
        value = (struct cxcore_System_Object *)root->value;
        if (value == NULL || value->__vtable == NULL) return 143;
        cxcore_test_System_Memory_RemoveObjectRoot(handle);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-object-root-escape") == 0) {
        struct cx_arena *arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena_scope scope;
        struct cxcore_System_Object *value;
        cx_arena_enter(arena, &scope);
        value = (struct cxcore_System_Object *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(*value));
        cxcore_System_Object___constructor(value);
        (void)cxcore_test_System_Memory_AddObjectRoot((cx_ptr)value);
        return 141;
    }
    if (argc == 2 && strcmp(argv[1], "arena-stack-nested-local") == 0) {
        struct cx_arena *arena = cx_arena_create(64, 0, 0);
        int stored;
        stored = 0;
        cx_arena_run_callback(arena, store_scoped_reference_in_arena_callback,
                              &stored);
        cx_arena_reset(arena);
        cx_arena_destroy(arena);
        return stored ? 0 : 30;
    }
    if (argc == 2 && strcmp(argv[1], "arena-callback-outer-stack-escape") == 0) {
        struct cx_arena *arena = cx_arena_create(64, 0, 0);
        cx_ptr outer_slot = NULL;
        cx_arena_run_callback(arena, store_scoped_reference_into_outer_slot,
                              &outer_slot);
        return 31;
    }
    if (argc == 2 && strcmp(argv[1], "arena-nested-reference-store-ordering") == 0) {
        struct cx_arena *outer = cx_arena_create(256u, 0u, 0);
        struct cx_arena *firstSibling = cx_arena_create(128u, 0u, 0);
        struct cx_arena *secondSibling = cx_arena_create(128u, 0u, 0);
        struct cx_arena_scope outerScope;
        struct cx_arena_scope firstSiblingScope;
        struct cx_arena_scope secondSiblingScope;
        cx_ptr outerValue;
        cx_ptr *outerSlot;
        cx_ptr firstSiblingValue;
        cx_ptr *firstSiblingSlot;
        cx_ptr secondSiblingValue;
        cx_ptr *secondSiblingSlot;

        if (outer == NULL || firstSibling == NULL || secondSibling == NULL) return 32;
        cx_arena_enter(outer, &outerScope);
        outerValue = cx_arena_allocate_current(sizeof(cx_uint), _Alignof(cx_uint));
        outerSlot = cx_arena_allocate_current(sizeof(cx_ptr), _Alignof(cx_ptr));
        if (outerValue == NULL || outerSlot == NULL) return 33;
        *outerSlot = NULL;

        cx_arena_enter(firstSibling, &firstSiblingScope);
        firstSiblingValue = cx_arena_allocate_current(
            sizeof(cx_uint), _Alignof(cx_uint));
        firstSiblingSlot = cx_arena_allocate_current(
            sizeof(cx_ptr), _Alignof(cx_ptr));
        if (firstSiblingValue == NULL || firstSiblingSlot == NULL) return 34;
        *firstSiblingSlot = NULL;
        if (!cx_arena_test_reference_store_allowed(
                firstSiblingSlot, outerValue, 0) ||
            !cx_arena_test_reference_store_allowed(
                firstSiblingSlot, firstSiblingValue, 0) ||
            cx_arena_test_reference_store_allowed(
                outerSlot, firstSiblingValue, 0)) return 35;
        (void)cx_gc_store_reference(firstSiblingSlot, outerValue);
        cx_arena_leave(&firstSiblingScope);

        cx_arena_enter(secondSibling, &secondSiblingScope);
        secondSiblingValue = cx_arena_allocate_current(
            sizeof(cx_uint), _Alignof(cx_uint));
        secondSiblingSlot = cx_arena_allocate_current(
            sizeof(cx_ptr), _Alignof(cx_ptr));
        if (secondSiblingValue == NULL || secondSiblingSlot == NULL) return 36;
        *secondSiblingSlot = NULL;
        if (cx_arena_test_reference_store_allowed(
                outerSlot, secondSiblingValue, 0) ||
            cx_arena_test_reference_store_allowed(
                secondSiblingSlot, firstSiblingValue, 0)) return 37;
        (void)cx_gc_store_reference(secondSiblingSlot, outerValue);
        cx_arena_leave(&secondSiblingScope);
        cx_arena_leave(&outerScope);
        cx_arena_destroy(secondSibling);
        cx_arena_destroy(firstSibling);
        cx_arena_destroy(outer);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-string-store") == 0) {
        struct cx_arena *arena = cx_arena_create(64, 0, 0);
        struct cx_arena_scope scope;
        struct cxcore_System_String *string =
            (struct cxcore_System_String *)cxcore_System_Memory_Alloc(
                (cx_uint)sizeof(*string));
        cx_ptr scoped_data;
        cx_arena_enter(arena, &scope);
        scoped_data = cx_arena_allocate_current(4, 1);
        cxcore_System_String___constructor__2(string, 4, scoped_data);
        return 29;
    }

    {
        struct cx_arena *arena = cx_arena_create(64, 256, 0);
        struct cx_arena_statistics statistics;
        struct cx_arena_scope scope;
        unsigned char *first;
        void *aligned;
        cx_ptr *slot;
        size_t index;
        cx_ptr stack_slot = NULL;
        cx_arena_enter(arena, &scope);
        first = (unsigned char *)cx_arena_allocate(arena, 9, 1);
        aligned = cx_arena_allocate(arena, 17, 16);
        slot = (cx_ptr *)cx_arena_allocate(arena, sizeof(cx_ptr), sizeof(cx_ptr));
        *slot = NULL;
        for (index = 0; index < 9; ++index) {
            if (first[index] != 0) return 10;
        }
        if (((uintptr_t)aligned & 15u) != 0 || !cx_arena_owns(arena, aligned) ||
            !cx_arena_current_owns(first) || cx_arena_allocate_current(0, 8) == NULL ||
            cx_arena_owns(arena, first + 9) || cx_arena_owns(arena, (unsigned char *)aligned - 1)) {
            return 11;
        }
        {
            int first_allowed = cx_arena_test_reference_store_allowed(
                first + 1, first, 0);
            int crossing_allowed = cx_arena_test_reference_store_allowed(
                first + sizeof(cx_ptr) + 2, first, 0);
            if (!first_allowed || crossing_allowed) return 25;
        }
        if (cx_arena_test_reference_store_allowed(&stack_slot, first + 9, 1)) {
            return 27;
        }
        if (cx_arena_test_reference_store_allowed(&stack_slot, first, 1)) {
            return 23;
        }
        retain_only_in_arena(arena, slot);
        cx_gc_collect();
        if (*slot == NULL || *(cx_uint *)*slot != 0x22a4u) {
            fputs("active arena did not retain a normal-heap object\n", stderr);
            return 17;
        }
        cx_arena_leave(&scope);
        cx_arena_get_statistics(arena, &statistics);
        if (statistics.allocation_count != 4 ||
            statistics.requested_bytes != 9 + 17 + sizeof(cx_ptr) ||
            statistics.chunk_count == 0 || statistics.capacity_bytes < 64 ||
            statistics.used_bytes == 0 || statistics.peak_used_bytes < statistics.used_bytes ||
            statistics.active_depth != 0) {
            return 12;
        }
        cx_arena_reset(arena);
        cx_arena_get_statistics(arena, &statistics);
        if (statistics.used_bytes != 0 || statistics.allocation_count != 0 ||
            statistics.generation != 2 || statistics.reset_count != 1 ||
            cx_arena_owns(arena, aligned)) {
            return 13;
        }
        if (cx_arena_test_reference_store_allowed(&stack_slot, aligned, 1)) {
            return 26;
        }
        cx_arena_enter(arena, &scope);
        (void)cx_arena_allocate(arena, 80, 8);
        cx_arena_leave(&scope);
        cx_arena_get_statistics(arena, &statistics);
        if (statistics.chunk_count != 2 || statistics.growth_count != 1 ||
            statistics.capacity_bytes < 144) {
            return 19;
        }
        cx_arena_reset(arena);
        cx_arena_enter(arena, &scope);
        first = (unsigned char *)cx_arena_allocate(arena, 9, 1);
        for (index = 0; index < 9; ++index) {
            if (first[index] != 0) return 14;
        }
        cx_arena_leave(&scope);
        cx_arena_destroy(arena);
    }

    {
        struct cx_arena *outer = cx_arena_create(64, 0, 0);
        struct cx_arena *inner = cx_arena_create(64, 0, 0);
        struct cx_arena_scope outer_scope;
        struct cx_arena_scope inner_scope;
        cx_ptr outer_value;
        cx_ptr inner_value;
        cx_ptr *inner_slot;
        cx_arena_enter(outer, &outer_scope);
        outer_value = cx_arena_allocate(outer, 8, 8);
        cx_arena_enter(inner, &inner_scope);
        inner_value = cx_arena_allocate_current(8, 8);
        inner_slot = (cx_ptr *)cx_arena_allocate_current(sizeof(cx_ptr), sizeof(cx_ptr));
        if (cx_gc_store_reference(inner_slot, outer_value) != outer_value ||
            *inner_slot != outer_value) {
            return 24;
        }
        if (!cx_arena_current_owns(inner_value)) return 20;
        cx_arena_leave(&inner_scope);
        if (!cx_arena_current_owns(outer_value) || cx_arena_current_owns(inner_value)) {
            return 21;
        }
        cx_arena_leave(&outer_scope);
        cx_arena_destroy(inner);
        cx_arena_destroy(outer);
    }

    {
        struct cx_arena *arena = cx_arena_create(0, 256, 1);
        struct cx_arena_statistics statistics;
        struct cx_arena_scope scope;
        void *block;
        cx_arena_enter(arena, &scope);
        block = cx_arena_allocate(arena, 128, 1);
        if (!cx_arena_owns(arena, (unsigned char *)block + 127)) return 15;
        cx_arena_leave(&scope);
        cx_arena_get_statistics(arena, &statistics);
        if (!statistics.fixed_capacity || statistics.capacity_bytes != 256 ||
            statistics.growth_count != 0 || statistics.chunk_count != 1) {
            return 16;
        }
        cx_arena_destroy(arena);
    }

    if (argc == 2 && strcmp(argv[1], "second-thread") == 0) {
#ifdef _WIN32
        HANDLE thread;
        (void)cxcore_System_Memory_Alloc(1);
        thread = CreateThread(NULL, 0, allocate_from_second_thread, NULL, 0, NULL);
        if (thread == NULL) {
            return 5;
        }
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
#else
        pthread_t thread;
        (void)cxcore_System_Memory_Alloc(1);
        if (pthread_create(&thread, NULL, allocate_from_second_thread, NULL) != 0) {
            return 5;
        }
        pthread_join(thread, NULL);
#endif
        return 0;
    }

    if (argc == 2 && (strcmp(argv[1], "arena-copy") == 0 ||
                      strcmp(argv[1], "arena-copy-source") == 0)) {
        struct cx_arena *arena = cx_arena_create(64, 0, 0);
        struct cx_arena_scope scope;
        unsigned char source[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
        unsigned char destination[8] = { 0 };
        void *scoped;
        cx_arena_enter(arena, &scope);
        scoped = cx_arena_allocate_current(sizeof(source), 8);
        if (strcmp(argv[1], "arena-copy-source") == 0) {
            cxcore_System_Memory_Copy(destination, scoped, (cx_uint)sizeof(source));
        } else {
            cxcore_System_Memory_Copy(scoped, source, (cx_uint)sizeof(source));
        }
        cx_arena_leave(&scope);
        cx_arena_destroy(arena);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "arena-copy-reference-escape") == 0) {
        struct cx_arena *arena = cx_arena_create(128, 0, 0);
        struct cx_arena_scope scope;
        struct typed_copy_test_value source = {0};
        struct typed_copy_test_value *destination =
            (struct typed_copy_test_value *)cxcore_System_Memory_Alloc(
                (cx_uint)sizeof(*destination));
        cx_arena_enter(arena, &scope);
        source.reference = cx_arena_allocate_current(sizeof(cx_uint),
                                                     sizeof(cx_uint));
        source.scalar = 23u;
        cxcore_System_Memory_Copy(destination, &source,
                                  (cx_uint)sizeof(source));
        cx_arena_leave(&scope);
        cx_arena_destroy(arena);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "arena-copy-padding-reference-escape") == 0) {
        struct cx_arena *arena = cx_arena_create(128, 0, 0);
        struct cx_arena_scope scope;
        cx_ptr source;
        cx_ptr destination = cxcore_System_Memory_Alloc((cx_uint)sizeof(source));
        void *first;
        cx_arena_enter(arena, &scope);
        first = cx_arena_allocate_current(1u, 1u);
        (void)cx_arena_allocate_current(sizeof(cx_ptr), 16u);
        source = (cx_ptr)((unsigned char *)first + 1u);
        // The candidate points into alignment padding, not a live allocation.
        cxcore_System_Memory_Copy(destination, &source, (cx_uint)sizeof(source));
        cx_arena_leave(&scope);
        cx_arena_destroy(arena);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "arena-copy-superseded-reference-escape") == 0) {
        struct cx_arena *arena = cx_arena_create(128, 0, 0);
        struct cx_arena_scope scope;
        cx_ptr source;
        cx_ptr destination = cxcore_System_Memory_Alloc((cx_uint)sizeof(source));
        void *superseded;
        cx_arena_enter(arena, &scope);
        superseded = cx_arena_allocate_current(sizeof(cx_ptr), sizeof(cx_ptr));
        (void)cx_arena_reallocate_current(superseded, sizeof(cx_ptr) * 2u);
        source = (cx_ptr)superseded;
        // Reallocation leaves the old bytes reserved until arena reset.
        cxcore_System_Memory_Copy(destination, &source, (cx_uint)sizeof(source));
        cx_arena_leave(&scope);
        cx_arena_destroy(arena);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "arena-copy-unaligned-reference-escape") == 0) {
        struct cx_arena *arena = cx_arena_create(128, 0, 0);
        struct cx_arena_scope scope;
        unsigned char source[sizeof(cx_ptr) + 2u] = { 0 };
        size_t offset = (uintptr_t)source % sizeof(cx_ptr) == sizeof(cx_ptr) - 1u
            ? 2u : 1u;
        cx_ptr scoped;
        cx_ptr destination = cxcore_System_Memory_Alloc((cx_uint)sizeof(source));
        cx_arena_enter(arena, &scope);
        scoped = cx_arena_allocate_current(sizeof(cx_uint), sizeof(cx_uint));
        memcpy(source + offset, &scoped, sizeof(scoped));
        cxcore_System_Memory_Copy(destination, source, (cx_uint)sizeof(source));
        cx_arena_leave(&scope);
        cx_arena_destroy(arena);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "arena-copy-durable-scalars") == 0) {
        struct cx_arena *arena = cx_arena_create(128, 0, 0);
        struct cx_arena_scope scope;
        cx_uint source[2] = { 23u, 47u };
        cx_uint *destination =
            (cx_uint *)cxcore_System_Memory_Alloc((cx_uint)sizeof(source));
        cx_arena_enter(arena, &scope);
        cxcore_System_Memory_Copy(destination, source, (cx_uint)sizeof(source));
        if (destination[0] != 23u || destination[1] != 47u) return 52;
        cx_arena_leave(&scope);
        cx_arena_destroy(arena);
        cxcore_System_Memory_Free(destination);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "arena-interface-escape") == 0) {
        struct cx_arena *arena = cx_arena_create(64, 0, 0);
        struct cx_arena_scope scope;
        struct cx_iface_ref *destination =
            (struct cx_iface_ref *)cxcore_System_Memory_Alloc(
                (cx_uint)sizeof(struct cx_iface_ref));
        struct cx_iface_ref value;
        void *scoped_value;
        cx_arena_enter(arena, &scope);
        scoped_value = cx_arena_allocate_current(8, 8);
        value.vtable = NULL;
        value.instance = scoped_value;
        value.owner = scoped_value;
        (void)cx_gc_store_interface_reference(destination, value);
        cx_arena_leave(&scope);
        cx_arena_destroy(arena);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-typed-copy-escape") == 0) {
        struct cx_arena *arena = cx_arena_create(64, 0, 0);
        struct cx_arena_scope scope;
        struct typed_copy_test_value source = {0};
        struct typed_copy_test_value *destination =
            (struct typed_copy_test_value *)cxcore_System_Memory_Alloc(
                (cx_uint)sizeof(*destination));
        cx_arena_enter(arena, &scope);
        source.reference = cx_arena_allocate_current(sizeof(cx_uint), sizeof(cx_uint));
        source.scalar = 0x1234u;
        cx_gc_copy_value(destination, &source, &typed_copy_value_type);
        return 30;
    }
    if (argc == 2 && strcmp(argv[1], "arena-nullable-return-escape") == 0) {
        struct cx_arena *arena = cx_arena_create(64, 0, 0);
        struct cx_arena_scope scope;
        struct cxcore_System_Nullable source = {0};
        struct cxcore_System_Nullable result = {0};
        cx_arena_enter(arena, &scope);
        source._obj = cx_arena_allocate_current(sizeof(cx_uint), sizeof(cx_ptr));
        cxcore_System_Nullable_Value___const_get(&source, &result);
        return 41;
    }
    if (argc == 2 && strcmp(argv[1], "arena-thread-start") == 0) {
        struct cx_arena *arena = cx_arena_create(64, 0, 0);
        struct cx_arena_scope scope;
        cx_action action = {0};
        action.__function = test_noop_action;
        cx_arena_enter(arena, &scope);
        (void)cxcore_System_Threading_Thread_Start(action);
        return 43;
    }
    if (argc == 2 && strcmp(argv[1], "arena-durable-thread-start") == 0) {
        struct cx_arena *arena = cx_arena_create(64, 0, 0);
        struct cx_arena_scope scope;
        struct cx_arena_durable_scope durable_scope;
        struct cxcore_System_Threading_Thread thread;
        cx_action action = {0};
        action.__function = test_noop_action;
        cx_arena_enter(arena, &scope);
        cx_arena_enter_durable(&durable_scope);
        thread = cxcore_System_Threading_Thread_Start(action);
        cx_arena_leave_durable(&durable_scope);
        cxcore_System_Threading_Thread_Join(&thread);
        cx_arena_leave(&scope);
        cx_arena_destroy(arena);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-memory-free") == 0) {
        struct cx_arena *arena = cx_arena_create(64, 0, 0);
        struct cx_arena_scope scope;
        void *block;
        cx_arena_enter(arena, &scope);
        block = cx_arena_allocate_current(16, sizeof(cx_ptr));
        cxcore_System_Memory_Free(block);
        return 32;
    }
    if (argc == 2 && strcmp(argv[1], "arena-memory-alloc-context") == 0) {
        struct cx_arena *arena = cx_arena_create(128, 0, 0);
        struct cx_arena_scope scope;
        struct cx_arena_durable_scope durable_scope;
        cx_ptr arena_block;
        cx_ptr durable_block;
        size_t heap_bytes_before = cx_memory_test_heap_bytes_get();
        cx_uint index;
        cx_arena_enter(arena, &scope);
        arena_block = cxcore_System_Memory_Alloc(16);
        for (index = 0; index < 16; ++index) {
            if (((cx_byte *)arena_block)[index] != 0) return 39;
        }
        if (!cx_arena_current_owns(arena_block) ||
            ((uintptr_t)arena_block % cx_arena_maximum_alignment()) != 0 ||
            cx_memory_test_heap_bytes_get() != heap_bytes_before) {
            return 40;
        }
        cx_arena_enter_durable(&durable_scope);
        durable_block = cxcore_System_Memory_Alloc(8);
        cx_arena_leave_durable(&durable_scope);
        if (cx_arena_current_owns(durable_block) ||
            cx_memory_test_heap_bytes_get() != heap_bytes_before + 8) {
            return 41;
        }
        cxcore_System_Memory_Free(durable_block);
        cx_arena_leave(&scope);
        cx_arena_reset(arena);
        cx_arena_destroy(arena);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-retains-interior-pointer") == 0) {
        struct cx_arena *arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena_scope scope;
        cx_ptr *slot;
        volatile cx_byte *interior;
        cx_arena_enter(arena, &scope);
        slot = (cx_ptr *)cx_arena_allocate_current(sizeof(cx_ptr), sizeof(cx_ptr));
        if (slot == NULL) return 60;
        retain_only_in_arena_interior_reference(slot, 24u);
        cx_gc_collect();
        interior = (volatile cx_byte *)*slot;
        if (interior == NULL || *interior != 0x6du) return 61;
        (void)cx_gc_store_reference(slot, NULL);
        cx_arena_leave(&scope);
        cx_arena_destroy(arena);
        cx_gc_collect();
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-runtime-once-durable") == 0) {
        struct cx_arena *arena = cx_arena_create(128, 0, 0);
        struct cx_arena_scope scope;
        cx_ptr outer_value;
        cx_ptr inner_value;
        cx_ptr resumed_value;
        cx_ptr failure_value;
        cx_ptr arena_value;
        cx_arena_enter(arena, &scope);
        if (!cx_runtime_once_begin(&arena_outer_initialization)) return 42;
        outer_value = cxcore_System_Memory_Alloc(8);
        if (cx_arena_current_owns(outer_value) ||
            !cx_runtime_once_begin(&arena_inner_initialization)) {
            return 43;
        }
        inner_value = cxcore_System_Memory_Alloc(8);
        cx_runtime_once_complete(&arena_inner_initialization);
        resumed_value = cxcore_System_Memory_Alloc(8);
        if (cx_arena_current_owns(inner_value) ||
            cx_arena_current_owns(resumed_value)) {
            return 44;
        }
        if (!cx_runtime_once_begin(&arena_failed_initialization)) return 46;
        failure_value = cxcore_System_Memory_Alloc(8);
        cx_runtime_once_fail(&arena_failed_initialization, failure_value);
        resumed_value = cxcore_System_Memory_Alloc(8);
        if (cx_arena_current_owns(resumed_value)) return 47;
        cx_runtime_once_complete(&arena_outer_initialization);
        arena_value = cxcore_System_Memory_Alloc(8);
        if (cx_arena_current_owns(outer_value) ||
            cx_arena_current_owns(resumed_value) ||
            !cx_arena_current_owns(arena_value)) {
            return 45;
        }
        cxcore_System_Memory_Free(outer_value);
        cxcore_System_Memory_Free(inner_value);
        cxcore_System_Memory_Free(resumed_value);
        cx_arena_leave(&scope);
        cx_arena_reset(arena);
        cx_arena_destroy(arena);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-runtime-once-publishes-durable-static") == 0) {
        struct cx_arena *arena = cx_arena_create(128u, 0u, 0);
        struct cx_arena_scope scope;
        cx_ptr initialized_value;
        cx_ptr resumed_value;
        if (arena == NULL) return 50;
        cx_gc_register_static_roots(
            static_roots,
            (cx_uint)(sizeof(static_roots) / sizeof(static_roots[0])));
        cx_arena_enter(arena, &scope);
        if (!cx_runtime_once_begin(&arena_published_initialization)) return 51;
        initialized_value = cxcore_System_Memory_Alloc((cx_uint)sizeof(cx_uint));
        if (cx_arena_current_owns(initialized_value)) return 52;
        *(cx_uint *)initialized_value = 0x22a5u;
        (void)cx_gc_store_reference(&arena_published_root, initialized_value);
        arena_published_expected_address = (uintptr_t)initialized_value;
        cx_runtime_once_complete(&arena_published_initialization);
        resumed_value = cxcore_System_Memory_Alloc((cx_uint)sizeof(cx_uint));
        if (!cx_arena_current_owns(resumed_value)) return 53;
        cx_arena_leave(&scope);
        cx_arena_destroy(arena);
        initialized_value = NULL;
        resumed_value = NULL;
        cx_gc_collect();
        if ((uintptr_t)arena_published_root != arena_published_expected_address ||
            *(cx_uint *)arena_published_root != 0x22a5u) {
            return 54;
        }
        arena_published_root = NULL;
        arena_published_expected_address = 0u;
        cx_gc_collect();
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-runtime-once-scoped-failure") == 0) {
        struct cx_arena *arena = cx_arena_create(128u, 0u, 0);
        struct cx_arena_scope scope;
        cx_ptr failure_value;
        cx_arena_enter(arena, &scope);
        failure_value = cx_arena_allocate_current(sizeof(cx_uint), sizeof(cx_uint));
        if (!cx_runtime_once_begin(&arena_failed_initialization)) return 48;
        cx_runtime_once_fail(&arena_failed_initialization, failure_value);
        return 49;
    }
    if (argc == 2 && strcmp(argv[1], "arena-memory-realloc") == 0) {
        struct cx_arena *arena = cx_arena_create(64, 0, 0);
        struct cx_arena_scope scope;
        cx_uint *block;
        cx_uint *grown;
        cx_uint *shrunk;
        cx_arena_enter(arena, &scope);
        block = (cx_uint *)cx_arena_allocate_current(2 * sizeof(cx_uint),
                                                      sizeof(cx_uint));
        block[0] = 0x12345678u;
        block[1] = 0x87654321u;
        grown = (cx_uint *)cxcore_System_Memory_Realloc(block, 4 * sizeof(cx_uint));
        if (grown == NULL || grown == block ||
            cx_arena_current_owns(block) || !cx_arena_current_owns(grown) ||
            grown[0] != 0x12345678u || grown[1] != 0x87654321u || grown[2] != 0) {
            return 33;
        }
        shrunk = (cx_uint *)cxcore_System_Memory_Realloc(grown, sizeof(cx_uint));
        if (shrunk == NULL || cx_arena_current_owns(grown) ||
            !cx_arena_current_owns(shrunk) || *shrunk != 0x12345678u) {
            return 37;
        }
        if (cxcore_System_Memory_Realloc(shrunk, 0) != NULL ||
            cx_arena_current_owns(shrunk)) {
            return 38;
        }
        cx_arena_leave(&scope);
        cx_arena_reset(arena);
        cx_arena_destroy(arena);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-store-overlaps-extents") == 0) {
        struct cx_arena *arena = cx_arena_create(64, 0, 0);
        struct cx_arena_scope scope;
        cx_ptr *second;
        cx_ptr value;
        cx_arena_enter(arena, &scope);
        (void)cx_arena_allocate_current(sizeof(cx_ptr), sizeof(cx_ptr));
        second = (cx_ptr *)cx_arena_allocate_current(sizeof(cx_ptr), sizeof(cx_ptr));
        value = cx_arena_allocate_current(sizeof(cx_uint), sizeof(cx_uint));
        (void)cx_gc_store_reference(
            (cx_ptr *)((unsigned char *)second - 1), value);
        return 35;
    }
    if (argc == 2 && strcmp(argv[1], "arena-interior-reference-escape") == 0) {
        struct cx_arena *arena = cx_arena_create(128, 0, 0);
        struct cx_arena_scope scope;
        cx_ptr *destination = (cx_ptr *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(cx_ptr));
        cx_ptr value;
        cx_arena_enter(arena, &scope);
        value = cx_arena_allocate_current(sizeof(cx_uint) * 2, sizeof(cx_uint));
        (void)cx_gc_store_reference(destination,
            (cx_ptr)((unsigned char *)value + sizeof(cx_uint)));
        return 105;
    }
    if (argc == 2 && strcmp(argv[1], "arena-interior-reference-local") == 0) {
        struct cx_arena *arena = cx_arena_create(128, 0, 0);
        struct cx_arena_scope scope;
        cx_ptr *destination;
        cx_ptr value;
        cx_arena_enter(arena, &scope);
        destination = (cx_ptr *)cx_arena_allocate_current(sizeof(cx_ptr), sizeof(cx_ptr));
        value = cx_arena_allocate_current(sizeof(cx_uint) * 2, sizeof(cx_uint));
        value = (cx_ptr)((unsigned char *)value + sizeof(cx_uint));
        if (cx_gc_store_reference(destination, value) != value ||
            *destination != value) {
            return 106;
        }
        cx_arena_leave(&scope);
        cx_arena_reset(arena);
        cx_arena_destroy(arena);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-reflection-array-follows-owner") == 0) {
        struct cx_arena *arena = cx_arena_create(128, 0, 0);
        struct cx_arena_scope scope;
        struct cx_arena_durable_scope durable_scope;
        struct cxcore_System_Collections_Array *durable_source;
        struct cxcore_System_Collections_Array *source;
        struct cxcore_System_Collections_Array *result;
        cx_ptr durable_value;
        cx_ptr scoped_value;
        durable_source = cx_array_with_type(cx_array_new(1u, (cx_uint)sizeof(cx_ptr)),
                                            "m22-reflection-array");
        durable_value = cxcore_System_Memory_Alloc((cx_uint)sizeof(cx_uint));
        cx_gc_store_reference((cx_ptr *)durable_source->_data, durable_value);
        cx_arena_enter(arena, &scope);
        result = cx_reflection_array_values(durable_source, "m22-reflection-array",
            (cx_uint)sizeof(cx_ptr), &typed_copy_reference_type, 0u, 1u);
        if (cx_arena_owns(arena, result) || cx_arena_owns(arena, result->_data) ||
            *(cx_ptr *)result->_data != durable_value) return 106;
        cx_gc_collect();
        if (*(cx_ptr *)result->_data != durable_value) return 108;
        source = cx_array_with_type(cx_array_new(1u, (cx_uint)sizeof(cx_ptr)),
                                    "m22-reflection-array");
        scoped_value = cxcore_System_Memory_Alloc((cx_uint)sizeof(cx_uint));
        cx_gc_store_reference((cx_ptr *)source->_data, scoped_value);
        cx_arena_enter_durable(&durable_scope);
        result = cx_reflection_array_values(source, "m22-reflection-array",
            (cx_uint)sizeof(cx_ptr), &typed_copy_reference_type, 0u, 1u);
        cx_arena_leave_durable(&durable_scope);
        if (!cx_arena_owns(arena, result) ||
            !cx_arena_owns(arena, result->_data) ||
            *(cx_ptr *)result->_data != scoped_value) return 107;
        cx_gc_collect();
        if (*(cx_ptr *)result->_data != scoped_value) return 109;
        cx_arena_leave(&scope);
        cx_arena_destroy(arena);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "array-type-key-stack-escape") == 0) {
        char type_key[] = "m22-array-type-key";
        struct cxcore_System_Collections_Array *array =
            cx_array_new(0u, (cx_uint)sizeof(cx_ptr));
        (void)cx_array_with_type(array, type_key);
        return 108;
    }
    if (argc == 2 && strcmp(argv[1], "arena-runtime-state-follows-owner") == 0) {
        struct cx_arena *outer = cx_arena_create(256, 0, 0);
        struct cx_arena *inner = cx_arena_create(128, 0, 0);
        struct cx_arena_scope outer_scope;
        struct cx_arena_scope inner_scope;
        struct cxcore_System_Threading_AtomicInt *atomic_value;
        struct cxcore_System_Threading_AtomicInt *durable_atomic_value =
            (struct cxcore_System_Threading_AtomicInt *)cxcore_System_Memory_Alloc(
                (cx_uint)sizeof(*durable_atomic_value));
        struct cxcore_System_Threading_Mutex *mutex_value;
        cx_arena_enter(outer, &outer_scope);
        atomic_value = (struct cxcore_System_Threading_AtomicInt *)
            cx_arena_allocate_current(sizeof(*atomic_value),
                                     cx_arena_maximum_alignment());
        mutex_value = (struct cxcore_System_Threading_Mutex *)
            cx_arena_allocate_current(sizeof(*mutex_value),
                                     cx_arena_maximum_alignment());
        cx_arena_enter(inner, &inner_scope);
        cxcore_System_Threading_AtomicInt___constructor(atomic_value, 41);
        cxcore_System_Threading_Mutex___constructor(mutex_value);
        cxcore_System_Threading_AtomicInt___constructor(durable_atomic_value, 6);
        if (!cx_arena_owns(outer, atomic_value->_state) ||
            !cx_arena_owns(outer, mutex_value->_state) ||
            !cx_gc_is_durable_reference(durable_atomic_value->_state) ||
            cxcore_System_Threading_AtomicInt_Increment(durable_atomic_value) != 7 ||
            cxcore_System_Threading_AtomicInt_Increment(atomic_value) != 42) {
            return 108;
        }
        cxcore_System_Threading_Mutex_Lock(mutex_value);
        cxcore_System_Threading_Mutex_Unlock(mutex_value);
        cx_arena_leave(&inner_scope);
        cx_arena_leave(&outer_scope);
        cx_arena_reset(inner);
        cx_arena_reset(outer);
        cx_arena_destroy(inner);
        cx_arena_destroy(outer);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "managed-store-crosses-boundary") == 0) {
        size_t size = sizeof(cx_ptr) + sizeof(cx_ptr) / 2u;
        cx_ptr block = cxcore_System_Memory_Alloc((cx_uint)size);
        cx_ptr value = cxcore_System_Memory_Alloc((cx_uint)sizeof(cx_uint));
        cx_gc_store_reference(
            (cx_ptr *)((unsigned char *)block + sizeof(cx_ptr)), value);
        return 34;
    }
    if (argc == 2 && strcmp(argv[1], "arena-native-unknown-copy-escape") == 0) {
        struct cx_arena *arena = cx_arena_create(128u, 0u, 0);
        struct cx_arena_scope scope;
        struct native_layout_pointer_test_value source = {0};
        struct native_layout_pointer_test_value *destination =
            (struct native_layout_pointer_test_value *)cxcore_System_Memory_Alloc(
                (cx_uint)sizeof(*destination));
        cx_arena_enter(arena, &scope);
        source.value = cx_arena_allocate_current(
            sizeof(cx_uint), sizeof(cx_uint));
        source.scalar = 0x1234u;
        (void)cx_gc_copy_native_value_unknown_layout(
            destination, &source, sizeof(source));
        return 106;
    }
    if (argc == 2 && strcmp(argv[1], "arena-native-unknown-copy-stack") == 0) {
        struct cx_arena *arena = cx_arena_create(128u, 0u, 0);
        struct native_unknown_layout_copy_context context = { CX_FALSE };
        cx_arena_run_callback(arena, copy_unknown_native_value_to_stack,
                              &context);
        if (!context.copied_scoped_reference) return 107;
        cx_arena_destroy(arena);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-native-value-copy-escape") == 0) {
        static const size_t pointer_offsets[] = {
            offsetof(struct native_layout_pointer_test_value, value),
        };
        struct cx_arena *arena = cx_arena_create(128u, 0u, 0);
        struct cx_arena_scope scope;
        struct native_layout_pointer_test_value source = {0};
        struct native_layout_pointer_test_value *destination =
            (struct native_layout_pointer_test_value *)cxcore_System_Memory_Alloc(
                (cx_uint)sizeof(*destination));
        cx_arena_enter(arena, &scope);
        source.value = cx_arena_allocate_current(
            sizeof(cx_uint), sizeof(cx_uint));
        source.scalar = 0x1234u;
        (void)cx_gc_copy_native_value(destination, &source, sizeof(source),
                                      pointer_offsets,
                                      sizeof(pointer_offsets) / sizeof(pointer_offsets[0]));
        return 104;
    }
    if (argc == 2 && strcmp(argv[1], "arena-native-value-copy-local") == 0) {
        static const size_t pointer_offsets[] = {
            offsetof(struct native_layout_pointer_test_value, value),
        };
        struct cx_arena *arena = cx_arena_create(128u, 0u, 0);
        struct cx_arena_scope scope;
        struct native_layout_pointer_test_value source = {0};
        struct native_layout_pointer_test_value *destination;
        cx_arena_enter(arena, &scope);
        source.value = cx_arena_allocate_current(
            sizeof(cx_uint), sizeof(cx_uint));
        source.scalar = 0x1234u;
        destination = (struct native_layout_pointer_test_value *)
            cx_arena_allocate_current(sizeof(*destination),
                                      cx_arena_maximum_alignment());
        (void)cx_gc_copy_native_value(destination, &source, sizeof(source),
                                      pointer_offsets,
                                      sizeof(pointer_offsets) / sizeof(pointer_offsets[0]));
        if (destination->value != source.value ||
            destination->scalar != source.scalar) return 105;
        cx_arena_leave(&scope);
        cx_arena_reset(arena);
        cx_arena_destroy(arena);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-native-nested-array-copy-local") == 0) {
        static const size_t pointer_offsets[] = {
            offsetof(struct native_layout_nested_test_value, items[0].value),
            offsetof(struct native_layout_nested_test_value, items[1].value),
        };
        struct cx_arena *arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena_scope scope;
        struct native_layout_nested_test_value source = {0};
        struct native_layout_nested_test_value *destination;
        cx_arena_enter(arena, &scope);
        source.scalar = 0x5678u;
        source.items[0].value = cx_arena_allocate_current(
            sizeof(cx_uint), sizeof(cx_uint));
        source.items[0].marker = 0x1111u;
        source.items[1].value = cx_arena_allocate_current(
            sizeof(cx_uint), sizeof(cx_uint));
        source.items[1].marker = 0x2222u;
        destination = (struct native_layout_nested_test_value *)
            cx_arena_allocate_current(sizeof(*destination),
                                      cx_arena_maximum_alignment());
        (void)cx_gc_copy_native_value(destination, &source, sizeof(source),
                                      pointer_offsets,
                                      sizeof(pointer_offsets) / sizeof(pointer_offsets[0]));
        if (destination->scalar != source.scalar ||
            destination->items[0].value != source.items[0].value ||
            destination->items[0].marker != source.items[0].marker ||
            destination->items[1].value != source.items[1].value ||
            destination->items[1].marker != source.items[1].marker) return 108;
        cx_arena_leave(&scope);
        cx_arena_reset(arena);
        cx_arena_destroy(arena);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-reflected-native-copy-local") == 0) {
        struct cx_arena *arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena_scope scope;
        struct reflected_native_embedded_value source = {0};
        struct reflected_native_embedded_value *destination;
        cx_arena_enter(arena, &scope);
        source.native.value = cx_arena_allocate_current(
            sizeof(cx_uint), sizeof(cx_uint));
        source.native.scalar = 0x2345u;
        source.scalar = 0x6789u;
        destination = (struct reflected_native_embedded_value *)
            cx_arena_allocate_current(sizeof(*destination),
                                      cx_arena_maximum_alignment());
        (void)cx_gc_copy_value(destination, &source,
                               &reflected_native_embedded_type);
        if (destination->native.value != source.native.value ||
            destination->native.scalar != source.native.scalar ||
            destination->scalar != source.scalar) return 109;
        cx_arena_leave(&scope);
        cx_arena_destroy(arena);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "typed-copy-native-function-field") == 0) {
        struct native_function_value_test source = {
            native_function_value_test_callback,
            cxcore_System_Memory_Alloc((cx_uint)sizeof(cx_uint)),
        };
        struct native_function_value_test *destination =
            (struct native_function_value_test *)cxcore_System_Memory_Alloc(
                (cx_uint)sizeof(*destination));
        (void)cx_gc_copy_value(destination, &source,
                               &native_function_value_test_value_type);
        if (destination->function != source.function ||
            destination->reference != source.reference ||
            destination->function(33) != 42) {
            return 113;
        }
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "typed-copy-native-function-field-escape") == 0) {
        struct cx_arena *arena = cx_arena_create(128u, 0u, 0);
        struct cx_arena_scope scope;
        struct native_function_value_test source = {
            native_function_value_test_callback, CX_NULL,
        };
        struct native_function_value_test *destination =
            (struct native_function_value_test *)cxcore_System_Memory_Alloc(
                (cx_uint)sizeof(*destination));
        cx_arena_enter(arena, &scope);
        source.reference = cx_arena_allocate_current(sizeof(cx_uint), sizeof(cx_uint));
        (void)cx_gc_copy_value(destination, &source,
                               &native_function_value_test_value_type);
        return 114;
    }
    if (argc == 2 && strcmp(argv[1], "arena-reflected-native-copy-escape") == 0) {
        struct cx_arena *arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena_scope scope;
        struct reflected_native_embedded_value source = {0};
        struct reflected_native_embedded_value *destination =
            (struct reflected_native_embedded_value *)cxcore_System_Memory_Alloc(
                (cx_uint)sizeof(*destination));
        cx_arena_enter(arena, &scope);
        source.native.value = cx_arena_allocate_current(
            sizeof(cx_uint), sizeof(cx_uint));
        (void)cx_gc_copy_value(destination, &source,
                               &reflected_native_embedded_type);
        return 110;
    }
    if (argc == 2 && strcmp(argv[1], "arena-reflected-unknown-copy-stack") == 0) {
        struct cx_arena *arena = cx_arena_create(256u, 0u, 0);
        struct native_unknown_layout_copy_context context = { CX_FALSE };
        cx_arena_run_callback(arena, copy_unknown_reflected_value_to_stack,
                              &context);
        if (!context.copied_scoped_reference) return 111;
        cx_arena_destroy(arena);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "arena-reflected-unknown-copy-escape") == 0) {
        struct cx_arena *arena = cx_arena_create(256u, 0u, 0);
        struct cx_arena_scope scope;
        struct native_unknown_embedded_value source = {0};
        struct native_unknown_embedded_value *destination =
            (struct native_unknown_embedded_value *)cxcore_System_Memory_Alloc(
                (cx_uint)sizeof(*destination));
        cx_arena_enter(arena, &scope);
        source.native.value = cx_arena_allocate_current(
            sizeof(cx_uint), sizeof(cx_uint));
        (void)cx_gc_copy_value(destination, &source,
                               &native_unknown_embedded_type);
        return 112;
    }
    if (argc == 2 && strcmp(argv[1], "managed-store-overlaps-start") == 0) {
        cx_ptr block = cxcore_System_Memory_Alloc((cx_uint)(sizeof(cx_ptr) * 2));
        cx_ptr value = cxcore_System_Memory_Alloc((cx_uint)sizeof(cx_uint));
        cx_gc_store_reference((cx_ptr *)((unsigned char *)block - 1), value);
        return 36;
    }
    if (argc == 2 && strcmp(argv[1], "managed-store-stack-source") == 0) {
        cx_ptr destination = cxcore_System_Memory_Alloc((cx_uint)sizeof(cx_ptr));
        cx_uint stack_value = 0x51a7u;
        (void)cx_gc_store_reference((cx_ptr *)destination, &stack_value);
        return 98;
    }
    if (argc == 2 && strcmp(argv[1], "managed-store-native-static-source") == 0) {
        cx_ptr destination = cxcore_System_Memory_Alloc((cx_uint)sizeof(cx_ptr));
        if (cx_gc_store_reference(
                (cx_ptr *)destination, &native_pointer_store_value) !=
                &native_pointer_store_value ||
            *(cx_ptr *)destination != &native_pointer_store_value) {
            return 100;
        }
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "managed-store-native-pointer") == 0) {
        cx_ptr destination = cxcore_System_Memory_Alloc((cx_uint)sizeof(cx_ptr));
        if (cx_gc_store_native_pointer(
                (cx_ptr *)destination, &native_pointer_store_value) !=
                &native_pointer_store_value ||
            *(cx_ptr *)destination != &native_pointer_store_value) {
            return 102;
        }
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "static-root-registry-index") == 0) {
        struct cx_gc_root_region roots[CX_TEST_STATIC_ROOT_INDEX_COUNT];
        size_t index;
        for (index = 0; index < CX_TEST_STATIC_ROOT_INDEX_COUNT; ++index) {
            roots[index].address = &static_root_index_values[index];
            roots[index].size = (cx_uint)sizeof(static_root_index_values[index]);
        }
        cx_gc_register_static_roots(
            roots, (cx_uint)CX_TEST_STATIC_ROOT_INDEX_COUNT);
        cx_gc_register_static_roots(
            roots, (cx_uint)CX_TEST_STATIC_ROOT_INDEX_COUNT);
        for (index = 0; index < CX_TEST_STATIC_ROOT_INDEX_COUNT; ++index) {
            cx_uint *value = (cx_uint *)cxcore_System_Memory_Alloc(
                (cx_uint)sizeof(cx_uint));
            if (value == NULL) return 113;
            *value = (cx_uint)(0x22a500u + (cx_uint)index);
            static_root_index_values[index] = value;
        }
        cx_gc_collect();
        for (index = 0; index < CX_TEST_STATIC_ROOT_INDEX_COUNT; ++index) {
            cx_uint *value = (cx_uint *)static_root_index_values[index];
            if (value == NULL || *value != (cx_uint)(0x22a500u + (cx_uint)index)) {
                return 114;
            }
            static_root_index_values[index] = NULL;
        }
        cx_gc_collect();
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "static-root-registry-index-allocation-failure") == 0) {
        struct cx_gc_root_region roots[CX_TEST_STATIC_ROOT_INDEX_COUNT];
        size_t index;
        cx_memory_test_reset_static_root_index();
        cx_memory_test_fail_next_static_root_index_allocation();
        for (index = 0; index < CX_TEST_STATIC_ROOT_INDEX_COUNT; ++index) {
            static_root_index_values[index] = NULL;
            roots[index].address = &static_root_index_values[index];
            roots[index].size = (cx_uint)sizeof(static_root_index_values[index]);
        }
        cx_gc_register_static_roots(
            roots, (cx_uint)CX_TEST_STATIC_ROOT_INDEX_COUNT);
        if (!cx_memory_test_static_root_index_is_disabled()) return 115;
        cx_gc_register_static_roots(
            roots, (cx_uint)CX_TEST_STATIC_ROOT_INDEX_COUNT);
        for (index = 0; index < CX_TEST_STATIC_ROOT_INDEX_COUNT; ++index) {
            cx_uint *value = (cx_uint *)cxcore_System_Memory_Alloc(
                (cx_uint)sizeof(cx_uint));
            if (value == NULL) return 116;
            *value = (cx_uint)(0x22b600u + (cx_uint)index);
            static_root_index_values[index] = value;
        }
        cx_gc_collect();
        for (index = 0; index < CX_TEST_STATIC_ROOT_INDEX_COUNT; ++index) {
            cx_uint *value = (cx_uint *)static_root_index_values[index];
            if (value == NULL || *value != (cx_uint)(0x22b600u + (cx_uint)index)) {
                return 117;
            }
            static_root_index_values[index] = NULL;
        }
        cx_gc_collect();
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "managed-interface-stack-source") == 0) {
        struct cx_iface_ref *destination =
            (struct cx_iface_ref *)cxcore_System_Memory_Alloc(
                (cx_uint)sizeof(struct cx_iface_ref));
        cx_uint stack_value = 0x51a7u;
        struct cx_iface_ref value = {0};
        value.instance = &stack_value;
        (void)cx_gc_store_interface_reference(destination, value);
        return 99;
    }
    if (argc != 1) {
        return 6;
    }

    cx_gc_register_static_roots(static_roots, (cx_uint)(sizeof(static_roots) / sizeof(static_roots[0])));
    {
        struct typed_copy_test_value source;
        struct typed_copy_test_value *destination;
        source.reference = cxcore_System_Memory_Alloc((cx_uint)sizeof(cx_uint));
        source.scalar = 0x1234u;
        destination = (struct typed_copy_test_value *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(*destination));
        cx_gc_copy_value(destination, &source, &typed_copy_value_type);
        if (destination->reference != source.reference ||
            destination->scalar != source.scalar) {
            fputs("typed aggregate copy changed its fields\n", stderr);
            return 31;
        }
    }
    {
        struct cxcore_System_Nullable source = {0};
        struct cxcore_System_Nullable result = {0};
        source._obj = cxcore_System_Memory_Alloc((cx_uint)sizeof(cx_uint));
        cxcore_System_Nullable_Value___const_get(&source, &result);
        if (result._obj != source._obj) {
            fputs("nullable value getter changed its reference\n", stderr);
            return 42;
        }
    }
    if (!retain_stack_root()) {
        fputs("stack root was not retained\n", stderr);
        return 1;
    }

    if (!retain_optimized_roots()) {
        fputs("optimized register roots were not retained\n", stderr);
        return 8;
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
    aggregate_array = (struct cxcore_System_Collections_Array *)aggregate_root;
    aggregate_nullable = aggregate_array == NULL
        ? NULL
        : (struct cxcore_System_Nullable *)*(cx_ptr *)aggregate_array->_data;
    aggregate_interface = aggregate_nullable == NULL
        ? NULL : (struct cx_iface_ref *)aggregate_nullable->_obj;
    if (aggregate_interface == NULL || string_root == NULL ||
        ((cx_uint *)aggregate_interface->instance)[0] != 0xacedu ||
        memcmp(((struct cxcore_System_String *)string_root)->_data, "root!", 5) != 0) {
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

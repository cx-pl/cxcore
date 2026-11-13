// Native acceptance checks must remain active in optimized builds.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <string.h>

#include "../src/cxcore.h"

typedef struct cxcore_System_Reflection_TypeInfo type_info;

static void assert_reflection_string(
    const struct cxcore_System_String *value,
    const char *expected) {
    cx_uint length = (cx_uint)strlen(expected);
    assert(value != CX_NULL);
    assert(cxcore_System_String_Length___const_get(value) == length);
    for (cx_uint index = 0; index < length; index++) {
        assert(cxcore_System_String_Item___const_get(value, index) ==
               (cx_char)(cx_byte)expected[index]);
    }
}

struct test_base {
    cx_ptr vtable;
};
struct test_derived {
    struct test_base __base;
};

static type_info base_type = {.Hash = 1};
static type_info derived_type;
static type_info interface_type = {.Hash = 3};
static type_info boxed_struct_type;
static union cx_vtable_entry derived_vtable[] = {{.data = &derived_type}};
static union cx_vtable_entry interface_vtable[] = {{.data = &derived_type}};
static const struct cx_interface_impl interfaces[] = {
    {&interface_type, interface_vtable},
};
static const struct cx_reflection_field fields[] = {
    {CX_REFLECTION_FLAG_VISIBILITY_PRIVATE, 0, &base_type, "value", sizeof(cx_int)},
};
static const struct cx_reflection_parameter parameters[] = {
    {0, &base_type, "scale", CX_NULL},
};
static const struct cx_reflection_function functions[] = {
    {
        CX_REFLECTION_FLAG_VISIBILITY_PUBLIC | CX_REFLECTION_FLAG_VIRTUAL,
        1,
        &base_type,
        "Read",
        parameters,
        1,
    },
};
static const struct cx_runtime_type_info runtime_type_info = {
    .interfaces = interfaces,
    .interfaceCount = 1,
    .fields = fields,
    .fieldCount = 1,
    .functions = functions,
    .functionCount = 1,
};
static type_info derived_type = {
    .Hash = 2,
    .BaseType = {._obj = &base_type},
    .RuntimeTypeInfo = (cx_ptr)&runtime_type_info,
    .GenericArity = 1,
};

static const cx_ptr echo_int_type_arguments[] = {&base_type};
static const cx_ptr echo_interface_type_arguments[] = {&interface_type};

static cx_int generic_virtual_echo_int(cx_ptr receiver, cx_int value) {
    (void)receiver;
    return value + 1;
}

static cx_int generic_virtual_echo_interface(cx_ptr receiver, cx_int value) {
    (void)receiver;
    return value + 2;
}

static struct cx_generic_virtual_registration echo_int_registration = {
    &derived_type,
    "Example.Base.Echo`1(T)->T",
    echo_int_type_arguments,
    1,
    "Example.Derived.Echo<int>",
    (cx_vtable_function)generic_virtual_echo_int,
    CX_NULL,
};
static struct cx_generic_virtual_registration echo_interface_registration = {
    &derived_type,
    "Example.Base.Echo`1(T)->T",
    echo_interface_type_arguments,
    1,
    "Example.Derived.Echo<IExample>",
    (cx_vtable_function)generic_virtual_echo_interface,
    CX_NULL,
};
static struct cx_generic_virtual_registration echo_int_duplicate_registration = {
    &derived_type,
    "Example.Base.Echo`1(T)->T",
    echo_int_type_arguments,
    1,
    "Example.Derived.Echo<int>",
    (cx_vtable_function)generic_virtual_echo_interface,
    CX_NULL,
};

struct boxed_struct_value {
    cx_int value;
};

static type_info boxed_struct_scalar_type = {
    .Flags = CX_REFLECTION_FLAG_TYPE_ENUM,
    .Size = (cx_uint)sizeof(cx_int),
};
static const struct cx_reflection_field boxed_struct_fields[] = {
    {CX_REFLECTION_FLAG_VISIBILITY_PRIVATE,
     (cx_uint)offsetof(struct boxed_struct_value, value),
     &boxed_struct_scalar_type, "value",
     (cx_uint)sizeof(((struct boxed_struct_value *)0)->value)},
};

static cx_int boxed_struct_add(cx_ptr instance, cx_int amount) {
    struct boxed_struct_value *value = (struct boxed_struct_value *)instance;
    value->value += amount;
    return value->value;
}

static union cx_vtable_entry boxed_struct_interface_vtable[] = {
    {.data = &boxed_struct_type},
    {.function = (cx_vtable_function)boxed_struct_add},
};
static const struct cx_interface_impl boxed_struct_interfaces[] = {
    {&interface_type, boxed_struct_interface_vtable},
};
static const struct cx_runtime_type_info boxed_struct_runtime_type_info = {
    .interfaces = boxed_struct_interfaces,
    .interfaceCount = 1,
    .fields = boxed_struct_fields,
    .fieldCount = 1,
};
static type_info boxed_struct_type = {
    .Hash = 4,
    .Flags = CX_REFLECTION_FLAG_TYPE_STRUCT,
    .Size = sizeof(struct boxed_struct_value),
    .RuntimeTypeInfo = (cx_ptr)&boxed_struct_runtime_type_info,
};

static const type_info *module_test_type(void) {
    return &cxcore_System_Int___typeinfo;
}

static struct cxcore_System_Collections_Array *module_test_array_reader(
    cx_ptr value, cx_uint maximumLength) {
    (void)value;
    (void)maximumLength;
    return CX_NULL;
}

static void test_module_metadata_format(void) {
    const cx_reflection_element_provider arguments[] = {module_test_type};
    const struct cx_reflection_generic_entry generic_types[] = {
        {module_test_type, arguments, 1u},
    };
    const struct cx_reflection_array_entry array_readers[] = {
        {&cxcore_System_Int___typeinfo, 3u, module_test_array_reader, module_test_type},
    };
    struct cx_module_info descriptor = {
        .formatVersion = 1u,
        .name = "format-test",
        .version = "1.0",
        .genericTypes = generic_types,
        .genericTypeCount = 1u,
        .arrayReaders = array_readers,
        .arrayReaderCount = 1u,
    };
    struct cxcore_System_Reflection_ModuleInfo module = cx_module_info_wrap(&descriptor);
    struct cxcore_System_Collections_Array *generic_arguments =
        cxcore_System_Reflection_ModuleInfo_GetGenericArguments(
            &module, cxcore_System_Int___typeinfo);
    type_info element_type = cxcore_System_Reflection_ModuleInfo_GetFunctionArrayElementType(
        &module, cxcore_System_Int___typeinfo, 3u);

    assert(CX_MODULE_METADATA_FORMAT_VERSION == 1u);
    assert(module._descriptor == &descriptor);
    assert_reflection_string(module.Name, "format-test");
    assert(generic_arguments->_length == 1u);
    assert(((type_info *)generic_arguments->_data)[0].Hash == cxcore_System_Int___typeinfo.Hash);
    assert(cxcore_System_Reflection_ModuleInfo_CanReadFunctionArray(
        &module, cxcore_System_Int___typeinfo, 3u));
    assert(element_type.Hash == cxcore_System_Int___typeinfo.Hash);

    const cx_uint unsupported_versions[] = {0u, 2u, 3u, 4u, 5u};
    for (cx_uint index = 0u; index < sizeof(unsupported_versions) / sizeof(unsupported_versions[0]); ++index) {
        descriptor.formatVersion = unsupported_versions[index];
        struct cxcore_System_Reflection_ModuleInfo unsupported = cx_module_info_wrap(&descriptor);
        assert(unsupported._descriptor == CX_NULL);
        assert(unsupported.Name == CX_NULL && unsupported.Types == CX_NULL);
        assert(cxcore_System_Reflection_ModuleInfo_GetGenericArguments(
            &module, cxcore_System_Int___typeinfo)->_length == 0u);
        assert(!cxcore_System_Reflection_ModuleInfo_CanReadFunctionArray(
            &module, cxcore_System_Int___typeinfo, 3u));
    }
}

int main(void) {
    test_module_metadata_format();
    struct cxcore_System_Reflection_ModuleInfo core_module =
        cxcore_System_Reflection_TypeInfo_GetModule(&cxcore_System_Int___typeinfo);
    assert_reflection_string(core_module.Name, "cxcore");
    assert(core_module.Types != CX_NULL && core_module.Types->_length > 0u);
    for (cx_uint module_type_index = 0u; module_type_index < core_module.Types->_length; ++module_type_index) {
        const type_info *module_type = &((const type_info *)core_module.Types->_data)[module_type_index];
        struct cxcore_System_Reflection_ModuleInfo owner = cxcore_System_Reflection_TypeInfo_GetModule(module_type);
        assert_reflection_string(owner.Name, "cxcore");
    }
    type_info array_info = cx_typeof_array(cxcore_System_String___typeinfo);
    struct cxcore_System_Reflection_ModuleInfo array_module =
        cxcore_System_Reflection_TypeInfo_GetModule(&array_info);
    assert_reflection_string(array_module.Name, "cxcore");
    struct cxcore_System_Reflection_ModuleInfo clock_module =
        cxcore_System_Reflection_TypeInfo_GetModule(&cxcore_System_Diagnostics_MonotonicClock___typeinfo);
    assert_reflection_string(clock_module.Name, "cxcore");
    cx_ulong timestamp = cxcore_System_Diagnostics_MonotonicClock_GetMicroseconds();
    assert(cxcore_System_Diagnostics_MonotonicClock_GetMicroseconds() >= timestamp);
    struct test_derived object = {{derived_vtable}};
    type_info collision_type = cxcore_System_Object___typeinfo;
    type_info duplicate_type = cxcore_System_String___typeinfo;
    struct cx_iface_ref interface_reference;
    const struct cx_interface_impl *reflected_interfaces;
    const struct cx_reflection_field *reflected_fields;
    const struct cx_reflection_function *reflected_functions;
    cx_uint count;

    cx_register_generic_virtual_method(&echo_int_registration);
    cx_register_generic_virtual_method(&echo_interface_registration);
    cx_register_generic_virtual_method(&echo_int_duplicate_registration);
    cx_register_generic_virtual_method(&echo_int_registration);
    assert(cx_get_generic_virtual_method(
               &derived_type, "Example.Base.Echo`1(T)->T",
               echo_int_type_arguments, 1) ==
           (cx_vtable_function)generic_virtual_echo_int);
    assert(cx_get_generic_virtual_method(
               &derived_type, "Example.Base.Echo`1(T)->T",
               echo_interface_type_arguments, 1) ==
           (cx_vtable_function)generic_virtual_echo_interface);
    assert(cx_get_generic_virtual_method(
               &base_type, "Example.Base.Echo`1(T)->T",
               echo_int_type_arguments, 1) == CX_NULL);
    assert(cx_get_generic_virtual_method(
               &derived_type, "Example.Base.Other`1(T)->T",
               echo_int_type_arguments, 1) == CX_NULL);
    const type_info *array_type = &cxcore_System_Collections_Array___typeinfo;
    collision_type.Hash = cxcore_System_String___typeinfo.Hash;
    collision_type.Name = cxcore_System_String___typeinfo.Name;
    collision_type.Namespace = cxcore_System_String___typeinfo.Namespace;
    collision_type.GenericArity = cxcore_System_String___typeinfo.GenericArity;
    collision_type.GenericParams = cxcore_System_String___typeinfo.GenericParams;
    assert(!cxcore_System_Reflection_TypeInfo_IsSameType(
        &cxcore_System_String___typeinfo, collision_type));
    assert(cxcore_System_Reflection_TypeInfo_IsSameType(
        &cxcore_System_String___typeinfo, duplicate_type));
    assert(array_type->Hash == 0xE0710BB22F195AB4ULL);
    assert_reflection_string(array_type->Name, "Array");
    assert_reflection_string(array_type->Namespace, "System.Collections");
    assert(cx_is_object(&object, &derived_type));
    assert(cx_is_object(&object, &base_type));
    assert(cx_is_object(&object, &interface_type));
    assert(
        cxcore_System_Reflection_TypeInfo_IsAssignableFrom(&base_type, derived_type));
    assert(cxcore_System_Reflection_TypeInfo_IsAssignableFrom(&interface_type,
                                                                           derived_type));
    assert(
        cxcore_System_Reflection_TypeInfo_IsInBaseTypeChain(&base_type, derived_type));
    assert(!cxcore_System_Reflection_TypeInfo_IsInBaseTypeChain(&interface_type,
                                                                             derived_type));
    assert(cxcore_System_Reflection_TypeInfo_HasRuntimeInterface(&derived_type,
                                                                              interface_type.Hash));
    assert(!cxcore_System_Reflection_TypeInfo_HasRuntimeInterface(&derived_type,
                                                                               base_type.Hash));
    assert(
        !cxcore_System_Reflection_TypeInfo_IsAssignableFrom(&derived_type, base_type));
    assert(!cx_is_object(CX_NULL, &derived_type));
    assert(cx_checked_cast_object(&object, &derived_type) == &object);
    assert(cx_checked_cast_object(CX_NULL, &derived_type) == CX_NULL);
    assert(cx_safe_cast_object(&object, &base_type) == &object);
    assert(cx_safe_cast_object(&object, &cxcore_System_String___typeinfo) == CX_NULL);
    assert(cx_safe_cast_object(CX_NULL, &derived_type) == CX_NULL);
    struct cx_iface_ref missing = cx_safe_cast_object_to_interface(&object, &base_type);
    assert(missing.vtable == CX_NULL && missing.instance == CX_NULL && missing.owner == CX_NULL);

    interface_reference = cx_checked_cast_object_to_interface(&object, &interface_type);
    assert(interface_reference.instance == &object);
    assert(interface_reference.vtable == interface_vtable);
    assert(cx_is_interface(interface_reference, &derived_type));
    assert(cx_checked_cast_interface(interface_reference, &derived_type) == &object);
    assert(cx_safe_cast_interface(interface_reference, &base_type) == &object);
    assert(cx_safe_cast_interface(interface_reference, &cxcore_System_String___typeinfo) == CX_NULL);
    assert(cx_safe_cast_interface_to_interface(interface_reference, &interface_type).instance == &object);
    missing = cx_safe_cast_interface_to_interface(interface_reference, &base_type);
    assert(missing.vtable == CX_NULL && missing.instance == CX_NULL && missing.owner == CX_NULL);
    assert(cx_checked_cast_interface_to_interface(interface_reference, &interface_type).vtable ==
           interface_vtable);
    assert(cx_checked_cast_object_to_interface(CX_NULL, &interface_type).instance == CX_NULL);

    const struct boxed_struct_value initial_value = {17};
    cx_ptr boxed_value = cx_box_value(
        &initial_value, (cx_uint)sizeof(initial_value), &boxed_struct_type);
    struct cx_iface_ref boxed_interface =
        cx_checked_cast_object_to_interface(boxed_value, &interface_type);
    assert(boxed_interface.vtable == boxed_struct_interface_vtable);
    assert(boxed_interface.instance == cx_unbox_value(boxed_value, &boxed_struct_type));
    assert(boxed_interface.owner == boxed_value);
    assert(cx_is_interface(boxed_interface, &interface_type));
    assert(cx_checked_cast_interface(
               boxed_interface, &cxcore_System_Object___typeinfo) == boxed_value);
    cx_int (*boxed_add)(cx_ptr, cx_int) =
        (cx_int (*)(cx_ptr, cx_int))((union cx_vtable_entry *)boxed_interface.vtable)[1].function;
    assert(boxed_add(boxed_interface.instance, 5) == 22);
    assert(((struct boxed_struct_value *)cx_unbox_value(
                boxed_value, &boxed_struct_type))->value == 22);

    reflected_interfaces = cx_reflection_interfaces(&derived_type, &count);
    assert(count == 1 && reflected_interfaces[0].typeInfo == &interface_type);
    reflected_fields = cx_reflection_fields(&derived_type, &count);
    assert(count == 1 && reflected_fields[0].typeInfo == &base_type);
    assert(reflected_fields[0].name[0] == 'v');
    reflected_functions = cx_reflection_functions(&derived_type, &count);
    assert(count == 1 && reflected_functions[0].slot == 1);
    assert(reflected_functions[0].parameterCount == 1);
    assert(reflected_functions[0].parameters[0].typeInfo == &base_type);
    assert(cx_reflection_fields(CX_NULL, &count) == CX_NULL && count == 0);
    assert(cx_reflection_functions(CX_NULL, &count) == CX_NULL && count == 0);
    assert(derived_type.GenericArity == 1);
    return 0;
}

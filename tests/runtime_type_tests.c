#include <assert.h>
#include <string.h>

#include "../src/cxcore.h"

typedef struct CX_ID_4(cxcore, System, Reflection, TypeInfo) type_info;

static void assert_reflection_string(
    const struct CX_ID_3(cxcore, System, String) *value,
    const char *expected) {
    cx_uint length = (cx_uint)strlen(expected);
    assert(value != CX_NULL);
    assert(CX_ID_5(cxcore, System, String, Length, __const_get)(value) == length);
    for (cx_uint index = 0; index < length; index++) {
        assert(CX_ID_5(cxcore, System, String, Item, __const_get)(value, index) ==
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
    {CX_REFLECTION_FLAG_VISIBILITY_PRIVATE, 0, &base_type, "value"},
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

struct boxed_struct_value {
    cx_int value;
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
};
static type_info boxed_struct_type = {
    .Hash = 4,
    .Size = sizeof(struct boxed_struct_value),
    .RuntimeTypeInfo = (cx_ptr)&boxed_struct_runtime_type_info,
};

int main(void) {
    struct test_derived object = {{derived_vtable}};
    struct cx_iface_ref interface_reference;
    const struct cx_interface_impl *reflected_interfaces;
    const struct cx_reflection_field *reflected_fields;
    const struct cx_reflection_function *reflected_functions;
    cx_uint count;
    const type_info *array_type = &CX_ID_5(cxcore, System, Collections, Array, __typeinfo);
    assert(array_type->Hash == 0xE0710BB22F195AB4ULL);
    assert_reflection_string(array_type->Name, "Array");
    assert_reflection_string(array_type->Namespace, "System.Collections");
    assert(cx_is_object(&object, &derived_type));
    assert(cx_is_object(&object, &base_type));
    assert(cx_is_object(&object, &interface_type));
    assert(
        CX_ID_5(cxcore, System, Reflection, TypeInfo, IsAssignableFrom)(&base_type, derived_type));
    assert(CX_ID_5(cxcore, System, Reflection, TypeInfo, IsAssignableFrom)(&interface_type,
                                                                           derived_type));
    assert(
        CX_ID_5(cxcore, System, Reflection, TypeInfo, IsInBaseTypeChain)(&base_type, derived_type));
    assert(!CX_ID_5(cxcore, System, Reflection, TypeInfo, IsInBaseTypeChain)(&interface_type,
                                                                             derived_type));
    assert(CX_ID_5(cxcore, System, Reflection, TypeInfo, HasRuntimeInterface)(&derived_type,
                                                                              interface_type.Hash));
    assert(!CX_ID_5(cxcore, System, Reflection, TypeInfo, HasRuntimeInterface)(&derived_type,
                                                                               base_type.Hash));
    assert(
        !CX_ID_5(cxcore, System, Reflection, TypeInfo, IsAssignableFrom)(&derived_type, base_type));
    assert(!cx_is_object(CX_NULL, &derived_type));
    assert(cx_checked_cast_object(&object, &derived_type) == &object);
    assert(cx_checked_cast_object(CX_NULL, &derived_type) == CX_NULL);

    interface_reference = cx_checked_cast_object_to_interface(&object, &interface_type);
    assert(interface_reference.instance == &object);
    assert(interface_reference.vtable == interface_vtable);
    assert(cx_is_interface(interface_reference, &derived_type));
    assert(cx_checked_cast_interface(interface_reference, &derived_type) == &object);
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
               boxed_interface, &CX_ID_4(cxcore, System, Object, __typeinfo)) == boxed_value);
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

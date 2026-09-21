#include <assert.h>

#include "../src/cxcore.h"

typedef struct CX_ID_4(cxcore, System, Reflection, TypeInfo) type_info;

struct test_base { cx_ptr vtable; };
struct test_derived { struct test_base __base; };

static type_info base_type = { 0 };
static type_info derived_type;
static type_info interface_type = { 0 };
static union cx_vtable_entry derived_vtable[] = { { .data = &derived_type } };
static union cx_vtable_entry interface_vtable[] = { { .data = &derived_type } };
static const struct cx_interface_impl interfaces[] = {
    { &interface_type, interface_vtable },
};
static type_info derived_type = {
    .BaseType = { ._obj = &base_type },
    .RuntimeInterfaces = (cx_ptr)interfaces,
    .RuntimeInterfaceCount = 1,
};

int main(void)
{
    struct test_derived object = { { derived_vtable } };
    struct cx_iface_ref interface_reference;
    assert(cx_is_object(&object, &derived_type));
    assert(cx_is_object(&object, &base_type));
    assert(cx_is_object(&object, &interface_type));
    assert(!cx_is_object(CX_NULL, &derived_type));
    assert(cx_checked_cast_object(&object, &derived_type) == &object);
    assert(cx_checked_cast_object(CX_NULL, &derived_type) == CX_NULL);

    interface_reference = cx_checked_cast_object_to_interface(&object, &interface_type);
    assert(interface_reference.instance == &object);
    assert(interface_reference.vtable == interface_vtable);
    assert(cx_is_interface(interface_reference, &derived_type));
    assert(cx_checked_cast_interface(interface_reference, &derived_type) == &object);
    assert(cx_checked_cast_interface_to_interface(interface_reference, &interface_type).vtable == interface_vtable);
    assert(cx_checked_cast_object_to_interface(CX_NULL, &interface_type).instance == CX_NULL);
    return 0;
}

#include <stdlib.h>

#include "cxcore.h"

typedef struct CX_ID_4(cxcore, System, Reflection, TypeInfo) cx_type_info;

static const cx_type_info* cx_object_type(cx_ptr instance)
{
    if (instance == CX_NULL || CX_GET_VTABLE(instance) == CX_NULL)
    {
        return CX_NULL;
    }
    return (const cx_type_info*)((union cx_vtable_entry*)CX_GET_VTABLE(instance))[0].data;
}

static const cx_type_info* cx_interface_type(struct cx_iface_ref source)
{
    if (source.instance == CX_NULL || source.vtable == CX_NULL)
    {
        return CX_NULL;
    }
    return (const cx_type_info*)((union cx_vtable_entry*)source.vtable)[0].data;
}

static struct cx_iface_ref cx_find_interface(
    cx_ptr instance,
    const cx_type_info* actualType,
    const cx_type_info* targetType)
{
    struct cx_iface_ref result = { CX_NULL, instance };
    if (instance == CX_NULL)
    {
        return result;
    }

    for (const cx_type_info* current = actualType;
         current != CX_NULL;
         current = (const cx_type_info*)current->BaseType._obj)
    {
        const struct cx_interface_impl* interfaces =
            (const struct cx_interface_impl*)current->RuntimeInterfaces;
        for (cx_uint index = 0; index < current->RuntimeInterfaceCount; index++)
        {
            if (interfaces[index].typeInfo == (cx_ptr)targetType)
            {
                result.vtable = interfaces[index].vtable;
                return result;
            }
        }
    }
    return result;
}

cx_bool cx_type_is(const cx_type_info* actualType, const cx_type_info* targetType)
{
    if (actualType == CX_NULL || targetType == CX_NULL)
    {
        return CX_FALSE;
    }
    for (const cx_type_info* current = actualType;
         current != CX_NULL;
         current = (const cx_type_info*)current->BaseType._obj)
    {
        if (current == targetType)
        {
            return CX_TRUE;
        }
        const struct cx_interface_impl* interfaces =
            (const struct cx_interface_impl*)current->RuntimeInterfaces;
        for (cx_uint index = 0; index < current->RuntimeInterfaceCount; index++)
        {
            if (interfaces[index].typeInfo == (cx_ptr)targetType)
            {
                return CX_TRUE;
            }
        }
    }
    return CX_FALSE;
}

cx_bool cx_is_object(cx_ptr instance, const cx_type_info* targetType)
{
    return cx_type_is(cx_object_type(instance), targetType);
}

cx_bool cx_is_interface(struct cx_iface_ref source, const cx_type_info* targetType)
{
    return cx_type_is(cx_interface_type(source), targetType);
}

cx_ptr cx_checked_cast_object(cx_ptr instance, const cx_type_info* targetType)
{
    if (instance == CX_NULL || cx_is_object(instance, targetType))
    {
        return instance;
    }
    abort();
}

cx_ptr cx_checked_cast_interface(struct cx_iface_ref source, const cx_type_info* targetType)
{
    if (source.instance == CX_NULL || cx_is_interface(source, targetType))
    {
        return source.instance;
    }
    abort();
}

struct cx_iface_ref cx_checked_cast_object_to_interface(
    cx_ptr instance,
    const cx_type_info* targetType)
{
    struct cx_iface_ref result = cx_find_interface(instance, cx_object_type(instance), targetType);
    if (instance == CX_NULL || result.vtable != CX_NULL)
    {
        return result;
    }
    abort();
}

struct cx_iface_ref cx_checked_cast_interface_to_interface(
    struct cx_iface_ref source,
    const cx_type_info* targetType)
{
    struct cx_iface_ref result = cx_find_interface(
        source.instance,
        cx_interface_type(source),
        targetType);
    if (source.instance == CX_NULL || result.vtable != CX_NULL)
    {
        return result;
    }
    abort();
}

const struct cx_interface_impl* cx_reflection_interfaces(
    const cx_type_info* typeInfo,
    cx_uint* count)
{
    if (count != CX_NULL)
    {
        *count = typeInfo == CX_NULL ? 0 : typeInfo->RuntimeInterfaceCount;
    }
    return typeInfo == CX_NULL
        ? CX_NULL
        : (const struct cx_interface_impl*)typeInfo->RuntimeInterfaces;
}

const struct cx_reflection_field* cx_reflection_fields(
    const cx_type_info* typeInfo,
    cx_uint* count)
{
    if (count != CX_NULL)
    {
        *count = typeInfo == CX_NULL ? 0 : typeInfo->RuntimeFieldCount;
    }
    return typeInfo == CX_NULL
        ? CX_NULL
        : (const struct cx_reflection_field*)typeInfo->RuntimeFields;
}

const struct cx_reflection_function* cx_reflection_functions(
    const cx_type_info* typeInfo,
    cx_uint* count)
{
    if (count != CX_NULL)
    {
        *count = typeInfo == CX_NULL ? 0 : typeInfo->RuntimeFunctionCount;
    }
    return typeInfo == CX_NULL
        ? CX_NULL
        : (const struct cx_reflection_function*)typeInfo->RuntimeFunctions;
}

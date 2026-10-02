#include "../../../../src/cxcore.h"

typedef struct CX_ID_4(cxcore, System, Reflection, TypeInfo) cx_type_info;

cx_bool CX_ID_5(cxcore, System, Reflection, TypeInfo, HasRuntimeInterface)(
    const cx_type_info typeInfo,
    cx_ulong typeHash)
{
    const struct cx_interface_impl* interfaces =
        (const struct cx_interface_impl*)typeInfo.RuntimeInterfaces;
    for (cx_uint index = 0; index < typeInfo.RuntimeInterfaceCount; index++)
    {
        const cx_type_info* interfaceType =
            (const cx_type_info*)interfaces[index].typeInfo;
        if (interfaceType != CX_NULL && interfaceType->Hash == typeHash)
        {
            return CX_TRUE;
        }
    }
    return CX_FALSE;
}

cx_bool CX_ID_5(cxcore, System, Reflection, TypeInfo, IsInBaseTypeChain)(
    const cx_type_info target,
    const cx_type_info source)
{
    cx_type_info current = source;
    for (;;)
    {
        if (current.Hash == target.Hash)
        {
            return CX_TRUE;
        }

        cx_type_info parent = current;
        if (current.BaseType._obj != CX_NULL)
        {
            parent = *(const cx_type_info*)current.BaseType._obj;
        }
        if (parent.Hash == current.Hash)
        {
            return CX_FALSE;
        }
        current = parent;
    }
}

cx_bool CX_ID_5(cxcore, System, Reflection, TypeInfo, IsAssignableFrom)(
    const cx_type_info target,
    const cx_type_info source)
{
    cx_type_info current = source;
    for (;;)
    {
        if (current.Hash == target.Hash ||
            CX_ID_5(cxcore, System, Reflection, TypeInfo, HasRuntimeInterface)(
                current,
                target.Hash))
        {
            return CX_TRUE;
        }

        cx_type_info parent = current;
        if (current.BaseType._obj != CX_NULL)
        {
            parent = *(const cx_type_info*)current.BaseType._obj;
        }
        if (parent.Hash == current.Hash)
        {
            return CX_FALSE;
        }
        current = parent;
    }
}

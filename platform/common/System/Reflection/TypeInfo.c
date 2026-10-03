#include "../../../../src/cxcore.h"

typedef struct CX_ID_4(cxcore, System, Reflection, TypeInfo) cx_type_info;

static cx_bool cx_typeinfo_has_runtime_interface(const cx_type_info *typeInfo, cx_ulong typeHash) {
    const struct cx_runtime_type_info *runtimeTypeInfo =
        (const struct cx_runtime_type_info *)typeInfo->RuntimeTypeInfo;
    if (runtimeTypeInfo == CX_NULL) {
        return CX_FALSE;
    }
    for (cx_uint index = 0; index < runtimeTypeInfo->interfaceCount; index++) {
        const cx_type_info *interfaceType =
            (const cx_type_info *)runtimeTypeInfo->interfaces[index].typeInfo;
        if (interfaceType != CX_NULL && interfaceType->Hash == typeHash) {
            return CX_TRUE;
        }
    }
    return CX_FALSE;
}

cx_bool CX_ID_5(cxcore, System, Reflection, TypeInfo,
                HasRuntimeInterface)(const cx_type_info *__this, cx_ulong typeHash) {
    return cx_typeinfo_has_runtime_interface(__this, typeHash);
}

cx_bool CX_ID_5(cxcore, System, Reflection, TypeInfo,
                IsInBaseTypeChain)(const cx_type_info *__this, const cx_type_info source) {
    const cx_type_info *current = &source;
    for (;;) {
        if (current->Hash == __this->Hash) {
            return CX_TRUE;
        }

        const cx_type_info *parent = current;
        if (current->BaseType._obj != CX_NULL) {
            parent = (const cx_type_info *)current->BaseType._obj;
        }
        if (parent->Hash == current->Hash) {
            return CX_FALSE;
        }
        current = parent;
    }
}

cx_bool CX_ID_5(cxcore, System, Reflection, TypeInfo, IsAssignableFrom)(const cx_type_info *__this,
                                                                        const cx_type_info source) {
    const cx_type_info *current = &source;
    for (;;) {
        if (current->Hash == __this->Hash ||
            cx_typeinfo_has_runtime_interface(current, __this->Hash)) {
            return CX_TRUE;
        }

        const cx_type_info *parent = current;
        if (current->BaseType._obj != CX_NULL) {
            parent = (const cx_type_info *)current->BaseType._obj;
        }
        if (parent->Hash == current->Hash) {
            return CX_FALSE;
        }
        current = parent;
    }
}

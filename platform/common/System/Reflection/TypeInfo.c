#include "../../../../src/cxcore.h"
#include <string.h>

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

struct CX_ID_3(cxcore, System, Object) *CX_ID_5(
    cxcore, System, Reflection, TypeInfo, CreateInstance)(const cx_type_info *__this) {
    if (__this == CX_NULL || __this->RuntimeTypeInfo == CX_NULL) {
        return CX_NULL;
    }

    const struct cx_runtime_type_info *runtimeTypeInfo =
        (const struct cx_runtime_type_info *)__this->RuntimeTypeInfo;
    return runtimeTypeInfo->createInstance != NULL
        ? runtimeTypeInfo->createInstance()
        : CX_NULL;
}

struct CX_ID_3(cxcore, System, Object) *CX_ID_6(
    cxcore, System, Reflection, TypeInfo, CreateInstance, _2)(
    const cx_type_info *__this,
    const struct CX_ID_4(cxcore, System, Collections, Array) *arguments) {
    if (__this == CX_NULL) {
        return CX_NULL;
    }
    return CX_ID_5(cxcore, System, Reflection, TypeInfo, Invoke)(
        __this, CX_NULL, __this->Name, arguments);
}

static cx_bool cx_reflection_function_name_matches(
    const struct cx_reflection_function *function,
    const struct CX_ID_3(cxcore, System, String) *name) {
    size_t functionNameLength;
    if (function == CX_NULL || function->name == CX_NULL || name == CX_NULL ||
        (name->_length != 0 && name->_data == CX_NULL)) {
        return CX_FALSE;
    }
    functionNameLength = strlen(function->name);
    return functionNameLength == name->_length &&
        (functionNameLength == 0 ||
         memcmp(function->name, name->_data, functionNameLength) == 0);
}

static cx_bool cx_reflection_arguments_match(
    const struct cx_reflection_function *function,
    const cx_ptr *arguments) {
    for (cx_uint index = 0; index < function->parameterCount; index++) {
        const struct cx_reflection_parameter *parameter = &function->parameters[index];
        if (parameter->typeInfo == CX_NULL) {
            return CX_FALSE;
        }
        if (arguments[index] == CX_NULL) {
            const cx_type_info *parameterType = (const cx_type_info *)parameter->typeInfo;
            if ((parameterType->Flags & (CX_REFLECTION_FLAG_TYPE_CLASS |
                                         CX_REFLECTION_FLAG_TYPE_INTERFACE)) == 0) {
                return CX_FALSE;
            }
            continue;
        }

        const cx_type_info *actualType;
        const cx_type_info *parameterType = (const cx_type_info *)parameter->typeInfo;
        if ((parameterType->Flags &
             (CX_REFLECTION_FLAG_TYPE_STRUCT | CX_REFLECTION_FLAG_TYPE_ENUM)) != 0) {
            actualType = cx_boxed_value_type(arguments[index]);
            if (actualType == CX_NULL || actualType->Hash !=
                    parameterType->Hash) {
                return CX_FALSE;
            }
            continue;
        }
        actualType = &CX_GET_TYPEINFO(arguments[index]);
        if (!CX_ID_5(cxcore, System, Reflection, TypeInfo, IsAssignableFrom)(
                (const cx_type_info *)parameter->typeInfo, *actualType)) {
            return CX_FALSE;
        }
    }
    return CX_TRUE;
}

struct CX_ID_3(cxcore, System, Object) *CX_ID_5(
    cxcore, System, Reflection, TypeInfo, Invoke)(
    const cx_type_info *__this,
    struct CX_ID_3(cxcore, System, Object) *target,
    const struct CX_ID_3(cxcore, System, String) *name,
    const struct CX_ID_4(cxcore, System, Collections, Array) *arguments) {
    const cx_type_info *declaringType;
    const cx_ptr *argumentData = CX_NULL;
    cx_uint argumentCount = 0;

    if (__this == CX_NULL || __this->RuntimeTypeInfo == CX_NULL || name == CX_NULL) {
        return CX_NULL;
    }

    if (arguments != CX_NULL) {
        if (arguments->_length != 0 && arguments->_data == CX_NULL) {
            return CX_NULL;
        }
        argumentCount = arguments->_length;
        argumentData = (const cx_ptr *)arguments->_data;
    }

    declaringType = __this;
    for (;;) {
        const struct cx_runtime_type_info *runtimeTypeInfo =
            (const struct cx_runtime_type_info *)declaringType->RuntimeTypeInfo;
        const struct cx_reflection_function *match = CX_NULL;
        cx_reflection_invoker invoker = CX_NULL;

        if (runtimeTypeInfo != CX_NULL) {
            if (runtimeTypeInfo->functionCount != 0 && runtimeTypeInfo->functions == CX_NULL) {
                return CX_NULL;
            }

            for (cx_uint index = 0; index < runtimeTypeInfo->functionCount; index++) {
                const struct cx_reflection_function *function =
                    &runtimeTypeInfo->functions[index];
                if (function->parameterCount != argumentCount ||
                    !cx_reflection_function_name_matches(function, name) ||
                    (function->parameters == CX_NULL && function->parameterCount != 0) ||
                    runtimeTypeInfo->invokers == CX_NULL ||
                    runtimeTypeInfo->invokers[index] == CX_NULL ||
                    !cx_reflection_arguments_match(function, argumentData)) {
                    continue;
                }

                if ((function->flags & CX_REFLECTION_FLAG_FUNCTION_CONSTRUCTOR) != 0) {
                    if (target != CX_NULL || declaringType->Hash != __this->Hash) {
                        continue;
                    }
                } else if ((function->flags & CX_REFLECTION_FLAG_STATIC) == 0) {
                    const cx_type_info *targetType = CX_NULL;
                    if (target != CX_NULL) {
                        if ((declaringType->Flags &
                             (CX_REFLECTION_FLAG_TYPE_STRUCT |
                              CX_REFLECTION_FLAG_TYPE_ENUM)) != 0) {
                            targetType = cx_boxed_value_type(target);
                        } else {
                            targetType = &CX_GET_TYPEINFO(target);
                        }
                    }
                    if (targetType == CX_NULL ||
                        !CX_ID_5(cxcore, System, Reflection, TypeInfo, IsAssignableFrom)(
                            declaringType, *targetType)) {
                        continue;
                    }
                }

                if (match != CX_NULL) {
                    return CX_NULL;
                }
                match = function;
                invoker = runtimeTypeInfo->invokers[index];
            }

            if (match != CX_NULL) {
                return invoker(target, argumentData, argumentCount);
            }
        }

        if (declaringType->BaseType._obj == CX_NULL) {
            break;
        }
        const cx_type_info *baseType =
            (const cx_type_info *)declaringType->BaseType._obj;
        if (baseType->Hash == declaringType->Hash) {
            break;
        }
        declaringType = baseType;
    }

    return CX_NULL;
}

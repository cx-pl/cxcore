#include "../../../../src/cxcore.h"
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <float.h>

typedef struct cxcore_System_Reflection_TypeInfo cx_type_info;

struct cxcore_System_Collections_Array *cxcore_System_Reflection_TypeInfo_GetInterfaces(const cx_type_info *self) {
    const struct cx_runtime_type_info *runtime = self == CX_NULL ? CX_NULL : self->RuntimeTypeInfo;
    cx_uint count = runtime == CX_NULL ? 0u : runtime->interfaceCount;
    struct cxcore_System_Collections_Array *result = cx_array_new(count, sizeof(cx_type_info));
    cx_uint index;
    for (index = 0u; index < count; ++index) {
        const cx_type_info *type = runtime->interfaces[index].typeInfo;
        if (type != CX_NULL) {
            cx_reflection_copy_type_info(&((cx_type_info *)result->_data)[index], type);
        }
    }
    return result;
}

CX_STRING_DEF(cx_reflection_invalid_invocation, "The reflected function, target, or arguments are not invocable.");
static void cx_reflection_reject_invocation(void) {
    struct cxcore_System_Exception *error = cx_exception_allocate_durable((cx_uint)sizeof(*error));
    cxcore_System_Exception___constructor(error, &cx_reflection_invalid_invocation);
    cx_exception_throw(error, __FILE__, __LINE__);
}

CX_STRING_DEF(cx_reflection_invalid_conversion, "The value cannot be converted to the reflected type.");
static void cx_reflection_reject_conversion(void) {
    struct cxcore_System_Exception *error = cx_exception_allocate_durable((cx_uint)sizeof(*error));
    cxcore_System_Exception___constructor(error, &cx_reflection_invalid_conversion);
    cx_exception_throw(error, __FILE__, __LINE__);
}

CX_STRING_DEF(cx_reflection_invalid_array, "The reflected array has an incompatible type or exceeds the requested length limit.");
static void cx_reflection_reject_array(void) {
    struct cxcore_System_Exception *error = cx_exception_allocate_durable((cx_uint)sizeof(*error));
    cxcore_System_Exception___constructor(error, &cx_reflection_invalid_array);
    cx_exception_throw(error, __FILE__, __LINE__);
}

struct cxcore_System_Collections_Array *cx_reflection_array_values(cx_ptr value, const char *typeKey,
    cx_uint elementSize, const cx_type_info *elementType, cx_uint kind, cx_uint maximumLength) {
    struct cxcore_System_Collections_Array *source = cx_safe_cast_array(value, typeKey);
    struct cxcore_System_Collections_Array *result;
    cx_uint index;
    if (source == CX_NULL || source->_length > maximumLength ||
        (source->_length != 0u && source->_data == CX_NULL) || elementType == CX_NULL ||
        (kind == 0u && elementSize != sizeof(cx_ptr)) ||
        (kind == 1u && (elementSize == 0u || elementSize != elementType->Size)) ||
        (kind == 2u && elementSize != sizeof(struct cx_iface_ref)) || kind > 2u)
        cx_reflection_reject_array();
    /* The snapshot has the same lifetime as the array whose values it copies.
       In particular, a durable source must not produce an arena-owned result
       just because reflection was called inside an arena scope. */
    result = cx_array_new_for_owner(source, source->_length,
                                    (cx_uint)sizeof(cx_ptr));
    for (index = 0u; index < source->_length; ++index) {
        const void *item = (const unsigned char *)source->_data + (size_t)index * elementSize;
        cx_ptr boxed;
        if (kind == 0u) boxed = *(cx_ptr const *)item;
        else if (kind == 1u) boxed = cx_box_value(item, elementSize, elementType);
        else boxed = cx_safe_cast_interface(*(const struct cx_iface_ref *)item, &cxcore_System_Object___typeinfo);
        cx_gc_store_reference(
            (cx_ptr *)((cx_byte *)result->_data + (size_t)index * sizeof(cx_ptr)),
            boxed);
    }
    return result;
}

struct cxcore_System_Object *cxcore_System_Reflection_TypeInfo_ConvertValue(
    const cx_type_info *self, struct cxcore_System_Object *value) {
    const cx_type_info *actual;
    cx_bool negative = CX_FALSE;
    cx_bool integral = CX_FALSE;
    cx_bool numeric = CX_FALSE;
    cx_ulong magnitude = 0u;
    cx_double floating = 0.0;
    const void *data;
    if (self == CX_NULL) cx_reflection_reject_conversion();
    if (value == CX_NULL) {
        if ((self->Flags & (CX_REFLECTION_FLAG_TYPE_CLASS | CX_REFLECTION_FLAG_TYPE_INTERFACE)) != 0u)
            return CX_NULL;
        cx_reflection_reject_conversion();
    }
    if (self->Hash == cxcore_System_Object___typeinfo.Hash) return value;
    actual = cx_boxed_value_type(value);
    if (actual == CX_NULL) actual = &CX_GET_TYPEINFO(value);
    if (actual->Hash == self->Hash) return value;
    if ((self->Flags & (CX_REFLECTION_FLAG_TYPE_CLASS | CX_REFLECTION_FLAG_TYPE_INTERFACE)) != 0u &&
        cxcore_System_Reflection_TypeInfo_IsAssignableFrom(self, *actual)) return value;
    if (cx_boxed_value_type(value) == CX_NULL) cx_reflection_reject_conversion();
    data = cx_unbox_value(value, actual);
#define CX_READ_SIGNED(name, type) \
    if (actual->Hash == cxcore_System_##name##___typeinfo.Hash) { \
        type item; cx_long widened; memcpy(&item, data, sizeof(item)); widened = (cx_long)item; \
        negative = widened < 0; magnitude = negative ? (cx_ulong)(-(widened + 1)) + 1u : (cx_ulong)widened; \
        integral = CX_TRUE; numeric = CX_TRUE; \
    }
#define CX_READ_UNSIGNED(name, type) \
    if (actual->Hash == cxcore_System_##name##___typeinfo.Hash) { \
        type item; memcpy(&item, data, sizeof(item)); magnitude = (cx_ulong)item; \
        integral = CX_TRUE; numeric = CX_TRUE; \
    }
    CX_READ_SIGNED(SByte, cx_sbyte)
    CX_READ_SIGNED(Short, cx_short)
    CX_READ_SIGNED(Int, cx_int)
    CX_READ_SIGNED(Long, cx_long)
    CX_READ_UNSIGNED(Byte, cx_byte)
    CX_READ_UNSIGNED(UShort, cx_ushort)
    CX_READ_UNSIGNED(UInt, cx_uint)
    CX_READ_UNSIGNED(ULong, cx_ulong)
#undef CX_READ_SIGNED
#undef CX_READ_UNSIGNED
    if (actual->Hash == cxcore_System_Float___typeinfo.Hash) {
        cx_float item; memcpy(&item, data, sizeof(item)); floating = item; numeric = CX_TRUE;
    }
    if (actual->Hash == cxcore_System_Double___typeinfo.Hash) {
        memcpy(&floating, data, sizeof(floating)); numeric = CX_TRUE;
    }
#define CX_CONVERT_SIGNED(name, type, maximum) \
    if (self->Hash == cxcore_System_##name##___typeinfo.Hash && integral) { \
        cx_ulong limit = (cx_ulong)(maximum); cx_long signedValue; type converted; \
        if (magnitude > limit + (negative ? 1u : 0u)) cx_reflection_reject_conversion(); \
        signedValue = negative ? (magnitude == ((cx_ulong)INT64_MAX + 1u) ? INT64_MIN : -(cx_long)magnitude) : (cx_long)magnitude; \
        converted = (type)signedValue; return cx_box_value(&converted, (cx_uint)sizeof(converted), &cxcore_System_##name##___typeinfo); \
    }
#define CX_CONVERT_UNSIGNED(name, type, maximum) \
    if (self->Hash == cxcore_System_##name##___typeinfo.Hash && integral) { \
        type converted; if (negative || magnitude > (cx_ulong)(maximum)) cx_reflection_reject_conversion(); \
        converted = (type)magnitude; return cx_box_value(&converted, (cx_uint)sizeof(converted), &cxcore_System_##name##___typeinfo); \
    }
    CX_CONVERT_SIGNED(SByte, cx_sbyte, INT8_MAX)
    CX_CONVERT_SIGNED(Short, cx_short, INT16_MAX)
    CX_CONVERT_SIGNED(Int, cx_int, INT32_MAX)
    CX_CONVERT_SIGNED(Long, cx_long, INT64_MAX)
    CX_CONVERT_UNSIGNED(Byte, cx_byte, UINT8_MAX)
    CX_CONVERT_UNSIGNED(UShort, cx_ushort, UINT16_MAX)
    CX_CONVERT_UNSIGNED(UInt, cx_uint, UINT32_MAX)
    CX_CONVERT_UNSIGNED(ULong, cx_ulong, UINT64_MAX)
#undef CX_CONVERT_SIGNED
#undef CX_CONVERT_UNSIGNED
    if (integral) floating = negative ? -(cx_double)magnitude : (cx_double)magnitude;
    if (numeric && isfinite(floating)) {
        if (self->Hash == cxcore_System_Double___typeinfo.Hash)
            return cx_box_value(&floating, (cx_uint)sizeof(floating), &cxcore_System_Double___typeinfo);
        if (self->Hash == cxcore_System_Float___typeinfo.Hash) {
            if (floating > FLT_MAX || floating < -FLT_MAX) cx_reflection_reject_conversion();
            cx_float converted = (cx_float)floating;
            if (!isfinite(converted)) cx_reflection_reject_conversion();
            return cx_box_value(&converted, (cx_uint)sizeof(converted), &cxcore_System_Float___typeinfo);
        }
    }
    cx_reflection_reject_conversion();
    return CX_NULL;
}

struct cxcore_System_Reflection_TypeInfoHolder {
    struct cxcore_System_Object __base;
    cx_type_info _value;
};

void cxcore_System_Reflection_TypeInfoHolder___constructor(
    struct cxcore_System_Reflection_TypeInfoHolder *__this,
    cx_type_info value) {
    cxcore_System_Object___constructor((struct cxcore_System_Object *)__this);
    cx_reflection_copy_type_info(&__this->_value, &value);
}

cx_type_info cxcore_System_Reflection_TypeInfoHolder_GetValue(
    const struct cxcore_System_Reflection_TypeInfoHolder *__this) {
    return __this->_value;
}

static cx_bool cx_typeinfo_string_equals(
    const struct cxcore_System_String *left,
    const struct cxcore_System_String *right) {
    if (left == right) {
        return CX_TRUE;
    }
    if (left == CX_NULL || right == CX_NULL || left->_length != right->_length) {
        return CX_FALSE;
    }
    if (left->_length == 0u) {
        return CX_TRUE;
    }
    return left->_data != CX_NULL && right->_data != CX_NULL &&
        memcmp(left->_data, right->_data, left->_length) == 0;
}

static cx_bool cx_typeinfo_same_identity(
    const cx_type_info *left, const cx_type_info *right, cx_uint depth) {
    if (left == right) {
        return CX_TRUE;
    }
    if (left == CX_NULL || right == CX_NULL || depth > 64u ||
        left->Hash != right->Hash || left->GenericArity != right->GenericArity ||
        left->RuntimeTypeInfo != right->RuntimeTypeInfo ||
        (left->RuntimeTypeInfo == CX_NULL && left != right) ||
        !cx_typeinfo_string_equals(left->Name, right->Name) ||
        !cx_typeinfo_string_equals(left->Namespace, right->Namespace)) {
        return CX_FALSE;
    }

    const struct cxcore_System_Collections_Array *leftArguments =
        (const struct cxcore_System_Collections_Array *)left->GenericParams;
    const struct cxcore_System_Collections_Array *rightArguments =
        (const struct cxcore_System_Collections_Array *)right->GenericParams;
    if (leftArguments == rightArguments) {
        return CX_TRUE;
    }
    if (leftArguments == CX_NULL || rightArguments == CX_NULL ||
        leftArguments->_length != rightArguments->_length) {
        return CX_FALSE;
    }
    const cx_type_info *leftTypes = (const cx_type_info *)leftArguments->_data;
    const cx_type_info *rightTypes = (const cx_type_info *)rightArguments->_data;
    if (leftArguments->_length != 0u &&
        (leftTypes == CX_NULL || rightTypes == CX_NULL)) {
        return CX_FALSE;
    }
    for (cx_uint index = 0u; index < leftArguments->_length; index++) {
        if (!cx_typeinfo_same_identity(&leftTypes[index], &rightTypes[index], depth + 1u)) {
            return CX_FALSE;
        }
    }
    return CX_TRUE;
}

cx_bool cxcore_System_Reflection_TypeInfo_IsSameType(
    const cx_type_info *__this, const cx_type_info other) {
    return cx_typeinfo_same_identity(__this, &other, 0u);
}

static cx_bool cx_typeinfo_c_name_equals(
    const char *left, const char *right) {
    return left != CX_NULL && right != CX_NULL && strcmp(left, right) == 0;
}

static cx_bool cx_typeinfo_string_equals_literal(
    const struct cxcore_System_String *value, const char *literal) {
    size_t length;
    if (value == CX_NULL || literal == CX_NULL ||
        (value->_length != 0u && value->_data == CX_NULL)) {
        return CX_FALSE;
    }
    length = strlen(literal);
    return value->_length == length &&
        (length == 0u || memcmp(value->_data, literal, length) == 0);
}

cx_type_info cxcore_System_Reflection_TypeInfo_GetEnumerableElementType(
    const cx_type_info *__this) {
    cx_type_info empty = { 0 };
    const struct cxcore_System_Collections_Array *genericArguments;
    const struct cx_runtime_type_info *enumerableRuntime;
    const cx_type_info *enumeratorType = CX_NULL;
    const struct cx_runtime_type_info *enumeratorRuntime;

    if (__this == CX_NULL || __this->GenericArity != 1u ||
        !cx_typeinfo_string_equals_literal(__this->Name, "IEnumerable") ||
        !cx_typeinfo_string_equals_literal(__this->Namespace, "System.Collections")) {
        return empty;
    }

    genericArguments = (const struct cxcore_System_Collections_Array *)
        __this->GenericParams;
    if (genericArguments != CX_NULL && genericArguments->_length == 1u &&
        genericArguments->_data != CX_NULL) {
        cx_type_info result = {0};
        cx_reflection_copy_type_info(
            &result, (const cx_type_info *)genericArguments->_data);
        return result;
    }

    enumerableRuntime = (const struct cx_runtime_type_info *)__this->RuntimeTypeInfo;
    if (enumerableRuntime == CX_NULL || enumerableRuntime->functions == CX_NULL) {
        return empty;
    }
    for (cx_uint index = 0u; index < enumerableRuntime->functionCount; index++) {
        const struct cx_reflection_function *function = &enumerableRuntime->functions[index];
        if (function->parameterCount == 0u &&
            cx_typeinfo_c_name_equals(function->name, "GetEnumerator") &&
            function->returnTypeInfo != CX_NULL) {
            enumeratorType = (const cx_type_info *)function->returnTypeInfo;
            break;
        }
    }
    if (enumeratorType == CX_NULL) {
        return empty;
    }

    enumeratorRuntime = (const struct cx_runtime_type_info *)enumeratorType->RuntimeTypeInfo;
    if (enumeratorRuntime == CX_NULL || enumeratorRuntime->functions == CX_NULL) {
        return empty;
    }
    for (cx_uint index = 0u; index < enumeratorRuntime->functionCount; index++) {
        const struct cx_reflection_function *function = &enumeratorRuntime->functions[index];
        if ((function->flags & CX_REFLECTION_FLAG_FUNCTION_PROPERTY_GET) != 0u &&
            cx_typeinfo_c_name_equals(function->name, "Current") &&
            function->returnTypeInfo != CX_NULL) {
            cx_type_info result = {0};
            cx_reflection_copy_type_info(
                &result, (const cx_type_info *)function->returnTypeInfo);
            return result;
        }
    }
    return empty;
}

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

cx_bool cxcore_System_Reflection_TypeInfo_HasRuntimeInterface(const cx_type_info *__this, cx_ulong typeHash) {
    return cx_typeinfo_has_runtime_interface(__this, typeHash);
}

cx_bool cxcore_System_Reflection_TypeInfo_IsInBaseTypeChain(const cx_type_info *__this, const cx_type_info source) {
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

cx_bool cxcore_System_Reflection_TypeInfo_IsAssignableFrom(const cx_type_info *__this,
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

struct cxcore_System_Object *cxcore_System_Reflection_TypeInfo_CreateInstance(const cx_type_info *__this) {
    if (__this == CX_NULL || __this->RuntimeTypeInfo == CX_NULL) {
        return CX_NULL;
    }

    const struct cx_runtime_type_info *runtimeTypeInfo =
        (const struct cx_runtime_type_info *)__this->RuntimeTypeInfo;
    return runtimeTypeInfo->createInstance != NULL
        ? runtimeTypeInfo->createInstance()
        : CX_NULL;
}

struct cxcore_System_Object *cxcore_System_Reflection_TypeInfo_CreateInstance__2(
    const cx_type_info *__this,
    const struct cxcore_System_Collections_Array *arguments) {
    if (__this == CX_NULL) {
        return CX_NULL;
    }
    return cxcore_System_Reflection_TypeInfo_Invoke(
        __this, CX_NULL, __this->Name, arguments);
}

cx_uint cxcore_System_Reflection_TypeInfo_GetFunctionCount(const cx_type_info *__this) {
    const struct cx_runtime_type_info *runtimeTypeInfo = __this == CX_NULL
        ? CX_NULL
        : (const struct cx_runtime_type_info *)__this->RuntimeTypeInfo;
    return runtimeTypeInfo == CX_NULL ? 0u : runtimeTypeInfo->functionCount;
}

cx_uint cxcore_System_Reflection_TypeInfo_GetFunctionFlags(
    const cx_type_info *__this, cx_uint functionIndex) {
    const struct cx_runtime_type_info *runtimeTypeInfo = __this == CX_NULL
        ? CX_NULL
        : (const struct cx_runtime_type_info *)__this->RuntimeTypeInfo;
    if (runtimeTypeInfo == CX_NULL || runtimeTypeInfo->functions == CX_NULL ||
        functionIndex >= runtimeTypeInfo->functionCount) {
        return 0u;
    }
    return runtimeTypeInfo->functions[functionIndex].flags;
}

cx_uint cxcore_System_Reflection_TypeInfo_GetFunctionParameterCount(
    const cx_type_info *__this, cx_uint functionIndex) {
    const struct cx_runtime_type_info *runtimeTypeInfo = __this == CX_NULL
        ? CX_NULL
        : (const struct cx_runtime_type_info *)__this->RuntimeTypeInfo;
    if (runtimeTypeInfo == CX_NULL || runtimeTypeInfo->functions == CX_NULL ||
        functionIndex >= runtimeTypeInfo->functionCount) {
        return 0u;
    }
    return runtimeTypeInfo->functions[functionIndex].parameterCount;
}

cx_type_info cxcore_System_Reflection_TypeInfo_GetFunctionParameterType(
    const cx_type_info *__this, cx_uint functionIndex, cx_uint parameterIndex) {
    cx_type_info empty = { 0 };
    cx_type_info result = { 0 };
    const struct cx_runtime_type_info *runtimeTypeInfo = __this == CX_NULL
        ? CX_NULL
        : (const struct cx_runtime_type_info *)__this->RuntimeTypeInfo;
    const struct cx_reflection_function *function;
    if (runtimeTypeInfo == CX_NULL || runtimeTypeInfo->functions == CX_NULL ||
        functionIndex >= runtimeTypeInfo->functionCount) {
        return empty;
    }
    function = &runtimeTypeInfo->functions[functionIndex];
    if (function->parameters == CX_NULL || parameterIndex >= function->parameterCount ||
        function->parameters[parameterIndex].typeInfo == CX_NULL) {
        return empty;
    }
    cx_reflection_copy_type_info(
        &result,
        (const cx_type_info *)function->parameters[parameterIndex].typeInfo);
    return result;
}

struct cxcore_System_Object *cxcore_System_Reflection_TypeInfo_InvokeConstructor(
    const cx_type_info *__this, cx_uint functionIndex,
    const struct cxcore_System_Collections_Array *arguments) {
    const struct cx_runtime_type_info *runtimeTypeInfo = __this == CX_NULL
        ? CX_NULL
        : (const struct cx_runtime_type_info *)__this->RuntimeTypeInfo;
    const struct cx_reflection_function *function;
    const cx_ptr *argumentData = CX_NULL;
    cx_uint argumentCount = arguments == CX_NULL ? 0u : arguments->_length;
    if (arguments != CX_NULL) {
        argumentData = (const cx_ptr *)arguments->_data;
    }
    if (runtimeTypeInfo == CX_NULL || runtimeTypeInfo->functions == CX_NULL ||
        runtimeTypeInfo->invokers == CX_NULL || functionIndex >= runtimeTypeInfo->functionCount ||
        runtimeTypeInfo->invokers[functionIndex] == CX_NULL) {
        return CX_NULL;
    }
    function = &runtimeTypeInfo->functions[functionIndex];
    if ((function->flags & CX_REFLECTION_FLAG_FUNCTION_CONSTRUCTOR) == 0u ||
        function->parameterCount != argumentCount ||
        (argumentCount != 0u && argumentData == CX_NULL)) {
        return CX_NULL;
    }
    return runtimeTypeInfo->invokers[functionIndex](CX_NULL, argumentData, argumentCount);
}

static cx_bool cx_reflection_function_name_matches(
    const struct cx_reflection_function *function,
    const struct cxcore_System_String *name) {
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
        if (!cxcore_System_Reflection_TypeInfo_IsAssignableFrom(
                (const cx_type_info *)parameter->typeInfo, *actualType)) {
            return CX_FALSE;
        }
    }
    return CX_TRUE;
}

static const struct cx_reflection_function *cx_typeinfo_find_function(
    const cx_type_info *typeInfo, const char *name, cx_uint parameterCount,
    cx_uint *functionIndex) {
    const struct cx_runtime_type_info *runtimeTypeInfo = typeInfo == CX_NULL
        ? CX_NULL
        : (const struct cx_runtime_type_info *)typeInfo->RuntimeTypeInfo;
    if (runtimeTypeInfo == CX_NULL || runtimeTypeInfo->functions == CX_NULL) {
        return CX_NULL;
    }
    for (cx_uint index = 0u; index < runtimeTypeInfo->functionCount; index++) {
        const struct cx_reflection_function *function = &runtimeTypeInfo->functions[index];
        if (function->parameterCount == parameterCount &&
            cx_typeinfo_c_name_equals(function->name, name)) {
            if (functionIndex != CX_NULL) {
                *functionIndex = index;
            }
            return function;
        }
    }
    return CX_NULL;
}

cx_bool cxcore_System_Reflection_TypeInfo_IsServiceSequence(
    const cx_type_info *__this, struct cxcore_System_Object *value) {
    const cx_type_info *valueType;
    (void)__this;
    if (value == CX_NULL) {
        return CX_FALSE;
    }
    valueType = &CX_GET_TYPEINFO(value);
    return cx_typeinfo_string_equals_literal(valueType->Name, "ServiceSequence") &&
        cx_typeinfo_string_equals_literal(valueType->Namespace,
            "System.DependencyInjection");
}

struct cxcore_System_Object *cxcore_System_Reflection_TypeInfo_CreateCollectionFromSequence(
    const cx_type_info *__this,
    struct cxcore_System_Object *value) {
    const struct cx_runtime_type_info *runtimeTypeInfo = __this == CX_NULL
        ? CX_NULL
        : (const struct cx_runtime_type_info *)__this->RuntimeTypeInfo;
    struct cxcore_System_Object *collection;
    const cx_type_info *sequenceType;
    const struct cx_runtime_type_info *sequenceRuntimeType;
    const struct cx_reflection_function *countFunction;
    const struct cx_reflection_function *getFunction;
    cx_uint countFunctionIndex = 0u;
    cx_uint getFunctionIndex = 0u;
    cx_ptr boxedCount;
    const cx_uint *count;
    cx_uint countValue;

    if (!cxcore_System_Reflection_TypeInfo_IsServiceSequence(__this, value) ||
        runtimeTypeInfo == CX_NULL || runtimeTypeInfo->createInstance == CX_NULL) {
        return CX_NULL;
    }
    sequenceType = &CX_GET_TYPEINFO(value);
    sequenceRuntimeType = (const struct cx_runtime_type_info *)sequenceType->RuntimeTypeInfo;
    countFunction = cx_typeinfo_find_function(sequenceType, "GetCount", 0u,
        &countFunctionIndex);
    getFunction = cx_typeinfo_find_function(sequenceType, "Get", 1u,
        &getFunctionIndex);
    if (sequenceRuntimeType == CX_NULL || sequenceRuntimeType->invokers == CX_NULL ||
        countFunction == CX_NULL || getFunction == CX_NULL ||
        sequenceRuntimeType->invokers[countFunctionIndex] == CX_NULL ||
        sequenceRuntimeType->invokers[getFunctionIndex] == CX_NULL ||
        countFunction->returnTypeInfo == CX_NULL || getFunction->returnTypeInfo == CX_NULL) {
        return CX_NULL;
    }
    boxedCount = sequenceRuntimeType->invokers[countFunctionIndex](value, CX_NULL, 0u);
    count = (const cx_uint *)cx_unbox_value(boxedCount,
        (const cx_type_info *)countFunction->returnTypeInfo);
    if (count == CX_NULL) {
        return CX_NULL;
    }
    countValue = *count;
    collection = runtimeTypeInfo->createInstance();
    if (collection == CX_NULL) {
        return CX_NULL;
    }

    for (cx_uint valueIndex = 0u; valueIndex < countValue; valueIndex++) {
        cx_ptr boxedIndex = cx_box_value(&valueIndex, (cx_uint)sizeof(valueIndex),
            &cxcore_System_UInt___typeinfo);
        cx_ptr item = sequenceRuntimeType->invokers[getFunctionIndex](
            value, &boxedIndex, 1u);
        cx_ptr argument = item;
        cx_bool added = CX_FALSE;
        if (runtimeTypeInfo->functions == CX_NULL || runtimeTypeInfo->invokers == CX_NULL) {
            return CX_NULL;
        }
        for (cx_uint functionIndex = 0u;
             functionIndex < runtimeTypeInfo->functionCount; functionIndex++) {
            const struct cx_reflection_function *function =
                &runtimeTypeInfo->functions[functionIndex];
            if (function->parameterCount != 1u || function->name == CX_NULL ||
                strcmp(function->name, "Add") != 0 ||
                (function->flags & (CX_REFLECTION_FLAG_STATIC |
                                    CX_REFLECTION_FLAG_FUNCTION_CONSTRUCTOR)) != 0u ||
                runtimeTypeInfo->invokers[functionIndex] == CX_NULL ||
                !cx_reflection_arguments_match(function, &argument)) {
                continue;
            }
            runtimeTypeInfo->invokers[functionIndex](collection, &argument, 1u);
            added = CX_TRUE;
            break;
        }
        if (!added) {
            return CX_NULL;
        }
    }
    return collection;
}

struct cxcore_System_Object *cxcore_System_Reflection_TypeInfo_Invoke(
    const cx_type_info *__this,
    struct cxcore_System_Object *target,
    const struct cxcore_System_String *name,
    const struct cxcore_System_Collections_Array *arguments) {
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
                        !cxcore_System_Reflection_TypeInfo_IsAssignableFrom(
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

struct cxcore_System_Reflection_FunctionInfo cxcore_System_Reflection_TypeInfo_GetFunction(
    const cx_type_info *self, cx_uint index) {
    const struct cx_runtime_type_info *runtime = self == CX_NULL ? CX_NULL : self->RuntimeTypeInfo;
    if (runtime == CX_NULL || runtime->functions == CX_NULL || index >= runtime->functionCount)
        cx_reflection_reject_invocation();
    return cx_reflection_get_function_info(&runtime->functions[index]);
}

cx_bool cxcore_System_Reflection_TypeInfo_CanInvokeFunction(const cx_type_info *self, cx_uint index) {
    const struct cx_runtime_type_info *runtime = self == CX_NULL ? CX_NULL : self->RuntimeTypeInfo;
    return runtime != CX_NULL && index < runtime->functionCount && runtime->invokers != CX_NULL &&
        runtime->invokers[index] != CX_NULL;
}

struct cxcore_System_Object *cxcore_System_Reflection_TypeInfo_InvokeFunction(
    const cx_type_info *self, struct cxcore_System_Object *target, cx_uint index,
    const struct cxcore_System_Collections_Array *arguments) {
    const struct cx_runtime_type_info *runtime = self == CX_NULL ? CX_NULL : self->RuntimeTypeInfo;
    const struct cx_reflection_function *function;
    cx_uint argumentCount = arguments == CX_NULL ? 0u : arguments->_length;
    const cx_ptr *argumentData = arguments == CX_NULL ? CX_NULL : arguments->_data;
    if (!cxcore_System_Reflection_TypeInfo_CanInvokeFunction(self, index) || runtime->functions == CX_NULL)
        cx_reflection_reject_invocation();
    function = &runtime->functions[index];
    if (argumentCount != function->parameterCount ||
        (argumentCount != 0u && (argumentData == CX_NULL || function->parameters == CX_NULL)) ||
        !cx_reflection_arguments_match(function, argumentData)) cx_reflection_reject_invocation();
    if ((function->flags & CX_REFLECTION_FLAG_FUNCTION_CONSTRUCTOR) != 0u) {
        if (target != CX_NULL) cx_reflection_reject_invocation();
    } else if ((function->flags & CX_REFLECTION_FLAG_STATIC) == 0u) {
        const cx_type_info *actualType = CX_NULL;
        if (target != CX_NULL) {
            actualType = (self->Flags & (CX_REFLECTION_FLAG_TYPE_STRUCT | CX_REFLECTION_FLAG_TYPE_ENUM)) != 0u
                ? cx_boxed_value_type(target) : &CX_GET_TYPEINFO(target);
        }
        if (actualType == CX_NULL || !cxcore_System_Reflection_TypeInfo_IsAssignableFrom(self, *actualType))
            cx_reflection_reject_invocation();
    }
    return runtime->invokers[index](target, argumentData, argumentCount);
}

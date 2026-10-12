#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "cxcore.h"

typedef struct CX_ID_4(cxcore, System, Reflection, TypeInfo) cx_type_info;

#define CX_BOXED_VALUE_MAGIC ((cx_ulong)0x4358424f58454431ULL)

struct cx_boxed_value {
    cx_ulong magic;
    const cx_type_info *typeInfo;
    cx_uint valueSize;
    union {
        cx_ulong unsignedValue;
        double doubleValue;
        void *pointerValue;
    } payloadAlignment;
    unsigned char payload[];
};

static struct cx_boxed_value *cx_get_boxed_value(cx_ptr instance) {
    struct CX_ID_3(cxcore, System, Object) *object =
        (struct CX_ID_3(cxcore, System, Object) *)instance;
    struct cx_boxed_value *boxed;
    if (instance == CX_NULL || CX_GET_VTABLE(instance) == CX_NULL ||
        ((union cx_vtable_entry *)CX_GET_VTABLE(instance))[0].data !=
            (cx_ptr)&CX_ID_4(cxcore, System, Object, __typeinfo)) {
        return CX_NULL;
    }
    if (object->_gc == CX_NULL) {
        return CX_NULL;
    }
    boxed = (struct cx_boxed_value *)object->_gc;
    return boxed->magic == CX_BOXED_VALUE_MAGIC ? boxed : CX_NULL;
}

cx_ptr cx_box_value(const void *value, cx_uint valueSize, const cx_type_info *valueType) {
    struct CX_ID_3(cxcore, System, Object) *object;
    struct cx_boxed_value *boxed;
    const size_t payloadOffset = offsetof(struct cx_boxed_value, payload);
    if (value == CX_NULL || valueType == CX_NULL || valueSize == 0 ||
        valueType->Size != valueSize || payloadOffset > (size_t)-1 - valueSize) {
        return CX_NULL;
    }

    object = (struct CX_ID_3(cxcore, System, Object) *)CX_ID_4(
        cxcore, System, Memory, Alloc)((cx_uint)sizeof(*object));
    if (object == CX_NULL) {
        return CX_NULL;
    }
    CX_ID_4(cxcore, System, Object, __constructor)(object);

    boxed = (struct cx_boxed_value *)CX_ID_4(cxcore, System, Memory, Alloc)(
        (cx_uint)(payloadOffset + valueSize));
    if (boxed == CX_NULL) {
        return CX_NULL;
    }
    boxed->magic = CX_BOXED_VALUE_MAGIC;
    boxed->typeInfo = valueType;
    boxed->valueSize = valueSize;
    memcpy(boxed->payload, value, valueSize);
    object->_gc = boxed;
    return object;
}

cx_ptr cx_unbox_value(cx_ptr instance, const cx_type_info *valueType) {
    struct cx_boxed_value *boxed = cx_get_boxed_value(instance);
    if (boxed == CX_NULL || valueType == CX_NULL || boxed->typeInfo->Hash != valueType->Hash ||
        boxed->valueSize != valueType->Size) {
        abort();
    }
    return boxed->payload;
}

const cx_type_info *cx_boxed_value_type(cx_ptr instance) {
    struct cx_boxed_value *boxed = cx_get_boxed_value(instance);
    return boxed == CX_NULL ? CX_NULL : boxed->typeInfo;
}

static const cx_type_info *cx_object_type(cx_ptr instance) {
    if (instance == CX_NULL || CX_GET_VTABLE(instance) == CX_NULL) {
        return CX_NULL;
    }
    return (const cx_type_info *)((union cx_vtable_entry *)CX_GET_VTABLE(instance))[0].data;
}

static const cx_type_info *cx_interface_type(struct cx_iface_ref source) {
    if (source.instance == CX_NULL || source.vtable == CX_NULL) {
        return CX_NULL;
    }
    return (const cx_type_info *)((union cx_vtable_entry *)source.vtable)[0].data;
}

static struct cx_iface_ref cx_find_interface(cx_ptr instance, const cx_type_info *actualType,
                                             const cx_type_info *targetType) {
    struct cx_iface_ref result = {CX_NULL, instance, CX_NULL};
    if (instance == CX_NULL) {
        return result;
    }

    for (const cx_type_info *current = actualType; current != CX_NULL;
         current = (const cx_type_info *)current->BaseType._obj) {
        const struct cx_runtime_type_info *runtimeTypeInfo =
            (const struct cx_runtime_type_info *)current->RuntimeTypeInfo;
        if (runtimeTypeInfo == CX_NULL) {
            continue;
        }
        for (cx_uint index = 0; index < runtimeTypeInfo->interfaceCount; index++) {
            if (runtimeTypeInfo->interfaces[index].typeInfo == (cx_ptr)targetType) {
                result.vtable = runtimeTypeInfo->interfaces[index].vtable;
                return result;
            }
        }
    }
    return result;
}

cx_bool cx_type_is(const cx_type_info *actualType, const cx_type_info *targetType) {
    if (actualType == CX_NULL || targetType == CX_NULL) {
        return CX_FALSE;
    }
    return CX_ID_5(cxcore, System, Reflection, TypeInfo, IsAssignableFrom)(targetType, *actualType);
}

cx_bool cx_is_object(cx_ptr instance, const cx_type_info *targetType) {
    return cx_type_is(cx_object_type(instance), targetType);
}

cx_bool cx_is_interface(struct cx_iface_ref source, const cx_type_info *targetType) {
    if (source.owner != CX_NULL &&
        targetType == &CX_ID_4(cxcore, System, Object, __typeinfo)) {
        return cx_is_object(source.owner, targetType);
    }
    return cx_type_is(cx_interface_type(source), targetType);
}

cx_ptr cx_checked_cast_object(cx_ptr instance, const cx_type_info *targetType) {
    if (instance == CX_NULL || cx_is_object(instance, targetType)) {
        return instance;
    }
    abort();
}

cx_ptr cx_checked_cast_interface(struct cx_iface_ref source, const cx_type_info *targetType) {
    if (source.instance == CX_NULL) {
        return CX_NULL;
    }
    if (source.owner != CX_NULL) {
        return cx_checked_cast_object(source.owner, targetType);
    }
    if (cx_is_interface(source, targetType)) {
        return source.instance;
    }
    abort();
}

struct cx_iface_ref cx_checked_cast_object_to_interface(cx_ptr instance,
                                                        const cx_type_info *targetType) {
    const cx_type_info *actualType = cx_boxed_value_type(instance);
    struct cx_iface_ref result;
    if (actualType != CX_NULL) {
        result = cx_find_interface(cx_unbox_value(instance, actualType), actualType, targetType);
    } else {
        result = cx_find_interface(instance, cx_object_type(instance), targetType);
    }
    result.owner = instance;
    if (instance == CX_NULL || result.vtable != CX_NULL) {
        return result;
    }
    abort();
}

struct cx_iface_ref cx_checked_cast_interface_to_interface(struct cx_iface_ref source,
                                                           const cx_type_info *targetType) {
    struct cx_iface_ref result =
        cx_find_interface(source.instance, cx_interface_type(source), targetType);
    result.owner = source.owner;
    if (source.instance == CX_NULL || result.vtable != CX_NULL) {
        return result;
    }
    abort();
}

const struct cx_interface_impl *cx_reflection_interfaces(const cx_type_info *typeInfo,
                                                         cx_uint *count) {
    const struct cx_runtime_type_info *runtimeTypeInfo =
        typeInfo == CX_NULL ? CX_NULL
                            : (const struct cx_runtime_type_info *)typeInfo->RuntimeTypeInfo;
    if (count != CX_NULL) {
        *count = runtimeTypeInfo == CX_NULL ? 0 : runtimeTypeInfo->interfaceCount;
    }
    return runtimeTypeInfo == CX_NULL ? CX_NULL : runtimeTypeInfo->interfaces;
}

const struct cx_reflection_field *cx_reflection_fields(const cx_type_info *typeInfo,
                                                       cx_uint *count) {
    const struct cx_runtime_type_info *runtimeTypeInfo =
        typeInfo == CX_NULL ? CX_NULL
                            : (const struct cx_runtime_type_info *)typeInfo->RuntimeTypeInfo;
    if (count != CX_NULL) {
        *count = runtimeTypeInfo == CX_NULL ? 0 : runtimeTypeInfo->fieldCount;
    }
    return runtimeTypeInfo == CX_NULL ? CX_NULL : runtimeTypeInfo->fields;
}

const struct cx_reflection_function *cx_reflection_functions(const cx_type_info *typeInfo,
                                                             cx_uint *count) {
    const struct cx_runtime_type_info *runtimeTypeInfo =
        typeInfo == CX_NULL ? CX_NULL
                            : (const struct cx_runtime_type_info *)typeInfo->RuntimeTypeInfo;
    if (count != CX_NULL) {
        *count = runtimeTypeInfo == CX_NULL ? 0 : runtimeTypeInfo->functionCount;
    }
    return runtimeTypeInfo == CX_NULL ? CX_NULL : runtimeTypeInfo->functions;
}

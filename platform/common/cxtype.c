#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "cxcore.h"
#include "cxgc.h"

typedef struct cxcore_System_Reflection_TypeInfo cx_type_info;

void cx_reflection_copy_type_info(cx_type_info *destination, const cx_type_info *source) {
    cx_type_info empty = {0};
    if (destination == CX_NULL) {
        abort();
    }
    if (source == CX_NULL) {
        source = &empty;
    }
    destination->Hash = source->Hash;
    destination->Flags = source->Flags;
    destination->Size = source->Size;
    cx_gc_store_reference((cx_ptr *)&destination->Name, (cx_ptr)source->Name);
    cx_gc_store_reference((cx_ptr *)&destination->Namespace, (cx_ptr)source->Namespace);
    cx_gc_store_reference((cx_ptr *)&destination->GenericParams, (cx_ptr)source->GenericParams);
    cx_gc_store_reference(&destination->BaseType._obj, source->BaseType._obj);
    cx_gc_store_reference((cx_ptr *)&destination->Interfaces, (cx_ptr)source->Interfaces);
    destination->GenericArity = source->GenericArity;
    cx_gc_store_reference((cx_ptr *)&destination->Fields, (cx_ptr)source->Fields);
    cx_gc_store_reference((cx_ptr *)&destination->Functions, (cx_ptr)source->Functions);
    cx_gc_store_native_pointer(&destination->RuntimeTypeInfo,
                               source->RuntimeTypeInfo);
    cx_gc_store_native_pointer(&destination->_module, source->_module);
}

static cx_type_info cx_constructed_storage_type(cx_type_info root, cx_type_info argument) {
    cx_type_info result = {0};
    cx_reflection_copy_type_info(&result, &root);
    cx_gc_store_native_pointer(&result._module,
                               (cx_ptr)__cx_module_info_CXCORE());
    result.Hash ^= ((argument.Hash << 17) | (argument.Hash >> 47)) * 0x9E3779B185EBCA87ULL;
    struct cxcore_System_Collections_Array *arguments = cx_array_new(1, sizeof(cx_type_info));
    cx_reflection_copy_type_info((cx_type_info *)arguments->_data, &argument);
    cx_gc_store_reference((cx_ptr *)&result.GenericParams,
                          (cx_ptr)arguments);
    result.GenericArity = 1;
    return result;
}

cx_type_info cx_typeof_array(cx_type_info elementType) {
    cx_type_info result = cx_constructed_storage_type(
        cxcore_System_Collections_Array___typeinfo, elementType);
    if (elementType.Name != CX_NULL) {
        cx_uint length = elementType.Name->_length;
        if (length > UINT_MAX - 2) abort();
        cx_byte *data = cxcore_System_Memory_Alloc(length + 2);
        memcpy(data, elementType.Name->_data, length);
        data[length] = '['; data[length + 1] = ']';
        struct cxcore_System_String *name = cxcore_System_Memory_Alloc(sizeof(*name));
        cxcore_System_String___constructor__2(name, length + 2, data);
        cx_gc_store_reference((cx_ptr *)&result.Name, (cx_ptr)name);
    }
    return result;
}

cx_type_info cx_typeof_nullable(cx_type_info underlyingType) {
    return cx_constructed_storage_type(cxcore_System_Nullable___typeinfo, underlyingType);
}

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

static struct cx_generic_virtual_registration *cx_generic_virtual_methods;

void cx_register_generic_virtual_method(
    struct cx_generic_virtual_registration *registration) {
    if (registration == CX_NULL || registration->receiverTypeInfo == CX_NULL ||
        registration->methodIdentity == CX_NULL ||
        registration->implementationIdentity == CX_NULL ||
        (registration->methodTypeArgumentCount != 0 &&
         registration->methodTypeArguments == CX_NULL) ||
        registration->implementation == CX_NULL) {
        abort();
    }

    cx_gc_runtime_lock();
    for (struct cx_generic_virtual_registration *current = cx_generic_virtual_methods;
         current != CX_NULL; current = current->next) {
        if (current->receiverTypeInfo == registration->receiverTypeInfo &&
            current->methodTypeArgumentCount == registration->methodTypeArgumentCount &&
            strcmp(current->methodIdentity, registration->methodIdentity) == 0) {
            cx_bool sameTypeArguments = CX_TRUE;
            for (cx_uint index = 0; index < current->methodTypeArgumentCount; index++) {
                if (current->methodTypeArguments[index] !=
                    registration->methodTypeArguments[index]) {
                    sameTypeArguments = CX_FALSE;
                    break;
                }
            }
            if (sameTypeArguments) {
                if (strcmp(current->implementationIdentity,
                           registration->implementationIdentity) != 0) {
                    abort();
                }
                cx_gc_runtime_unlock();
                return;
            }
        }
    }

    registration->next = cx_generic_virtual_methods;
    cx_generic_virtual_methods = registration;
    cx_gc_runtime_unlock();
}

cx_vtable_function cx_get_generic_virtual_method(
    cx_ptr receiverTypeInfo, const char *methodIdentity,
    const cx_ptr *methodTypeArguments, cx_uint methodTypeArgumentCount) {
    if (receiverTypeInfo == CX_NULL || methodIdentity == CX_NULL ||
        (methodTypeArgumentCount != 0 && methodTypeArguments == CX_NULL)) {
        return CX_NULL;
    }

    cx_gc_runtime_lock();
    for (struct cx_generic_virtual_registration *current = cx_generic_virtual_methods;
         current != CX_NULL; current = current->next) {
        if (current->receiverTypeInfo != receiverTypeInfo ||
            current->methodTypeArgumentCount != methodTypeArgumentCount ||
            strcmp(current->methodIdentity, methodIdentity) != 0) {
            continue;
        }
        cx_bool sameTypeArguments = CX_TRUE;
        for (cx_uint index = 0; index < methodTypeArgumentCount; index++) {
            if (current->methodTypeArguments[index] != methodTypeArguments[index]) {
                sameTypeArguments = CX_FALSE;
                break;
            }
        }
        if (sameTypeArguments) {
            cx_vtable_function result = current->implementation;
            cx_gc_runtime_unlock();
            return result;
        }
    }

    cx_gc_runtime_unlock();
    return CX_NULL;
}

cx_vtable_function cx_require_generic_virtual_method(
    cx_ptr receiverTypeInfo, const char *methodIdentity,
    const cx_ptr *methodTypeArguments, cx_uint methodTypeArgumentCount) {
    cx_vtable_function implementation = cx_get_generic_virtual_method(
        receiverTypeInfo, methodIdentity, methodTypeArguments,
        methodTypeArgumentCount);
    if (implementation == CX_NULL) {
        abort();
    }
    return implementation;
}

static struct cx_boxed_value *cx_get_boxed_value(cx_ptr instance) {
    struct cxcore_System_Object *object =
        (struct cxcore_System_Object *)instance;
    struct cx_boxed_value *boxed;
    if (instance == CX_NULL || CX_GET_VTABLE(instance) == CX_NULL ||
        ((union cx_vtable_entry *)CX_GET_VTABLE(instance))[0].data !=
            (cx_ptr)&cxcore_System_Object___typeinfo) {
        return CX_NULL;
    }
    if (object->_box == CX_NULL) {
        return CX_NULL;
    }
    boxed = (struct cx_boxed_value *)object->_box;
    return boxed->magic == CX_BOXED_VALUE_MAGIC ? boxed : CX_NULL;
}

static void cx_gc_copy_function_value(void *destination, const void *source,
                                      size_t size) {
    if (size != 5u * sizeof(cx_ptr)) abort();
    memcpy(destination, source, sizeof(cx_ptr));
    cx_gc_store_reference((cx_ptr *)((unsigned char *)destination + sizeof(cx_ptr)),
        *(cx_ptr const *)((const unsigned char *)source + sizeof(cx_ptr)));
    cx_gc_store_reference((cx_ptr *)((unsigned char *)destination + 2u * sizeof(cx_ptr)),
        *(cx_ptr const *)((const unsigned char *)source + 2u * sizeof(cx_ptr)));
    memcpy((unsigned char *)destination + 3u * sizeof(cx_ptr),
        (const unsigned char *)source + 3u * sizeof(cx_ptr), sizeof(cx_ptr));
    cx_gc_store_reference((cx_ptr *)((unsigned char *)destination + 4u * sizeof(cx_ptr)),
        *(cx_ptr const *)((const unsigned char *)source + 4u * sizeof(cx_ptr)));
}

static int cx_gc_value_fields_are_complete(
    const cx_type_info *typeInfo, const struct cx_reflection_field *fields,
    cx_uint fieldCount, unsigned int depth);

static int cx_gc_value_layout_is_complete(
    const cx_type_info *typeInfo, unsigned int depth) {
    const struct cx_reflection_field *fields;
    cx_uint fieldCount = 0u;

    if (typeInfo == CX_NULL || depth > 64u) return 0;
    if ((typeInfo->Flags & CX_REFLECTION_FLAG_TYPE_ENUM) != 0u ||
        typeInfo->Hash == cxcore_System_Nullable___typeinfo.Hash) {
        return 1;
    }
    if ((typeInfo->Flags & CX_REFLECTION_FLAG_TYPE_DELEGATE) != 0u) {
        fields = cx_reflection_fields(typeInfo, &fieldCount);
        return fields == CX_NULL || fieldCount == 0u
            ? typeInfo->Size == 5u * sizeof(cx_ptr)
            : cx_gc_value_fields_are_complete(typeInfo, fields, fieldCount, depth);
    }
    if ((typeInfo->Flags & CX_REFLECTION_FLAG_TYPE_STRUCT) == 0u) return 0;
    fields = cx_reflection_fields(typeInfo, &fieldCount);
    return fields != CX_NULL && fieldCount != 0u &&
        cx_gc_value_fields_are_complete(typeInfo, fields, fieldCount, depth);
}

static int cx_gc_value_fields_are_complete(
    const cx_type_info *typeInfo, const struct cx_reflection_field *fields,
    cx_uint fieldCount, unsigned int depth) {
    cx_uint fieldIndex;
    for (fieldIndex = 0u; fieldIndex < fieldCount; ++fieldIndex) {
        const struct cx_reflection_field *field = &fields[fieldIndex];
        const cx_type_info *fieldType;
        if ((field->flags & CX_REFLECTION_FLAG_STATIC) != 0u) continue;
        if (field->offset == CX_REFLECTION_NO_OFFSET ||
            field->offset > typeInfo->Size || field->size == 0u ||
            field->size > typeInfo->Size - field->offset) {
            return 0;
        }
        if ((field->flags & CX_REFLECTION_FLAG_FIELD_NATIVE_LAYOUT) != 0u) {
            const struct cx_native_value_layout *layout =
                (const struct cx_native_value_layout *)field->typeInfo;
            if (layout == CX_NULL || !layout->complete ||
                layout->size != field->size ||
                (layout->pointerCount != 0u && layout->pointerOffsets == CX_NULL)) {
                return 0;
            }
            continue;
        }
        fieldType = (const cx_type_info *)field->typeInfo;
        /* A native function pointer has TypeInfo for reflection, but its
         * one-pointer representation is unmanaged code data rather than a CX
         * closure. Other typed raw-byte fields retain their reflected walk. */
        if ((field->flags & CX_REFLECTION_FLAG_FIELD_RAW_BYTES) != 0u) {
            if (fieldType == CX_NULL) continue;
            if ((fieldType->Flags & CX_REFLECTION_FLAG_TYPE_DELEGATE) != 0u &&
                fieldType->Size != 5u * sizeof(cx_ptr)) {
                if (fieldType->Size != field->size) return 0;
                continue;
            }
        }
        if (fieldType == CX_NULL) {
            if ((field->flags & CX_REFLECTION_FLAG_FIELD_POINTER) != 0u) {
                if (field->size != sizeof(cx_ptr)) return 0;
                continue;
            }
            if ((field->flags & CX_REFLECTION_FLAG_FIELD_FUNCTION_VALUE) != 0u) {
                if (field->size != 5u * sizeof(cx_ptr)) return 0;
                continue;
            }
            return 0;
        }
        /* Core scalar wrappers expose their primitive storage as a field of
         * the same reflected type. The recursive copier already recognizes
         * this representation and copies those bytes directly. */
        if (fieldType->Hash == typeInfo->Hash &&
            field->size == typeInfo->Size) {
            continue;
        }
        if ((fieldType->Flags & (CX_REFLECTION_FLAG_TYPE_STRUCT |
                                 CX_REFLECTION_FLAG_TYPE_DELEGATE)) != 0u &&
            !cx_gc_value_layout_is_complete(fieldType, depth + 1u)) {
            return 0;
        }
    }
    return 1;
}

static void cx_gc_copy_value_recursive(
    void *destination, const void *source, const cx_type_info *typeInfo,
    unsigned int depth) {
    const struct cx_reflection_field *fields;
    cx_uint fieldCount = 0u;
    cx_uint fieldIndex;
    if (destination == CX_NULL || source == CX_NULL || typeInfo == CX_NULL ||
        depth > 64u) {
        abort();
    }
    if ((typeInfo->Flags & CX_REFLECTION_FLAG_TYPE_ENUM) != 0u) {
        memcpy(destination, source, typeInfo->Size);
        return;
    }
    if (typeInfo->Hash == cxcore_System_Nullable___typeinfo.Hash) {
        if (typeInfo->Size != sizeof(struct cxcore_System_Nullable)) abort();
        cx_gc_store_reference(
            &((struct cxcore_System_Nullable *)destination)->_obj,
            ((const struct cxcore_System_Nullable *)source)->_obj);
        return;
    }
    if ((typeInfo->Flags & (CX_REFLECTION_FLAG_TYPE_STRUCT |
                            CX_REFLECTION_FLAG_TYPE_DELEGATE)) == 0u) {
        abort();
    }
    fields = cx_reflection_fields(typeInfo, &fieldCount);
    if ((typeInfo->Flags & CX_REFLECTION_FLAG_TYPE_DELEGATE) != 0u &&
        (fields == CX_NULL || fieldCount == 0u)) {
        cx_gc_copy_function_value(destination, source, typeInfo->Size);
        return;
    }
    if (fields == CX_NULL || fieldCount == 0u) {
        abort();
    }
    for (fieldIndex = 0u; fieldIndex < fieldCount; ++fieldIndex) {
        const struct cx_reflection_field *field = &fields[fieldIndex];
        const cx_type_info *fieldType = (const cx_type_info *)field->typeInfo;
        size_t fieldOffset = field->offset;
        cx_uint fieldSize;
        unsigned int fieldKind;
        if ((field->flags & CX_REFLECTION_FLAG_STATIC) != 0u) continue;
        if (fieldOffset == CX_REFLECTION_NO_OFFSET) {
            abort();
        }
        if ((field->flags & CX_REFLECTION_FLAG_FIELD_NATIVE_LAYOUT) != 0u) {
            const struct cx_native_value_layout *nativeLayout =
                (const struct cx_native_value_layout *)field->typeInfo;
            if (nativeLayout == CX_NULL || nativeLayout->size != field->size ||
                fieldOffset > typeInfo->Size ||
                field->size > typeInfo->Size - fieldOffset) {
                abort();
            }
            if (nativeLayout->complete) {
                (void)cx_gc_copy_native_value(
                    (unsigned char *)destination + fieldOffset,
                    (const unsigned char *)source + fieldOffset,
                    nativeLayout->size,
                    nativeLayout->pointerOffsets,
                    nativeLayout->pointerCount);
            } else {
                (void)cx_gc_copy_native_value_unknown_layout(
                    (unsigned char *)destination + fieldOffset,
                    (const unsigned char *)source + fieldOffset,
                    nativeLayout->size);
            }
            continue;
        }
        if ((field->flags & CX_REFLECTION_FLAG_FIELD_RAW_BYTES) != 0u &&
            fieldType == CX_NULL) {
            if (fieldOffset > typeInfo->Size || field->size == 0u ||
                field->size > typeInfo->Size - fieldOffset) {
                abort();
            }
            memcpy((unsigned char *)destination + fieldOffset,
                   (const unsigned char *)source + fieldOffset, field->size);
            continue;
        }
        if ((field->flags & CX_REFLECTION_FLAG_FIELD_RAW_BYTES) != 0u &&
            (fieldType->Flags & CX_REFLECTION_FLAG_TYPE_DELEGATE) != 0u &&
            fieldType->Size != 5u * sizeof(cx_ptr)) {
            if (fieldOffset > typeInfo->Size || field->size == 0u ||
                field->size > typeInfo->Size - fieldOffset ||
                fieldType->Size != field->size) {
                abort();
            }
            memcpy((unsigned char *)destination + fieldOffset,
                   (const unsigned char *)source + fieldOffset, field->size);
            continue;
        }
        if (fieldType == CX_NULL) {
            if (fieldOffset > typeInfo->Size || field->size == 0u ||
                field->size > typeInfo->Size - fieldOffset) {
                abort();
            }
            if ((field->flags & CX_REFLECTION_FLAG_FIELD_POINTER) != 0u) {
                if (field->size != sizeof(cx_ptr)) abort();
                cx_gc_store_reference(
                    (cx_ptr *)((unsigned char *)destination + fieldOffset),
                    *(cx_ptr const *)((const unsigned char *)source + fieldOffset));
                continue;
            }
            if ((field->flags & CX_REFLECTION_FLAG_FIELD_FUNCTION_VALUE) != 0u) {
                cx_gc_copy_function_value(
                    (unsigned char *)destination + fieldOffset,
                    (const unsigned char *)source + fieldOffset,
                    field->size);
                continue;
            }
            abort();
        }
        fieldKind = fieldType->Flags;
        if ((fieldKind & CX_REFLECTION_FLAG_TYPE_CLASS) != 0u) {
            if (fieldOffset > typeInfo->Size || sizeof(cx_ptr) > typeInfo->Size - fieldOffset) {
                abort();
            }
            cx_gc_store_reference(
                (cx_ptr *)((unsigned char *)destination + fieldOffset),
                *(cx_ptr const *)((const unsigned char *)source + fieldOffset));
            continue;
        }
        if ((fieldKind & CX_REFLECTION_FLAG_TYPE_INTERFACE) != 0u) {
            if (fieldOffset > typeInfo->Size ||
                sizeof(struct cx_iface_ref) > typeInfo->Size - fieldOffset) {
                abort();
            }
            (void)cx_gc_store_interface_reference(
                (struct cx_iface_ref *)((unsigned char *)destination + fieldOffset),
                *(const struct cx_iface_ref *)((const unsigned char *)source + fieldOffset));
            continue;
        }
        fieldSize = (fieldKind & CX_REFLECTION_FLAG_TYPE_CLASS) != 0u
            ? sizeof(cx_ptr)
            : (fieldKind & CX_REFLECTION_FLAG_TYPE_INTERFACE) != 0u
                ? sizeof(struct cx_iface_ref)
                : fieldType->Size;
        if (fieldOffset > typeInfo->Size || fieldSize > typeInfo->Size - fieldOffset) {
            abort();
        }
        if (field->size != fieldSize) abort();
        if (fieldType->Hash == typeInfo->Hash && fieldSize == typeInfo->Size) {
            memcpy((unsigned char *)destination + fieldOffset,
                   (const unsigned char *)source + fieldOffset, fieldSize);
        } else if ((fieldKind & (CX_REFLECTION_FLAG_TYPE_STRUCT |
                                 CX_REFLECTION_FLAG_TYPE_DELEGATE)) != 0u) {
            cx_gc_copy_value_recursive(
                (unsigned char *)destination + fieldOffset,
                (const unsigned char *)source + fieldOffset,
                fieldType, depth + 1u);
        } else {
            memcpy((unsigned char *)destination + fieldOffset,
                   (const unsigned char *)source + fieldOffset, fieldSize);
        }
    }
}

cx_ptr cx_gc_copy_value(void *destination, const void *source,
                        const cx_type_info *typeInfo) {
    if (destination == CX_NULL || source == CX_NULL || typeInfo == CX_NULL) {
        abort();
    }
    if (!cx_gc_value_layout_is_complete(typeInfo, 0u)) {
        if ((typeInfo->Flags & (CX_REFLECTION_FLAG_TYPE_STRUCT |
                                CX_REFLECTION_FLAG_TYPE_DELEGATE)) == 0u) {
            abort();
        }
        return cx_gc_copy_native_value_unknown_layout(
            destination, source, typeInfo->Size);
    }
    cx_gc_copy_value_recursive(destination, source, typeInfo, 0u);
    return destination;
}

cx_ptr cx_box_value(const void *value, cx_uint valueSize, const cx_type_info *valueType) {
    struct cxcore_System_Object *object;
    struct cx_boxed_value *boxed;
    const size_t payloadOffset = offsetof(struct cx_boxed_value, payload);
    if (value == CX_NULL || valueType == CX_NULL || valueSize == 0 ||
        valueType->Size != valueSize || (size_t)valueSize > SIZE_MAX - payloadOffset) {
        return CX_NULL;
    }

    object = (struct cxcore_System_Object *)cxcore_System_Memory_Alloc((cx_uint)sizeof(*object));
    if (object == CX_NULL) {
        return CX_NULL;
    }
    cxcore_System_Object___constructor(object);

    boxed = (struct cx_boxed_value *)cxcore_System_Memory_Alloc(
        (cx_uint)(payloadOffset + valueSize));
    if (boxed == CX_NULL) {
        return CX_NULL;
    }
    boxed->magic = CX_BOXED_VALUE_MAGIC;
    (void)cx_gc_store_reference(
        (cx_ptr *)&boxed->typeInfo, (cx_ptr)valueType);
    boxed->valueSize = valueSize;
    cx_gc_copy_value(boxed->payload, value, valueType);
    cx_gc_store_reference(&object->_box, boxed);
    return object;
}

cx_ptr cx_box_generic_value(const void *value, cx_uint valueSize,
                            const cx_type_info *valueType) {
    if (value == CX_NULL) {
        return CX_NULL;
    }
    if (valueType == CX_NULL ||
        (valueType->Flags & CX_REFLECTION_FLAG_TYPE_CLASS) != 0) {
        return *(const cx_ptr *)value;
    }
    return cx_box_value(value, valueSize, valueType);
}

cx_ptr cx_box_nullable_value(const void *value, cx_uint valueSize, const cx_type_info *valueType) {
    return value == CX_NULL ? CX_NULL : cx_box_value(value, valueSize, valueType);
}

cx_ptr cx_unbox_value(cx_ptr instance, const cx_type_info *valueType) {
    struct cx_boxed_value *boxed = cx_get_boxed_value(instance);
    if (boxed == CX_NULL || valueType == CX_NULL || boxed->typeInfo->Hash != valueType->Hash ||
        boxed->valueSize != valueType->Size) {
        abort();
    }
    return boxed->payload;
}

cx_ptr cx_unbox_generic_value(cx_ptr instance, const cx_type_info *valueType) {
    if (valueType == CX_NULL ||
        (valueType->Flags & CX_REFLECTION_FLAG_TYPE_CLASS) != 0) {
        return instance;
    }
    return cx_unbox_value(instance, valueType);
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
    return cxcore_System_Reflection_TypeInfo_IsAssignableFrom(targetType, *actualType);
}

cx_bool cx_is_object(cx_ptr instance, const cx_type_info *targetType) {
    return cx_type_is(cx_object_type(instance), targetType);
}

cx_bool cx_is_interface(struct cx_iface_ref source, const cx_type_info *targetType) {
    if (source.owner != CX_NULL &&
        targetType == &cxcore_System_Object___typeinfo) {
        return cx_is_object(source.owner, targetType);
    }
    return cx_type_is(cx_interface_type(source), targetType);
}

static CX_NORETURN void cx_throw_invalid_cast(void) {
    cx_ptr exception = cx_exception_allocate_durable(
        cxcore_System_InvalidCastException___typeinfo.Size);
    cxcore_System_InvalidCastException___constructor(
        (struct cxcore_System_InvalidCastException *)exception);
    cx_exception_throw(exception, __FILE__, __LINE__);
}

cx_ptr cx_checked_cast_object(cx_ptr instance, const cx_type_info *targetType) {
    if (instance == CX_NULL || cx_is_object(instance, targetType)) {
        return instance;
    }
    cx_throw_invalid_cast();
}

cx_ptr cx_safe_cast_object(cx_ptr instance, const cx_type_info *targetType) {
    return cx_is_object(instance, targetType) ? instance : CX_NULL;
}

struct cxcore_System_Collections_Array *cx_array_with_type(
    struct cxcore_System_Collections_Array *array, const char *typeKey) {
    cx_gc_store_reference(&array->__base._box, (cx_ptr)typeKey);
    return array;
}

cx_ptr cx_safe_cast_array(cx_ptr instance, const char *typeKey) {
    if (!cx_is_object(instance, &cxcore_System_Collections_Array___typeinfo)) return CX_NULL;
    struct cxcore_System_Collections_Array *array = instance;
    const char *actualKey = (const char *)array->__base._box;
    return actualKey != CX_NULL && typeKey != CX_NULL && strcmp(actualKey, typeKey) == 0 ? instance : CX_NULL;
}

cx_ptr cx_safe_cast_interface_to_array(struct cx_iface_ref source, const char *typeKey) {
    return cx_safe_cast_array(source.owner != CX_NULL ? source.owner : source.instance, typeKey);
}

cx_ptr cx_safe_cast_interface(struct cx_iface_ref source, const cx_type_info *targetType) {
    if (source.owner != CX_NULL) return cx_safe_cast_object(source.owner, targetType);
    return cx_is_interface(source, targetType) ? source.instance : CX_NULL;
}

struct cx_iface_ref cx_safe_cast_object_to_interface(cx_ptr instance, const cx_type_info *targetType) {
    const cx_type_info *actualType = cx_boxed_value_type(instance);
    struct cx_iface_ref result = actualType == CX_NULL
        ? cx_find_interface(instance, cx_object_type(instance), targetType)
        : cx_find_interface(cx_unbox_value(instance, actualType), actualType, targetType);
    if (result.vtable == CX_NULL) return (struct cx_iface_ref){0};
    result.owner = instance;
    return result;
}

struct cx_iface_ref cx_safe_cast_interface_to_interface(struct cx_iface_ref source, const cx_type_info *targetType) {
    struct cx_iface_ref result = cx_find_interface(source.instance, cx_interface_type(source), targetType);
    if (result.vtable == CX_NULL) return (struct cx_iface_ref){0};
    result.owner = source.owner;
    return result;
}

cx_ptr cx_safe_unbox_nullable(cx_ptr instance, const cx_type_info *valueType) {
    struct cx_boxed_value *boxed = cx_get_boxed_value(instance);
    if (boxed == CX_NULL || valueType == CX_NULL || boxed->typeInfo->Hash != valueType->Hash ||
        boxed->valueSize != valueType->Size) return CX_NULL;
    return cxcore_System_Nullable_CreateValueStorage(
        cx_unbox_value(instance, valueType), valueType->Size);
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
    cx_throw_invalid_cast();
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
    cx_throw_invalid_cast();
}

struct cx_iface_ref cx_checked_cast_interface_to_interface(struct cx_iface_ref source,
                                                           const cx_type_info *targetType) {
    const cx_type_info *sourceType = cx_interface_type(source);
    if (sourceType == targetType) {
        return source;
    }
    const cx_type_info *actualType = source.owner != CX_NULL
        ? cx_object_type(source.owner)
        : sourceType;
    struct cx_iface_ref result = cx_find_interface(source.instance, actualType, targetType);
    result.owner = source.owner;
    if (source.instance == CX_NULL || result.vtable != CX_NULL) {
        return result;
    }
    cx_throw_invalid_cast();
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

const struct cx_reflection_function *cx_reflection_find_function(
    const cx_type_info *typeInfo, const char *name, cx_uint parameterCount,
    const cx_type_info *const *parameterTypes) {
    const struct cx_reflection_function *functions;
    cx_uint functionCount;
    cx_uint functionIndex;

    if (typeInfo == CX_NULL || name == CX_NULL) {
        return CX_NULL;
    }
    functions = cx_reflection_functions(typeInfo, &functionCount);
    for (functionIndex = 0; functionIndex < functionCount; ++functionIndex) {
        const struct cx_reflection_function *function = &functions[functionIndex];
        cx_uint parameterIndex;
        if (function->name == CX_NULL || strcmp(function->name, name) != 0 ||
            function->parameterCount != parameterCount) {
            continue;
        }
        for (parameterIndex = 0; parameterIndex < parameterCount; ++parameterIndex) {
            const cx_type_info *expected = parameterTypes == CX_NULL
                                               ? CX_NULL
                                               : parameterTypes[parameterIndex];
            const cx_type_info *actual =
                (const cx_type_info *)function->parameters[parameterIndex].typeInfo;
            if ((expected == CX_NULL) != (actual == CX_NULL) ||
                (expected != CX_NULL && expected->Hash != actual->Hash)) {
                break;
            }
        }
        if (parameterIndex == parameterCount) {
            return function;
        }
    }
    return CX_NULL;
}

static struct cxcore_System_String *cx_reflection_copy_name(const char *name) {
    struct cxcore_System_String *result;
    size_t byteLength;
    cx_ptr data;

    if (name == CX_NULL) {
        return CX_NULL;
    }
    byteLength = strlen(name);
    if (byteLength > UINT_MAX) {
        abort();
    }
    result = (struct cxcore_System_String *)
        cxcore_System_Memory_Alloc((cx_uint)sizeof(*result));
    data = byteLength == 0
               ? CX_NULL
               : cxcore_System_Memory_Alloc((cx_uint)byteLength);
    if (byteLength != 0) {
        memcpy(data, name, byteLength);
    }
    cxcore_System_String___constructor__2(
        result, (cx_uint)byteLength, data);
    return result;
}

struct cxcore_System_Reflection_FunctionInfo
cx_reflection_get_function_info(const struct cx_reflection_function *function) {
    struct cxcore_System_Reflection_TypeInfo returnType =
        {0};
    struct cxcore_System_String *name;
    struct cxcore_System_Collections_Array *parameters = CX_NULL;
    cx_uint parameterIndex;

    if (function == CX_NULL) {
        return (struct cxcore_System_Reflection_FunctionInfo){0};
    }
    if (function->returnTypeInfo != CX_NULL) {
        cx_reflection_copy_type_info(&returnType, function->returnTypeInfo);
    }
    name = cx_reflection_copy_name(function->name);
    if (function->parameterCount != 0) {
        parameters = cx_array_new(
            function->parameterCount,
            (cx_uint)sizeof(struct cxcore_System_Reflection_FunctionParamInfo));
    }
    for (parameterIndex = 0; parameterIndex < function->parameterCount; ++parameterIndex) {
        const struct cx_reflection_parameter *source = &function->parameters[parameterIndex];
        struct cxcore_System_Reflection_FunctionParamInfo *destination =
            (struct cxcore_System_Reflection_FunctionParamInfo *)cx_array_at(
                parameters, parameterIndex,
                (cx_uint)sizeof(struct cxcore_System_Reflection_FunctionParamInfo));
        destination->Flags = source->flags;
        cx_reflection_copy_type_info(
            (cx_type_info *)&destination->Type,
            (const cx_type_info *)source->typeInfo);
        cx_gc_store_reference((cx_ptr *)&destination->Name,
                              (cx_ptr)cx_reflection_copy_name(source->name));
        cx_gc_store_reference((cx_ptr *)&destination->DefaultValue,
                              source->defaultValue);
    }
    struct cxcore_System_Reflection_FunctionInfo result = {
        .Flags = function->flags,
        .Offset = function->slot,
        .ReturnType = returnType,
    };
    cx_reflection_copy_type_info(
        (cx_type_info *)(void *)&result.ReturnType, &returnType);
    cx_gc_store_reference((cx_ptr *)&result.Name, (cx_ptr)name);
    cx_gc_store_reference((cx_ptr *)&result.Params, (cx_ptr)parameters);
    return result;
}

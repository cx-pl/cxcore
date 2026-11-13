#include "../../../../src/cxcore.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#if !defined(CX_USE_GENERATED_CORE)
CX_STRING_DEF(cx_annotation_info_name, "AnnotationInfo");
CX_STRING_DEF(cx_annotation_info_namespace, "System.Reflection");
CX_BEGIN_VTABLE_DEF(cxcore_System_Reflection_AnnotationInfo)
CX_END_VTABLE_DEF;
#define CX_ANNOTATION_FIELD(member, type) \
    { CX_REFLECTION_FLAG_VISIBILITY_PUBLIC, \
      (cx_uint)offsetof(struct cxcore_System_Reflection_AnnotationInfo, member), \
      &cxcore_System_##type##___typeinfo, #member, \
      (cx_uint)sizeof(((struct cxcore_System_Reflection_AnnotationInfo *)0)->member) }
static const struct cx_reflection_field cx_annotation_info_fields[] = {
    CX_ANNOTATION_FIELD(DeclaringType, Reflection_TypeInfo),
    CX_ANNOTATION_FIELD(FunctionIndex, Int),
    CX_ANNOTATION_FIELD(ParameterIndex, Int),
    CX_ANNOTATION_FIELD(Target, String),
    CX_ANNOTATION_FIELD(MemberName, String),
    CX_ANNOTATION_FIELD(TypeName, String),
    CX_ANNOTATION_FIELD(ArgumentNames, Collections_Array),
    CX_ANNOTATION_FIELD(ArgumentExpressions, Collections_Array),
    CX_ANNOTATION_FIELD(ArgumentValues, Collections_Array),
    CX_ANNOTATION_FIELD(SourceFile, String),
    CX_ANNOTATION_FIELD(SourceLine, UInt),
    CX_ANNOTATION_FIELD(SourceColumn, UInt),
};
#undef CX_ANNOTATION_FIELD
static cx_ptr cx_annotation_info_create(void) {
    struct cxcore_System_Reflection_AnnotationInfo value = {0};
    return cx_box_value(&value, (cx_uint)sizeof(value),
        &cxcore_System_Reflection_AnnotationInfo___typeinfo);
}
static const struct cx_runtime_type_info cx_annotation_info_runtime = {
    .fields = cx_annotation_info_fields,
    .fieldCount = 12u,
    .createInstance = cx_annotation_info_create,
};
struct cxcore_System_Reflection_TypeInfo cxcore_System_Reflection_AnnotationInfo___typeinfo = {
    .Hash = 0x51420ABD06E106E4ULL,
    ._module = (cx_ptr)&CXCORE___module_info,
    .Flags = CX_REFLECTION_FLAG_VISIBILITY_PUBLIC | CX_REFLECTION_FLAG_TYPE_STRUCT,
    .Size = sizeof(struct cxcore_System_Reflection_AnnotationInfo),
    .Name = &cx_annotation_info_name,
    .Namespace = &cx_annotation_info_namespace,
    .RuntimeTypeInfo = (cx_ptr)&cx_annotation_info_runtime,
};
#endif

static struct cxcore_System_String *cx_module_string(const char *text) {
    struct cxcore_System_String *result;
    cx_uint length = text == CX_NULL ? 0u : (cx_uint)strlen(text);
    cx_ptr data;
    result = cxcore_System_Memory_Alloc((cx_uint)sizeof(*result));
    data = length == 0u ? CX_NULL : cxcore_System_Memory_Alloc(length);
    if (length != 0u) memcpy(data, text, length);
    cxcore_System_String___constructor__2(result, length, data);
    return result;
}

cx_ptr cx_annotation_string_value(const unsigned char *bytes, cx_uint byteLength) {
    struct cxcore_System_String *result = cxcore_System_Memory_Alloc((cx_uint)sizeof(*result));
    cx_ptr data = byteLength == 0u ? CX_NULL : cxcore_System_Memory_Alloc(byteLength);
    if (byteLength != 0u) memcpy(data, bytes, byteLength);
    cxcore_System_String___constructor__2(result, byteLength, data);
    return result;
}

static struct cxcore_System_Reflection_VersionInfo cx_module_version(const char *text) {
    cx_int parts[4] = {0};
    cx_uint index;
    const char *cursor = text;
    for (index = 0u; index < 4u && cursor != CX_NULL; ++index) {
        unsigned long number;
        char *end;
        if (*cursor < '0' || *cursor > '9') break;
        number = strtoul(cursor, &end, 10);
        if (number > INT_MAX) break;
        parts[index] = (cx_int)number;
        if (*end != '.') break;
        cursor = end + 1;
    }
    return (struct cxcore_System_Reflection_VersionInfo){parts[0], parts[1], parts[2], parts[3]};
}

static cx_bool cx_module_descriptor_supported(const struct cx_module_info *descriptor) {
    return descriptor != CX_NULL && descriptor->formatVersion == CX_MODULE_METADATA_FORMAT_VERSION;
}

struct cxcore_System_Reflection_ModuleInfo cx_module_info_wrap(const struct cx_module_info *descriptor) {
    struct cxcore_System_Reflection_ModuleInfo result = {
        .Version = cx_module_version(cx_module_descriptor_supported(descriptor)
            ? descriptor->version : "")
    };
    struct cxcore_System_Collections_Array *types;
    cx_uint index;
    if (!cx_module_descriptor_supported(descriptor)) return result;
    cx_gc_store_native_pointer((cx_ptr *)&result._descriptor,
                               (cx_ptr)descriptor);
    cx_gc_store_reference((cx_ptr *)&result.Name,
                          (cx_ptr)cx_module_string(descriptor->name));
    cx_gc_store_reference((cx_ptr *)&result.Description,
                          (cx_ptr)cx_module_string(descriptor->description));
    cx_gc_store_reference((cx_ptr *)&result.Author,
                          (cx_ptr)cx_module_string(descriptor->author));
    types = cx_array_new(descriptor->typeCount,
        (cx_uint)sizeof(struct cxcore_System_Reflection_TypeInfo));
    cx_gc_store_reference((cx_ptr *)&result.Types, (cx_ptr)types);
    for (index = 0; index < descriptor->typeCount; ++index) {
        cx_reflection_copy_type_info(
            (struct cxcore_System_Reflection_TypeInfo *)cx_array_at(
                types, index, (cx_uint)sizeof(struct cxcore_System_Reflection_TypeInfo)),
            descriptor->types[index]);
    }
    return result;
}

CX_STRING_DEF(cx_type_module_unavailable, "The type has no defining module metadata.");
struct cxcore_System_Reflection_ModuleInfo cxcore_System_Reflection_TypeInfo_GetModule(
    const struct cxcore_System_Reflection_TypeInfo *self) {
    if (self == CX_NULL || self->_module == CX_NULL) {
        struct cxcore_System_Exception *error = cx_exception_allocate_durable((cx_uint)sizeof(*error));
        cxcore_System_Exception___constructor(error, &cx_type_module_unavailable);
        cx_exception_throw(error, __FILE__, __LINE__);
    }
    return cx_module_info_wrap(self->_module);
}

static const struct cx_reflection_array_entry *cx_module_array_reader(
    const struct cxcore_System_Reflection_ModuleInfo *self,
    struct cxcore_System_Reflection_TypeInfo owner, cx_uint functionIndex) {
    const struct cx_module_info *descriptor = self == CX_NULL ? CX_NULL : self->_descriptor;
    cx_uint index;
    if (!cx_module_descriptor_supported(descriptor)) return CX_NULL;
    for (index = 0u; index < descriptor->arrayReaderCount; ++index) {
        const struct cx_reflection_array_entry *entry = &descriptor->arrayReaders[index];
        if (entry->declaringType->Hash == owner.Hash && entry->functionIndex == functionIndex) return entry;
    }
    return CX_NULL;
}

struct cxcore_System_Collections_Array *cxcore_System_Reflection_ModuleInfo_GetGenericArguments(
    const struct cxcore_System_Reflection_ModuleInfo *self, struct cxcore_System_Reflection_TypeInfo type) {
    const struct cx_module_info *descriptor = self == CX_NULL ? CX_NULL : self->_descriptor;
    cx_uint index;
    if (cx_module_descriptor_supported(descriptor)) {
        for (index = 0u; index < descriptor->genericTypeCount; ++index) {
            const struct cx_reflection_generic_entry *entry = &descriptor->genericTypes[index];
            if (entry->type()->Hash == type.Hash) {
                struct cxcore_System_Collections_Array *result = cx_array_new(entry->argumentCount, sizeof(type));
                cx_uint argument;
                for (argument = 0u; argument < entry->argumentCount; ++argument)
                    cx_reflection_copy_type_info(
                        &((struct cxcore_System_Reflection_TypeInfo *)result->_data)[argument],
                        entry->arguments[argument]());
                return result;
            }
        }
    }
    return cx_array_new(0u, sizeof(type));
}

cx_bool cxcore_System_Reflection_ModuleInfo_CanReadFunctionArray(
    const struct cxcore_System_Reflection_ModuleInfo *self,
    struct cxcore_System_Reflection_TypeInfo owner, cx_uint functionIndex) {
    return cx_module_array_reader(self, owner, functionIndex) != CX_NULL;
}

CX_STRING_DEF(cx_module_bad_array, "The reflected function has no supported array result reader.");
static void cx_module_reject_array(void) {
    struct cxcore_System_Exception *error = cx_exception_allocate_durable((cx_uint)sizeof(*error));
    cxcore_System_Exception___constructor(error, &cx_module_bad_array);
    cx_exception_throw(error, __FILE__, __LINE__);
}

struct cxcore_System_Reflection_TypeInfo cxcore_System_Reflection_ModuleInfo_GetFunctionArrayElementType(
    const struct cxcore_System_Reflection_ModuleInfo *self,
    struct cxcore_System_Reflection_TypeInfo owner, cx_uint functionIndex) {
    const struct cx_reflection_array_entry *entry = cx_module_array_reader(self, owner, functionIndex);
    struct cxcore_System_Reflection_TypeInfo result = {0};
    if (entry == CX_NULL) cx_module_reject_array();
    cx_reflection_copy_type_info(&result, entry->elementType());
    return result;
}

struct cxcore_System_Collections_Array *cxcore_System_Reflection_ModuleInfo_ReadFunctionArray(
    const struct cxcore_System_Reflection_ModuleInfo *self,
    struct cxcore_System_Reflection_TypeInfo owner, cx_uint functionIndex,
    struct cxcore_System_Object *value, cx_uint maximumLength) {
    const struct cx_reflection_array_entry *entry = cx_module_array_reader(self, owner, functionIndex);
    if (entry == CX_NULL) cx_module_reject_array();
    return entry->read(value, maximumLength);
}

struct cxcore_System_String *cxcore_System_Reflection_ModuleInfo_VersionText___const_get(
    const struct cxcore_System_Reflection_ModuleInfo *self) {
    const struct cx_module_info *descriptor = self == CX_NULL ? CX_NULL : self->_descriptor;
    return cx_module_string(cx_module_descriptor_supported(descriptor) ? descriptor->version : "");
}

cx_uint cxcore_System_Reflection_ModuleInfo_ReferenceCount___const_get(
    const struct cxcore_System_Reflection_ModuleInfo *self) {
    const struct cx_module_info *descriptor = self == CX_NULL ? CX_NULL : self->_descriptor;
    return cx_module_descriptor_supported(descriptor) ? descriptor->referenceCount : 0u;
}

CX_STRING_DEF(cx_module_bad_reference, "The module reference index is out of range.");
struct cxcore_System_Reflection_ModuleInfo cxcore_System_Reflection_ModuleInfo_GetReference(
    const struct cxcore_System_Reflection_ModuleInfo *self, cx_uint index) {
    const struct cx_module_info *descriptor = self == CX_NULL ? CX_NULL : self->_descriptor;
    if (!cx_module_descriptor_supported(descriptor) || index >= descriptor->referenceCount) {
        struct cxcore_System_Exception *error = cx_exception_allocate_durable((cx_uint)sizeof(*error));
        cxcore_System_Exception___constructor(error, &cx_module_bad_reference);
        cx_exception_throw(error, __FILE__, __LINE__);
    }
    return cx_module_info_wrap(descriptor->references[index]());
}

cx_uint cxcore_System_Reflection_ModuleInfo_AnnotationCount___const_get(
    const struct cxcore_System_Reflection_ModuleInfo *self) {
    const struct cx_module_info *descriptor = self == CX_NULL ? CX_NULL : self->_descriptor;
    return cx_module_descriptor_supported(descriptor) ? descriptor->annotationCount : 0u;
}

CX_STRING_DEF(cx_module_bad_annotation, "The module annotation index is out of range.");
struct cxcore_System_Reflection_AnnotationInfo cxcore_System_Reflection_ModuleInfo_GetAnnotation(
    const struct cxcore_System_Reflection_ModuleInfo *self, cx_uint index) {
    const struct cx_module_info *descriptor = self == CX_NULL ? CX_NULL : self->_descriptor;
    const struct cx_annotation_info *annotation;
    struct cxcore_System_Reflection_AnnotationInfo result = {0};
    struct cxcore_System_Collections_Array *names;
    struct cxcore_System_Collections_Array *expressions;
    struct cxcore_System_Collections_Array *values;
    cx_uint argumentIndex;
    if (!cx_module_descriptor_supported(descriptor) || index >= descriptor->annotationCount) {
        struct cxcore_System_Exception *error = cx_exception_allocate_durable((cx_uint)sizeof(*error));
        cxcore_System_Exception___constructor(error, &cx_module_bad_annotation);
        cx_exception_throw(error, __FILE__, __LINE__);
    }
    annotation = &descriptor->annotations[index];
    cx_reflection_copy_type_info(&result.DeclaringType, annotation->declaringType);
    result.FunctionIndex = annotation->functionIndex;
    result.ParameterIndex = annotation->parameterIndex;
    cx_gc_store_reference((cx_ptr *)&result.Target,
                          (cx_ptr)cx_module_string(annotation->target));
    cx_gc_store_reference((cx_ptr *)&result.MemberName,
                          (cx_ptr)cx_module_string(annotation->memberName));
    cx_gc_store_reference((cx_ptr *)&result.TypeName,
                          (cx_ptr)cx_module_string(annotation->typeName));
    if (descriptor->annotationSources != CX_NULL) {
        const struct cx_source_location *source = &descriptor->annotationSources[index];
        cx_gc_store_reference((cx_ptr *)&result.SourceFile,
                              (cx_ptr)cx_module_string(source->file));
        result.SourceLine = source->line;
        result.SourceColumn = source->column;
    } else {
        cx_gc_store_reference((cx_ptr *)&result.SourceFile,
                              (cx_ptr)cx_module_string(""));
    }
    names = cx_array_new(annotation->argumentCount, (cx_uint)sizeof(cx_ptr));
    cx_gc_store_reference((cx_ptr *)&result.ArgumentNames, (cx_ptr)names);
    expressions = cx_array_new(annotation->argumentCount, (cx_uint)sizeof(cx_ptr));
    cx_gc_store_reference((cx_ptr *)&result.ArgumentExpressions,
                          (cx_ptr)expressions);
    values = cx_array_new(annotation->argumentCount, (cx_uint)sizeof(cx_ptr));
    cx_gc_store_reference((cx_ptr *)&result.ArgumentValues, (cx_ptr)values);
    for (argumentIndex = 0; argumentIndex < annotation->argumentCount; ++argumentIndex) {
        const struct cx_annotation_argument *argument = &annotation->arguments[argumentIndex];
        cx_gc_store_reference((cx_ptr *)((cx_byte *)names->_data +
            (size_t)argumentIndex * sizeof(cx_ptr)), cx_module_string(argument->name));
        cx_gc_store_reference((cx_ptr *)((cx_byte *)expressions->_data +
            (size_t)argumentIndex * sizeof(cx_ptr)),
            cx_module_string(argument->expression));
        cx_gc_store_reference((cx_ptr *)((cx_byte *)values->_data +
            (size_t)argumentIndex * sizeof(cx_ptr)),
            argument->value == CX_NULL ? CX_NULL : argument->value());
    }
    return result;
}

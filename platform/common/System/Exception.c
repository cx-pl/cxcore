#include "cxcore.h"
#include "cxgc.h"
#include "../../platform.h"
#include "../cxarena.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static CX_THREAD_LOCAL struct cx_exception_frame *cx_current_exception_frame;
static CX_THREAD_LOCAL struct cx_exception cx_current_exception;

static void cx_exception_write_string(const struct cxcore_System_String *value) {
    if (value == CX_NULL) {
        fputs("<no message>", stderr);
        return;
    }
    if (value->_length == 0) {
        return;
    }
    if (value->_data == CX_NULL) {
        fputs("<invalid string>", stderr);
        return;
    }
    (void)fwrite(value->_data, 1, value->_length, stderr);
}

static void cx_exception_write_description(
    const struct cxcore_System_Exception *exception) {
    struct cxcore_System_Reflection_TypeInfo typeInfo;
    const struct cxcore_System_String *name;
    if (exception == CX_NULL) {
        fputs("<null exception>", stderr);
        return;
    }
    typeInfo = cxcore_System_Exception_RuntimeType___const_get(exception);
    name = typeInfo.Name;
    if (typeInfo.Namespace != CX_NULL && typeInfo.Namespace->_length != 0) {
        cx_exception_write_string(typeInfo.Namespace);
        fputc('.', stderr);
    }
    cx_exception_write_string(name);
    fputs(": ", stderr);
    cx_exception_write_string(exception->_message);
}

static struct cxcore_System_String *cx_exception_make_stack_trace(
    const char *fileName, int line) {
    char buffer[1024];
    int formattedLength = snprintf(buffer, sizeof(buffer), "%s:%d",
                                   fileName == NULL ? "<unknown>" : fileName,
                                   line);
    cx_uint length;
    cx_byte *data;
    struct cxcore_System_String *trace;
    if (formattedLength <= 0) {
        return CX_NULL;
    }
    length = (cx_uint)((size_t)formattedLength < sizeof(buffer)
                           ? (size_t)formattedLength
                           : sizeof(buffer) - 1u);
    data = (cx_byte *)cx_exception_allocate_durable(length);
    memcpy(data, buffer, length);
    trace = (struct cxcore_System_String *)cx_exception_allocate_durable(
        (cx_uint)sizeof(*trace));
    cxcore_System_String___constructor__2(trace, length, data);
    return trace;
}

cx_ptr *cx_exception_thread_root(void) {
    return &cx_current_exception.exceptionObject;
}

const struct cx_exception *cx_exception_thread_state(void) {
    return &cx_current_exception;
}

void cx_exception_restore(const struct cx_exception *state) {
    cx_gc_store_reference((cx_ptr *)&cx_current_exception.exceptionObject,
                          state->exceptionObject);
    cx_current_exception.thrownFileName = state->thrownFileName;
    cx_current_exception.thrownLine = state->thrownLine;
}

cx_bool cx_exception_thread_detach(void) {
    if (cx_current_exception_frame != NULL) {
        return CX_FALSE;
    }
    // cx_gc_runtime_detach calls this while holding the collector mutex.
    (void)cx_gc_store_reference_locked(
        (cx_ptr *)&cx_current_exception.exceptionObject, CX_NULL);
    cx_current_exception.thrownFileName = CX_NULL;
    cx_current_exception.thrownLine = 0;
    return CX_TRUE;
}

struct cxcore_System_String *
cxcore_System_Exception_StackTrace___const_get(
    const struct cxcore_System_Exception *self) {
    return self == CX_NULL ? CX_NULL : self->_stackTrace;
}

static CX_NORETURN void cx_exception_dispatch(void) {
    if (cx_current_exception_frame != CX_NULL) {
        cx_platform_exception_jump(cx_current_exception_frame->environment);
    }

    fputs("Uncaught ", stderr);
    cx_exception_write_description(
        (const struct cxcore_System_Exception *)cx_current_exception.exceptionObject);
    {
        const struct cxcore_System_Exception *inner =
            (const struct cxcore_System_Exception *)
                ((const struct cxcore_System_Exception *)
                    cx_current_exception.exceptionObject)->_innerException._obj;
        cx_uint depth = 0;
        while (inner != CX_NULL && depth < 16) {
            fputs("\n  caused by ", stderr);
            cx_exception_write_description(inner);
            inner = (const struct cxcore_System_Exception *)inner->_innerException._obj;
            ++depth;
        }
        if (inner != CX_NULL) {
            fputs("\n  caused by <inner exception chain truncated>", stderr);
        }
    }
    fprintf(stderr, "\nThrown at %s:%d\n",
            cx_current_exception.thrownFileName == CX_NULL ? "<unknown>"
                                                           : cx_current_exception.thrownFileName,
            cx_current_exception.thrownLine);
    exit(EXIT_FAILURE);
}
void cx_exception_push(struct cx_exception_frame *frame) {
    cx_gc_check_thread();
    frame->previous = cx_current_exception_frame;
    frame->arena_scope_checkpoint = cx_arena_scope_checkpoint();
    frame->durable_scope_checkpoint = cx_arena_durable_scope_checkpoint();
    cx_current_exception_frame = frame;
}

void cx_exception_pop(struct cx_exception_frame *frame) {
    cx_gc_check_thread();
    if (cx_current_exception_frame != frame) {
        fputs("Invalid cx exception handler stack\n", stderr);
        abort();
    }
    cx_arena_scope_unwind_to(frame->arena_scope_checkpoint);
    cx_arena_durable_scope_unwind_to(frame->durable_scope_checkpoint);
    cx_current_exception_frame = frame->previous;
}

void cx_exception_throw(cx_ptr exceptionObject, const char *fileName, int line) {
    struct cxcore_System_Exception *exception;
    struct cxcore_System_String *stackTrace;
    cx_gc_check_thread();
    if (exceptionObject == CX_NULL) {
        fputs("Cannot throw a null exception\n", stderr);
        abort();
    }
    exception = (struct cxcore_System_Exception *)exceptionObject;
    if (cxcore_System_Exception_Matches(
            exception, cxcore_System_Exception___typeinfo)) {
        if (exception->_stackTrace == CX_NULL) {
            stackTrace = cx_exception_make_stack_trace(fileName, line);
            if (stackTrace != CX_NULL) {
                (void)cx_gc_store_reference((cx_ptr *)&exception->_stackTrace,
                                            (cx_ptr)stackTrace);
            }
        }
    }
    // An exception crossing a handler boundary must outlive any arena scope
    // that the handler unwinds. Durable exceptions retain identity; scoped or
    // stack exceptions are copied with durable message and inner-exception data.
    exceptionObject = cx_exception_copy_durable(exceptionObject);
    cx_gc_store_reference((cx_ptr *)&cx_current_exception.exceptionObject,
                          exceptionObject);
    cx_current_exception.thrownFileName = fileName;
    cx_current_exception.thrownLine = line;
    if (cx_current_exception_frame != NULL) {
        cx_arena_scope_unwind_to(cx_current_exception_frame->arena_scope_checkpoint);
        cx_arena_durable_scope_unwind_to(
            cx_current_exception_frame->durable_scope_checkpoint);
    }
    cx_exception_dispatch();
}

void cx_exception_rethrow(void) {
    cx_gc_check_thread();
    if (cx_current_exception.exceptionObject == CX_NULL) {
        fputs("Cannot rethrow without an active exception\n", stderr);
        abort();
    }
    if (cx_current_exception_frame != NULL) {
        cx_arena_scope_unwind_to(cx_current_exception_frame->arena_scope_checkpoint);
        cx_arena_durable_scope_unwind_to(
            cx_current_exception_frame->durable_scope_checkpoint);
    }
    cx_exception_dispatch();
}

cx_ptr cx_exception_current(void) {
    cx_gc_check_thread();
    return cx_current_exception.exceptionObject;
}

cx_bool cx_exception_pending(void) {
    cx_gc_check_thread();
    return cx_current_exception.exceptionObject != CX_NULL;
}

void cx_exception_clear(void) {
    cx_gc_check_thread();
    cx_gc_store_reference((cx_ptr *)&cx_current_exception.exceptionObject, CX_NULL);
    cx_current_exception.thrownFileName = CX_NULL;
    cx_current_exception.thrownLine = 0;
}

cx_ptr cx_exception_allocate_durable(cx_uint size) {
    struct cx_arena_durable_scope scope;
    cx_ptr exception;
    cx_arena_enter_durable(&scope);
    exception = cxcore_System_Memory_Alloc(size);
    cx_arena_leave_durable(&scope);
    return exception;
}

static const struct cxcore_System_String *cx_exception_make_message_durable(
    const struct cxcore_System_String *message) {
    struct cxcore_System_String *copy;
    cx_ptr data = CX_NULL;
    if (message == CX_NULL ||
        (cx_gc_is_durable_reference((cx_ptr)message) &&
         (message->_length == 0 ||
          cx_gc_is_durable_reference(message->_data)))) {
        return message;
    }
    if (message->_length != 0 && message->_data == CX_NULL) abort();
    copy = (struct cxcore_System_String *)cxcore_System_Memory_Alloc(
        (cx_uint)sizeof(*copy));
    cxcore_System_String___constructor(copy);
    if (message->_length != 0) {
        data = cxcore_System_Memory_Alloc(message->_length);
        memcpy(data, message->_data, message->_length);
    }
    copy->_length = message->_length;
    cx_gc_store_reference(&copy->_data, data);
    return copy;
}

static struct cxcore_System_Exception *cx_exception_copy_durable_inner(
    const struct cxcore_System_Exception *source, unsigned depth);

static cx_bool cx_exception_function_value_is_durable(
    const void *source, cx_uint size) {
    cx_ptr environment;
    cx_ptr target;
    cx_ptr invocationList;
    if (source == NULL || size != 5u * sizeof(cx_ptr)) return CX_FALSE;
    memcpy(&environment, (const unsigned char *)source + sizeof(cx_ptr),
           sizeof(environment));
    memcpy(&target, (const unsigned char *)source + 2u * sizeof(cx_ptr),
           sizeof(target));
    memcpy(&invocationList,
           (const unsigned char *)source + 4u * sizeof(cx_ptr),
           sizeof(invocationList));
    return cx_gc_is_durable_reference(environment) &&
        cx_gc_is_durable_reference(target) &&
        cx_gc_is_durable_reference(invocationList);
}

static cx_bool cx_exception_copy_function_value(
    void *destination, const void *source, cx_uint size) {
    if (destination == NULL || source == NULL ||
        size != 5u * sizeof(cx_ptr)) return CX_FALSE;
    memcpy(destination, source, sizeof(cx_ptr));
    cx_gc_store_reference(
        (cx_ptr *)((unsigned char *)destination + sizeof(cx_ptr)),
        *(cx_ptr const *)((const unsigned char *)source + sizeof(cx_ptr)));
    cx_gc_store_reference(
        (cx_ptr *)((unsigned char *)destination + 2u * sizeof(cx_ptr)),
        *(cx_ptr const *)((const unsigned char *)source + 2u * sizeof(cx_ptr)));
    memcpy((unsigned char *)destination + 3u * sizeof(cx_ptr),
           (const unsigned char *)source + 3u * sizeof(cx_ptr),
           sizeof(cx_ptr));
    cx_gc_store_reference(
        (cx_ptr *)((unsigned char *)destination + 4u * sizeof(cx_ptr)),
        *(cx_ptr const *)((const unsigned char *)source + 4u * sizeof(cx_ptr)));
    return CX_TRUE;
}

static cx_bool cx_exception_is_inline_value_type(
    const struct cxcore_System_Reflection_TypeInfo *typeInfo) {
    const struct cx_reflection_field *fields = NULL;
    cx_uint fieldCount = 0u;
    cx_uint index;
    if (typeInfo == NULL ||
        (typeInfo->Flags & (CX_REFLECTION_FLAG_TYPE_CLASS |
                            CX_REFLECTION_FLAG_TYPE_STRUCT)) == 0u) {
        return CX_FALSE;
    }
    fields = cx_reflection_fields(typeInfo, &fieldCount);
    if (fieldCount == 0u || fields == NULL) return CX_FALSE;
    for (index = 0u; index < fieldCount; ++index) {
        const struct cx_reflection_field *field = &fields[index];
        const struct cxcore_System_Reflection_TypeInfo *fieldType =
            (const struct cxcore_System_Reflection_TypeInfo *)field->typeInfo;
        if ((field->flags & CX_REFLECTION_FLAG_STATIC) != 0u) continue;
        // CX primitive wrappers have a reflected value field that points back
        // to the wrapper type itself. Older fallback metadata omits its size.
        return fieldType != NULL && fieldType->Hash == typeInfo->Hash &&
            (field->size == typeInfo->Size || field->size == 0u);
    }
    return CX_FALSE;
}

static cx_bool cx_exception_value_fields_are_durable(
    const void *source, const struct cxcore_System_Reflection_TypeInfo *typeInfo,
    unsigned depth) {
    const struct cx_reflection_field *fields = NULL;
    const struct cxcore_System_Reflection_TypeInfo *baseType;
    cx_uint fieldCount = 0u;
    cx_uint index;
    if (source == NULL || typeInfo == NULL || depth >= 64u) return CX_FALSE;
    if ((typeInfo->Flags & CX_REFLECTION_FLAG_TYPE_ENUM) != 0u) {
        return CX_TRUE;
    }
    if ((typeInfo->Flags & CX_REFLECTION_FLAG_TYPE_DELEGATE) != 0u) {
        fields = cx_reflection_fields(typeInfo, &fieldCount);
        if (fields == NULL || fieldCount == 0u) {
            return cx_exception_function_value_is_durable(source, typeInfo->Size);
        }
    } else if ((typeInfo->Flags & CX_REFLECTION_FLAG_TYPE_CLASS) != 0u) {
        if (typeInfo->Hash == cxcore_System_Exception___typeinfo.Hash)
            return CX_TRUE;
        if (!cx_exception_is_inline_value_type(typeInfo)) {
            baseType = (const struct cxcore_System_Reflection_TypeInfo *)
                typeInfo->BaseType._obj;
            if (baseType == NULL ||
                !cx_exception_value_fields_are_durable(
                    source, baseType, depth + 1u)) {
                return CX_FALSE;
            }
        }
    } else if ((typeInfo->Flags & CX_REFLECTION_FLAG_TYPE_STRUCT) == 0u) {
        return CX_TRUE;
    }
    if (fields == NULL) fields = cx_reflection_fields(typeInfo, &fieldCount);
    if (fieldCount != 0u && fields == NULL) return CX_FALSE;
    for (index = 0u; index < fieldCount; ++index) {
        const struct cx_reflection_field *field = &fields[index];
        const struct cxcore_System_Reflection_TypeInfo *fieldType =
            (const struct cxcore_System_Reflection_TypeInfo *)field->typeInfo;
        const unsigned char *value;
        if ((field->flags & CX_REFLECTION_FLAG_STATIC) != 0u) continue;
        if (field->offset == CX_REFLECTION_NO_OFFSET ||
            field->offset > typeInfo->Size ||
            field->size > typeInfo->Size - field->offset) {
            return CX_FALSE;
        }
        value = (const unsigned char *)source + field->offset;
        if (fieldType == NULL) {
            if ((field->flags & CX_REFLECTION_FLAG_FIELD_POINTER) != 0u) {
                cx_ptr pointer;
                if (field->size != sizeof(pointer)) return CX_FALSE;
                memcpy(&pointer, value, sizeof(pointer));
                if (!cx_gc_is_durable_reference(pointer)) return CX_FALSE;
            } else if ((field->flags & CX_REFLECTION_FLAG_FIELD_FUNCTION_VALUE) != 0u) {
                if (!cx_exception_function_value_is_durable(value, field->size))
                    return CX_FALSE;
            } else if ((field->flags & CX_REFLECTION_FLAG_FIELD_RAW_BYTES) == 0u) {
                return CX_FALSE;
            }
        } else if ((fieldType->Flags & CX_REFLECTION_FLAG_TYPE_CLASS) != 0u) {
            if (field->size == fieldType->Size &&
                cx_exception_is_inline_value_type(fieldType)) {
                if (!cx_exception_value_fields_are_durable(
                        value, fieldType, depth + 1u)) return CX_FALSE;
            } else if (field->size == sizeof(cx_ptr)) {
                cx_ptr pointer;
                memcpy(&pointer, value, sizeof(pointer));
                if (!cx_gc_is_durable_reference(pointer)) return CX_FALSE;
            } else {
                return CX_FALSE;
            }
        } else if ((fieldType->Flags & CX_REFLECTION_FLAG_TYPE_INTERFACE) != 0u) {
            struct cx_iface_ref reference;
            if (field->size != sizeof(reference)) return CX_FALSE;
            memcpy(&reference, value, sizeof(reference));
            if (!cx_gc_is_durable_reference(reference.instance) ||
                !cx_gc_is_durable_reference(reference.owner)) {
                return CX_FALSE;
            }
        } else if (fieldType->Hash == typeInfo->Hash &&
                   (field->size == typeInfo->Size || field->size == 0u)) {
            // Primitive wrapper metadata points its value field back to itself.
            continue;
        } else if ((fieldType->Flags & CX_REFLECTION_FLAG_TYPE_STRUCT) != 0u &&
                   !cx_exception_value_fields_are_durable(
                       value, fieldType, depth + 1u)) {
            return CX_FALSE;
        } else if ((fieldType->Flags & CX_REFLECTION_FLAG_TYPE_DELEGATE) != 0u &&
                   !cx_exception_value_fields_are_durable(
                       value, fieldType, depth + 1u)) {
            return CX_FALSE;
        }
    }
    return CX_TRUE;
}

static cx_bool cx_exception_copy_class_fields(
    void *destination, const void *source,
    const struct cxcore_System_Reflection_TypeInfo *typeInfo,
    unsigned depth) {
    const struct cx_reflection_field *fields;
    const struct cxcore_System_Reflection_TypeInfo *baseType;
    cx_uint fieldCount = 0u;
    cx_uint index;
    if (destination == NULL || source == NULL || typeInfo == NULL || depth >= 64u)
        return CX_FALSE;

    if (typeInfo->Hash == cxcore_System_Exception___typeinfo.Hash) {
        const struct cxcore_System_Exception *sourceException = source;
        struct cxcore_System_Exception *destinationException = destination;
        const struct cxcore_System_Exception *innerSource =
            (const struct cxcore_System_Exception *)sourceException->_innerException._obj;
        struct cxcore_System_Exception *innerCopy =
            cx_exception_copy_durable_inner(innerSource, depth + 1u);
        const struct cxcore_System_String *message =
            cx_exception_make_message_durable(sourceException->_message);
        const struct cxcore_System_String *stackTrace =
            cx_exception_make_message_durable(sourceException->_stackTrace);
        cxcore_System_Exception___constructor(destinationException, message);
        if (stackTrace != NULL) {
            (void)cx_gc_store_reference(
                (cx_ptr *)&destinationException->_stackTrace,
                (cx_ptr)stackTrace);
        }
        if (innerCopy != NULL) {
            (void)cx_gc_store_reference(
                (cx_ptr *)&destinationException->_innerException._obj,
                (cx_ptr)innerCopy);
        }
        return CX_TRUE;
    }

    baseType = (const struct cxcore_System_Reflection_TypeInfo *)
        typeInfo->BaseType._obj;
    if (baseType == NULL ||
        (baseType->Flags & CX_REFLECTION_FLAG_TYPE_CLASS) == 0u ||
        !cx_exception_copy_class_fields(
            destination, source, baseType, depth + 1u)) {
        return CX_FALSE;
    }

    fields = cx_reflection_fields(typeInfo, &fieldCount);
    if (fieldCount != 0u && fields == NULL) return CX_FALSE;
    for (index = 0u; index < fieldCount; ++index) {
        const struct cx_reflection_field *field = &fields[index];
        const struct cxcore_System_Reflection_TypeInfo *fieldType =
            (const struct cxcore_System_Reflection_TypeInfo *)field->typeInfo;
        const unsigned char *sourceField;
        unsigned char *destinationField;
        if ((field->flags & CX_REFLECTION_FLAG_STATIC) != 0u) continue;
        if (field->offset == CX_REFLECTION_NO_OFFSET ||
            field->offset > typeInfo->Size ||
            field->size > typeInfo->Size - field->offset) {
            return CX_FALSE;
        }
        sourceField = (const unsigned char *)source + field->offset;
        destinationField = (unsigned char *)destination + field->offset;
        if (fieldType == NULL) {
            if ((field->flags & CX_REFLECTION_FLAG_FIELD_POINTER) != 0u) {
                cx_ptr pointer;
                if (field->size != sizeof(pointer)) return CX_FALSE;
                memcpy(&pointer, sourceField, sizeof(pointer));
                (void)cx_gc_store_native_pointer((cx_ptr *)destinationField, pointer);
            } else if ((field->flags & CX_REFLECTION_FLAG_FIELD_RAW_BYTES) != 0u) {
                memcpy(destinationField, sourceField, field->size);
            } else if ((field->flags & CX_REFLECTION_FLAG_FIELD_FUNCTION_VALUE) != 0u) {
                if (!cx_exception_copy_function_value(
                        destinationField, sourceField, field->size)) return CX_FALSE;
            } else {
                return CX_FALSE;
            }
        } else if ((fieldType->Flags & CX_REFLECTION_FLAG_TYPE_CLASS) != 0u) {
            if (field->size == fieldType->Size &&
                cx_exception_is_inline_value_type(fieldType)) {
                (void)cx_gc_copy_value(destinationField, sourceField, fieldType);
            } else if (field->size == sizeof(cx_ptr)) {
                cx_ptr pointer;
                memcpy(&pointer, sourceField, sizeof(pointer));
                (void)cx_gc_store_reference((cx_ptr *)destinationField, pointer);
            } else {
                return CX_FALSE;
            }
        } else if ((fieldType->Flags & CX_REFLECTION_FLAG_TYPE_INTERFACE) != 0u) {
            struct cx_iface_ref reference;
            if (field->size != sizeof(reference)) return CX_FALSE;
            memcpy(&reference, sourceField, sizeof(reference));
            (void)cx_gc_store_interface_reference(
                (struct cx_iface_ref *)destinationField, reference);
        } else if ((fieldType->Flags & CX_REFLECTION_FLAG_TYPE_STRUCT) != 0u) {
            if (cx_exception_is_inline_value_type(fieldType)) {
                memcpy(destinationField, sourceField, fieldType->Size);
            } else {
                (void)cx_gc_copy_value(destinationField, sourceField, fieldType);
            }
        } else if ((fieldType->Flags & CX_REFLECTION_FLAG_TYPE_DELEGATE) != 0u) {
            (void)cx_gc_copy_value(destinationField, sourceField, fieldType);
        } else {
            if (field->size != fieldType->Size) return CX_FALSE;
            memcpy(destinationField, sourceField, field->size);
        }
    }
    return CX_TRUE;
}

static cx_bool cx_exception_has_exception_base(
    const struct cxcore_System_Reflection_TypeInfo *typeInfo, unsigned depth) {
    const struct cxcore_System_Reflection_TypeInfo *baseType;
    if (typeInfo == NULL || depth >= 64u) return CX_FALSE;
    if (typeInfo->Hash == cxcore_System_Exception___typeinfo.Hash)
        return CX_TRUE;
    baseType = (const struct cxcore_System_Reflection_TypeInfo *)
        typeInfo->BaseType._obj;
    return cx_exception_has_exception_base(baseType, depth + 1u);
}

static struct cxcore_System_Exception *cx_exception_copy_durable_inner(
    const struct cxcore_System_Exception *source, unsigned depth) {
    struct cxcore_System_Exception *copy;
    const struct cxcore_System_Reflection_TypeInfo *typeInfo;
    const struct cxcore_System_Exception *innerSource;
    struct cxcore_System_Exception *innerCopy;
    const struct cxcore_System_String *message;
    if (source == NULL) return NULL;
    if (depth >= 64u) abort();
    if (cx_gc_is_durable_reference((cx_ptr)source))
        return (struct cxcore_System_Exception *)source;

    typeInfo = &CX_GET_TYPEINFO(source);
    if (typeInfo->Hash != cxcore_System_Exception___typeinfo.Hash &&
        (typeInfo->Flags & CX_REFLECTION_FLAG_TYPE_CLASS) != 0u &&
        typeInfo->Size >= sizeof(struct cxcore_System_Exception) &&
        cx_exception_has_exception_base(typeInfo, 0u) &&
        cx_exception_value_fields_are_durable(source, typeInfo, 0u)) {
        copy = (struct cxcore_System_Exception *)cxcore_System_Memory_Alloc(
            typeInfo->Size);
        if (copy != NULL && cx_exception_copy_class_fields(
                copy, source, typeInfo, 0u)) {
            memcpy(copy, source, sizeof(cx_ptr));
            return copy;
        }
    }

    message = cx_exception_make_message_durable(source->_message);
    copy = (struct cxcore_System_Exception *)cxcore_System_Memory_Alloc(
        (cx_uint)sizeof(*copy));
    cxcore_System_Exception___constructor(copy, message);
    innerSource = (const struct cxcore_System_Exception *)
        source->_innerException._obj;
    innerCopy = cx_exception_copy_durable_inner(innerSource, depth + 1u);
    if (innerCopy != NULL) {
        (void)cx_gc_store_reference((cx_ptr *)&copy->_innerException._obj,
                                    (cx_ptr)innerCopy);
    }
    return copy;
}

cx_ptr cx_exception_copy_durable(cx_ptr exception) {
    struct cx_arena_durable_scope scope;
    struct cxcore_System_Exception *copy;
    if (exception == NULL || cx_gc_is_durable_reference(exception)) {
        return exception;
    }
    cx_arena_enter_durable(&scope);
    copy = cx_exception_copy_durable_inner(
        (const struct cxcore_System_Exception *)exception, 0u);
    cx_arena_leave_durable(&scope);
    return (cx_ptr)copy;
}

CX_NORETURN void cx_exception_raise_message(
    const struct cxcore_System_String *message, const char *fileName, int line) {
    struct cx_arena_durable_scope scope;
    const struct cxcore_System_String *durable_message;
    struct cxcore_System_Exception *exception;
    cx_arena_enter_durable(&scope);
    durable_message = cx_exception_make_message_durable(message);
    exception = (struct cxcore_System_Exception *)cxcore_System_Memory_Alloc(
        (cx_uint)sizeof(*exception));
    cxcore_System_Exception___constructor(exception, durable_message);
    cx_arena_leave_durable(&scope);
    cx_exception_throw(exception, fileName, line);
}

#if !defined(CX_USE_GENERATED_CORE)
void cxcore_System_Exception___constructor__3(
    struct cxcore_System_Exception *self) {
    cxcore_System_Exception___constructor(self, CX_NULL);
}
#endif

struct cxcore_System_Reflection_TypeInfo
    cxcore_System_Exception_RuntimeType___const_get(const struct cxcore_System_Exception * __this) {
    return CX_GET_TYPEINFO(__this);
}

cx_bool cxcore_System_Exception_Matches(const struct cxcore_System_Exception * exception,
                         const struct cxcore_System_Reflection_TypeInfo typeInfo) {
    return cxcore_System_Reflection_TypeInfo_IsInBaseTypeChain(
        &typeInfo, cxcore_System_Exception_RuntimeType___const_get(exception));
}

#ifndef __CX_TYPES_H__
#define __CX_TYPES_H__

#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>

struct cxcore_System_Collections_Array;
struct cxcore_System_Reflection_TypeInfo;
struct cxcore_System_String;

typedef char cx_bool;
typedef int cx_char;
typedef unsigned short cx_wchar;
typedef unsigned char cx_byte;
typedef signed char cx_sbyte;
typedef signed short cx_short;
typedef unsigned short cx_ushort;
typedef signed int cx_int;
typedef unsigned int cx_uint;
typedef signed long long cx_long;
typedef unsigned long long cx_ulong;
typedef float cx_float;
typedef double cx_double;
// Unsigned 96-bit coefficient, scale 0..28, and sign (flags bit 31).
// The remaining flags bits are zero; scale occupies bits 16..23.
typedef struct cx_decimal {
    cx_uint lo, mid, hi, flags;
} cx_decimal;
typedef void *cx_ptr;

typedef void (*cx_vtable_function)(void);
typedef void (*cx_gc_blocking_callback)(void *context);
typedef void (*cx_native_call_callback)(void *context);

union cx_vtable_entry {
    cx_ptr data;
    cx_vtable_function function;
};

struct cx_struct_ref {
    cx_ptr vtable;
    cx_ptr instance;
};

struct cx_iface_ref {
    cx_ptr vtable;
    cx_ptr instance;
    cx_ptr owner;
};

struct cx_interface_impl {
    cx_ptr typeInfo;
    cx_ptr vtable;
};

struct cx_reflection_field;
struct cx_reflection_function;
struct cx_generic_virtual_registration {
    cx_ptr receiverTypeInfo;
    const char *methodIdentity;
    const cx_ptr *methodTypeArguments;
    cx_uint methodTypeArgumentCount;
    const char *implementationIdentity;
    cx_vtable_function implementation;
    struct cx_generic_virtual_registration *next;
};
typedef cx_ptr (*cx_reflection_factory)(void);
typedef cx_ptr (*cx_reflection_invoker)(cx_ptr target, const cx_ptr *arguments,
                                        cx_uint argumentCount);

struct cx_runtime_type_info {
    const struct cx_interface_impl *interfaces;
    cx_uint interfaceCount;
    const struct cx_reflection_field *fields;
    cx_uint fieldCount;
    const struct cx_reflection_function *functions;
    cx_uint functionCount;
    cx_reflection_factory createInstance;
    const cx_reflection_invoker *invokers;
};

// Compiler-emitted layout for a native aggregate embedded in a reflected value.
// Pointer offsets are relative to the start of the native field.
struct cx_native_value_layout {
    size_t size;
    const size_t *pointerOffsets;
    size_t pointerCount;
    cx_bool complete;
};

struct cx_reflection_field {
    cx_uint flags;
    cx_uint offset;
    cx_ptr typeInfo;
    const char *name;
    cx_uint size;
};

struct cx_reflection_parameter {
    cx_uint flags;
    cx_ptr typeInfo;
    const char *name;
    cx_ptr defaultValue;
};

struct cx_reflection_function {
    cx_uint flags;
    cx_uint slot;
    cx_ptr returnTypeInfo;
    const char *name;
    const struct cx_reflection_parameter *parameters;
    cx_uint parameterCount;
};

// Immutable module metadata. Providers describe linked project references
// without running module initializers or allocating managed objects.
struct cx_module_info;
typedef struct cxcore_System_Collections_Array *(*cx_reflection_array_reader)(cx_ptr value, cx_uint maximumLength);
typedef const struct cxcore_System_Reflection_TypeInfo *(*cx_reflection_element_provider)(void);
struct cx_reflection_generic_entry {
    cx_reflection_element_provider type;
    const cx_reflection_element_provider *arguments;
    cx_uint argumentCount;
};
struct cx_reflection_array_entry {
    const struct cxcore_System_Reflection_TypeInfo *declaringType;
    cx_uint functionIndex;
    cx_reflection_array_reader read;
    cx_reflection_element_provider elementType;
};
typedef cx_ptr (*cx_annotation_value_provider)(void);
struct cx_annotation_argument {
    const char *name;
    const char *expression;
    cx_annotation_value_provider value;
};
struct cx_annotation_info {
    const struct cxcore_System_Reflection_TypeInfo *declaringType;
    cx_int functionIndex;
    cx_int parameterIndex;
    const char *target;
    const char *memberName;
    const char *typeName;
    const struct cx_annotation_argument *arguments;
    cx_uint argumentCount;
};
typedef const struct cx_module_info *(*cx_module_info_provider)(void);
struct cx_source_location {
    const char *file;
    cx_uint line;
    cx_uint column;
};
struct cx_module_info {
    cx_uint formatVersion;
    const char *name;
    const char *version;
    const char *description;
    const char *author;
    const struct cxcore_System_Reflection_TypeInfo *const *types;
    cx_uint typeCount;
    const cx_module_info_provider *references;
    cx_uint referenceCount;
    const struct cx_annotation_info *annotations;
    cx_uint annotationCount;
    const struct cx_reflection_array_entry *arrayReaders;
    cx_uint arrayReaderCount;
    const struct cx_reflection_generic_entry *genericTypes;
    cx_uint genericTypeCount;
    const struct cx_source_location *annotationSources;
};

// External event waits.
struct cx_scheduler_wait_entry {
    cx_ptr context;
    int ready;
};
// Runs on the scheduler owner thread in a GC blocking region. Only mark entries
// ready; do not allocate managed objects, raise CX exceptions, or run callbacks.
// UINT32_MAX means no deadline. Return a negative value on failure.
typedef int (*cx_scheduler_wait_function)(struct cx_scheduler_wait_entry *entries,
    size_t count, uint32_t timeout_milliseconds);

struct cx_exception {
    void *exceptionObject;
    const char *thrownFileName;
    int thrownLine;
};

struct cx_arena_scope;
struct cx_arena_durable_scope;

struct cx_exception_frame {
    jmp_buf environment;
    struct cx_exception_frame *previous;
    struct cx_arena_scope *arena_scope_checkpoint;
    struct cx_arena_durable_scope *durable_scope_checkpoint;
};

struct cx_native_callback_scope {
    cx_bool attached;
    cx_bool resumed;
    cx_int saved_errno;
    cx_uint saved_last_error;
    struct cx_exception previous_exception;
};

struct cx_gc_root_handle;

struct cx_gc_root_region {
    const void *address;
    cx_uint size;
};

// Zero-initialized, process-lifetime gates used by generated module/metadata code.
struct cx_runtime_once {
    cx_uint state;
    cx_ptr owner;
    cx_ptr failure;
};

// The runtime owns the void() function-value ABI shared with generated code.
// Note: 136FFEE706DF35B2 is a deterministic hash of the void() signature
struct CX_FN_136FFEE706DF35B2_invocation;
typedef struct CX_FN_136FFEE706DF35B2 {
    void (*__function)(void *environment);
    void *__environment;
    cx_ptr __target;
    const struct cx_reflection_function *__method;
    struct CX_FN_136FFEE706DF35B2_invocation *__invocation_list;
} CX_FN_136FFEE706DF35B2;

struct CX_FN_136FFEE706DF35B2_invocation {
    CX_FN_136FFEE706DF35B2 value;
    struct CX_FN_136FFEE706DF35B2_invocation *next;
};
typedef CX_FN_136FFEE706DF35B2 cx_action;

#endif // __CX_TYPES_H__

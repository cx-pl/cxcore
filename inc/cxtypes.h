#ifndef __CX_TYPES_H__
#define __CX_TYPES_H__

#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>

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

struct cx_reflection_field {
    cx_uint flags;
    cx_uint offset;
    cx_ptr typeInfo;
    const char *name;
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

struct cx_exception {
    void *exceptionObject;
    const char *thrownFileName;
    int thrownLine;
};

struct cx_exception_frame {
    jmp_buf environment;
    struct cx_exception_frame *previous;
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

// The void() function-value ABI is shared with generated headers and threading.
// Note: 136FFEE706DF35B2 is a deterministic hash of the void() signature
#ifndef CX_FUNCTION_TYPE_CX_FN_136FFEE706DF35B2
#define CX_FUNCTION_TYPE_CX_FN_136FFEE706DF35B2
struct CX_FN_136FFEE706DF35B2_invocation;
typedef struct CX_FN_136FFEE706DF35B2 {
    void (*__function)(void *environment);
    void *__environment;
    cx_ptr __target;
    const struct cx_reflection_function *__method;
    struct CX_FN_136FFEE706DF35B2_invocation *__invocation_list;
} CX_FN_136FFEE706DF35B2;
#endif
#ifndef CX_FUNCTION_NODE_CX_FN_136FFEE706DF35B2
#define CX_FUNCTION_NODE_CX_FN_136FFEE706DF35B2
struct CX_FN_136FFEE706DF35B2_invocation {
    CX_FN_136FFEE706DF35B2 value;
    struct CX_FN_136FFEE706DF35B2_invocation *next;
};
#endif
typedef CX_FN_136FFEE706DF35B2 cx_action;

#endif // __CX_TYPES_H__

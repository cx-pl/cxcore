#ifndef __CX_TYPES_H__
#define __CX_TYPES_H__

#include <stddef.h>
#include <setjmp.h>

typedef char cx_bool;
typedef int cx_char;
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
typedef void* cx_ptr;
typedef void (*cx_vtable_function)(void);

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
};

struct cx_interface_impl {
	cx_ptr typeInfo;
	cx_ptr vtable;
};

struct cx_reflection_field;
struct cx_reflection_function;

struct cx_runtime_type_info {
	const struct cx_interface_impl* interfaces;
	cx_uint interfaceCount;
	const struct cx_reflection_field* fields;
	cx_uint fieldCount;
	const struct cx_reflection_function* functions;
	cx_uint functionCount;
};

struct cx_reflection_field {
	cx_uint flags;
	cx_uint offset;
	cx_ptr typeInfo;
	const char* name;
};

struct cx_reflection_parameter {
	cx_uint flags;
	cx_ptr typeInfo;
	const char* name;
	cx_ptr defaultValue;
};

struct cx_reflection_function {
	cx_uint flags;
	cx_uint slot;
	cx_ptr returnTypeInfo;
	const char* name;
	const struct cx_reflection_parameter* parameters;
	cx_uint parameterCount;
};

static inline struct cx_iface_ref cx_iface_upcast(
	struct cx_iface_ref source,
	cx_uint vtableSlot)
{
	struct cx_iface_ref result = {
		source.vtable == (cx_ptr)0
			? (cx_ptr)0
			: ((union cx_vtable_entry*)source.vtable)[vtableSlot].data,
		source.instance
	};
	return result;
}

struct cx_exception {
	void* exceptionObject;
	const char* thrownFileName;
	int thrownLine;
};

struct cx_exception_frame {
	jmp_buf environment;
	struct cx_exception_frame* previous;
};

#endif // __CX_TYPES_H__

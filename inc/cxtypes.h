#ifndef __CX_TYPES_H__
#define __CX_TYPES_H__

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

struct cx_struct_ref {
	cx_ptr vtable;
	cx_ptr instance;
};

struct cx_iface_ref {
	cx_ptr vtable;
	cx_ptr instance;
};

#endif // __CX_TYPES_H__

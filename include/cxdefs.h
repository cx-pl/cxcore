#ifndef __CX_DEFS_H__
#define __CX_DEFS_H__

//
// Version
//

#define CX_RUNTIME_ABI_VERSION 1

//
// Compiler-specific attributes
//

#if defined(__GNUC__) || defined(__clang__)
#define CX_MAYBE_UNUSED __attribute__((unused))
#else
#define CX_MAYBE_UNUSED
#endif

#if defined(_MSC_VER)
#define CX_NORETURN __declspec(noreturn)
#else
#define CX_NORETURN __attribute__((noreturn))
#endif

//
// Import / Export
//

#if defined(_WIN32) || defined(WIN32)
#define CX_EXPORT __declspec(dllexport)
#define CX_IMPORT __declspec(dllimport)
#elif defined(__GNUC__) || defined(__clang__)
#define CX_EXPORT __attribute__((visibility("default")))
#define CX_IMPORT
#else
#define CX_EXPORT
#define CX_IMPORT
#endif

#if defined(CX_CXCORE_BUILD)
#define CX_API CX_EXPORT
#elif defined(CX_STATIC_LINK)
#define CX_API
#else
#define CX_API CX_IMPORT
#endif

//
// Native
//

#if defined(_WIN32)
#define CX_NATIVE_STDCALL __stdcall
#define CX_NATIVE_CDECL __cdecl
#else
#define CX_NATIVE_STDCALL
#define CX_NATIVE_CDECL
#endif
#define CX_NATIVE_DEFAULT
#if defined(_MSC_VER)
#include <malloc.h>
#define CX_NATIVE_STACK_ALLOC(size) _alloca((size_t)size)
#else
#define CX_NATIVE_STACK_ALLOC(size) __builtin_alloca((size_t)size)
#endif

//
// Constants
//

#define CX_FALSE 0
#define CX_TRUE 1
#define CX_NULL ((cx_ptr)0)

// Closed generic metadata can be instantiated independently by peer static libraries.
// The linker retains one definition for its stable type identity.
#if defined(CX_STATIC_LINK) && defined(_MSC_VER)
#define CX_GENERIC_DATA __declspec(selectany)
#elif defined(CX_STATIC_LINK) && (defined(__GNUC__) || defined(__clang__))
#define CX_GENERIC_DATA __attribute__((weak))
#else
#define CX_GENERIC_DATA
#endif

//
// Strings
//

#if defined(CX_DYNAMIC_MODULE)
#define CX_STRING_DEF(name, text) \
    struct cxcore_System_String name = { \
        ._length = sizeof(text) - 1, \
        ._data = text \
    }
#else
#define CX_STRING_DEF(name, text) \
    struct cxcore_System_String name = { \
        .__base = { \
            .__vtable = cxcore_System_String___vtable \
        }, \
        ._length = sizeof(text) - 1, \
        ._data = text \
    }
#endif

//
// vtable
//

#define CX_VTABLE_DECL(fullName) \
    extern CX_CURRENT_TYPE_API union cx_vtable_entry fullName ## ___vtable[]

#define CX_BEGIN_VTABLE_DEF(fullName) \
    union cx_vtable_entry fullName ## ___vtable[] = { \
        { .data = &fullName ## ___typeinfo },

#define CX_BEGIN_INTERFACE_VTABLE_DEF(name, dynamicTypeFullName) \
    union cx_vtable_entry name[] = { \
        { .data = &dynamicTypeFullName ## ___typeinfo },

#define CX_VTABLE_ENTRY(entry) \
    { .function = (cx_vtable_function)(entry) },

#define CX_END_VTABLE_DEF \
    }

#define CX_GET_VTABLE(obj) (((cx_ptr*)(obj))[0])

#define CX_INIT_VTABLE(obj, fullName) \
    CX_GET_VTABLE(obj) = fullName ## ___vtable

//
// TypeInfo
//

#define CX_TYPEINFO_NAME(fullName) \
    fullName ## ___typeinfo

#define CX_TYPEINFO_DECL(fullName) \
    extern CX_CURRENT_TYPE_API struct cxcore_System_Reflection_TypeInfo CX_TYPEINFO_NAME(fullName)

#define CX_GET_TYPEINFO(obj) (*((struct cxcore_System_Reflection_TypeInfo*)((union cx_vtable_entry*)CX_GET_VTABLE(obj))[0].data))

//
// Types
//

#define CX_TYPE_DEF(fullName) \
    CX_VTABLE_DECL(fullName); \
    CX_TYPEINFO_DECL(fullName); \
    struct fullName

#define CX_STATIC_TYPE_DEF(fullName) \
    CX_TYPEINFO_DECL(fullName)

//
// Reflection flags
//

#define CX_REFLECTION_FLAG_CONST_CALL               (0x01)
#define CX_REFLECTION_FLAG_STATIC                   (0x04)
#define CX_REFLECTION_FLAG_VIRTUAL                  (0x08)
#define CX_REFLECTION_FLAG_ABSTRACT                 (0x10)
#define CX_REFLECTION_FLAG_FINAL                    (0x20)
#define CX_REFLECTION_FLAG_EXTERN                   (0x40)
#define CX_REFLECTION_FLAG_GENERIC                  (0x80)

#define CX_REFLECTION_FLAG_VISIBILITY_HIDDEN        (0x00 << 8)
#define CX_REFLECTION_FLAG_VISIBILITY_PUBLIC        (0x01 << 8)
#define CX_REFLECTION_FLAG_VISIBILITY_PROTECTED     (0x02 << 8)
#define CX_REFLECTION_FLAG_VISIBILITY_PRIVATE       (0x04 << 8)
#define CX_REFLECTION_FLAG_VISIBILITY_INTERNAL      (0x08 << 8)

#define CX_REFLECTION_FLAG_TYPE_CLASS               (0x01 << 16)
#define CX_REFLECTION_FLAG_TYPE_STRUCT              (0x02 << 16)
#define CX_REFLECTION_FLAG_TYPE_INTERFACE           (0x04 << 16)
#define CX_REFLECTION_FLAG_TYPE_ENUM                (0x08 << 16)
#define CX_REFLECTION_FLAG_TYPE_DELEGATE            (0x10 << 16)
#define CX_REFLECTION_FLAG_TYPE_GENERIC             (0x80 << 16)

#define CX_REFLECTION_FLAG_FUNCTION_CONSTRUCTOR     (0x01 << 24)
#define CX_REFLECTION_FLAG_FUNCTION_NO_ESCAPE       (0x02 << 24) /* non-retaining receiver/arguments */
#define CX_REFLECTION_FLAG_FUNCTION_OPERATOR        (0x04 << 24)
#define CX_REFLECTION_FLAG_FUNCTION_PROPERTY_GET    (0x10 << 24)
#define CX_REFLECTION_FLAG_FUNCTION_PROPERTY_SET    (0x20 << 24)

#define CX_REFLECTION_FLAG_FIELD_POINTER            (0x01 << 28)
#define CX_REFLECTION_FLAG_FIELD_FUNCTION_VALUE     (0x02 << 28)
#define CX_REFLECTION_FLAG_FIELD_RAW_BYTES          (0x04 << 28)
#define CX_REFLECTION_FLAG_FIELD_NATIVE_LAYOUT      (0x08 << 28)

#define CX_REFLECTION_NO_OFFSET ((cx_uint)-1)
#define CX_REFLECTION_NO_SLOT ((cx_uint)-1)

//
// Exceptions
//

#define CX_THROW(ex) cx_exception_throw((cx_ptr)(ex), __FILE__, __LINE__)
#define CX_RETHROW() cx_exception_rethrow()

#endif // __CX_DEFS_H__

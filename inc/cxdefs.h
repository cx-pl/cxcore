#ifndef __CX_DEFS_H__
#define __CX_DEFS_H__

//
// Naming
//

#define CX_ID_1(_1) \
    _1
#define CX_ID_2(_1, _2) \
    _1 ## _ ## _2
#define CX_ID_3(_1, _2, _3) \
    _1 ## _ ## _2 ## _ ## _3
#define CX_ID_4(_1, _2, _3, _4) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4
#define CX_ID_5(_1, _2, _3, _4, _5) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5
#define CX_ID_6(_1, _2, _3, _4, _5, _6) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6
#define CX_ID_7(_1, _2, _3, _4, _5, _6, _7) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7
#define CX_ID_8(_1, _2, _3, _4, _5, _6, _7, _8) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7 ## _ ## _8
#define CX_ID_9(_1, _2, _3, _4, _5, _6, _7, _8, _9) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7 ## _ ## _8 ## _ ## _9
#define CX_ID_10(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7 ## _ ## _8 ## _ ## _9 ## _ ## _10
#define CX_ID_11(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7 ## _ ## _8 ## _ ## _9 ## _ ## _10 ## _ ## \
    _11
#define CX_ID_12(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7 ## _ ## _8 ## _ ## _9 ## _ ## _10 ## _ ## \
    _11 ## _ ## _12
#define CX_ID_13(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7 ## _ ## _8 ## _ ## _9 ## _ ## _10 ## _ ## \
    _11 ## _ ## _12 ## _ ## _13
#define CX_ID_14(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7 ## _ ## _8 ## _ ## _9 ## _ ## _10 ## _ ## \
    _11 ## _ ## _12 ## _ ## _13 ## _ ## _14
#define CX_ID_15(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7 ## _ ## _8 ## _ ## _9 ## _ ## _10 ## _ ## \
    _11 ## _ ## _12 ## _ ## _13 ## _ ## _14 ## _ ## _15
#define CX_ID_16(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7 ## _ ## _8 ## _ ## _9 ## _ ## _10 ## _ ## \
    _11 ## _ ## _12 ## _ ## _13 ## _ ## _14 ## _ ## _15 ## _ ## _16
#define CX_ID_17(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7 ## _ ## _8 ## _ ## _9 ## _ ## _10 ## _ ## \
    _11 ## _ ## _12 ## _ ## _13 ## _ ## _14 ## _ ## _15 ## _ ## _16 ## _ ## _17
#define CX_ID_18(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7 ## _ ## _8 ## _ ## _9 ## _ ## _10 ## _ ## \
    _11 ## _ ## _12 ## _ ## _13 ## _ ## _14 ## _ ## _15 ## _ ## _16 ## _ ## _17 ## _ ## _18
#define CX_ID_19(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7 ## _ ## _8 ## _ ## _9 ## _ ## _10 ## _ ## \
    _11 ## _ ## _12 ## _ ## _13 ## _ ## _14 ## _ ## _15 ## _ ## _16 ## _ ## _17 ## _ ## _18 ## _ ## _19
#define CX_ID_20(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7 ## _ ## _8 ## _ ## _9 ## _ ## _10 ## _ ## \
    _11 ## _ ## _12 ## _ ## _13 ## _ ## _14 ## _ ## _15 ## _ ## _16 ## _ ## _17 ## _ ## _18 ## _ ## _19 ## _ ## _20
#define CX_ID_21(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7 ## _ ## _8 ## _ ## _9 ## _ ## _10 ## _ ## \
    _11 ## _ ## _12 ## _ ## _13 ## _ ## _14 ## _ ## _15 ## _ ## _16 ## _ ## _17 ## _ ## _18 ## _ ## _19 ## _ ## _20 ## _ ## \
    _21
#define CX_ID_22(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7 ## _ ## _8 ## _ ## _9 ## _ ## _10 ## _ ## \
    _11 ## _ ## _12 ## _ ## _13 ## _ ## _14 ## _ ## _15 ## _ ## _16 ## _ ## _17 ## _ ## _18 ## _ ## _19 ## _ ## _20 ## _ ## \
    _21 ## _ ## _22
#define CX_ID_23(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7 ## _ ## _8 ## _ ## _9 ## _ ## _10 ## _ ## \
    _11 ## _ ## _12 ## _ ## _13 ## _ ## _14 ## _ ## _15 ## _ ## _16 ## _ ## _17 ## _ ## _18 ## _ ## _19 ## _ ## _20 ## _ ## \
    _21 ## _ ## _22 ## _ ## _23
#define CX_ID_24(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7 ## _ ## _8 ## _ ## _9 ## _ ## _10 ## _ ## \
    _11 ## _ ## _12 ## _ ## _13 ## _ ## _14 ## _ ## _15 ## _ ## _16 ## _ ## _17 ## _ ## _18 ## _ ## _19 ## _ ## _20 ## _ ## \
    _21 ## _ ## _22 ## _ ## _23 ## _ ## _24
#define CX_ID_25(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7 ## _ ## _8 ## _ ## _9 ## _ ## _10 ## _ ## \
    _11 ## _ ## _12 ## _ ## _13 ## _ ## _14 ## _ ## _15 ## _ ## _16 ## _ ## _17 ## _ ## _18 ## _ ## _19 ## _ ## _20 ## _ ## \
    _21 ## _ ## _22 ## _ ## _23 ## _ ## _24 ## _ ## _25
#define CX_ID_26(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7 ## _ ## _8 ## _ ## _9 ## _ ## _10 ## _ ## \
    _11 ## _ ## _12 ## _ ## _13 ## _ ## _14 ## _ ## _15 ## _ ## _16 ## _ ## _17 ## _ ## _18 ## _ ## _19 ## _ ## _20 ## _ ## \
    _21 ## _ ## _22 ## _ ## _23 ## _ ## _24 ## _ ## _25 ## _ ## _26
#define CX_ID_27(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7 ## _ ## _8 ## _ ## _9 ## _ ## _10 ## _ ## \
    _11 ## _ ## _12 ## _ ## _13 ## _ ## _14 ## _ ## _15 ## _ ## _16 ## _ ## _17 ## _ ## _18 ## _ ## _19 ## _ ## _20 ## _ ## \
    _21 ## _ ## _22 ## _ ## _23 ## _ ## _24 ## _ ## _25 ## _ ## _26 ## _ ## _27
#define CX_ID_28(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7 ## _ ## _8 ## _ ## _9 ## _ ## _10 ## _ ## \
    _11 ## _ ## _12 ## _ ## _13 ## _ ## _14 ## _ ## _15 ## _ ## _16 ## _ ## _17 ## _ ## _18 ## _ ## _19 ## _ ## _20 ## _ ## \
    _21 ## _ ## _22 ## _ ## _23 ## _ ## _24 ## _ ## _25 ## _ ## _26 ## _ ## _27 ## _ ## _28
#define CX_ID_29(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7 ## _ ## _8 ## _ ## _9 ## _ ## _10 ## _ ## \
    _11 ## _ ## _12 ## _ ## _13 ## _ ## _14 ## _ ## _15 ## _ ## _16 ## _ ## _17 ## _ ## _18 ## _ ## _19 ## _ ## _20 ## _ ## \
    _21 ## _ ## _22 ## _ ## _23 ## _ ## _24 ## _ ## _25 ## _ ## _26 ## _ ## _27 ## _ ## _28 ## _ ## _29
#define CX_ID_30(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7 ## _ ## _8 ## _ ## _9 ## _ ## _10 ## _ ## \
    _11 ## _ ## _12 ## _ ## _13 ## _ ## _14 ## _ ## _15 ## _ ## _16 ## _ ## _17 ## _ ## _18 ## _ ## _19 ## _ ## _20 ## _ ## \
    _21 ## _ ## _22 ## _ ## _23 ## _ ## _24 ## _ ## _25 ## _ ## _26 ## _ ## _27 ## _ ## _28 ## _ ## _29 ## _ ## _30
#define CX_ID_31(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7 ## _ ## _8 ## _ ## _9 ## _ ## _10 ## _ ## \
    _11 ## _ ## _12 ## _ ## _13 ## _ ## _14 ## _ ## _15 ## _ ## _16 ## _ ## _17 ## _ ## _18 ## _ ## _19 ## _ ## _20 ## _ ## \
    _21 ## _ ## _22 ## _ ## _23 ## _ ## _24 ## _ ## _25 ## _ ## _26 ## _ ## _27 ## _ ## _28 ## _ ## _29 ## _ ## _30 ## _ ## \
    _31
#define CX_ID_32(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32) \
    _1 ## _ ## _2 ## _ ## _3 ## _ ## _4 ## _ ## _5 ## _ ## _6 ## _ ## _7 ## _ ## _8 ## _ ## _9 ## _ ## _10 ## _ ## \
    _11 ## _ ## _12 ## _ ## _13 ## _ ## _14 ## _ ## _15 ## _ ## _16 ## _ ## _17 ## _ ## _18 ## _ ## _19 ## _ ## _20 ## _ ## \
    _21 ## _ ## _22 ## _ ## _23 ## _ ## _24 ## _ ## _25 ## _ ## _26 ## _ ## _27 ## _ ## _28 ## _ ## _29 ## _ ## _30 ## _ ## \
    _31 ## _ ## _32
    
//
// Import / Export
//

#define CX_EXPORT __declspec(dllexport)
#define CX_IMPORT __declspec(dllimport)

//
// Constants
//

#define CX_FALSE 0
#define CX_TRUE 1
#define CX_NULL ((cx_ptr)0)

//
// Strings
//

#define CX_STRING_DECL(name) \
    extern struct CX_ID_3(cxcore, System, String) name

#define CX_STRING_DEF(name, text) \
    struct CX_ID_3(cxcore, System, String) name = { \
        .__base = { \
            .__vtable = &CX_ID_4(cxcore, System, String, __vtable) \
        }, \
        ._length = sizeof(text) - 1, \
        ._data = text \
    }

//
// Arrays
//

//
// vtable
//

#define CX_VTABLE_DECL(fullName) \
    extern cx_ptr CX_ID_2(fullName, __vtable)

#define CX_BEGIN_VTABLE_DEF(fullName) \
    cx_ptr CX_ID_2(fullName, __vtable) = { \
        &CX_ID_2(fullName, __typeinfo),

#define CX_VTABLE_ENTRY(entry) \
    &entry,

#define CX_END_VTABLE_DEF \
    }

#define CX_GET_VTABLE(obj) (((cx_ptr*)(obj))[0])

//
// TypeInfo
//

#define CX_TYPEINFO_NAME(fullName) \
    CX_ID_2(fullName, __typeinfo)

#define CX_TYPEINFO_DECL(fullName) \
    extern struct CX_ID_4(cxcore, System, Reflection, TypeInfo) CX_TYPEINFO_NAME(fullName)

#define CX_STRUCT_TYPEINFO_DEF(fullName, nameString, namespaceString, hash, flags) \
    struct CX_ID_4(cxcore, System, Reflection, TypeInfo) CX_TYPEINFO_NAME(fullName) = { \
        .Hash = hash,\
        .Flags = flags, \
        .Size = sizeof(struct fullName), \
        .Name = &nameString, \
        .Namespace = &namespaceString, \
        /* TODO: .GenericParams = CX_ID_4(_, fullName, __typeinfo, _genericParamsArray),*/ \
        .BaseType = CX_NULL, \
        /* TODO: .Interfaces = CX_ID_4(_, fullName, __typeinfo, _interfacesArray),*/ \
        /* TODO: .Fields = CX_ID_4(_, fullName, __typeinfo, _fieldsArray),*/ \
        /* TODO: .Functions = CX_ID_4(_, fullName, __typeinfo, _functionsArray)*/ \
    }

#define CX_CLASS_TYPEINFO_DEF(fullName, nameString, namespaceString, baseType, hash, flags) \
    struct CX_ID_4(cxcore, System, Reflection, TypeInfo) CX_TYPEINFO_NAME(fullName) = { \
        .Hash = hash,\
        .Flags = flags, \
        .Size = sizeof(struct fullName), \
        .Name = &nameString, \
        .Namespace = &namespaceString, \
        /* TODO: .GenericParams = CX_ID_4(_, fullName, __typeinfo, _genericParamsArray),*/ \
        .BaseType = { \
            ._obj = baseType\
        },\
        /* TODO: .Interfaces = CX_ID_4(_, fullName, __typeinfo, _interfacesArray),*/ \
        /* TODO: .Fields = CX_ID_4(_, fullName, __typeinfo, _fieldsArray),*/ \
        /* TODO: .Functions = CX_ID_4(_, fullName, __typeinfo, _functionsArray)*/ \
    }

#define CX_TYPEINFO_GENERIC_PARAMS_EMPTY \
    struct CX_ID_3(cxcore, System, Array) CX_ID_4(_, fullName, __typeinfo, _genericParamsArray) = CX_ARRAY_EMPTY

#define CX_GET_TYPEINFO(obj) (((struct CX_ID_4(cxcore, System, Reflection, TypeInfo)*)CX_GET_VTABLE(obj))[0])

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

#define CX_REFLECTION_FLAG_CONST_CALL               0x01
#define CX_REFLECTION_FLAG_CONST_TYPE               0x02
#define CX_REFLECTION_FLAG_STATIC                   0x04
#define CX_REFLECTION_FLAG_VIRTUAL                  0x08
#define CX_REFLECTION_FLAG_ABSTRACT                 0x10
#define CX_REFLECTION_FLAG_FINAL                    0x20
#define CX_REFLECTION_FLAG_EXTERN                   0x40
#define CX_REFLECTION_FLAG_GENERIC                  0x80

#define CX_REFLECTION_FLAG_VISIBILITY_HIDDEN        0x00 << 8
#define CX_REFLECTION_FLAG_VISIBILITY_PUBLIC        0x01 << 8
#define CX_REFLECTION_FLAG_VISIBILITY_PROTECTED     0x02 << 8
#define CX_REFLECTION_FLAG_VISIBILITY_PRIVATE       0x04 << 8
#define CX_REFLECTION_FLAG_VISIBILITY_INTERNAL      0x08 << 8
#define CX_REFLECTION_FLAG_GENERIC_PARAM_BASE       0x10 << 8
#define CX_REFLECTION_FLAG_GENERIC_PARAM_SPEC       0x20 << 8

#define CX_REFLECTION_FLAG_TYPE_CLASS               0x01 << 16
#define CX_REFLECTION_FLAG_TYPE_STRUCT              0x02 << 16
#define CX_REFLECTION_FLAG_TYPE_INTERFACE           0x04 << 16
#define CX_REFLECTION_FLAG_TYPE_ENUM                0x08 << 16
#define CX_REFLECTION_FLAG_TYPE_CUSTOM              0x40 << 16
#define CX_REFLECTION_FLAG_TYPE_GENERIC             0x80 << 16

#define CX_REFLECTION_FLAG_FUNCTION_CONSTRUCTOR     0x01 << 24
#define CX_REFLECTION_FLAG_FUNCTION_DESTRUCTOR      0x02 << 24
#define CX_REFLECTION_FLAG_FUNCTION_OPERATOR        0x04 << 24
#define CX_REFLECTION_FLAG_FUNCTION_PROPERTY_GET    0x10 << 24
#define CX_REFLECTION_FLAG_FUNCTION_PROPERTY_SET    0x20 << 24

#endif // __CX_DEFS_H__

#ifndef __CX_PARSE_API_H__
#define __CX_PARSE_API_H__

// Included by cxcore.h after its public value layouts.
CX_TYPE_DEF(cxcore_System_Decimal) { cx_decimal _value; };
extern CX_API cx_decimal cxcore_System_Decimal_MinValue;
extern CX_API cx_decimal cxcore_System_Decimal_MaxValue;

#define CX_TRY_PARSE_DECL(type) \
    CX_API struct cxcore_System_Nullable cxcore_System_ ## type ## _TryParse( \
        const struct cxcore_System_String *text)
CX_TRY_PARSE_DECL(Byte);
CX_TRY_PARSE_DECL(SByte);
CX_TRY_PARSE_DECL(Short);
CX_TRY_PARSE_DECL(UShort);
CX_TRY_PARSE_DECL(Int);
CX_TRY_PARSE_DECL(UInt);
CX_TRY_PARSE_DECL(Long);
CX_TRY_PARSE_DECL(ULong);
CX_TRY_PARSE_DECL(Float);
CX_TRY_PARSE_DECL(Double);
CX_TRY_PARSE_DECL(Decimal);
#undef CX_TRY_PARSE_DECL

extern CX_API const struct cxcore_System_Uuid cxcore_System_Uuid_NamespaceDns;
extern CX_API const struct cxcore_System_Uuid cxcore_System_Uuid_NamespaceUrl;
extern CX_API const struct cxcore_System_Uuid cxcore_System_Uuid_NamespaceOid;
extern CX_API const struct cxcore_System_Uuid cxcore_System_Uuid_NamespaceX500;
#define CX_UUID_NEW_DECL(version) \
    CX_API struct cxcore_System_Uuid cxcore_System_Uuid_NewUuidV##version(void)
CX_UUID_NEW_DECL(1);
CX_UUID_NEW_DECL(4);
CX_UUID_NEW_DECL(6);
CX_UUID_NEW_DECL(7);
CX_UUID_NEW_DECL(8);
#undef CX_UUID_NEW_DECL
CX_API struct cxcore_System_Uuid cxcore_System_Uuid_NewUuidV2(cx_byte domain, cx_uint identifier);
CX_API struct cxcore_System_Uuid cxcore_System_Uuid_NewUuidV3(
    struct cxcore_System_Uuid namespaceId, const struct cxcore_System_String *name);
CX_API struct cxcore_System_Uuid cxcore_System_Uuid_NewUuidV5(
    struct cxcore_System_Uuid namespaceId, const struct cxcore_System_String *name);
CX_API struct cxcore_System_Uuid cxcore_System_Uuid_NewUuidV8__2(
    cx_ulong customA, cx_ushort customB, cx_ulong customC);
CX_API cx_uint cxcore_System_Uuid_Version___const_get(const struct cxcore_System_Uuid *self);
CX_API struct cxcore_System_String *cxcore_System_Uuid_ToString(
    const struct cxcore_System_Uuid *self);

#endif // __CX_PARSE_API_H__

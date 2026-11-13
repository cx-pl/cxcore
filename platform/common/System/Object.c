#include "../../../src/cxcore.h"

#include <stdint.h>

struct cxcore_System_Reflection_TypeInfo
    cxcore_System_Object_Type___const_get(const struct cxcore_System_Object * __this) {
    const struct cxcore_System_Reflection_TypeInfo *boxedType =
        cx_boxed_value_type((cx_ptr)__this);
    struct cxcore_System_Reflection_TypeInfo result = {0};
    if (boxedType != CX_NULL) {
        cx_reflection_copy_type_info(&result, boxedType);
    } else {
        cx_reflection_copy_type_info(&result, &CX_GET_TYPEINFO(__this));
    }
    return result;
}

struct cxcore_System_String *cxcore_System_Object_ToString(const struct cxcore_System_Object * __this) {
    const struct cxcore_System_Reflection_TypeInfo typeInfo =
        cxcore_System_Object_Type___const_get(__this);
    return (struct cxcore_System_String *)typeInfo.Name;
}

cx_bool cxcore_System_Object_Equals(
    const struct cxcore_System_Object * __this,
    struct cxcore_System_Object *other) {
    return __this == other;
}

cx_int cxcore_System_Object_GetHashCode(
    const struct cxcore_System_Object * __this) {
    const cx_ulong identity = (cx_ulong)(uintptr_t)__this;
    return (cx_int)(identity ^ (identity >> 32));
}

cx_bool cxcore_System_Object_ReferenceEquals(
    struct cxcore_System_Object *left,
    struct cxcore_System_Object *right) {
    return left == right;
}

cx_int cxcore_System_Object_IdentityHashCode(
    struct cxcore_System_Object *value) {
    const cx_ulong identity = (cx_ulong)(uintptr_t)value;
    return (cx_int)(identity ^ (identity >> 32));
}

void cxcore_System_Object___constructor(struct cxcore_System_Object * __this) {
    CX_INIT_VTABLE(__this, cxcore_System_Object);

    cx_gc_store_reference(&__this->_box, CX_NULL);
}

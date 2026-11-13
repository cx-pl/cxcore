#include <assert.h>

#include "../src/cxcore.h"

int main(void) {
    const cx_int sourceValue = 42;
    struct cxcore_System_Object *boxed =
        (struct cxcore_System_Object *)cx_box_value(
            &sourceValue, (cx_uint)sizeof(sourceValue),
            &cxcore_System_Int___typeinfo);
    const struct cxcore_System_Reflection_TypeInfo *boxedType;

    assert(boxed != CX_NULL);
    boxedType = cx_boxed_value_type(boxed);
    assert(boxedType != CX_NULL && boxedType->Hash ==
        cxcore_System_Int___typeinfo.Hash);
    assert(cxcore_System_Object_Type___const_get(boxed).Hash ==
        cxcore_System_Int___typeinfo.Hash);
    assert(*(const cx_int *)cx_unbox_value(
        boxed, &cxcore_System_Int___typeinfo) == sourceValue);
    return 0;
}

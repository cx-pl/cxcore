#include <assert.h>

#include "../src/cxcore.h"

int main(void) {
    const cx_int sourceValue = 42;
    struct CX_ID_3(cxcore, System, Object) *boxed =
        (struct CX_ID_3(cxcore, System, Object) *)cx_box_value(
            &sourceValue, (cx_uint)sizeof(sourceValue),
            &CX_ID_4(cxcore, System, Int, __typeinfo));
    const struct CX_ID_4(cxcore, System, Reflection, TypeInfo) *boxedType;

    assert(boxed != CX_NULL);
    boxedType = cx_boxed_value_type(boxed);
    assert(boxedType != CX_NULL && boxedType->Hash ==
        CX_ID_4(cxcore, System, Int, __typeinfo).Hash);
    assert(CX_ID_5(cxcore, System, Object, Type, __const_get)(boxed).Hash ==
        CX_ID_4(cxcore, System, Int, __typeinfo).Hash);
    assert(*(const cx_int *)cx_unbox_value(
        boxed, &CX_ID_4(cxcore, System, Int, __typeinfo)) == sourceValue);
    return 0;
}

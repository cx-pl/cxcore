#include "../../../src/cxcore.h"
#include <stdlib.h>

struct cxcore_System_Nullable _null = {CX_NULL};

void cxcore_System_Nullable_Null___const_get(void *__returnValue) {
    *(struct cxcore_System_Nullable *)__returnValue = _null;
}

cx_bool cxcore_System_Nullable_HasValue___const_get(const struct cxcore_System_Nullable * __this) {
    return __this->_obj != CX_NULL;
}

void cxcore_System_Nullable_Value___const_get(const struct cxcore_System_Nullable * __this,
                          void *__returnValue) {
    struct cxcore_System_Nullable *destination =
        (struct cxcore_System_Nullable *)__returnValue;
    (void)cx_gc_store_reference(&destination->_obj, __this->_obj);
}

void cxcore_System_Nullable_Value___set(struct cxcore_System_Nullable * __this, void *value) {
    cx_gc_store_reference(&__this->_obj, value);
}

void cxcore_System_Nullable___constructor(struct cxcore_System_Nullable * __this, cx_ptr obj) {
    cx_gc_store_reference(&__this->_obj, obj);
}

cx_ptr cxcore_System_Nullable_CreateValueStorage(cx_ptr value, cx_uint size) {
    cx_ptr storage = cxcore_System_Memory_Alloc(size);
    cxcore_System_Memory_Copy(storage, value, size);
    return storage;
}

cx_ptr cxcore_System_Nullable_CreateTypedValueStorage(
    cx_ptr value, cx_uint size,
    const struct cxcore_System_Reflection_TypeInfo *typeInfo) {
    cx_ptr storage;
    if (value == CX_NULL || typeInfo == CX_NULL || typeInfo->Size != size) abort();
    storage = cxcore_System_Memory_Alloc(size);
    cx_gc_copy_value(storage, value, typeInfo);
    return storage;
}

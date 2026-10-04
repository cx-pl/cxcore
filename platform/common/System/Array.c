#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include "../../../src/cxcore.h"

extern union cx_vtable_entry CX_ID_4(cxcore, System, Array, __vtable)[];

CX_EXPORT struct CX_ID_3(cxcore, System, Array) *
    cx_array_new(cx_uint length, cx_uint elementSize) {
    struct CX_ID_3(cxcore, System, Array) * array;

    if (length != 0 && elementSize == 0) {
        abort();
    }
    if (length != 0) {
        if ((size_t)length > SIZE_MAX / (size_t)elementSize || length > UINT_MAX / elementSize) {
            abort();
        }
    }

    array = (struct CX_ID_3(cxcore, System, Array) *)CX_ID_4(cxcore, System, Memory,
                                                             Alloc)((cx_uint)sizeof(*array));
    array->__base.__vtable = CX_ID_4(cxcore, System, Array, __vtable);
    array->_length = length;
    if (length != 0) {
        array->_data = CX_ID_4(cxcore, System, Memory, Alloc)(length * elementSize);
    }

    return array;
}

CX_EXPORT cx_uint CX_ID_5(cxcore, System, Array, Length, __const_get)(
    const struct CX_ID_3(cxcore, System, Array) *__this) {
    return __this == NULL ? 0 : __this->_length;
}

CX_EXPORT cx_ptr cx_array_at(struct CX_ID_3(cxcore, System, Array) * array, cx_uint index,
                             cx_uint elementSize) {
    if (array == NULL || index >= array->_length || (elementSize == 0 && array->_length != 0) ||
        (elementSize != 0 && (size_t)index > SIZE_MAX / (size_t)elementSize) ||
        (array->_length != 0 && array->_data == NULL)) {
        return CX_NULL;
    }

    return (cx_ptr)((cx_byte *)array->_data + index * elementSize);
}

#include <stdlib.h>
#include "../../../src/cxcore.h"

extern cx_ptr CX_ID_4(cxcore, System, Array, __vtable);

CX_EXPORT struct CX_ID_3(cxcore, System, Array)* cx_array_new(
    cx_uint length,
    cx_uint elementSize
) {
    struct CX_ID_3(cxcore, System, Array)* array = calloc(1, sizeof(*array));
    if (array == NULL) {
        abort();
    }

    array->__base.__vtable = &CX_ID_4(cxcore, System, Array, __vtable);
    array->_length = length;
    if (length != 0) {
        array->_data = calloc(length, elementSize);
        if (array->_data == NULL) {
            free(array);
            abort();
        }
    }

    return array;
}

CX_EXPORT cx_ptr cx_array_at(
    struct CX_ID_3(cxcore, System, Array)* array,
    cx_uint index,
    cx_uint elementSize
) {
    if (array == NULL || index >= array->_length) {
        abort();
    }

    return (cx_ptr)((cx_byte*)array->_data + index * elementSize);
}

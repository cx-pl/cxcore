#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include "../../../../src/cxcore.h"
#include "../../cxarena.h"
#include "../../cxgc.h"

extern union cx_vtable_entry cxcore_System_Collections_Array___vtable[];

CX_EXPORT struct cxcore_System_Collections_Array *
    cx_array_new(cx_uint length, cx_uint elementSize) {
    struct cxcore_System_Collections_Array * array;

    if (length != 0 && elementSize == 0) {
        abort();
    }
    if (length != 0) {
        if ((size_t)length > SIZE_MAX / (size_t)elementSize || length > UINT_MAX / elementSize) {
            abort();
        }
    }

    array = (struct cxcore_System_Collections_Array *)cxcore_System_Memory_Alloc((cx_uint)sizeof(*array));
    array->__base.__vtable = cxcore_System_Collections_Array___vtable;
    array->_length = length;
    if (length != 0) {
        cx_gc_store_reference(
            (cx_ptr *)&array->_data,
            cxcore_System_Memory_Alloc(length * elementSize));
    }

    return array;
}

CX_EXPORT struct cxcore_System_Collections_Array *
    cx_array_new_for_owner(const void *owner, cx_uint length,
                           cx_uint elementSize) {
    struct cxcore_System_Collections_Array *array;

    if (owner == NULL || (length != 0 && elementSize == 0)) abort();
    if (length != 0 &&
        ((size_t)length > SIZE_MAX / (size_t)elementSize ||
         length > UINT_MAX / elementSize)) {
        abort();
    }

    array = (struct cxcore_System_Collections_Array *)cx_gc_alloc_for_owner(
        owner, sizeof(*array), cx_arena_maximum_alignment());
    array->__base.__vtable = cxcore_System_Collections_Array___vtable;
    array->_length = length;
    if (length != 0) {
        cx_gc_store_reference(
            (cx_ptr *)&array->_data,
            cx_gc_alloc_for_owner(array, (size_t)length * elementSize,
                                  cx_arena_maximum_alignment()));
    }
    return array;
}

CX_EXPORT cx_ptr cx_array_at(struct cxcore_System_Collections_Array * array, cx_uint index,
                             cx_uint elementSize) {
    if (array == NULL || index >= array->_length || (elementSize == 0 && array->_length != 0) ||
        (elementSize != 0 && (size_t)index > SIZE_MAX / (size_t)elementSize) ||
        (array->_length != 0 && array->_data == NULL)) {
        return CX_NULL;
    }

    return (cx_ptr)((cx_byte *)array->_data + index * elementSize);
}

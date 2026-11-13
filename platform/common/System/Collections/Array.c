#include <limits.h>
#include <stdint.h>
#include <stdio.h>
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

CX_EXPORT cx_ptr cx_array_at_checked(
    struct cxcore_System_Collections_Array *array,
    cx_uint index,
    cx_uint elementSize,
    const char *fileName,
    int line) {
    struct cxcore_System_Exception *exception;
    if (array == NULL) {
        exception = (struct cxcore_System_Exception *)cx_exception_allocate_durable(
            cxcore_System_NullReferenceException___typeinfo.Size);
        cxcore_System_NullReferenceException___constructor(
            (struct cxcore_System_NullReferenceException *)exception);
        cx_exception_throw(exception, fileName, line);
    }
    if (index >= array->_length) {
        exception = (struct cxcore_System_Exception *)cx_exception_allocate_durable(
            cxcore_System_IndexOutOfRangeException___typeinfo.Size);
        cxcore_System_IndexOutOfRangeException___constructor(
            (struct cxcore_System_IndexOutOfRangeException *)exception);
        cx_exception_throw(exception, fileName, line);
    }
    if ((elementSize == 0u && array->_length != 0u) ||
        (elementSize != 0u &&
            ((size_t)index > SIZE_MAX / (size_t)elementSize ||
             index > UINT_MAX / elementSize)) ||
        (array->_length != 0u && array->_data == NULL)) {
        fputs("Invalid managed array layout during checked access\n", stderr);
        abort();
    }
    return (cx_ptr)((cx_byte *)array->_data + (size_t)index * elementSize);
}

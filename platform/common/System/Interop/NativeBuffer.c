#include "cxcore.h"
#include "../Text.internal.h"
#include "../Utf8.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef struct CX_ID_3(cxcore, System, WideString) cx_wide_string;

typedef struct CX_ID_4(cxcore, System, Interop, NativeBuffer) cx_native_buffer;

cx_native_buffer *CX_ID_5(cxcore, System, Interop, NativeBuffer, Allocate)(cx_ulong length) {
    cx_native_buffer *result;
    cx_gc_check_thread();
    if (length > SIZE_MAX) cx_text_error();
    result = CX_ID_4(cxcore, System, Memory, Alloc)((cx_uint)sizeof(*result));
    CX_ID_5(cxcore, System, Interop, NativeBuffer, __constructor)(result);
    result->_data = malloc(length == 0 ? 1 : (size_t)length);
    if (result->_data == NULL) cx_text_error();
    result->_allocation = result->_data;
    result->_byteLength = length;
    return result;
}
cx_native_buffer *CX_ID_5(cxcore, System, Interop, NativeBuffer, AllocateAligned)(cx_ulong length, cx_uint alignment) {
    cx_native_buffer *result;
    size_t capacity;
    cx_gc_check_thread();
    if (alignment == 0 || alignment > 1024 * 1024 || (alignment & (alignment - 1)) != 0 ||
        length > SIZE_MAX - alignment) cx_text_error();
    result = CX_ID_4(cxcore, System, Memory, Alloc)((cx_uint)sizeof(*result));
    CX_ID_5(cxcore, System, Interop, NativeBuffer, __constructor)(result);
    capacity = (size_t)(length == 0 ? 1 : length) + alignment - 1;
    result->_allocation = malloc(capacity);
    if (result->_allocation == NULL) cx_text_error();
    result->_data = cx_interop_align_pointer(result->_allocation, alignment);
    result->_byteLength = length;
    return result;
}
cx_ptr CX_ID_6(cxcore, System, Interop, NativeBuffer, Data, __const_get)(const cx_native_buffer *self) { return self->_data; }
cx_ulong CX_ID_6(cxcore, System, Interop, NativeBuffer, ByteLength, __const_get)(const cx_native_buffer *self) { return self->_byteLength; }
void CX_ID_5(cxcore, System, Interop, NativeBuffer, Dispose)(cx_native_buffer *self) {
    cx_gc_check_thread();
    free(self->_allocation); self->_allocation = NULL; self->_data = NULL; self->_byteLength = 0;
}
void CX_ID_5(cxcore, System, Interop, NativeBuffer, Copy)(cx_ptr destination, cx_ptr source, cx_ulong length) {
    cx_gc_check_thread();
    if (length > SIZE_MAX || (length != 0 && (destination == NULL || source == NULL))) cx_text_error();
    if (length != 0) memmove(destination, source, (size_t)length);
}
cx_native_buffer *CX_ID_5(cxcore, System, Interop, NativeBuffer, FromUtf8)(
    struct CX_ID_3(cxcore, System, String) *value, cx_bool terminated) {
    cx_native_buffer *result;
    if (value == NULL) cx_text_error();
    result = CX_ID_5(cxcore, System, Interop, NativeBuffer, Allocate)((cx_ulong)value->_length + (terminated ? 1 : 0));
    if (value->_length != 0) memcpy(result->_data, value->_data, value->_length);
    if (terminated) ((cx_byte *)result->_data)[value->_length] = 0;
    return result;
}
cx_native_buffer *CX_ID_5(cxcore, System, Interop, NativeBuffer, FromUtf16)(cx_wide_string *value, cx_bool terminated) {
    cx_native_buffer *result;
    if (value == NULL) cx_text_error();
    result = CX_ID_5(cxcore, System, Interop, NativeBuffer, Allocate)(((cx_ulong)value->_length + (terminated ? 1 : 0)) * sizeof(cx_wchar));
    if (value->_length != 0) memcpy(result->_data, value->_data, value->_length * sizeof(cx_wchar));
    if (terminated) ((cx_wchar *)result->_data)[value->_length] = 0;
    return result;
}

struct CX_ID_3(cxcore, System, String) *CX_ID_5(cxcore, System, Interop, NativeBuffer, ToUtf8String)(
    const cx_native_buffer *self, cx_uint length) {
    cx_uint scalar;
    size_t offset = 0, count;
    cx_byte *data;
    struct CX_ID_3(cxcore, System, String) *result;
    cx_gc_check_thread();
    if (self == NULL || length > self->_byteLength || (length != 0 && self->_data == NULL)) cx_text_error();
    while (offset < length) {
        if (!cx_utf8_decode((const cx_byte *)self->_data + offset, length - offset, &scalar, &count)) cx_encoding_error((cx_uint)offset);
        offset += count;
    }
    data = length == 0 ? NULL : CX_ID_4(cxcore, System, Memory, Alloc)(length);
    if (length != 0) memcpy(data, self->_data, length);
    result = CX_ID_4(cxcore, System, Memory, Alloc)((cx_uint)sizeof(*result));
    CX_ID_5(cxcore, System, String, __constructor, _2)(result, length, data);
    return result;
}
cx_wide_string *CX_ID_5(cxcore, System, Interop, NativeBuffer, ToWideString)(const cx_native_buffer *self, cx_uint length) {
    cx_wide_string *result;
    cx_gc_check_thread();
    if (self == NULL || (cx_ulong)length * sizeof(cx_wchar) > self->_byteLength ||
        (length != 0 && self->_data == NULL)) cx_text_error();
    result = CX_ID_4(cxcore, System, Memory, Alloc)((cx_uint)sizeof(*result));
    CX_ID_5(cxcore, System, WideString, __constructor, _2)(result, self->_data, length);
    return result;
}

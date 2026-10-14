#include "cxcore.h"
#include "Text.internal.h"
#include "Utf8.h"
#include <limits.h>
#include <string.h>

typedef struct CX_ID_3(cxcore, System, WideString) cx_wide_string;

static cx_wide_string cx_wide_empty = {
    .__base = { .__vtable = CX_ID_4(cxcore, System, WideString, __vtable) },
    ._length = 0, ._data = NULL
};

void CX_ID_4(cxcore, System, WideString, __constructor)(cx_wide_string *self) {
    cx_gc_check_thread();
    CX_ID_4(cxcore, System, Object, __constructor)(&self->__base);
    CX_INIT_VTABLE(self, CX_ID_3(cxcore, System, WideString));
    self->_length = 0;
    self->_data = NULL;
}
void CX_ID_5(cxcore, System, WideString, __constructor, _2)(
    cx_wide_string *self, const cx_wchar *data, cx_uint length) {
    CX_ID_4(cxcore, System, WideString, __constructor)(self);
    if ((length != 0 && data == NULL) || length > UINT_MAX / sizeof(cx_wchar)) cx_text_error();
    if (length != 0) {
        self->_data = CX_ID_4(cxcore, System, Memory, Alloc)(length * (cx_uint)sizeof(cx_wchar));
        memcpy(self->_data, data, length * sizeof(cx_wchar));
    }
    self->_length = length;
}
cx_wide_string *CX_ID_5(cxcore, System, WideString, Empty, __const_get)(void) { return &cx_wide_empty; }
cx_uint CX_ID_5(cxcore, System, WideString, Length, __const_get)(const cx_wide_string *self) { return self->_length; }
cx_wchar CX_ID_5(cxcore, System, WideString, Item, __const_get)(const cx_wide_string *self, cx_uint index) {
    if (index >= self->_length) cx_text_error();
    return ((const cx_wchar *)self->_data)[index];
}

static cx_uint cx_wide_decode(const cx_wide_string *self, cx_uint *offset) {
    const cx_wchar *data = self->_data;
    cx_uint position = *offset;
    cx_uint first = data[(*offset)++];
    if (first >= 0xDC00 && first <= 0xDFFF) cx_encoding_error(position);
    if (first >= 0xD800 && first <= 0xDBFF) {
        cx_uint second;
        if (*offset >= self->_length) cx_encoding_error(position);
        second = data[(*offset)++];
        if (second < 0xDC00 || second > 0xDFFF) cx_encoding_error(position);
        return 0x10000 + ((first - 0xD800) << 10) + second - 0xDC00;
    }
    return first;
}
cx_uint CX_ID_5(cxcore, System, WideString, ScalarCount, __const_get)(const cx_wide_string *self) {
    cx_uint count = 0, offset = 0;
    cx_gc_check_thread();
    while (offset < self->_length) { cx_wide_decode(self, &offset); ++count; }
    return count;
}
cx_char CX_ID_4(cxcore, System, WideString, ScalarAt)(const cx_wide_string *self, cx_uint index) {
    cx_uint count = 0, offset = 0;
    cx_gc_check_thread();
    while (offset < self->_length) {
        cx_uint scalar = cx_wide_decode(self, &offset);
        if (count++ == index) return (cx_char)scalar;
    }
    return -1;
}

struct CX_ID_3(cxcore, System, String) *CX_ID_4(cxcore, System, WideString, ToString)(const cx_wide_string *self) {
    cx_uint offset = 0, length = 0, index = 0;
    cx_byte encoded[4], *data;
    size_t count;
    struct CX_ID_3(cxcore, System, String) *result;
    cx_gc_check_thread();
    while (offset < self->_length) {
        count = cx_utf8_encode(cx_wide_decode(self, &offset), encoded);
        if (length > UINT_MAX - count) cx_text_error();
        length += (cx_uint)count;
    }
    data = length == 0 ? NULL : CX_ID_4(cxcore, System, Memory, Alloc)(length);
    offset = 0;
    while (offset < self->_length) {
        count = cx_utf8_encode(cx_wide_decode(self, &offset), encoded);
        memcpy(data + index, encoded, count);
        index += (cx_uint)count;
    }
    result = CX_ID_4(cxcore, System, Memory, Alloc)((cx_uint)sizeof(*result));
    CX_ID_5(cxcore, System, String, __constructor, _2)(result, length, data);
    return result;
}

cx_int CX_ID_4(cxcore, System, WideString, CompareOrdinal)(const cx_wide_string *self, cx_wide_string *other) {
    cx_uint index, common;
    const cx_wchar *left, *right;
    if (self == other) return 0;
    if (self == NULL) return -1;
    if (other == NULL) return 1;
    common = self->_length < other->_length ? self->_length : other->_length;
    left = self->_data; right = other->_data;
    for (index = 0; index < common; ++index) if (left[index] != right[index]) return left[index] < right[index] ? -1 : 1;
    return self->_length == other->_length ? 0 : self->_length < other->_length ? -1 : 1;
}
cx_bool CX_ID_4(cxcore, System, WideString, __cx_operator_equal)(const cx_wide_string *self, cx_wide_string *other) {
    return CX_ID_4(cxcore, System, WideString, CompareOrdinal)(self, other) == 0;
}
cx_bool CX_ID_4(cxcore, System, WideString, __cx_operator_not_equal)(const cx_wide_string *self, cx_wide_string *other) {
    return !CX_ID_4(cxcore, System, WideString, __cx_operator_equal)(self, other);
}
cx_bool CX_ID_4(cxcore, System, WideString, Equals)(const cx_wide_string *self, struct CX_ID_3(cxcore, System, Object) *other) {
    if (other == NULL) return 0;
    if (CX_GET_TYPEINFO(other).Hash != CX_ID_4(cxcore, System, WideString, __typeinfo).Hash) return 0;
    return CX_ID_4(cxcore, System, WideString, __cx_operator_equal)(self, (cx_wide_string *)other);
}
cx_int CX_ID_4(cxcore, System, WideString, GetHashCode)(const cx_wide_string *self) {
    cx_uint hash = 2166136261u, index;
    const cx_wchar *data = self->_data;
    for (index = 0; index < self->_length; ++index) { hash ^= data[index]; hash *= 16777619u; }
    return (cx_int)hash;
}

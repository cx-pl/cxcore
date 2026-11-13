#include "cxcore.h"
#include "../cxarena.h"
#include "../cxgc.h"
#include "Text.internal.h"
#include "Utf8.h"
#include <limits.h>
#include <string.h>

typedef struct cxcore_System_WideString cx_wide_string;

static cx_wide_string cx_wide_empty = {
    .__base = { .__vtable = cxcore_System_WideString___vtable },
    ._length = 0, ._data = NULL
};

void cxcore_System_WideString___constructor(cx_wide_string *self) {
    cx_gc_check_thread();
    cxcore_System_Object___constructor(&self->__base);
    CX_INIT_VTABLE(self, cxcore_System_WideString);
    self->_length = 0;
    cx_gc_store_reference(&self->_data, NULL);
}
void cxcore_System_WideString___constructor__2(
    cx_wide_string *self, const cx_wchar *data, cx_uint length) {
    if (self == NULL) cx_text_null_reference();
    cxcore_System_WideString___constructor(self);
    if (length != 0 && data == NULL) cx_text_argument_null("data");
    if (length > UINT_MAX / sizeof(cx_wchar)) cx_text_overflow();
    if (length != 0) {
        cx_gc_store_reference(
            &self->_data,
            cx_gc_alloc_for_owner(
                self, length * (cx_uint)sizeof(cx_wchar),
                cx_arena_maximum_alignment()));
        memcpy(self->_data, data, length * sizeof(cx_wchar));
    }
    self->_length = length;
}
cx_wide_string *cxcore_System_WideString_Empty___const_get(void) { return &cx_wide_empty; }
cx_uint cxcore_System_WideString_Length___const_get(const cx_wide_string *self) { return self->_length; }
cx_wchar cxcore_System_WideString_Item___const_get(const cx_wide_string *self, cx_uint index) {
    if (self == NULL) cx_text_null_reference();
    if (index >= self->_length) cx_text_index_range();
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
cx_uint cxcore_System_WideString_ScalarCount___const_get(const cx_wide_string *self) {
    cx_uint count = 0, offset = 0;
    cx_gc_check_thread();
    while (offset < self->_length) { cx_wide_decode(self, &offset); ++count; }
    return count;
}
cx_char cxcore_System_WideString_ScalarAt(const cx_wide_string *self, cx_uint index) {
    cx_uint count = 0, offset = 0;
    cx_gc_check_thread();
    while (offset < self->_length) {
        cx_uint scalar = cx_wide_decode(self, &offset);
        if (count++ == index) return (cx_char)scalar;
    }
    return -1;
}
cx_wchar cxcore_System_WideString_CodeUnitAt(
    const cx_wide_string *self, cx_uint index) {
    if (index >= self->_length) cx_encoding_error(index);
    return ((const cx_wchar *)self->_data)[index];
}

struct cxcore_System_String *cxcore_System_WideString_ToString(const cx_wide_string *self) {
    cx_uint offset = 0, length = 0, index = 0;
    cx_byte encoded[4], *data;
    size_t count;
    struct cxcore_System_String *result;
    cx_gc_check_thread();
    while (offset < self->_length) {
        count = cx_utf8_encode(cx_wide_decode(self, &offset), encoded);
        if (length > UINT_MAX - count) cx_text_overflow();
        length += (cx_uint)count;
    }
    data = length == 0 ? NULL : cxcore_System_Memory_Alloc(length);
    offset = 0;
    while (offset < self->_length) {
        count = cx_utf8_encode(cx_wide_decode(self, &offset), encoded);
        memcpy(data + index, encoded, count);
        index += (cx_uint)count;
    }
    result = cxcore_System_Memory_Alloc((cx_uint)sizeof(*result));
    cxcore_System_String___constructor__2(result, length, data);
    return result;
}

cx_int cxcore_System_WideString_CompareOrdinal(const cx_wide_string *self, cx_wide_string *other) {
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
cx_bool cxcore_System_WideString___cx_operator_equal(const cx_wide_string *self, cx_wide_string *other) {
    return cxcore_System_WideString_CompareOrdinal(self, other) == 0;
}
cx_bool cxcore_System_WideString___cx_operator_not_equal(const cx_wide_string *self, cx_wide_string *other) {
    return !cxcore_System_WideString___cx_operator_equal(self, other);
}
cx_bool cxcore_System_WideString_Equals(const cx_wide_string *self, struct cxcore_System_Object *other) {
    if (other == NULL) return 0;
    if (CX_GET_TYPEINFO(other).Hash != cxcore_System_WideString___typeinfo.Hash) return 0;
    return cxcore_System_WideString___cx_operator_equal(self, (cx_wide_string *)other);
}
cx_int cxcore_System_WideString_GetHashCode(const cx_wide_string *self) {
    cx_uint hash = 2166136261u, index;
    const cx_wchar *data = self->_data;
    for (index = 0; index < self->_length; ++index) { hash ^= data[index]; hash *= 16777619u; }
    return (cx_int)hash;
}

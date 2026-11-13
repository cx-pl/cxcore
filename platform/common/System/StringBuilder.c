#include "cxcore.h"
#include "Text.internal.h"
#include "Utf8.h"
#include "../cxarena.h"
#include "../cxgc.h"
#include "../../stdlib.h"
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct cxcore_System_StringBuilder builder;
typedef struct cxcore_System_String string;

cx_uint cxcore_System_StringBuilder_EnsureCapacity(builder *self, cx_uint capacity) {
    if (self == CX_NULL) cx_text_null_reference();
    if (capacity <= self->_capacity) return self->_capacity;
    cx_uint grown = self->_capacity < 64 ? 64 : self->_capacity;
    while (grown < capacity) {
        if (grown > UINT_MAX / 2) { grown = capacity; break; }
        grown *= 2;
    }
    void *replacement = cx_gc_alloc_for_owner(
        self, grown, cx_arena_maximum_alignment());
    if (self->_data != CX_NULL && self->_length != 0) {
        memcpy(replacement, self->_data, self->_length);
    }
    cx_gc_store_reference(&self->_data, replacement);
    self->_capacity = grown;
    return grown;
}
void cxcore_System_StringBuilder___constructor__2(builder *self, cx_uint capacity) {
    cxcore_System_Object___constructor(&self->__base);
    CX_INIT_VTABLE(self, cxcore_System_StringBuilder);
    self->_length = self->_capacity = self->_characters = 0;
    cx_gc_store_reference(&self->_data, CX_NULL);
    if (capacity) cxcore_System_StringBuilder_EnsureCapacity(self, capacity);
}
void cxcore_System_StringBuilder___constructor(builder *self) {
    cxcore_System_StringBuilder___constructor__2(self, 64);
}
cx_uint cxcore_System_StringBuilder_Length___const_get(const builder *self) {
    if (!self) cx_text_null_reference();
    return self->_characters;
}
cx_uint cxcore_System_StringBuilder_Capacity___const_get(const builder *self) {
    if (!self) cx_text_null_reference();
    return self->_capacity;
}
static builder *append(builder *self, const cx_byte *bytes, cx_uint length) {
    if (!self) cx_text_null_reference();
    if (length && !bytes) cx_text_argument_null("value");
    if (length > UINT_MAX - self->_length) cx_text_overflow();
    if (!length) return self;
    cx_uint characters = 0;
    size_t position = 0;
    while (position < length) {
        cx_uint scalar; size_t count;
        if (!cx_utf8_decode(bytes + position, length - position, &scalar, &count)) count = 1;
        position += count; ++characters;
    }
    // A native caller can append a borrowed slice of this builder's buffer.
    // Preserve the offset across reallocation and allow overlapping copies.
    uintptr_t start = (uintptr_t)self->_data, input = (uintptr_t)bytes;
    int aliased = self->_data && input >= start && input - start < self->_length;
    size_t offset = aliased ? (size_t)(input - start) : 0;
    if (aliased && length > self->_length - offset) cx_text_argument_range("value");
    cxcore_System_StringBuilder_EnsureCapacity(self, self->_length + length);
    if (aliased) bytes = (const cx_byte *)self->_data + offset;
    memmove((cx_byte *)self->_data + self->_length, bytes, length);
    self->_length += length;
    self->_characters += characters;
    return self;
}
builder *cxcore_System_StringBuilder_Append(builder *self, string *value) {
    return append(self, value ? value->_data : CX_NULL, value ? value->_length : 0);
}
builder *cxcore_System_StringBuilder_Append__2(builder *self, cx_char value) {
    cx_byte bytes[4];
    size_t length = cx_utf8_encode((cx_uint)value, bytes);
    if (!length) cx_encoding_error(0);
    return append(self, bytes, (cx_uint)length);
}
builder *cxcore_System_StringBuilder_Append__3(builder *self, cx_bool value) {
    return append(self, (const cx_byte *)(value ? "True" : "False"), value ? 4 : 5);
}
static builder *append_integer(builder *self, cx_ulong magnitude, int negative) {
    cx_byte reversed[20], text[21];
    unsigned count = 0, length = 0;
    do { reversed[count++] = (cx_byte)('0' + magnitude % 10); magnitude /= 10; } while (magnitude);
    if (negative) text[length++] = '-';
    while (count) text[length++] = reversed[--count];
    return append(self, text, length);
}
builder *cxcore_System_StringBuilder_Append__4(builder *self, cx_int value) {
    return append_integer(self, value < 0 ? (cx_ulong)0 - (cx_ulong)value : (cx_ulong)value, value < 0);
}
builder *cxcore_System_StringBuilder_Append__5(builder *self, cx_uint value) {
    return append_integer(self, value, 0);
}
builder *cxcore_System_StringBuilder_Append__6(builder *self, cx_long value) {
    return append_integer(self, value < 0 ? (cx_ulong)0 - (cx_ulong)value : (cx_ulong)value, value < 0);
}
builder *cxcore_System_StringBuilder_Append__7(builder *self, cx_ulong value) {
    return append_integer(self, value, 0);
}
static builder *append_float(builder *self, double value, int single) {
    if (isnan(value)) return append(self,(const cx_byte *)"NaN",3);
    if (isinf(value)) return append(self,(const cx_byte *)(value < 0 ? "-Infinity" : "Infinity"),value < 0 ? 9 : 8);
    char text[64];
    int length = cx_platform_format_float(text, sizeof(text), value, single);
    if (length < 0 || (size_t)length >= sizeof(text)) cx_text_format();
    return append(self, (cx_byte *)text, (cx_uint)length);
}
builder *cxcore_System_StringBuilder_Append__8(builder *self, cx_float value) {
    return append_float(self, value, 1);
}
builder *cxcore_System_StringBuilder_Append__9(builder *self, cx_double value) {
    return append_float(self, value, 0);
}
builder *cxcore_System_StringBuilder_Append__10(builder *self, cx_decimal value) {
    char text[32];
    cx_uint length = cx_decimal_format(value, text, sizeof(text));
    return append(self, (const cx_byte *)text, length);
}
builder *cxcore_System_StringBuilder_Append__11(builder *self, cx_byte value) {
    return append_integer(self,value,0);
}
builder *cxcore_System_StringBuilder_Append__12(builder *self, cx_sbyte value) {
    return cxcore_System_StringBuilder_Append__4(self,value);
}
builder *cxcore_System_StringBuilder_Append__13(builder *self, cx_short value) {
    return cxcore_System_StringBuilder_Append__4(self,value);
}
builder *cxcore_System_StringBuilder_Append__14(builder *self, cx_ushort value) {
    return append_integer(self,value,0);
}
builder *cxcore_System_StringBuilder_AppendLine(builder *self) {
#if defined(_WIN32)
    return append(self, (const cx_byte *)"\r\n", 2);
#else
    return append(self, (const cx_byte *)"\n", 1);
#endif
}
builder *cxcore_System_StringBuilder_AppendLine__2(builder *self, string *value) {
    cxcore_System_StringBuilder_Append(self, value);
    return cxcore_System_StringBuilder_AppendLine(self);
}
builder *cxcore_System_StringBuilder_Clear(builder *self) {
    if (!self) cx_text_null_reference();
    self->_length = self->_characters = 0;
    return self;
}
string *cxcore_System_StringBuilder_ToString(const builder *self) {
    if (!self) cx_text_null_reference();
    if (!self->_length) return (string *)cxcore_System_String_Empty___const_get();
    void *data = cxcore_System_Memory_Alloc(self->_length);
    memcpy(data, self->_data, self->_length);
    string *result = cxcore_System_Memory_Alloc((cx_uint)sizeof(*result));
    cxcore_System_String___constructor__2(result, self->_length, data);
    return result;
}

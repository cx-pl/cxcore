#ifndef CXCORE_SYSTEM_UTF8_H
#define CXCORE_SYSTEM_UTF8_H

#include <stddef.h>
#include "../../../inc/cxtypes.h"

/* Decode one Unicode scalar from a bounded UTF-8 byte sequence. */
static int cx_utf8_decode(
    const cx_byte* bytes,
    size_t remaining,
    cx_uint* scalar,
    size_t* byteCount)
{
    cx_uint value;
    cx_byte first;
    size_t count;
    size_t index;

    if (bytes == NULL || remaining == 0 || scalar == NULL || byteCount == NULL) {
        return 0;
    }

    first = bytes[0];
    if (first <= 0x7F) {
        *scalar = first;
        *byteCount = 1;
        return 1;
    }
    if (first >= 0xC2 && first <= 0xDF) {
        value = first & 0x1F;
        count = 2;
    }
    else if (first >= 0xE0 && first <= 0xEF) {
        value = first & 0x0F;
        count = 3;
    }
    else if (first >= 0xF0 && first <= 0xF4) {
        value = first & 0x07;
        count = 4;
    }
    else {
        return 0;
    }

    if (remaining < count) {
        return 0;
    }
    for (index = 1; index < count; ++index) {
        if ((bytes[index] & 0xC0) != 0x80) {
            return 0;
        }
        value = (value << 6) | (bytes[index] & 0x3F);
    }

    if ((count == 2 && value < 0x80) ||
        (count == 3 && value < 0x800) ||
        (count == 4 && value < 0x10000) ||
        (value >= 0xD800 && value <= 0xDFFF) ||
        value > 0x10FFFF) {
        return 0;
    }

    *scalar = value;
    *byteCount = count;
    return 1;
}

static size_t cx_utf8_encode(cx_uint scalar, cx_byte output[4])
{
    if (scalar <= 0x7F) {
        output[0] = (cx_byte)scalar;
        return 1;
    }
    if (scalar <= 0x7FF) {
        output[0] = (cx_byte)(0xC0 | (scalar >> 6));
        output[1] = (cx_byte)(0x80 | (scalar & 0x3F));
        return 2;
    }
    if (scalar >= 0xD800 && scalar <= 0xDFFF) {
        return 0;
    }
    if (scalar <= 0xFFFF) {
        output[0] = (cx_byte)(0xE0 | (scalar >> 12));
        output[1] = (cx_byte)(0x80 | ((scalar >> 6) & 0x3F));
        output[2] = (cx_byte)(0x80 | (scalar & 0x3F));
        return 3;
    }
    if (scalar <= 0x10FFFF) {
        output[0] = (cx_byte)(0xF0 | (scalar >> 18));
        output[1] = (cx_byte)(0x80 | ((scalar >> 12) & 0x3F));
        output[2] = (cx_byte)(0x80 | ((scalar >> 6) & 0x3F));
        output[3] = (cx_byte)(0x80 | (scalar & 0x3F));
        return 4;
    }
    return 0;
}

#endif

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "../../../src/cxcore.h"
#include "Utf8.h"

CX_STRING_DEF(_empty, "");

const struct CX_ID_3(cxcore, System, String)* CX_ID_5(cxcore, System, String, Empty, __const_get)(
) {
    return &_empty;
}

cx_uint CX_ID_5(cxcore, System, String, Length, __const_get)(
    const struct CX_ID_3(cxcore, System, String)* __this
) {
    const cx_byte* bytes;
    size_t offset = 0;
    cx_uint count = 0;

    if (__this == NULL || __this->_data == NULL) {
        return 0;
    }

    bytes = (const cx_byte*)__this->_data;
    while (offset < __this->_length) {
        cx_uint scalar;
        size_t byteCount;
        if (cx_utf8_decode(bytes + offset, __this->_length - offset, &scalar, &byteCount)) {
            offset += byteCount;
        }
        else {
            /* Invalid input bytes each count as one replacement character. */
            ++offset;
        }
        ++count;
    }
    return count;
}

cx_char CX_ID_5(cxcore, System, String, Item, __const_get)(
    const struct CX_ID_3(cxcore, System, String)* __this,
    cx_uint index
) {
    const cx_byte* bytes;
    size_t offset = 0;
    cx_uint current = 0;

    if (__this == NULL || (__this->_length != 0 && __this->_data == NULL)) {
        return -1;
    }

    bytes = (const cx_byte*)__this->_data;
    while (offset < __this->_length) {
        cx_uint scalar;
        size_t byteCount;
        if (!cx_utf8_decode(bytes + offset, __this->_length - offset, &scalar, &byteCount)) {
            if (current == index) {
                return -1;
            }
            ++offset;
        }
        else {
            if (current == index) {
                return (cx_char)scalar;
            }
            offset += byteCount;
        }
        ++current;
    }
    return -1;
}

void CX_ID_4(cxcore, System, String, __constructor)(
    struct CX_ID_3(cxcore, System, String)* __this
) {
    CX_INIT_VTABLE(__this, CX_ID_3(cxcore, System, String));
    CX_ID_4(cxcore, System, Object, __constructor)(&__this->__base);
    __this->_length = 0;
    __this->_data = CX_NULL;
}

CX_EXPORT void CX_ID_5(cxcore, System, String, __constructor, _2)(
    struct CX_ID_3(cxcore, System, String)* __this,
    cx_uint length,
    cx_ptr data
) {
    CX_INIT_VTABLE(__this, CX_ID_3(cxcore, System, String));
    CX_ID_4(cxcore, System, Object, __constructor)(&__this->__base);
    __this->_length = length;
    __this->_data = length == 0 ? CX_NULL : data;
}

void CX_ID_5(cxcore, System, String, __constructor, _3)(
    struct CX_ID_3(cxcore, System, String)* __this,
    cx_uint length,
    cx_char ch
) {
    cx_byte encoded[4];
    size_t encodedLength = cx_utf8_encode((cx_uint)ch, encoded);
    size_t byteLength;
    cx_byte* data;
    cx_uint index;

    if (length != 0 && (encodedLength == 0 || (size_t)length > (size_t)UINT_MAX / encodedLength)) {
        abort();
    }
    byteLength = length == 0 ? 0 : (size_t)length * encodedLength;
    data = byteLength == 0 ? NULL : (cx_byte*)CX_ID_4(cxcore, System, Memory, Alloc)((cx_uint)byteLength);
    for (index = 0; index < length; ++index) {
        memcpy(data + ((size_t)index * encodedLength), encoded, encodedLength);
    }

    CX_ID_5(cxcore, System, String, __constructor, _2)(__this, (cx_uint)byteLength, data);
}

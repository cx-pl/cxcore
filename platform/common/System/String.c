#include <string.h>
#include "../../../src/cxcore.h"

CX_STRING_DEF(_empty, "");

const struct CX_ID_3(cxcore, System, String)* CX_ID_5(cxcore, System, String, Empty, __const_get)(
) {
	return &_empty;
}

cx_uint CX_ID_5(cxcore, System, String, Length, __const_get)(
    const struct CX_ID_3(cxcore, System, String)* __this
) {
    return __this->_length;
}

cx_char CX_ID_5(cxcore, System, String, Item, __const_get)(
    const struct CX_ID_3(cxcore, System, String)* __this,
	cx_uint index
) {
    cx_uint i;
    cx_byte* pch;
	cx_char result = 0;

    // Validate index is in range
    if (index >= __this->_length) {
        // TODO: Throw
    }

	// Walk through UTF-8 string to find the character at the specified index
    pch = __this->_data;
    for (i = 0; i < __this->_length; i++) {
        if ((*pch & 0x80) == 0x00) {
            pch++;
        }
        else if ((*((cx_ushort*)pch) & 0xC0E0) == 0x80C0) {
            pch += 2;
        }
        else if ((*((cx_uint*)pch) & 0xC0C0F0) == 0x8080E0) {
            pch += 3;
        }
        else if ((*((cx_uint*)pch) & 0xC0C0C0F8) == 0x808080F0) {
            pch += 4;
        }
        else {
            // TODO: Throw
            return -1; // Invalid UTF-8 character
        }
    }

	// Decode UTF-8 character
    if ((*pch & 0x80) == 0x00) {
        result = *((cx_byte*)pch);
    }
    else if ((*((cx_ushort*)pch) & 0xC0E0) == 0x80C0) {
        result = *((cx_ushort*)pch);
    }
    else if ((*((cx_uint*)pch) & 0xC0C0F0) == 0x8080E0) {
        result = *((cx_uint*)pch) & 0xFFFFFF;
    }
    else if ((*((cx_uint*)pch) & 0xC0C0C0F8) == 0x808080F0) {
        result = *((cx_uint*)pch);
    }

	return result;
}

void CX_ID_4(cxcore, System, String, __constructor)(
    struct CX_ID_3(cxcore, System, String)* __this
) {
    CX_INIT_VTABLE(__this, CX_ID_3(cxcore, System, String));

    CX_ID_4(cxcore, System, Object, __constructor)(&__this->__base);

    __this->_length = 0;
    __this->_data = CX_NULL;
}

void CX_ID_5(cxcore, System, String, __constructor, _2)(
    struct CX_ID_3(cxcore, System, String)* __this,
    cx_uint length,
    cx_ptr data
) {
    CX_INIT_VTABLE(__this, CX_ID_3(cxcore, System, String));

    CX_ID_4(cxcore, System, Object, __constructor)(&__this->__base);

    __this->_length = length;
    __this->_data = data;
}

void CX_ID_5(cxcore, System, String, __constructor, _3)(
    struct CX_ID_3(cxcore, System, String)* __this,
    cx_uint length,
    cx_char ch
) {
    CX_INIT_VTABLE(__this, CX_ID_3(cxcore, System, String));

    CX_ID_4(cxcore, System, Object, __constructor)(&__this->__base);

    __this->_length = length;
    __this->_data = CX_ID_4(cxcore, System, Memory, Alloc)(length);
    memset(__this->_data, ch, length);
}

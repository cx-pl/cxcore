#include "../../../src/cxcore.h"

cx_bool CX_ID_4(cxcore, System, Char, IsBetween)(    
    cx_char c,
    cx_char min,
    cx_char max
){
    return (c >= min && c <= max) ? CX_TRUE : CX_FALSE;
}

cx_bool CX_ID_4(cxcore, System, Char, IsDigit)(    
    cx_char c
) {
    return CX_ID_4(cxcore, System, Char, IsBetween)(c, '0', '9');
}

cx_bool CX_ID_4(cxcore, System, Char, IsLetter)(    
    cx_char c
) {
    return 
        CX_ID_4(cxcore, System, Char, IsBetween)(c, 'A', 'Z') ||
        CX_ID_4(cxcore, System, Char, IsBetween)(c, 'a', 'z');
}

cx_bool CX_ID_4(cxcore, System, Char, IsWhiteSpace)(    
    cx_char c
) {
    return (
        c == ' ' ||
        c == '\t' ||
        c == '\r' ||
        c == '\n'
        ) ? CX_TRUE : CX_FALSE;
}

cx_bool CX_ID_4(cxcore, System, Char, IsLower)(    
    cx_char c
) {
    return CX_ID_4(cxcore, System, Char, IsBetween)(c, 'a', 'z');
}

cx_bool CX_ID_4(cxcore, System, Char, IsUpper)(    
    cx_char c
) {
    return CX_ID_4(cxcore, System, Char, IsBetween)(c, 'A', 'Z');
}

cx_char CX_ID_4(cxcore, System, Char, ToLower)(
    const struct CX_ID_3(cxcore, System, Char)* __this
) {
    return CX_ID_4(cxcore, System, Char, IsUpper)(__this->_value)
        ? __this->_value + ('a' - 'A')
        : __this->_value;
}

cx_char CX_ID_4(cxcore, System, Char, ToUpper)(
    const struct CX_ID_3(cxcore, System, Char)* __this
) {
    return CX_ID_4(cxcore, System, Char, IsLower)(__this->_value)
        ? __this->_value + ('A' - 'a')
        : __this->_value;
}

cx_int CX_ID_4(cxcore, System, Char, NumBytes)(
    const struct CX_ID_3(cxcore, System, Char)* __this
) {
    // TODO: Validate

	// 0xxxxxxx -> 1 byte
	// 110xxxxx 10xxxxxx -> 2 bytes
	// 1110xxxx 10xxxxxx 10xxxxxx -> 3 bytes
	// 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx -> 4 bytes
	int ch = __this->_value;

    if ((ch & 0x80) == 0x00) {
        return 1;
	} else if ((ch & 0xC0E0) == 0x80C0) {
        return 2;
    } else if ((ch & 0xC0C0F0) == 0x8080E0) {
        return 3;
    } else if ((ch & 0xC0C0C0F8) == 0x808080F0) {
        return 4;
    } else {
		return -1; // Invalid UTF-8 character
    }
}

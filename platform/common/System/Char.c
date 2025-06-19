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
    struct CX_ID_3(cxcore, System, Char)* __this
) {
    return CX_ID_4(cxcore, System, Char, IsUpper)(__this->_value)
        ? __this->_value + ('a' - 'A')
        : __this->_value;
}

cx_char CX_ID_4(cxcore, System, Char, ToUpper)(
    struct CX_ID_3(cxcore, System, Char)* __this
) {
    return CX_ID_4(cxcore, System, Char, IsLower)(__this->_value)
        ? __this->_value + ('A' - 'a')
        : __this->_value;
}

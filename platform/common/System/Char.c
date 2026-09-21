#include <stdio.h>
#include "../../../src/cxcore.h"

cx_int CX_ID_5(cxcore, System, Char, NumBytes, __const_get)(
    const struct CX_ID_3(cxcore, System, Char)* __this
    ) {
    cx_char value = __this->_value;

    if (value & 0x80 == 0x00) {
        return 1;
    }
    else if ((value & 0xC0E0) == 0x80C0) {
        return 2;
    }
    else if ((value & 0xC0C0F0) == 0x8080E0) {
        return 3;
    }
    else if ((value & 0xC0C0C0F8) == 0x808080F0) {
        return 4;
    }
    else {
        // TODO: Throw exception
        return -1; // Invalid UTF-8 character
    }
}
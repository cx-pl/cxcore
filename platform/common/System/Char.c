#include "../../../src/cxcore.h"
#include "Utf8.h"

cx_int CX_ID_5(cxcore, System, Char, NumBytes, __const_get)(
    const struct CX_ID_3(cxcore, System, Char)* __this
    ) {
    cx_byte encoded[4];
    size_t count = cx_utf8_encode((cx_uint)__this->_value, encoded);
    return count == 0 ? -1 : (cx_int)count;
}

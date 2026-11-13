#include "../../../src/cxcore.h"
#include "Utf8.h"

cx_int cxcore_System_Char_NumBytes___const_get(const struct cxcore_System_Char * __this) {
    cx_byte encoded[4];
    size_t count = cx_utf8_encode((cx_uint)__this->_value, encoded);
    return count == 0 ? -1 : (cx_int)count;
}

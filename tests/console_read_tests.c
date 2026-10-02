#include <assert.h>
#include <string.h>
#include "../src/cxcore.h"

int main(void)
{
    struct CX_ID_3(cxcore, System, String)* line = CX_ID_4(cxcore, System, Console, ReadLine)();
    struct CX_ID_3(cxcore, System, String)* prefix = CX_ID_4(cxcore, System, Console, Read)(3);
    assert(line->_length == 300);
    assert(((const char*)line->_data)[0] == 'a');
    assert(((const char*)line->_data)[299] == 'a');
    assert(prefix->_length == 3);
    assert(memcmp(prefix->_data, "xyz", 3) == 0);
    assert(((const char*)line->_data)[10] == 'a');
    CX_ID_4(cxcore, System, Memory, Free)(line->_data);
    CX_ID_4(cxcore, System, Memory, Free)(line);
    CX_ID_4(cxcore, System, Memory, Free)(prefix->_data);
    CX_ID_4(cxcore, System, Memory, Free)(prefix);
    return 0;
}

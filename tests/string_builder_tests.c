#include "cxcore.h"
#include <assert.h>
#include <limits.h>
#include <string.h>

typedef struct cxcore_System_StringBuilder builder;
typedef struct cxcore_System_String string;
#define SB(name) cxcore_System_StringBuilder_ ## name
#define SBO(name,index) cxcore_System_StringBuilder_ ## name ## _ ## index
static string input(const char *text) {
    string value;
    cxcore_System_String___constructor__2(&value,(cx_uint)strlen(text),(cx_ptr)text);
    return value;
}
static void equal(const string *value, const char *expected) {
    assert(value && value->_length == strlen(expected));
    assert(!value->_length || memcmp(value->_data,expected,value->_length) == 0);
}
int main(void) {
    builder value;
    SBO(__constructor,_2)(&value,1);
    assert(cxcore_System_StringBuilder_Length___const_get(&value) == 0);
    string hello = input("hello");
    assert(SB(Append)(&value,&hello) == &value);
    const string *snapshot = SB(ToString)(&value);
    SBO(Append,_2)(&value,0x1f600);
    assert(cxcore_System_StringBuilder_Length___const_get(&value) == 6);
    equal(snapshot,"hello");
    equal(SB(ToString)(&value),"hello\xf0\x9f\x98\x80");
    cx_uint capacity = value._capacity;
    assert(SB(Clear)(&value) == &value && value._capacity == capacity);
    equal(SB(ToString)(&value),"");
    SBO(Append,_6)(&value,LLONG_MIN);
    SBO(Append,_2)(&value,' ');
    SBO(Append,_7)(&value,ULLONG_MAX);
    SBO(Append,_2)(&value,' ');
    SBO(Append,_10)(&value,cx_decimal_parse("123.4500"));
    equal(SB(ToString)(&value),"-9223372036854775808 18446744073709551615 123.4500");
    SB(Clear)(&value);
    SBO(Append,_3)(&value,1); SBO(Append,_3)(&value,0);
    SBO(Append,_8)(&value,1.25f); SBO(Append,_9)(&value,2.5);
    equal(SB(ToString)(&value),"TrueFalse1.252.5");
    SB(Clear)(&value);
    for (unsigned i = 0; i < 10000; ++i) SBO(Append,_2)(&value,'x');
    assert(value._length == 10000 && value._characters == 10000);
    assert(value._capacity >= value._length && value._capacity < 2*value._length);
    const string *large = SB(ToString)(&value);
    assert(large->_length == 10000);
    for (unsigned i = 0; i < large->_length; ++i) assert(((cx_byte *)large->_data)[i] == 'x');
    // A ToString snapshot stays immutable across clear, growth, and later appends.
    SB(Clear)(&value); SB(Append)(&value,&hello);
    assert(large->_length == 10000 && ((cx_byte *)large->_data)[0] == 'x' && ((cx_byte *)large->_data)[9999] == 'x');
    assert(SB(Append)(&value,CX_NULL) == &value);
    SB(AppendLine)(&value);
#if defined(_WIN32)
    equal(SB(ToString)(&value),"hello\r\n");
#else
    equal(SB(ToString)(&value),"hello\n");
#endif
    return 0;
}

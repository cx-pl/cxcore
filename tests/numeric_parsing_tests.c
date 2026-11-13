#include "cxcore.h"
#include <assert.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

typedef struct cxcore_System_String string;
typedef struct cxcore_System_Nullable nullable;
typedef nullable (*parser)(const string *);
static string input(const char *text) {
    string result;
    cxcore_System_String___constructor__2(&result, (cx_uint)strlen(text), (cx_ptr)text);
    return result;
}
static nullable parse(parser function, const char *text) { string value = input(text); return function(&value); }
#define PARSER(Name) cxcore_System_ ## Name ## _TryParse
int main(void) {
    nullable value;
    value = parse(PARSER(SByte), "-128"); assert(value._obj && *(cx_sbyte *)value._obj == SCHAR_MIN);
    value = parse(PARSER(Byte), "255"); assert(value._obj && *(cx_byte *)value._obj == UCHAR_MAX);
    value = parse(PARSER(Short), "-32768"); assert(value._obj && *(cx_short *)value._obj == SHRT_MIN);
    value = parse(PARSER(UShort), "65535"); assert(value._obj && *(cx_ushort *)value._obj == USHRT_MAX);
    value = parse(PARSER(Int), " -2147483648\t"); assert(value._obj && *(cx_int *)value._obj == INT_MIN);
    value = parse(PARSER(UInt), "+4294967295"); assert(value._obj && *(cx_uint *)value._obj == UINT_MAX);
    value = parse(PARSER(Long), "-9223372036854775808"); assert(value._obj && *(cx_long *)value._obj == LLONG_MIN);
    value = parse(PARSER(ULong), "18446744073709551615"); assert(value._obj && *(cx_ulong *)value._obj == ULLONG_MAX);
    value = parse(PARSER(Int), "2147483647"); assert(value._obj && *(cx_int *)value._obj == INT_MAX);
    assert(!parse(PARSER(SByte), "128")._obj);
    assert(!parse(PARSER(Byte), "256")._obj);
    assert(!parse(PARSER(Short), "-32769")._obj);
    assert(!parse(PARSER(UShort), "65536")._obj);
    assert(!parse(PARSER(Int), "2147483648")._obj);
    assert(!parse(PARSER(UInt), "4294967296")._obj);
    assert(!parse(PARSER(Long), "-9223372036854775809")._obj);
    assert(!parse(PARSER(ULong), "18446744073709551616")._obj);
    assert(!parse(PARSER(ULong), "-1")._obj);
    value = parse(PARSER(ULong), "-0"); assert(value._obj && *(cx_ulong *)value._obj == 0);
    value = parse(PARSER(Float), "1.25e2"); assert(value._obj && *(cx_float *)value._obj == 125.0f);
    value = parse(PARSER(Double), "-1.7976931348623157e308"); assert(value._obj && *(cx_double *)value._obj == -DBL_MAX);
    value = parse(PARSER(Float), "1e-1000"); assert(value._obj && *(cx_float *)value._obj == 0.0f);
    assert(!parse(PARSER(Float), "3.5e38")._obj);
    assert(!parse(PARSER(Double), "1.8e308")._obj);
    value = parse(PARSER(Decimal), "79228162514264337593543950335");
    assert(value._obj && cx_decimal_compare(*(cx_decimal *)value._obj, cx_decimal_parse("79228162514264337593543950335")) == 0);
    assert(!parse(PARSER(Decimal), "79228162514264337593543950336")._obj);
    value = parse(PARSER(Decimal), "1e-28");
    assert(value._obj && cx_decimal_compare(*(cx_decimal *)value._obj, cx_decimal_parse("0.0000000000000000000000000001")) == 0);
    value = parse(PARSER(Decimal), "0.00000000000000000000000000015");
    assert(value._obj && cx_decimal_compare(*(cx_decimal *)value._obj, cx_decimal_parse("0.0000000000000000000000000002")) == 0);
    const char *bad[] = {"", " ", "+", "-", "1x", "1 2", "0x10", "1,234", "NaN", "Infinity"};
    parser parsers[] = {PARSER(Byte),PARSER(SByte),PARSER(Short),PARSER(UShort),PARSER(Int),PARSER(UInt),
        PARSER(Long),PARSER(ULong),PARSER(Float),PARSER(Double),PARSER(Decimal)};
    for (unsigned i = 0; i < sizeof(parsers)/sizeof(parsers[0]); ++i) {
        assert(!parsers[i](CX_NULL)._obj);
        for (unsigned j = 0; j < sizeof(bad)/sizeof(bad[0]); ++j) assert(!parse(parsers[i],bad[j])._obj);
        string embedded;
        cxcore_System_String___constructor__2(&embedded,3,"1\0x");
        assert(!parsers[i](&embedded)._obj);
        assert(!cx_exception_pending());
    }
    char long_fraction[600];
    memset(long_fraction,'0',sizeof(long_fraction));
    long_fraction[1] = '.'; long_fraction[598] = '1'; long_fraction[599] = 0;
    value = parse(PARSER(Decimal),long_fraction);
    assert(value._obj && cx_decimal_compare(*(cx_decimal *)value._obj,cx_decimal_from_i64(0)) == 0);
    return 0;
}

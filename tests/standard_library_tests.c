#include <assert.h>
#include <string.h>
#include "../src/cxcore.h"

static void test_char_utf8_lengths(void)
{
    struct CX_ID_3(cxcore, System, Char) value;
    value._value = 'A';
    assert(CX_ID_5(cxcore, System, Char, NumBytes, __const_get)(&value) == 1);
    value._value = 0x00A2;
    assert(CX_ID_5(cxcore, System, Char, NumBytes, __const_get)(&value) == 2);
    value._value = 0x20AC;
    assert(CX_ID_5(cxcore, System, Char, NumBytes, __const_get)(&value) == 3);
    value._value = 0x1F600;
    assert(CX_ID_5(cxcore, System, Char, NumBytes, __const_get)(&value) == 4);
    value._value = 0xD800;
    assert(CX_ID_5(cxcore, System, Char, NumBytes, __const_get)(&value) == -1);

    value._value = '7';
    assert(CX_ID_5(cxcore, System, Char, IsDigit, __const_get)(&value));
    assert(!CX_ID_5(cxcore, System, Char, IsLetter, __const_get)(&value));
    value._value = 'z';
    assert(CX_ID_5(cxcore, System, Char, IsLower, __const_get)(&value));
    assert(CX_ID_4(cxcore, System, Char, ToUpper)(&value) == 'Z');
    value._value = ' ';
    assert(CX_ID_5(cxcore, System, Char, IsWhiteSpace, __const_get)(&value));
}

static void test_string_utf8_indexing(void)
{
    static const cx_byte utf8[] = {
        'A', 0xC2, 0xA2, 0xE2, 0x82, 0xAC, 0xF0, 0x9F, 0x98, 0x80
    };
    struct CX_ID_3(cxcore, System, String) value;
    CX_ID_5(cxcore, System, String, __constructor, _2)(&value, (cx_uint)sizeof(utf8), (cx_ptr)utf8);
    assert(CX_ID_5(cxcore, System, String, Length, __const_get)(&value) == 4);
    assert(CX_ID_5(cxcore, System, String, Item, __const_get)(&value, 0) == 'A');
    assert(CX_ID_5(cxcore, System, String, Item, __const_get)(&value, 1) == 0xA2);
    assert(CX_ID_5(cxcore, System, String, Item, __const_get)(&value, 2) == 0x20AC);
    assert(CX_ID_5(cxcore, System, String, Item, __const_get)(&value, 3) == 0x1F600);
    assert(CX_ID_5(cxcore, System, String, Item, __const_get)(&value, 4) == -1);

    {
        struct CX_ID_3(cxcore, System, String) repeated;
        CX_ID_5(cxcore, System, String, __constructor, _3)(&repeated, 3, 0x20AC);
        assert(repeated._length == 9);
        assert(CX_ID_5(cxcore, System, String, Length, __const_get)(&repeated) == 3);
        assert(CX_ID_5(cxcore, System, String, Item, __const_get)(&repeated, 2) == 0x20AC);
        CX_ID_4(cxcore, System, Memory, Free)(repeated._data);
    }

    {
        static const cx_byte invalid[] = { 0xF0, 0x80, 0x80, 0x80 };
        struct CX_ID_3(cxcore, System, String) malformed;
        CX_ID_5(cxcore, System, String, __constructor, _2)(&malformed, (cx_uint)sizeof(invalid), (cx_ptr)invalid);
        assert(CX_ID_5(cxcore, System, String, Length, __const_get)(&malformed) == 4);
        assert(CX_ID_5(cxcore, System, String, Item, __const_get)(&malformed, 0) == -1);
    }
}

static void test_array_and_nullable(void)
{
    struct CX_ID_3(cxcore, System, Array)* array = cx_array_new(3, (cx_uint)sizeof(cx_int));
    cx_int* first = (cx_int*)cx_array_at(array, 0, (cx_uint)sizeof(cx_int));
    cx_int* last = (cx_int*)cx_array_at(array, 2, (cx_uint)sizeof(cx_int));
    struct CX_ID_3(cxcore, System, Nullable) nullable;
    assert(array->_length == 3);
    assert(*first == 0 && *last == 0);
    *last = 42;
    assert(*(cx_int*)cx_array_at(array, 2, (cx_uint)sizeof(cx_int)) == 42);
    assert(cx_array_at(array, 3, (cx_uint)sizeof(cx_int)) == CX_NULL);
    assert(cx_array_at(array, 2, 0) == CX_NULL);
    {
        struct CX_ID_3(cxcore, System, Array)* empty = cx_array_new(0, 0);
        assert(empty->_data == CX_NULL);
        CX_ID_4(cxcore, System, Memory, Free)(empty);
    }

    CX_ID_4(cxcore, System, Nullable, __constructor)(&nullable, CX_NULL);
    assert(!CX_ID_5(cxcore, System, Nullable, HasValue, __const_get)(&nullable));
    CX_ID_5(cxcore, System, Nullable, Value, __set)(&nullable, last);
    assert(CX_ID_5(cxcore, System, Nullable, HasValue, __const_get)(&nullable));
    assert(nullable._obj == last);
    CX_ID_4(cxcore, System, Memory, Free)(array->_data);
    CX_ID_4(cxcore, System, Memory, Free)(array);
}

static void test_memory_helpers(void)
{
    cx_byte source[3] = { 1, 2, 3 };
    cx_byte destination[3] = { 0, 0, 0 };
    cx_int nullableValue = 17;
    cx_byte* block = (cx_byte*)CX_ID_4(cxcore, System, Memory, Alloc)(0);
    assert(block != CX_NULL);
    CX_ID_4(cxcore, System, Memory, Free)(block);
    CX_ID_4(cxcore, System, Memory, Copy)(NULL, NULL, 0);
    CX_ID_4(cxcore, System, Memory, Copy)(destination, source, 3);
    assert(memcmp(destination, source, sizeof(source)) == 0);
    {
        block = (cx_byte*)CX_ID_4(cxcore, System, Nullable, CreateValueStorage)(
            &nullableValue,
            (cx_uint)sizeof(nullableValue));
        assert(*(cx_int*)block == nullableValue);
        CX_ID_4(cxcore, System, Memory, Free)(block);
    }
    block = (cx_byte*)CX_ID_4(cxcore, System, Memory, Alloc)(3);
    block[0] = 9;
    block = (cx_byte*)CX_ID_4(cxcore, System, Memory, Realloc)(block, 8);
    assert(block[0] == 9);
    assert(CX_ID_4(cxcore, System, Memory, Realloc)(block, 0) == CX_NULL);
}

static void test_object_constructor(void)
{
    struct CX_ID_3(cxcore, System, Object) value;
    CX_ID_4(cxcore, System, Object, __constructor)(&value);
    assert(value.__vtable != CX_NULL);
    assert(value._gc == CX_NULL);
    assert(CX_GET_TYPEINFO(&value).Size == sizeof(value));
}

static void test_public_runtime_constants(void)
{
    assert(CX_ID_4(cxcore, System, Int, MinValue) < 0);
    assert(CX_ID_4(cxcore, System, Int, MaxValue) > 0);
    assert(CX_ID_4(cxcore, System, Bool, FalseString) != CX_NULL);
    assert(CX_ID_4(cxcore, System, Bool, TrueString) != CX_NULL);
}

int main(void)
{
    test_char_utf8_lengths();
    test_string_utf8_indexing();
    test_array_and_nullable();
    test_memory_helpers();
    test_object_constructor();
    test_public_runtime_constants();
    return 0;
}

#include <assert.h>
#include <string.h>
#include "../src/cxcore.h"

static void test_char_utf8_lengths(void) {
    struct cxcore_System_Char value;
    value._value = 'A';
    assert(cxcore_System_Char_NumBytes___const_get(&value) == 1);
    value._value = 0x00A2;
    assert(cxcore_System_Char_NumBytes___const_get(&value) == 2);
    value._value = 0x20AC;
    assert(cxcore_System_Char_NumBytes___const_get(&value) == 3);
    value._value = 0x1F600;
    assert(cxcore_System_Char_NumBytes___const_get(&value) == 4);
    value._value = 0xD800;
    assert(cxcore_System_Char_NumBytes___const_get(&value) == -1);

    value._value = '7';
    assert(cxcore_System_Char_IsDigit___const_get(&value));
    assert(!cxcore_System_Char_IsLetter___const_get(&value));
    assert(!cxcore_System_Char_IsLower___const_get(&value));
    assert(!cxcore_System_Char_IsUpper___const_get(&value));
    assert(cxcore_System_Char_ToLower(&value) == '7');
    assert(cxcore_System_Char_ToUpper(&value) == '7');
    value._value = 'z';
    assert(cxcore_System_Char_IsLower___const_get(&value));
    assert(!cxcore_System_Char_IsUpper___const_get(&value));
    assert(cxcore_System_Char_ToUpper(&value) == 'Z');
    assert(cxcore_System_Char_ToLower(&value) == 'z');
    value._value = 'Q';
    assert(cxcore_System_Char_IsUpper___const_get(&value));
    assert(cxcore_System_Char_ToLower(&value) == 'q');
    assert(cxcore_System_Char_ToUpper(&value) == 'Q');
    value._value = ' ';
    assert(cxcore_System_Char_IsWhiteSpace___const_get(&value));
    value._value = '\t';
    assert(cxcore_System_Char_IsWhiteSpace___const_get(&value));
    value._value = '\r';
    assert(cxcore_System_Char_IsWhiteSpace___const_get(&value));
    value._value = '\n';
    assert(cxcore_System_Char_IsWhiteSpace___const_get(&value));
    value._value = 0x00E9;
    assert(!cxcore_System_Char_IsLetter___const_get(&value));
    assert(cxcore_System_Char_ToUpper(&value) == 0x00E9);
    assert(cxcore_System_Char_ToLower(&value) == 0x00E9);
}

static void test_string_utf8_indexing(void) {
    static const cx_byte utf8[] = {'A', 0xC2, 0xA2, 0xE2, 0x82, 0xAC, 0xF0, 0x9F, 0x98, 0x80};
    struct cxcore_System_String value;
    cxcore_System_String___constructor__2(&value, (cx_uint)sizeof(utf8), (cx_ptr)utf8);
    assert(cxcore_System_String_Length___const_get(&value) == 4);
    assert(cxcore_System_String_Item___const_get(&value, 0) == 'A');
    assert(cxcore_System_String_Item___const_get(&value, 1) == 0xA2);
    assert(cxcore_System_String_Item___const_get(&value, 2) == 0x20AC);
    assert(cxcore_System_String_Item___const_get(&value, 3) == 0x1F600);
    assert(cxcore_System_String_Item___const_get(&value, 4) == -1);

    {
        static const cx_byte same_left[] = {'c', 'x'};
        static const cx_byte same_right[] = {'c', 'x'};
        static const cx_byte different[] = {'c', 'y'};
        struct cxcore_System_String left;
        struct cxcore_System_String right;
        struct cxcore_System_String other;
        cxcore_System_String___constructor__2(
            &left, (cx_uint)sizeof(same_left), (cx_ptr)same_left);
        cxcore_System_String___constructor__2(
            &right, (cx_uint)sizeof(same_right), (cx_ptr)same_right);
        cxcore_System_String___constructor__2(
            &other, (cx_uint)sizeof(different), (cx_ptr)different);

        assert(cxcore_System_String___cx_operator_equal(&left, &right));
        assert(cxcore_System_String___cx_operator_not_equal(&left, &other));
        assert(cxcore_System_String_Equals(
            &left, (struct cxcore_System_Object *)&right));
        assert(!cxcore_System_String_Equals(
            &left, (struct cxcore_System_Object *)&other));
        assert(cxcore_System_String_GetHashCode(&left) ==
               cxcore_System_String_GetHashCode(&right));
        assert(cxcore_System_String_ToString(&left) == &left);
        assert(CX_GET_TYPEINFO(&left).Hash == 0x39E2CB3A11136EA4ULL);
    }

    {
        static const cx_byte left_bytes[] = {'A', 0xE2, 0x82, 0xAC};
        static const cx_byte right_bytes[] = {0xF0, 0x9F, 0x98, 0x80};
        static const cx_byte expected_bytes[] = {'A', 0xE2, 0x82, 0xAC,
                                                 0xF0, 0x9F, 0x98, 0x80};
        struct cxcore_System_String left;
        struct cxcore_System_String right;
        struct cxcore_System_String *joined;

        cxcore_System_String___constructor__2(
            &left, (cx_uint)sizeof(left_bytes), (cx_ptr)left_bytes);
        cxcore_System_String___constructor__2(
            &right, (cx_uint)sizeof(right_bytes), (cx_ptr)right_bytes);
        joined = cxcore_System_String___cx_operator_plus(&left, &right);
        assert(joined->_length == (cx_uint)sizeof(expected_bytes));
        assert(memcmp(joined->_data, expected_bytes, sizeof(expected_bytes)) == 0);
        assert(cxcore_System_String_Length___const_get(joined) == 3);
        cxcore_System_Memory_Free(joined->_data);
        cxcore_System_Memory_Free(joined);
    }

    {
        struct cxcore_System_String *joined;
        joined = cxcore_System_String___cx_operator_plus(CX_NULL, CX_NULL);
        assert(joined->_length == 0);
        assert(joined->_data == CX_NULL);
        cxcore_System_Memory_Free(joined);
    }

    {
        struct cxcore_System_String repeated;
        cxcore_System_String___constructor__3(&repeated, 3, 0x20AC);
        assert(repeated._length == 9);
        assert(cxcore_System_String_Length___const_get(&repeated) == 3);
        assert(cxcore_System_String_Item___const_get(&repeated, 2) == 0x20AC);
        cxcore_System_Memory_Free(repeated._data);
    }

    {
        static const cx_byte invalid[] = {0xF0, 0x80, 0x80, 0x80};
        struct cxcore_System_String malformed;
        cxcore_System_String___constructor__2(&malformed, (cx_uint)sizeof(invalid),
                                                           (cx_ptr)invalid);
        assert(cxcore_System_String_Length___const_get(&malformed) == 4);
        assert(cxcore_System_String_Item___const_get(&malformed, 0) == -1);
    }
}

static void test_array(void) {
    struct cxcore_System_Collections_Array *array = cx_array_new(3, (cx_uint)sizeof(cx_int));
    cx_int *first = (cx_int *)cx_array_at(array, 0, (cx_uint)sizeof(cx_int));
    cx_int *last = (cx_int *)cx_array_at(array, 2, (cx_uint)sizeof(cx_int));
    assert(array->_length == 3);
    assert(*first == 0 && *last == 0);
    *last = 42;
    assert(*(cx_int *)cx_array_at(array, 2, (cx_uint)sizeof(cx_int)) == 42);
    assert(cx_array_at(array, 3, (cx_uint)sizeof(cx_int)) == CX_NULL);
    assert(cx_array_at(array, 2, 0) == CX_NULL);
    assert(cx_array_at(CX_NULL, 0, (cx_uint)sizeof(cx_int)) == CX_NULL);
    {
        struct cxcore_System_Collections_Array *empty = cx_array_new(0, 0);
        assert(empty->_data == CX_NULL);
        assert(cx_array_at(empty, 0, 0) == CX_NULL);
        cxcore_System_Memory_Free(empty);
    }

    cxcore_System_Memory_Free(array->_data);
    cxcore_System_Memory_Free(array);
}

static void test_nullable(void) {
    struct cxcore_System_Nullable nullable;
    cx_int value = 42;
    cxcore_System_Nullable___constructor(&nullable, CX_NULL);
    assert(!cxcore_System_Nullable_HasValue___const_get(&nullable));
    cxcore_System_Nullable_Value___set(&nullable, &value);
    assert(cxcore_System_Nullable_HasValue___const_get(&nullable));
    assert(nullable._obj == &value);
    {
        struct cxcore_System_Nullable copied;
        cx_int *storage;
        cxcore_System_Nullable_Value___const_get(&nullable, &copied);
        assert(copied._obj == &value);
        storage = (cx_int *)cxcore_System_Nullable_CreateValueStorage(&value, (cx_uint)sizeof(value));
        assert(storage != &value && *storage == value);
        cxcore_System_Memory_Free(storage);
    }
}

static void test_memory_helpers(void) {
    cx_byte source[3] = {1, 2, 3};
    cx_byte destination[3] = {0, 0, 0};
    cx_int nullableValue = 17;
    cx_byte *block = (cx_byte *)cxcore_System_Memory_Alloc(0);
    assert(block != CX_NULL);
    cxcore_System_Memory_Free(block);
    cxcore_System_Memory_Copy(NULL, NULL, 0);
    cxcore_System_Memory_Copy(destination, source, 3);
    assert(memcmp(destination, source, sizeof(source)) == 0);
    {
        block = (cx_byte *)cxcore_System_Nullable_CreateValueStorage(
            &nullableValue, (cx_uint)sizeof(nullableValue));
        assert(*(cx_int *)block == nullableValue);
        cxcore_System_Memory_Free(block);
    }
    block = (cx_byte *)cxcore_System_Memory_Alloc(3);
    block[0] = 9;
    block = (cx_byte *)cxcore_System_Memory_Realloc(block, 8);
    assert(block[0] == 9);
    assert(cxcore_System_Memory_Realloc(block, 0) == CX_NULL);
}

static void test_object_constructor(void) {
    struct cxcore_System_Object value;
    cx_uint fieldCount = 0;
    const struct cx_reflection_field *fields;
    cxcore_System_Object___constructor(&value);
    assert(value.__vtable != CX_NULL);
    assert(value._box == CX_NULL);
    assert(CX_GET_TYPEINFO(&value).Size == sizeof(value));
    fields = cx_reflection_fields(
        &cxcore_System_Object___typeinfo, &fieldCount);
    assert(fieldCount == 1 && strcmp(fields[0].name, "_box") == 0);
}

static void test_public_runtime_constants(void) {
    assert(cxcore_System_Int_MinValue < 0);
    assert(cxcore_System_Int_MaxValue > 0);
    assert(cxcore_System_Bool_FalseString != CX_NULL);
    assert(cxcore_System_Bool_TrueString != CX_NULL);
}

static void test_reflection_function_info(void) {
    const struct cxcore_System_Reflection_TypeInfo *stringType =
        &cxcore_System_String___typeinfo;
    const struct cxcore_System_Reflection_TypeInfo *equalsParameterTypes[] = {
        &cxcore_System_String___typeinfo,
    };
    const struct cx_reflection_function *toString = cx_reflection_find_function(
        stringType, "ToString", 0, CX_NULL);
    const struct cx_reflection_function *equalOperator = cx_reflection_find_function(
        stringType, "__cx_operator_equal", 1, equalsParameterTypes);
    struct cxcore_System_Reflection_FunctionInfo info =
        cx_reflection_get_function_info(toString);
    struct cxcore_System_Reflection_FunctionInfo equalsInfo =
        cx_reflection_get_function_info(equalOperator);
    struct cxcore_System_Reflection_FunctionParamInfo *parameter;
    struct cxcore_System_Reflection_FunctionInfo empty =
        cx_reflection_get_function_info(CX_NULL);

    assert(toString != CX_NULL);
    assert(equalOperator != CX_NULL);
    assert(info.Flags == toString->flags);
    assert(info.Offset == toString->slot);
    assert(info.ReturnType.Hash == stringType->Hash);
    assert(info.Name != CX_NULL && info.Name->_length == strlen("ToString"));
    assert(memcmp(info.Name->_data, "ToString", strlen("ToString")) == 0);
    assert(info.Params == CX_NULL);

    assert(equalsInfo.Params != CX_NULL && equalsInfo.Params->_length == 1);
    parameter = (struct cxcore_System_Reflection_FunctionParamInfo *)
        cx_array_at((struct cxcore_System_Collections_Array *)equalsInfo.Params,
                    0, (cx_uint)sizeof(*parameter));
    assert(parameter->Type.Hash == stringType->Hash);
    assert(parameter->Name != CX_NULL && parameter->Name->_length == strlen("other"));
    assert(memcmp(parameter->Name->_data, "other", strlen("other")) == 0);

    assert(empty.Flags == 0 && empty.Offset == 0 && empty.Name == CX_NULL &&
           empty.Params == CX_NULL && empty.ReturnType.Hash == 0);

    {
        const struct cx_reflection_function *builder_to_string =
            cx_reflection_find_function(
                &cxcore_System_StringBuilder___typeinfo, "ToString", 0, CX_NULL);
        struct cxcore_System_Reflection_FunctionInfo builder_info =
            cx_reflection_get_function_info(builder_to_string);
        assert(builder_to_string != CX_NULL);
        assert((builder_info.Flags & CX_REFLECTION_FLAG_FUNCTION_NO_ESCAPE) != 0u);
    }

    {
        const struct cxcore_System_Reflection_TypeInfo *convert_parameters[] = {
            &cxcore_System_Object___typeinfo,
        };
        const struct cx_reflection_function *convert_value =
            cx_reflection_find_function(
                &cxcore_System_Reflection_TypeInfo___typeinfo,
                "ConvertValue", 1, convert_parameters);
        assert(convert_value != CX_NULL);
        assert((convert_value->flags & CX_REFLECTION_FLAG_FUNCTION_NO_ESCAPE) != 0u);
    }

    {
        const struct cxcore_System_Reflection_TypeInfo *read_array_parameters[] = {
            &cxcore_System_Reflection_TypeInfo___typeinfo,
            &cxcore_System_UInt___typeinfo,
            &cxcore_System_Object___typeinfo,
            &cxcore_System_UInt___typeinfo,
        };
        const struct cx_reflection_function *read_function_array =
            cx_reflection_find_function(
                &cxcore_System_Reflection_ModuleInfo___typeinfo,
                "ReadFunctionArray", 4, read_array_parameters);
        assert(read_function_array != CX_NULL);
        assert((read_function_array->flags &
                CX_REFLECTION_FLAG_FUNCTION_NO_ESCAPE) != 0u);
    }

    {
        const struct cxcore_System_Reflection_TypeInfo *uint_parameters[] = {
            &cxcore_System_UInt___typeinfo,
        };
        const struct cxcore_System_Reflection_TypeInfo *type_parameters[] = {
            &cxcore_System_Reflection_TypeInfo___typeinfo,
        };
        const struct cxcore_System_Reflection_TypeInfo *type_uint_parameters[] = {
            &cxcore_System_Reflection_TypeInfo___typeinfo,
            &cxcore_System_UInt___typeinfo,
        };
        const struct cx_reflection_function *functions[] = {
            cx_reflection_find_function(
                &cxcore_System_Reflection_ModuleInfo___typeinfo,
                "GetReference", 1, uint_parameters),
            cx_reflection_find_function(
                &cxcore_System_Reflection_ModuleInfo___typeinfo,
                "GetAnnotation", 1, uint_parameters),
            cx_reflection_find_function(
                &cxcore_System_Reflection_ModuleInfo___typeinfo,
                "GetGenericArguments", 1, type_parameters),
            cx_reflection_find_function(
                &cxcore_System_Reflection_ModuleInfo___typeinfo,
                "CanReadFunctionArray", 2, type_uint_parameters),
            cx_reflection_find_function(
                &cxcore_System_Reflection_ModuleInfo___typeinfo,
                "GetFunctionArrayElementType", 2, type_uint_parameters),
        };
        size_t index;
        for (index = 0; index < sizeof(functions) / sizeof(functions[0]); ++index) {
            assert(functions[index] != CX_NULL);
            assert((functions[index]->flags &
                    CX_REFLECTION_FLAG_FUNCTION_NO_ESCAPE) != 0u);
        }
    }

    {
        const struct cxcore_System_Reflection_TypeInfo *type_parameters[] = {
            &cxcore_System_Reflection_TypeInfo___typeinfo,
        };
        const struct cxcore_System_Reflection_TypeInfo *ulong_parameters[] = {
            &cxcore_System_ULong___typeinfo,
        };
        const struct cxcore_System_Reflection_TypeInfo *uint_parameters[] = {
            &cxcore_System_UInt___typeinfo,
        };
        const struct cxcore_System_Reflection_TypeInfo *two_uint_parameters[] = {
            &cxcore_System_UInt___typeinfo,
            &cxcore_System_UInt___typeinfo,
        };
        const struct cxcore_System_Reflection_TypeInfo *object_parameters[] = {
            &cxcore_System_Object___typeinfo,
        };
#define ASSERT_TYPEINFO_NO_ESCAPE(name, count, parameters) do { \
        const struct cx_reflection_function *function = cx_reflection_find_function( \
            &cxcore_System_Reflection_TypeInfo___typeinfo, name, count, parameters); \
        assert(function != CX_NULL); \
        assert((function->flags & CX_REFLECTION_FLAG_FUNCTION_NO_ESCAPE) != 0u); \
    } while (0)
        ASSERT_TYPEINFO_NO_ESCAPE("GetModule", 0, CX_NULL);
        ASSERT_TYPEINFO_NO_ESCAPE("IsAssignableFrom", 1, type_parameters);
        ASSERT_TYPEINFO_NO_ESCAPE("IsSameType", 1, type_parameters);
        ASSERT_TYPEINFO_NO_ESCAPE("IsInBaseTypeChain", 1, type_parameters);
        ASSERT_TYPEINFO_NO_ESCAPE("HasRuntimeInterface", 1, ulong_parameters);
        ASSERT_TYPEINFO_NO_ESCAPE("CreateInstance", 0, CX_NULL);
        ASSERT_TYPEINFO_NO_ESCAPE("GetFunction", 1, uint_parameters);
        ASSERT_TYPEINFO_NO_ESCAPE("CanInvokeFunction", 1, uint_parameters);
        ASSERT_TYPEINFO_NO_ESCAPE("GetFunctionCount", 0, CX_NULL);
        ASSERT_TYPEINFO_NO_ESCAPE("GetInterfaces", 0, CX_NULL);
        ASSERT_TYPEINFO_NO_ESCAPE("GetFunctionFlags", 1, uint_parameters);
        ASSERT_TYPEINFO_NO_ESCAPE("GetFunctionParameterCount", 1, uint_parameters);
        ASSERT_TYPEINFO_NO_ESCAPE("GetFunctionParameterType", 2, two_uint_parameters);
        ASSERT_TYPEINFO_NO_ESCAPE("GetEnumerableElementType", 0, CX_NULL);
        ASSERT_TYPEINFO_NO_ESCAPE("IsServiceSequence", 1, object_parameters);
#undef ASSERT_TYPEINFO_NO_ESCAPE
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        return 2;
    }
    if (strcmp(argv[1], "array") == 0)
        test_array();
    else if (strcmp(argv[1], "char") == 0)
        test_char_utf8_lengths();
    else if (strcmp(argv[1], "memory") == 0)
        test_memory_helpers();
    else if (strcmp(argv[1], "nullable") == 0)
        test_nullable();
    else if (strcmp(argv[1], "object") == 0) {
        test_object_constructor();
        test_public_runtime_constants();
    } else if (strcmp(argv[1], "string") == 0)
        test_string_utf8_indexing();
    else if (strcmp(argv[1], "reflection") == 0)
        test_reflection_function_info();
    else
        return 2;
    return 0;
}

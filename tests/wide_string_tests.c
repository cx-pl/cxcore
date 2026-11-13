#include "cxcore.h"
#include <assert.h>
#include <string.h>

typedef struct cxcore_System_WideString wide;
typedef struct cxcore_System_String string;
typedef struct cxcore_System_Interop_NativeBuffer buffer;
extern void cxcore_System_String___constructor__2(
    string *self, cx_uint length, cx_ptr data);

static void round_trip(const cx_byte *bytes, cx_uint length, cx_uint units, cx_uint scalars) {
    string input, *output;
    wide *value;
    buffer *lease;
    cxcore_System_String___constructor__2(&input, length, (cx_ptr)bytes);
    value = cxcore_System_String_ToWideString(&input);
    assert(cxcore_System_WideString_Length___const_get(value) == units);
    assert(cxcore_System_WideString_ScalarCount___const_get(value) == scalars);
    output = cxcore_System_WideString_ToString(value);
    assert(output->_length == length);
    assert(length == 0 || memcmp(output->_data, bytes, length) == 0);
    lease = cxcore_System_Interop_NativeBuffer_FromUtf16(value, CX_TRUE);
    assert(lease->_byteLength == ((cx_ulong)units + 1) * sizeof(cx_wchar));
    assert(((cx_wchar *)lease->_data)[units] == 0);
    if (units != 0) assert(memcmp(lease->_data, value->_data, units * sizeof(cx_wchar)) == 0);
    cxcore_System_Interop_NativeBuffer_Dispose(lease);
    cxcore_System_Interop_NativeBuffer_Dispose(lease);
    assert(lease->_allocation == NULL && lease->_data == NULL && lease->_byteLength == 0);
}

static void malformed_utf8(void) {
    const cx_byte bytes[] = {'A', 0xC0, 0xAF};
    string input;
    struct cx_exception_frame frame;
    cxcore_System_String___constructor__2(&input, sizeof(bytes), (cx_ptr)bytes);
    cx_exception_push(&frame);
    if (setjmp(frame.environment) == 0) {
        cxcore_System_String_ToWideString(&input);
        assert(!"Malformed UTF-8 must throw");
    } else {
        struct cxcore_System_EncodingException *error = cx_exception_current();
        assert(CX_GET_TYPEINFO(error).Hash == cxcore_System_EncodingException___typeinfo.Hash);
        assert(error->_position == 1);
    }
    cx_exception_pop(&frame);
}

static void malformed_utf16(cx_wchar bad) {
    const cx_wchar units[] = {'A', bad};
    wide value;
    struct cx_exception_frame frame;
    cxcore_System_WideString___constructor__2(&value, units, 2);
    cx_exception_push(&frame);
    if (setjmp(frame.environment) == 0) {
        cxcore_System_WideString_ToString(&value);
        assert(!"Unpaired surrogate must throw");
    } else {
        struct cxcore_System_EncodingException *error = cx_exception_current();
        assert(CX_GET_TYPEINFO(error).Hash == cxcore_System_EncodingException___typeinfo.Hash);
        assert(error->_position == 1);
    }
    cx_exception_pop(&frame);
}

int main(void) {
    static const cx_byte ascii[] = "abc";
    static const cx_byte bmp[] = {0xC3, 0xA4, 0xE2, 0x82, 0xAC};
    static const cx_byte supplementary[] = {0xF0, 0x9F, 0x98, 0x80};
    static const cx_byte zero[] = {'a', 0, 'b'};
    const cx_wchar pair[] = {0xD83D, 0xDE00};
    wide value, copy;
    cx_gc_attach_thread();
    assert(sizeof(cx_wchar) == 2);
    round_trip(NULL, 0, 0, 0);
    round_trip(ascii, 3, 3, 3);
    round_trip(bmp, sizeof(bmp), 2, 2);
    round_trip(supplementary, sizeof(supplementary), 2, 1);
    round_trip(zero, sizeof(zero), 3, 3);
    cxcore_System_WideString___constructor__2(&value, pair, 2);
    cxcore_System_WideString___constructor__2(&copy, pair, 2);
    assert(cxcore_System_WideString_Item___const_get(&value, 0) == 0xD83D);
    assert(cxcore_System_WideString_CodeUnitAt(&value, 0) == 0xD83D);
    assert(cxcore_System_WideString_CodeUnitAt(&value, 1) == 0xDE00);
    assert(cxcore_System_WideString_ScalarAt(&value, 0) == 0x1F600);
    {
        struct cx_exception_frame frame;
        cx_exception_push(&frame);
        if (setjmp(frame.environment) == 0) {
            (void)cxcore_System_WideString_CodeUnitAt(&value, 2);
            assert(!"WideString.CodeUnitAt must reject an out-of-range index");
        } else {
            struct cxcore_System_EncodingException *error = cx_exception_current();
            assert(CX_GET_TYPEINFO(error).Hash ==
                   cxcore_System_EncodingException___typeinfo.Hash);
        }
        cx_exception_pop(&frame);
        cx_exception_clear();
    }
    {
        struct cx_exception_frame frame;
        cx_exception_push(&frame);
        if (setjmp(frame.environment) == 0) {
            (void)cxcore_System_WideString_Item___const_get(&value, 2);
            assert(!"WideString indexer must throw IndexOutOfRangeException");
        }
        assert(cxcore_System_Exception_Matches(
            (const struct cxcore_System_Exception *)cx_exception_current(),
            cxcore_System_IndexOutOfRangeException___typeinfo));
        cx_exception_pop(&frame);
        cx_exception_clear();
    }
    {
        static CX_STRING_DEF(expected_alignment, "alignment");
        struct cx_exception_frame frame;
        cx_exception_push(&frame);
        if (setjmp(frame.environment) == 0) {
            (void)cxcore_System_Interop_NativeBuffer_AllocateAligned(8, 3);
            assert(!"Invalid NativeBuffer alignment must throw");
        }
        assert(cxcore_System_Exception_Matches(
            (const struct cxcore_System_Exception *)cx_exception_current(),
            cxcore_System_ArgumentOutOfRangeException___typeinfo));
        assert(cxcore_System_String___cx_operator_equal(
            cxcore_System_ArgumentException_ParamName___const_get(
                (const struct cxcore_System_ArgumentException *)
                    cx_exception_current()),
            &expected_alignment));
        cx_exception_pop(&frame);
        cx_exception_clear();
    }
    {
        static CX_STRING_DEF(expected_value, "value");
        struct cx_exception_frame frame;
        cx_exception_push(&frame);
        if (setjmp(frame.environment) == 0) {
            (void)cxcore_System_Interop_NativeBuffer_FromUtf8(CX_NULL, CX_TRUE);
            assert(!"Null NativeBuffer input must throw ArgumentNullException");
        }
        assert(cxcore_System_Exception_Matches(
            (const struct cxcore_System_Exception *)cx_exception_current(),
            cxcore_System_ArgumentNullException___typeinfo));
        assert(cxcore_System_String___cx_operator_equal(
            cxcore_System_ArgumentException_ParamName___const_get(
                (const struct cxcore_System_ArgumentException *)
                    cx_exception_current()),
            &expected_value));
        cx_exception_pop(&frame);
        cx_exception_clear();
    }
    {
        buffer *disposed = cxcore_System_Interop_NativeBuffer_Allocate(1);
        struct cx_exception_frame frame;
        cxcore_System_Interop_NativeBuffer_Dispose(disposed);
        cx_exception_push(&frame);
        if (setjmp(frame.environment) == 0) {
            (void)cxcore_System_Interop_NativeBuffer_ReadByte(disposed, 0);
            assert(!"Reading a disposed NativeBuffer must throw");
        }
        assert(cxcore_System_Exception_Matches(
            (const struct cxcore_System_Exception *)cx_exception_current(),
            cxcore_System_ObjectDisposedException___typeinfo));
        cx_exception_pop(&frame);
        cx_exception_clear();
    }
    assert(cxcore_System_WideString___cx_operator_equal(&value, &copy));
    assert(cxcore_System_WideString_GetHashCode(&value) == cxcore_System_WideString_GetHashCode(&copy));
    assert(cxcore_System_WideString_CompareOrdinal(&value, &copy) == 0);
    // Native storage is a copy, so changing the original does not mutate a value.
    assert(value._data != pair && copy._data != value._data);
    malformed_utf8(); malformed_utf16(0xD800); malformed_utf16(0xDC00);
    return 0;
}

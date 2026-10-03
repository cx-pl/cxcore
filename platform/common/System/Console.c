#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../../src/cxcore.h"

#define INITIAL_READ_CAPACITY 128U

static struct CX_ID_3(cxcore, System, String) *
    cx_console_string_from_buffer(char *buffer, cx_uint length) {
    struct CX_ID_3(cxcore, System, String) *result =
        (struct CX_ID_3(cxcore, System, String) *)CX_ID_4(cxcore, System, Memory, Alloc)(
            (cx_uint)sizeof(struct CX_ID_3(cxcore, System, String)));
    CX_ID_5(cxcore, System, String, __constructor, _2)(result, length, buffer);
    return result;
}

struct CX_ID_3(cxcore, System, String) * CX_ID_4(cxcore, System, Console, Read)(cx_uint length) {
    char *input;
    cx_uint count = 0;

    if (length == 0) {
        return cx_console_string_from_buffer(NULL, 0);
    }
    input = (char *)CX_ID_4(cxcore, System, Memory, Alloc)(length);
    while (count < length) {
        int ch = getchar();
        if (ch == EOF) {
            break;
        }
        input[count++] = (char)ch;
    }
    if (count == 0) {
        CX_ID_4(cxcore, System, Memory, Free)(input);
        input = NULL;
    }
    return cx_console_string_from_buffer(input, count);
}

struct CX_ID_3(cxcore, System, String) * CX_ID_4(cxcore, System, Console, ReadLine)() {
    char *input = (char *)CX_ID_4(cxcore, System, Memory, Alloc)(INITIAL_READ_CAPACITY);
    cx_uint capacity = INITIAL_READ_CAPACITY;
    cx_uint count = 0;
    int ch;

    while ((ch = getchar()) != '\n' && ch != EOF) {
        if (count == capacity) {
            cx_uint newCapacity;
            if (capacity > UINT_MAX / 2U) {
                CX_ID_4(cxcore, System, Memory, Free)(input);
                abort();
            }
            newCapacity = capacity * 2U;
            input = (char *)CX_ID_4(cxcore, System, Memory, Realloc)(input, newCapacity);
            capacity = newCapacity;
        }
        input[count++] = (char)ch;
    }

    if (count == 0) {
        CX_ID_4(cxcore, System, Memory, Free)(input);
        input = NULL;
    }
    return cx_console_string_from_buffer(input, count);
}

cx_uint CX_ID_4(cxcore, System, Console,
                Write)(const struct CX_ID_3(cxcore, System, String) * str) {
    size_t written;
    if (str == NULL || (str->_length != 0 && str->_data == NULL)) {
        return 0;
    }
    written = fwrite(str->_data, 1, str->_length, stdout);
    return written > UINT_MAX ? UINT_MAX : (cx_uint)written;
}

cx_uint CX_ID_4(cxcore, System, Console,
                WriteLine)(const struct CX_ID_3(cxcore, System, String) * str) {
    cx_uint written = CX_ID_4(cxcore, System, Console, Write)(str);
    const struct CX_ID_3(cxcore, System, String) *newline =
        CX_ID_5(cxcore, System, Environment, NewLine, __const_get)();
    cx_uint newlineWritten = CX_ID_4(cxcore, System, Console, Write)(newline);
    return UINT_MAX - written < newlineWritten ? UINT_MAX : written + newlineWritten;
}

#include <stdio.h>
#include "../../../src/cxcore.h"

#define MAX_READ_BUFFER_SIZE 256

struct CX_ID_3(cxcore, System, String)* CX_ID_4(cxcore, System, Console, Read)(    
    cx_uint length
) {
    char input[MAX_READ_BUFFER_SIZE];
    int count = 0;
    int ch;
    char* str;
    struct CX_ID_3(cxcore, System, String)* result;

    // TODO: Throw if length > MAX_READ_BUFFER_SIZE

    // Read getchar() until:
    // * EOF
    // * chars read reach MAX_READ_BUFFER_SIZE
    // * chars read reach length argument
    while ((ch = getchar()) != EOF && count < MAX_READ_BUFFER_SIZE && (cx_uint)count < length) {
        input[count++] = (char)ch;
    }

    // Allocate string buffer on heap and copy from stack-allocated buffer
    str = CX_ID_4(cxcore, System, Memory, Alloc)(
        count
    );
    CX_ID_4(cxcore, System, Memory, Copy)(str, input, count);

    // Allocate String object and call a constructor with allocated string
    result = CX_ID_4(cxcore, System, Memory, Alloc)(
        sizeof(struct CX_ID_3(cxcore, System, String))
    );
    CX_ID_5(cxcore, System, String, __constructor, _2)(result, (cx_uint)count, input);

    return result;
}

struct CX_ID_3(cxcore, System, String)* CX_ID_4(cxcore, System, Console, ReadLine)(
) {
    char input[MAX_READ_BUFFER_SIZE];
    char count = 0;
    int ch;
    char* str;
    struct CX_ID_3(cxcore, System, String)* result;
    
    // TODO: Throw if length > MAX_READ_BUFFER_SIZE

    // Read getchar() until:
    // * newline
    // * EOF
    // * chars read reach MAX_READ_BUFFER_SIZE
    while ((ch = getchar()) != '\n' && ch != EOF && count < sizeof(input)) {
        input[count++] = (char)ch;
    }

    // Allocate string buffer on heap and copy from stack-allocated buffer
    str = CX_ID_4(cxcore, System, Memory, Alloc)(
        count
    );
    CX_ID_4(cxcore, System, Memory, Copy)(str, input, count);

    // Allocate String object and call a constructor with allocated string
    result = CX_ID_4(cxcore, System, Memory, Alloc)(
        sizeof(struct CX_ID_3(cxcore, System, String))
    );
    CX_ID_5(cxcore, System, String, __constructor, _2)(result, (cx_uint)count, input);

    return result;
}

cx_uint CX_ID_4(cxcore, System, Console, Write)(    
    const struct CX_ID_3(cxcore, System, String)* str
) {
    cx_uint n;
    const char* buf = str->_data;

    for (n = 0; n < str->_length; n++, buf += CX_ID_4(cxcore, System, Char, NumBytes)((const struct CX_ID_3(cxcore, System, Char)*)buf)) {
        putchar(*(int*)buf);
    }

    return n;
}

cx_uint CX_ID_4(cxcore, System, Console, WriteLine)(    
    const struct CX_ID_3(cxcore, System, String)* str
) {
    return
        CX_ID_4(cxcore, System, Console, Write)(str) +
        CX_ID_4(cxcore, System, Console, Write)(CX_ID_5(cxcore, System, Environment, NewLine, __const_get)());
}

#include <stdio.h>
#include "src/cxcore.h"

#define MAX_READ_BUFFER_SIZE 256

struct CX_ID_3(cxcore, System, String)* CX_ID_4(cxcore, System, Console, Read)(    
    cx_uint length
) {
    char input[MAX_READ_BUFFER_SIZE];
    char count = 0;
    int ch;
    char* str;
    struct CX_ID_3(cxcore, System, String)* result;

    // TODO: Throw if length > MAX_READ_BUFFER_SIZE

    // Read getchar() until:
    // * EOF
    // * chars read reach MAX_READ_BUFFER_SIZE
    // * chars read reach length argument
    while ((ch = getchar()) != EOF && count < MAX_READ_BUFFER_SIZE && count < length) {
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
    CX_ID_4(cxcore, System, String, __constructor_2)(result, (cx_uint)count, input);

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
    CX_ID_4(cxcore, System, String, __constructor_2)(result, (cx_uint)count, input);

    return result;
}

cx_uint CX_ID_4(cxcore, System, Console, Write)(    
    struct CX_ID_3(cxcore, System, String)* str
) {
    cx_uint n;
    char* buf = str->_data;

    // TODO: Support UTF-8 variable-length characters
    for (n = 0; n < str->_length; n++) {
        putchar(buf[n]);
    }

    return n;
}

cx_uint CX_ID_4(cxcore, System, Console, WriteLine)(    
    struct CX_ID_3(cxcore, System, String)* str
) {
    cx_uint n;

    n = CX_ID_4(cxcore, System, Console, Write)(str);

    // TODO: Read from Environment CX_ID_5(cxcore, System, Environment, __get, NewLine)
    puts("\r\n");
    n += 2;

    return n;
}

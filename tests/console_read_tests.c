#include <assert.h>
#include <string.h>
#include "../src/cxcore.h"
#include "../platform/common/cxarena.h"

int main(void) {
    static const char output[] = "xyz";
    struct cxcore_System_String *line = cxcore_System_Console_ReadLine();
    struct cxcore_System_String *emptyLine;
    struct cxcore_System_String *prefix;
    struct cxcore_System_String outputString;
    struct cxcore_System_Interop_NativeBuffer *inputBuffer =
        cxcore_System_Interop_NativeBuffer_Allocate(3);
    struct cxcore_System_Interop_NativeBuffer *outputBuffer =
        cxcore_System_Interop_NativeBuffer_Allocate(3);
    cx_uint written;
    cx_uint nullWritten;
    cx_uint rawRead;
    cx_uint rawWritten;
    struct cx_arena *arena = cx_arena_create(128u, 0u, 0);
    struct cx_arena_scope arena_scope;
    assert(line->_length == 300);
    assert(((const char *)line->_data)[0] == 'a');
    assert(((const char *)line->_data)[299] == 'a');
    cx_arena_enter(arena, &arena_scope);
    emptyLine = cxcore_System_Console_ReadLine();
    assert(emptyLine->_length == 0);
    cx_arena_leave(&arena_scope);
    cx_arena_destroy(arena);
    prefix = cxcore_System_Console_Read(3);
    assert(prefix->_length == 3);
    assert(memcmp(prefix->_data, "xyz", 3) == 0);
    assert(((const char *)line->_data)[10] == 'a');
    rawRead = cxcore_System_Interop_NativeBuffer_ReadFromStandardInput(inputBuffer, 0, 3);
    assert(rawRead == 3);
    assert(cxcore_System_Interop_NativeBuffer_ReadByte(inputBuffer, 0) == 'R');
    assert(cxcore_System_Interop_NativeBuffer_ReadByte(inputBuffer, 1) == 'A');
    assert(cxcore_System_Interop_NativeBuffer_ReadByte(inputBuffer, 2) == 'W');
    cxcore_System_Interop_NativeBuffer_WriteByte(outputBuffer, 0, 'X');
    cxcore_System_Interop_NativeBuffer_WriteByte(outputBuffer, 1, 'Y');
    cxcore_System_Interop_NativeBuffer_WriteByte(outputBuffer, 2, 'Z');
    cxcore_System_Interop_NativeBuffer_CopyTo(inputBuffer, outputBuffer, 0, 0, 3);
    assert(cxcore_System_Interop_NativeBuffer_ReadByte(outputBuffer, 0) == 'R');
    rawWritten = cxcore_System_Interop_NativeBuffer_WriteToStandardOutput(
        outputBuffer, 0, 3);
    assert(rawWritten == 3);
    cxcore_System_String___constructor__2(&outputString, (cx_uint)(sizeof(output) - 1),
                                                       (cx_ptr)output);
    written = cxcore_System_Console_Write(&outputString);
    nullWritten = cxcore_System_Console_Write(CX_NULL);
    assert(written == (cx_uint)(sizeof(output) - 1));
    assert(nullWritten == 0);
    cxcore_System_Memory_Free(line->_data);
    cxcore_System_Memory_Free(line);
    cxcore_System_Memory_Free(prefix->_data);
    cxcore_System_Memory_Free(prefix);
    cxcore_System_Interop_NativeBuffer_Dispose(inputBuffer);
    cxcore_System_Interop_NativeBuffer_Dispose(outputBuffer);
    return 0;
}

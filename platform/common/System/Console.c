#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../platform.h"
#include "../../../src/cxcore.h"
#include "../../../include/cxruntime.h"

#define INITIAL_READ_CAPACITY 128U

struct cx_console_native_io {
    int character;
    const char *bytes;
    size_t length;
    size_t written;
};
static CX_THREAD_LOCAL struct cx_console_native_io cx_console_io;

static void cx_console_read_character(void *context) {
    ((struct cx_console_native_io *)context)->character = getchar();
}

static int cx_console_getchar(void) {
    cx_gc_run_blocking(cx_console_read_character, &cx_console_io);
    return cx_console_io.character;
}

static void cx_console_write_bytes(void *context) {
    struct cx_console_native_io *io = (struct cx_console_native_io *)context;
    io->written = fwrite(io->bytes, 1, io->length, stdout);
}

static struct cxcore_System_String *
    cx_console_string_from_buffer(char *buffer, cx_uint length) {
    if (length == 0) {
        return (struct cxcore_System_String *)cxcore_System_String_Empty___const_get();
    }

    struct cxcore_System_String *result =
        (struct cxcore_System_String *)cxcore_System_Memory_Alloc(
            (cx_uint)sizeof(struct cxcore_System_String));
    cxcore_System_String___constructor__2(result, length, buffer);
    return result;
}

struct cxcore_System_String * cxcore_System_Console_Read(cx_uint length) {
    char *input;
    cx_uint count = 0;

    if (length == 0) {
        return cx_console_string_from_buffer(NULL, 0);
    }
    input = (char *)cxcore_System_Memory_Alloc(length);
    while (count < length) {
        int ch = cx_console_getchar();
        if (ch == EOF) {
            break;
        }
        input[count++] = (char)ch;
    }
    if (count == 0) {
        if (cx_gc_is_durable_reference((cx_ptr)input))
            cxcore_System_Memory_Free(input);
        input = NULL;
    }
    return cx_console_string_from_buffer(input, count);
}

struct cxcore_System_String * cxcore_System_Console_ReadLine() {
    char *input = (char *)cxcore_System_Memory_Alloc(INITIAL_READ_CAPACITY);
    cx_uint capacity = INITIAL_READ_CAPACITY;
    cx_uint count = 0;
    int ch;

    while ((ch = cx_console_getchar()) != '\n' && ch != EOF) {
        if (count == capacity) {
            cx_uint newCapacity;
            if (capacity > UINT_MAX / 2U) {
                if (cx_gc_is_durable_reference((cx_ptr)input))
                    cxcore_System_Memory_Free(input);
                abort();
            }
            newCapacity = capacity * 2U;
            input = (char *)cxcore_System_Memory_Realloc(input, newCapacity);
            capacity = newCapacity;
        }
        input[count++] = (char)ch;
    }

    if (count == 0) {
        if (cx_gc_is_durable_reference((cx_ptr)input))
            cxcore_System_Memory_Free(input);
        input = NULL;
    }
    return cx_console_string_from_buffer(input, count);
}

cx_uint cxcore_System_Console_Write(const struct cxcore_System_String * str) {
    size_t written;
    char *copy;
    if (str == NULL || (str->_length != 0 && str->_data == NULL)) {
        return 0;
    }
    // The parked native callback must not read managed storage. Its private
    // copy and writable wait state live outside the scanned managed stack.
    copy = (char *)malloc(str->_length == 0 ? 1 : str->_length);
    if (copy == NULL) {
        fputs("Out of memory\n", stderr);
        exit(EXIT_FAILURE);
    }
    if (str->_length != 0) {
        memcpy(copy, str->_data, str->_length);
    }
    cx_console_io.bytes = copy;
    cx_console_io.length = str->_length;
    cx_gc_run_blocking(cx_console_write_bytes, &cx_console_io);
    written = cx_console_io.written;
    free(copy);
    cx_console_io.bytes = NULL;
    return written > UINT_MAX ? UINT_MAX : (cx_uint)written;
}

cx_uint cxcore_System_Console_WriteLine(const struct cxcore_System_String * str) {
    cx_uint written = cxcore_System_Console_Write(str);
    const struct cxcore_System_String *newline =
        cxcore_System_Environment_NewLine___const_get();
    cx_uint newlineWritten = cxcore_System_Console_Write(newline);
    return UINT_MAX - written < newlineWritten ? UINT_MAX : written + newlineWritten;
}

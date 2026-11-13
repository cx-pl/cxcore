#include "cxcore.h"
#include "../../../platform.h"
#include "../Text.internal.h"
#include "../Utf8.h"
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(CX_PLATFORM_WIN)
#include <fcntl.h>
#include <io.h>
#else
#include <unistd.h>
#endif

typedef struct cxcore_System_WideString cx_wide_string;

typedef struct cxcore_System_Interop_NativeBuffer cx_native_buffer;

struct cx_native_buffer_stream_io {
    char *readBytes;
    const char *writeBytes;
    size_t length;
    size_t transferred;
    int error;
    int nativeErrorCode;
};
static CX_THREAD_LOCAL struct cx_native_buffer_stream_io cx_native_buffer_io;

static void cx_native_buffer_read_stdin(void *context) {
    struct cx_native_buffer_stream_io *io = (struct cx_native_buffer_stream_io *)context;
#if defined(CX_PLATFORM_WIN)
    if (_setmode(_fileno(stdin), _O_BINARY) == -1) {
        io->error = 1;
        io->nativeErrorCode = errno;
        return;
    }
#endif
    io->transferred = fread(io->readBytes, 1, io->length, stdin);
    if (ferror(stdin)) {
        io->error = 1;
        io->nativeErrorCode = errno;
    }
}

static void cx_native_buffer_write_stdout(void *context) {
    struct cx_native_buffer_stream_io *io = (struct cx_native_buffer_stream_io *)context;
#if defined(CX_PLATFORM_WIN)
    int descriptor = _fileno(stdout);
    int previous_mode;
    if (fflush(stdout) != 0) {
        io->error = 1;
        io->nativeErrorCode = errno;
        return;
    }
    previous_mode = _setmode(descriptor, _O_BINARY);
    if (previous_mode == -1) {
        io->error = 1;
        io->nativeErrorCode = errno;
        return;
    }
    while (io->transferred < io->length) {
        size_t remaining = io->length - io->transferred;
        unsigned int chunk = remaining > INT_MAX ? INT_MAX : (unsigned int)remaining;
        int written = _write(descriptor,
            io->writeBytes + io->transferred, chunk);
        if (written <= 0) {
            io->error = 1;
            io->nativeErrorCode = errno;
            break;
        }
        io->transferred += (size_t)written;
    }
#else
    if (fflush(stdout) != 0) {
        io->error = 1;
        io->nativeErrorCode = errno;
        return;
    }
    while (io->transferred < io->length) {
        ssize_t written = write(STDOUT_FILENO,
            io->writeBytes + io->transferred, io->length - io->transferred);
        if (written < 0 && errno == EINTR) continue;
        if (written <= 0) {
            io->error = 1;
            io->nativeErrorCode = errno;
            break;
        }
        io->transferred += (size_t)written;
    }
#endif
}

cx_native_buffer *cxcore_System_Interop_NativeBuffer_Allocate(cx_ulong length) {
    cx_native_buffer *result;
    cx_gc_check_thread();
    if (length > SIZE_MAX) cx_text_overflow();
    result = cxcore_System_Memory_Alloc((cx_uint)sizeof(*result));
    cxcore_System_Interop_NativeBuffer___constructor(result);
    (void)cx_gc_store_native_pointer(&result->_data,
        malloc(length == 0 ? 1 : (size_t)length));
    if (result->_data == NULL) abort();
    (void)cx_gc_store_native_pointer(&result->_allocation, result->_data);
    result->_byteLength = length;
    return result;
}
cx_native_buffer *cxcore_System_Interop_NativeBuffer_AllocateAligned(cx_ulong length, cx_uint alignment) {
    cx_native_buffer *result;
    size_t capacity;
    cx_gc_check_thread();
    if (alignment == 0 || alignment > 1024 * 1024 ||
        (alignment & (alignment - 1)) != 0) {
        cx_text_argument_range("alignment");
    }
    if (length > SIZE_MAX - alignment) cx_text_overflow();
    result = cxcore_System_Memory_Alloc((cx_uint)sizeof(*result));
    cxcore_System_Interop_NativeBuffer___constructor(result);
    capacity = (size_t)(length == 0 ? 1 : length) + alignment - 1;
    (void)cx_gc_store_native_pointer(&result->_allocation, malloc(capacity));
    if (result->_allocation == NULL) abort();
    (void)cx_gc_store_native_pointer(&result->_data,
        cx_interop_align_pointer(result->_allocation, alignment));
    result->_byteLength = length;
    return result;
}
cx_ptr cxcore_System_Interop_NativeBuffer_Data___const_get(const cx_native_buffer *self) { return self->_data; }
cx_ulong cxcore_System_Interop_NativeBuffer_ByteLength___const_get(const cx_native_buffer *self) { return self->_byteLength; }
cx_byte cxcore_System_Interop_NativeBuffer_ReadByte(const cx_native_buffer *self, cx_ulong offset) {
    if (self == NULL) cx_text_argument_null("self");
    if (self->_data == NULL) cx_text_object_disposed();
    if (offset >= self->_byteLength) cx_text_argument_range("offset");
    return ((const cx_byte *)self->_data)[offset];
}
void cxcore_System_Interop_NativeBuffer_WriteByte(cx_native_buffer *self, cx_ulong offset, cx_byte value) {
    if (self == NULL) cx_text_argument_null("self");
    if (self->_data == NULL) cx_text_object_disposed();
    if (offset >= self->_byteLength) cx_text_argument_range("offset");
    ((cx_byte *)self->_data)[offset] = value;
}
cx_uint cxcore_System_Interop_NativeBuffer_ReadFromStandardInput(
    cx_native_buffer *self, cx_ulong offset, cx_uint length) {
    if (self == NULL) cx_text_argument_null("self");
    if (offset > self->_byteLength ||
        (cx_ulong)length > self->_byteLength - offset) {
        cx_text_argument_range("offset");
    }
    if (length != 0 && self->_data == NULL) cx_text_object_disposed();
    if (length == 0) return 0;
    cx_native_buffer_io.readBytes = (char *)self->_data + (size_t)offset;
    cx_native_buffer_io.length = length;
    cx_native_buffer_io.transferred = 0;
    cx_native_buffer_io.error = 0;
    cx_native_buffer_io.nativeErrorCode = 0;
    cx_gc_run_blocking(cx_native_buffer_read_stdin, &cx_native_buffer_io);
    cx_native_buffer_io.readBytes = NULL;
    if (cx_native_buffer_io.error) {
        cx_text_io("ReadFromStandardInput", "errno",
            cx_native_buffer_io.nativeErrorCode);
    }
    return cx_native_buffer_io.transferred > UINT_MAX
        ? UINT_MAX : (cx_uint)cx_native_buffer_io.transferred;
}
cx_uint cxcore_System_Interop_NativeBuffer_WriteToStandardOutput(
    cx_native_buffer *self, cx_ulong offset, cx_uint length) {
    if (self == NULL) cx_text_argument_null("self");
    if (offset > self->_byteLength ||
        (cx_ulong)length > self->_byteLength - offset) {
        cx_text_argument_range("offset");
    }
    if (length != 0 && self->_data == NULL) cx_text_object_disposed();
    if (length == 0) return 0;
    cx_native_buffer_io.writeBytes = (const char *)self->_data + (size_t)offset;
    cx_native_buffer_io.length = length;
    cx_native_buffer_io.transferred = 0;
    cx_native_buffer_io.error = 0;
    cx_native_buffer_io.nativeErrorCode = 0;
    cx_gc_run_blocking(cx_native_buffer_write_stdout, &cx_native_buffer_io);
    cx_native_buffer_io.writeBytes = NULL;
    if (cx_native_buffer_io.error) {
        cx_text_io("WriteToStandardOutput", "errno",
            cx_native_buffer_io.nativeErrorCode);
    }
    return cx_native_buffer_io.transferred > UINT_MAX
        ? UINT_MAX : (cx_uint)cx_native_buffer_io.transferred;
}
void cxcore_System_Interop_NativeBuffer_CopyTo(
    const cx_native_buffer *self, cx_native_buffer *destination,
    cx_ulong sourceOffset, cx_ulong destinationOffset, cx_ulong length) {
    if (self == NULL) cx_text_argument_null("self");
    if (destination == NULL) cx_text_argument_null("destination");
    if (sourceOffset > self->_byteLength ||
        length > self->_byteLength - sourceOffset ||
        destinationOffset > destination->_byteLength ||
        length > destination->_byteLength - destinationOffset) {
        cx_text_argument_range("length");
    }
    if (length != 0 && (self->_data == NULL || destination->_data == NULL)) {
        cx_text_object_disposed();
    }
    if (length != 0) {
        memmove((char *)destination->_data + (size_t)destinationOffset,
            (const char *)self->_data + (size_t)sourceOffset, (size_t)length);
    }
}
void cxcore_System_Interop_NativeBuffer_Dispose(cx_native_buffer *self) {
    cx_ptr allocation;
    cx_gc_check_thread();
    if (self == NULL) cx_text_null_reference();
    allocation = self->_allocation;
    (void)cx_gc_store_native_pointer(&self->_allocation, NULL);
    (void)cx_gc_store_native_pointer(&self->_data, NULL);
    self->_byteLength = 0;
    free(allocation);
}
void cxcore_System_Interop_NativeBuffer_Copy(cx_ptr destination, cx_ptr source, cx_ulong length) {
    cx_gc_check_thread();
    if (length > SIZE_MAX) cx_text_overflow();
    if (length != 0 && destination == NULL) cx_text_argument_null("destination");
    if (length != 0 && source == NULL) cx_text_argument_null("source");
    if (length != 0) memmove(destination, source, (size_t)length);
}
cx_native_buffer *cxcore_System_Interop_NativeBuffer_FromUtf8(
    struct cxcore_System_String *value, cx_bool terminated) {
    cx_native_buffer *result;
    if (value == NULL) cx_text_argument_null("value");
    result = cxcore_System_Interop_NativeBuffer_Allocate((cx_ulong)value->_length + (terminated ? 1 : 0));
    if (value->_length != 0) memcpy(result->_data, value->_data, value->_length);
    if (terminated) ((cx_byte *)result->_data)[value->_length] = 0;
    return result;
}
cx_native_buffer *cxcore_System_Interop_NativeBuffer_FromUtf16(cx_wide_string *value, cx_bool terminated) {
    cx_native_buffer *result;
    if (value == NULL) cx_text_argument_null("value");
    result = cxcore_System_Interop_NativeBuffer_Allocate(((cx_ulong)value->_length + (terminated ? 1 : 0)) * sizeof(cx_wchar));
    if (value->_length != 0) memcpy(result->_data, value->_data, value->_length * sizeof(cx_wchar));
    if (terminated) ((cx_wchar *)result->_data)[value->_length] = 0;
    return result;
}

struct cxcore_System_String *cxcore_System_Interop_NativeBuffer_ToUtf8String(
    const cx_native_buffer *self, cx_uint length) {
    cx_uint scalar;
    size_t offset = 0, count;
    cx_byte *data;
    struct cxcore_System_String *result;
    cx_gc_check_thread();
    if (self == NULL) cx_text_argument_null("self");
    if (length > self->_byteLength) cx_text_argument_range("length");
    if (length != 0 && self->_data == NULL) cx_text_object_disposed();
    while (offset < length) {
        if (!cx_utf8_decode((const cx_byte *)self->_data + offset, length - offset, &scalar, &count)) cx_encoding_error((cx_uint)offset);
        offset += count;
    }
    data = length == 0 ? NULL : cxcore_System_Memory_Alloc(length);
    if (length != 0) memcpy(data, self->_data, length);
    result = cxcore_System_Memory_Alloc((cx_uint)sizeof(*result));
    cxcore_System_String___constructor__2(result, length, data);
    return result;
}
cx_wide_string *cxcore_System_Interop_NativeBuffer_ToWideString(const cx_native_buffer *self, cx_uint length) {
    cx_wide_string *result;
    cx_gc_check_thread();
    if (self == NULL) cx_text_argument_null("self");
    if ((cx_ulong)length * sizeof(cx_wchar) > self->_byteLength) {
        cx_text_argument_range("length");
    }
    if (length != 0 && self->_data == NULL) cx_text_object_disposed();
    result = cxcore_System_Memory_Alloc((cx_uint)sizeof(*result));
    cxcore_System_WideString___constructor__2(result, self->_data, length);
    return result;
}

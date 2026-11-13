#ifndef __CX_STDLIB_PLATFORM_H__
#define __CX_STDLIB_PLATFORM_H__

#include <stddef.h>
#include <stdint.h>

// Generated delegate invocation helpers use abort for a null delegate. This
// platform compatibility header shadows the system stdlib.h in CX consumers,
// so keep the CRT declaration visible when generated C includes it.
#if defined(_MSC_VER)
#include <corecrt.h>
_ACRTIMP __declspec(noreturn) void __cdecl abort(void);
#elif defined(__GNUC__) || defined(__clang__)
_Noreturn void abort(void);
#else
void abort(void);
#endif

int cx_platform_random_bytes(void *buffer, size_t size);
int cx_platform_unix_time_100ns(uint64_t *time);
void cx_platform_uuid_lock(void);
void cx_platform_uuid_unlock(void);
int cx_platform_parse_float(const char *text, int single, double *result);
int cx_platform_format_float(char *buffer, size_t capacity, double value, int single);

#endif // __CX_STDLIB_PLATFORM_H__

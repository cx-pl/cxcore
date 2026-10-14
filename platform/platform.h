#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>

#include "cxdefs.h"

#if defined(_WIN32) || defined(WIN32)
#define CX_PLATFORM_WIN
#define CX_THREAD_LOCAL __declspec(thread)
typedef uint32_t cx_platform_thread_id;
typedef volatile long cx_platform_atomic_thread_id;
#elif defined(__linux__)
#define CX_PLATFORM_LINUX
#define CX_THREAD_LOCAL _Thread_local
#include <stdatomic.h>
typedef uint32_t cx_platform_thread_id;
typedef _Atomic cx_platform_thread_id cx_platform_atomic_thread_id;
#elif defined(__APPLE__) || defined(MACOSX)
#define CX_PLATFORM_MACOSX
#error "MacOS platform not implemented yet"
#else
#error "Unsupported platform"
#endif

cx_platform_thread_id cx_platform_current_thread_id(void);
cx_platform_thread_id cx_platform_owner_thread_load(
    const cx_platform_atomic_thread_id *owner);
cx_platform_thread_id cx_platform_owner_thread_claim(
    cx_platform_atomic_thread_id *owner,
    cx_platform_thread_id current_thread);
int cx_platform_current_stack_bounds(uintptr_t *low, uintptr_t *high);
CX_NORETURN void cx_platform_exception_jump(jmp_buf environment);

uint32_t cx_platform_last_error(void);
void cx_platform_set_last_error(uint32_t error);
int cx_platform_native_library_uses_utf16(void);
void *cx_platform_native_library_open(const void *path);
int cx_platform_native_library_resolve(void *handle, const char *name,
    void *result, size_t result_size);

#endif // _PLATFORM_H_

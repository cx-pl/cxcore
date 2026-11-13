#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include "../../../../src/cxcore.h"
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

CX_STRING_DEF(cx_clock_unavailable, "The monotonic clock is unavailable.");
static CX_NORETURN void cx_clock_fail(void) {
    struct cxcore_System_InvalidOperationException *error =
        cx_exception_allocate_durable(
            cxcore_System_InvalidOperationException___typeinfo.Size);
    cxcore_System_InvalidOperationException___constructor__2(
        error, &cx_clock_unavailable);
    cx_exception_throw((struct cxcore_System_Exception *)error, __FILE__, __LINE__);
}

cx_ulong cxcore_System_Diagnostics_MonotonicClock_GetMicroseconds(void) {
#if defined(_WIN32)
    LARGE_INTEGER counter, frequency;
    cx_ulong whole, remainder;
    if (!QueryPerformanceCounter(&counter) || !QueryPerformanceFrequency(&frequency) ||
        counter.QuadPart < 0 || frequency.QuadPart <= 0) {
        cx_clock_fail();
    }
    whole = (cx_ulong)counter.QuadPart / (cx_ulong)frequency.QuadPart;
    remainder = (cx_ulong)counter.QuadPart % (cx_ulong)frequency.QuadPart;
    /* Split before scaling: long uptimes cannot overflow an intermediate product. */
    return whole * UINT64_C(1000000) +
        (cx_ulong)((long double)remainder * 1000000.0L / (long double)frequency.QuadPart);
#else
    struct timespec value;
    if (clock_gettime(CLOCK_MONOTONIC, &value) != 0 || value.tv_sec < 0) cx_clock_fail();
    return (cx_ulong)value.tv_sec * UINT64_C(1000000) + (cx_ulong)value.tv_nsec / 1000u;
#endif
}

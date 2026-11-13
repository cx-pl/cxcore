#include "../platform.h"
#ifdef CX_PLATFORM_WIN
#include "../stdlib.h"
#include <windows.h>
#include <bcrypt.h>
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static SRWLOCK uuid_lock = SRWLOCK_INIT;
void cx_platform_uuid_lock(void) { AcquireSRWLockExclusive(&uuid_lock); }
void cx_platform_uuid_unlock(void) { ReleaseSRWLockExclusive(&uuid_lock); }
int cx_platform_random_bytes(void *buffer, size_t size) {
    while (size) {
        ULONG count = size > ULONG_MAX ? ULONG_MAX : (ULONG)size;
        if (BCryptGenRandom(NULL, buffer, count, BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) return 0;
        buffer = (unsigned char *)buffer + count;
        size -= count;
    }
    return 1;
}
int cx_platform_unix_time_100ns(uint64_t *time) {
    FILETIME value;
    GetSystemTimePreciseAsFileTime(&value);
    uint64_t ticks = ((uint64_t)value.dwHighDateTime << 32) | value.dwLowDateTime;
    if (ticks < 116444736000000000ULL) return 0;
    *time = ticks - 116444736000000000ULL;
    return 1;
}
int cx_platform_parse_float(const char *text, int single, double *result) {
    int saved_errno = errno;
    _locale_t locale = _create_locale(LC_ALL, "C");
    if (!locale) return 0;
    char *end;
    double value = single ? (double)_strtof_l(text, &end, locale) : _strtod_l(text, &end, locale);
    _free_locale(locale);
    errno = saved_errno;
    if (*end != 0 || !isfinite(value)) return 0;
    *result = value;
    return 1;
}
int cx_platform_format_float(char *buffer, size_t capacity, double value, int single) {
    _locale_t locale = _create_locale(LC_ALL, "C");
    if (!locale) return -1;
    int length = _snprintf_l(buffer, capacity, single ? "%.9g" : "%.17g", locale, value);
    _free_locale(locale);
    return length;
}

#endif // CX_PLATFORM_WIN

#define _GNU_SOURCE

#include "../platform.h"
#ifdef CX_PLATFORM_LINUX
#include "../stdlib.h"
#include <errno.h>
#include <locale.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/random.h>
#include <time.h>

static pthread_mutex_t uuid_lock = PTHREAD_MUTEX_INITIALIZER;
void cx_platform_uuid_lock(void) { if (pthread_mutex_lock(&uuid_lock) != 0) abort(); }
void cx_platform_uuid_unlock(void) { if (pthread_mutex_unlock(&uuid_lock) != 0) abort(); }
int cx_platform_random_bytes(void *buffer, size_t size) {
    while (size) {
        ssize_t count = getrandom(buffer, size, 0);
        if (count < 0) { if (errno == EINTR) continue; return 0; }
        if (count == 0) return 0;
        buffer = (unsigned char *)buffer + count;
        size -= (size_t)count;
    }
    return 1;
}
int cx_platform_unix_time_100ns(uint64_t *time) {
    struct timespec value;
    if (clock_gettime(CLOCK_REALTIME, &value) != 0 || value.tv_sec < 0) return 0;
    *time = (uint64_t)value.tv_sec * 10000000ULL + (uint64_t)value.tv_nsec / 100;
    return 1;
}
int cx_platform_parse_float(const char *text, int single, double *result) {
    int saved_errno = errno;
    locale_t locale = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
    if (!locale) return 0;
    char *end;
    double value = single ? (double)strtof_l(text, &end, locale) : strtod_l(text, &end, locale);
    freelocale(locale);
    errno = saved_errno;
    if (*end != 0 || !isfinite(value)) return 0;
    *result = value;
    return 1;
}
int cx_platform_format_float(char *buffer, size_t capacity, double value, int single) {
    locale_t locale = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
    if (!locale) return -1;
    locale_t previous = uselocale(locale);
    if (previous == (locale_t)0) { freelocale(locale); return -1; }
    int length = snprintf(buffer, capacity, single ? "%.9g" : "%.17g", value);
    uselocale(previous);
    freelocale(locale);
    return length;
}

#endif // CX_PLATFORM_LINUX

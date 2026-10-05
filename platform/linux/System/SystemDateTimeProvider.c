#define _GNU_SOURCE

#include "../../platform.h"

#ifdef CX_PLATFORM_LINUX

#include <time.h>

#include "../../../src/cxcore.h"

#define CX_UNIX_EPOCH_MILLISECONDS INT64_C(62135596800000)

static int64_t cx_unix_time_milliseconds(const struct timespec *time) {
    return (int64_t)time->tv_sec * INT64_C(1000) +
        (int64_t)(time->tv_nsec / 1000000);
}

struct CX_ID_3(cxcore, System, DateTime)
    CX_ID_4(cxcore, System, SystemDateTimeProvider,
            GetUtcNow)(const struct CX_ID_3(cxcore, System, SystemDateTimeProvider) * __this) {
    struct timespec now;
    int64_t milliseconds = 0;
    if (clock_gettime(CLOCK_REALTIME, &now) == 0) {
        milliseconds = CX_UNIX_EPOCH_MILLISECONDS + cx_unix_time_milliseconds(&now);
    }
    struct CX_ID_3(cxcore, System, DateTime) result = {
        ._offset = 0,
        ._milliseconds = milliseconds,
    };
    return result;
}

struct CX_ID_3(cxcore, System, DateTime)
    CX_ID_4(cxcore, System, SystemDateTimeProvider,
            GetLocalNow)(const struct CX_ID_3(cxcore, System, SystemDateTimeProvider) * __this) {
    struct timespec now;
    struct tm local_time;
    int64_t milliseconds = 0;
    cx_short offset_hours = 0;

    if (clock_gettime(CLOCK_REALTIME, &now) == 0 &&
        localtime_r(&now.tv_sec, &local_time) != NULL) {
        milliseconds = CX_UNIX_EPOCH_MILLISECONDS +
            cx_unix_time_milliseconds(&now) +
            (int64_t)local_time.tm_gmtoff * INT64_C(1000);
        offset_hours = (cx_short)(local_time.tm_gmtoff / 3600);
    }
    struct CX_ID_3(cxcore, System, DateTime) result = {
        ._offset = offset_hours,
        ._milliseconds = milliseconds,
    };
    return result;
}

#endif // CX_PLATFORM_LINUX

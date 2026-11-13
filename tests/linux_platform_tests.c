#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../src/cxcore.h"

#define CX_UNIX_EPOCH_MILLISECONDS INT64_C(62135596800000)
#define CX_MILLISECONDS_PER_HOUR INT64_C(3600000)

typedef struct cxcore_System_DateTime datetime;
typedef struct cxcore_System_String cx_string;

static int64_t milliseconds_from_timespec(const struct timespec *time) {
    return (int64_t)time->tv_sec * INT64_C(1000) +
        (int64_t)(time->tv_nsec / 1000000);
}

static void assert_local_offset(cx_short expected_offset) {
    datetime utc;
    datetime local;
    int64_t difference;
    setenv("TZ", expected_offset > 0 ? "UTC-5" : "UTC+3", 1);
    tzset();

    utc = cxcore_System_SystemDateTimeProvider_GetUtcNow(NULL);
    local = cxcore_System_SystemDateTimeProvider_GetLocalNow(NULL);
    assert(local._offset == expected_offset);
    difference = local._milliseconds - utc._milliseconds;
    assert(llabs(difference - (int64_t)expected_offset * CX_MILLISECONDS_PER_HOUR) < 2000);
}

int main(void) {
    const cx_string *system_name =
        cxcore_System_Environment_SystemName___const_get();
    const cx_string *newline = cxcore_System_Environment_NewLine___const_get();
    datetime utc;
    struct timespec before;
    struct timespec after;
    int64_t earliest;
    int64_t latest;

    assert(system_name->_length == 5);
    assert(memcmp(system_name->_data, "Linux", 5) == 0);
    assert(newline->_length == 1);
    assert(memcmp(newline->_data, "\n", 1) == 0);

    assert(clock_gettime(CLOCK_REALTIME, &before) == 0);
    utc = cxcore_System_SystemDateTimeProvider_GetUtcNow(NULL);
    assert(clock_gettime(CLOCK_REALTIME, &after) == 0);
    earliest = CX_UNIX_EPOCH_MILLISECONDS + milliseconds_from_timespec(&before);
    latest = CX_UNIX_EPOCH_MILLISECONDS + milliseconds_from_timespec(&after);
    assert(utc._offset == 0);
    assert(utc._milliseconds >= earliest - 1);
    assert(utc._milliseconds <= latest + 1);

    assert_local_offset(5);
    assert_local_offset(-3);
    return 0;
}

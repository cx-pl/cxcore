#include "../../platform.h"
#ifdef CX_PLATFORM_WIN32

#include <windows.h>
#include "../../../src/cxcore.h"


// Millisecond offset from Year 1 to Year 1601
// There are exactly 504,911,232,000,000 milliseconds in those 1600 years
// (This accounts for all leap years using the Gregorian calendar rules)
const ULONGLONG _MILISECONDS_FROM_YEAR_1_TO_1601 = 504911232000000ULL;

struct CX_ID_3(cxcore, System, DateTime) CX_ID_4(cxcore, System, SystemDateTimeProvider, GetUtcNow)(
    const struct CX_ID_3(cxcore, System, SystemDateTimeProvider)* __this
    ) {
    // Get the highest precision UTC time from Windows
    FILETIME utcFileTime;
    GetSystemTimePreciseAsFileTime(&utcFileTime);

    // Combine low and high bits into 64-bit integer
    ULARGE_INTEGER utcTicks;
    utcTicks.LowPart = utcFileTime.dwLowDateTime;
    utcTicks.HighPart = utcFileTime.dwHighDateTime;

    // Convert Windows 100-nanosecond ticks to Milliseconds
    ULONGLONG utcMsFrom1601 = utcTicks.QuadPart / 10000;

    // Add the millisecond offset from Year 1 to Year 1601
    ULONGLONG utcMsYear1 = utcMsFrom1601 + _MILISECONDS_FROM_YEAR_1_TO_1601;

    // Build result and return
    struct CX_ID_3(cxcore, System, DateTime) result = { ._offset = 0, ._milliseconds = utcMsYear1 };
    return result;
}

struct CX_ID_3(cxcore, System, DateTime) CX_ID_4(cxcore, System, SystemDateTimeProvider, GetLocalNow)(
    const struct CX_ID_3(cxcore, System, SystemDateTimeProvider)* __this
    ) {
    // Get the highest precision UTC time from Windows
    FILETIME utcFileTime;
    GetSystemTimePreciseAsFileTime(&utcFileTime);

    // Convert to Local time
    SYSTEMTIME utcSystemTime;
    FileTimeToSystemTime(&utcFileTime, &utcSystemTime);
    SYSTEMTIME localSystemTime;
    SystemTimeToTzSpecificLocalTime(NULL, &utcSystemTime, &localSystemTime);
    FILETIME localFileTime;
    SystemTimeToFileTime(&localSystemTime, &localFileTime);

    // Combine low and high bits into 64-bit integer
    ULARGE_INTEGER localTicks;
    localTicks.LowPart = localFileTime.dwLowDateTime;
    localTicks.HighPart = localFileTime.dwHighDateTime;

    // Convert Windows 100-nanosecond ticks to Milliseconds
    ULONGLONG localMsFrom1601 = localTicks.QuadPart / 10000;

    // Add the millisecond offset from Year 1 to Year 1601
    ULONGLONG localMsYear1 = localMsFrom1601 + _MILISECONDS_FROM_YEAR_1_TO_1601;

    // Get time zone information
    TIME_ZONE_INFORMATION tzInfo;
    DWORD tzResult = GetTimeZoneInformation(&tzInfo);

    // Calculate bias and offset
    LONG totalBiasInMinutes = tzInfo.Bias;
    if (tzResult == TIME_ZONE_ID_STANDARD) {
        totalBiasInMinutes += tzInfo.StandardBias;
    }
    else if (tzResult == TIME_ZONE_ID_DAYLIGHT) {
        totalBiasInMinutes += tzInfo.DaylightBias;
    }
    LONG offset = (totalBiasInMinutes * -1) / 60;

    // Build result and return
    struct CX_ID_3(cxcore, System, DateTime) result = { ._offset = offset, ._milliseconds = localMsYear1 };
    return result;
}


#endif // CX_PLATFORM_WIN32

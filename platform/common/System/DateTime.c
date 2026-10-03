#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../../src/cxcore.h"

#define CX_MILLISECONDS_PER_DAY ((cx_long)86400000)
#define CX_DAYS_PER_GREGORIAN_CYCLE ((cx_long)146097)

static void cx_datetime_fail(const char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(EXIT_FAILURE);
}

static cx_bool cx_datetime_is_leap_year(cx_long year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static cx_long cx_datetime_days_in_month(cx_long month, cx_long year) {
    switch (month) {
    case 2:
        return cx_datetime_is_leap_year(year) ? 29 : 28;
    case 4:
    case 6:
    case 9:
    case 11:
        return 30;
    default:
        return 31;
    }
}

static cx_long cx_datetime_days_before_year(cx_long year) {
    cx_long completedYears = year - 1;
    return completedYears * 365 + completedYears / 4 - completedYears / 100 + completedYears / 400;
}

static cx_long cx_datetime_checked_milliseconds(cx_long days, cx_long timeOfDay) {
    const cx_long maxMilliseconds = (cx_long)INT64_MAX;
    cx_long maxDays = maxMilliseconds / CX_MILLISECONDS_PER_DAY;
    cx_long maxTimeOfDay = maxMilliseconds % CX_MILLISECONDS_PER_DAY;

    if (days < 0 || days > maxDays || (days == maxDays && timeOfDay > maxTimeOfDay)) {
        cx_datetime_fail("DateTime value is out of range");
    }
    return days * CX_MILLISECONDS_PER_DAY + timeOfDay;
}

void CX_ID_5(cxcore, System, DateTime, __constructor,
             _2)(struct CX_ID_3(cxcore, System, DateTime) * __this, cx_int year, cx_int month,
                 cx_int day, cx_int hour, cx_int minute, cx_int second, cx_int millisecond,
                 cx_short offset) {
    cx_long dayNumber;
    cx_long timeOfDay;

    if (year < 1 || month < 1 || month > 12 || day < 1 ||
        day > cx_datetime_days_in_month(month, year) || hour < 0 || hour > 23 || minute < 0 ||
        minute > 59 || second < 0 || second > 59 || millisecond < 0 || millisecond > 999) {
        cx_datetime_fail("Invalid DateTime components");
    }

    dayNumber = cx_datetime_days_before_year(year);
    for (cx_int currentMonth = 1; currentMonth < month; currentMonth++) {
        dayNumber += cx_datetime_days_in_month(currentMonth, year);
    }
    dayNumber += day - 1;
    timeOfDay = ((cx_long)hour * 60 + minute) * 60000 + (cx_long)second * 1000 + millisecond;
    __this->_milliseconds = cx_datetime_checked_milliseconds(dayNumber, timeOfDay);
    __this->_offset = offset;
}

struct CX_ID_3(cxcore, System, DateTime)
    CX_ID_4(cxcore, System, DateTime,
            AddMonths)(const struct CX_ID_3(cxcore, System, DateTime) * __this, cx_long months) {
    struct CX_ID_3(cxcore, System, DateTime) result;
    cx_long days = __this->_milliseconds / CX_MILLISECONDS_PER_DAY;
    cx_long timeOfDay = __this->_milliseconds % CX_MILLISECONDS_PER_DAY;
    cx_long cycle = days / CX_DAYS_PER_GREGORIAN_CYCLE;
    cx_long remainingDays = days % CX_DAYS_PER_GREGORIAN_CYCLE;
    cx_long year = cycle * 400 + 1;
    cx_long month = 1;
    cx_long day;
    cx_long monthIndex;
    cx_long targetMonthIndex;
    cx_long targetYear;
    cx_long targetMonth;
    cx_long targetDay;
    cx_long targetDays;

    if (__this->_milliseconds < 0) {
        cx_datetime_fail("DateTime value is out of range");
    }

    while (remainingDays >= (cx_datetime_is_leap_year(year) ? 366 : 365)) {
        remainingDays -= cx_datetime_is_leap_year(year) ? 366 : 365;
        year++;
    }
    while (remainingDays >= cx_datetime_days_in_month(month, year)) {
        remainingDays -= cx_datetime_days_in_month(month, year);
        month++;
    }
    day = remainingDays + 1;

    monthIndex = (year - 1) * 12 + month - 1;
    if (months > (cx_long)INT64_MAX - monthIndex || months < -monthIndex) {
        cx_datetime_fail("DateTime value is out of range");
    }
    targetMonthIndex = monthIndex + months;
    targetYear = targetMonthIndex / 12 + 1;
    targetMonth = targetMonthIndex % 12 + 1;
    targetDay = day;
    if (targetDay > cx_datetime_days_in_month(targetMonth, targetYear)) {
        targetDay = cx_datetime_days_in_month(targetMonth, targetYear);
    }

    if (targetYear - 1 > (cx_long)INT64_MAX / 365) {
        cx_datetime_fail("DateTime value is out of range");
    }
    targetDays = cx_datetime_days_before_year(targetYear);
    for (cx_long currentMonth = 1; currentMonth < targetMonth; currentMonth++) {
        targetDays += cx_datetime_days_in_month(currentMonth, targetYear);
    }
    targetDays += targetDay - 1;

    result._milliseconds = cx_datetime_checked_milliseconds(targetDays, timeOfDay);
    result._offset = __this->_offset;
    return result;
}

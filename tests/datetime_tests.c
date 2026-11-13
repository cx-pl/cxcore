#include <assert.h>
#include "../src/cxcore.h"

typedef struct cxcore_System_DateTime datetime;

static datetime make_datetime(cx_int year, cx_int month, cx_int day) {
    datetime value;
    cxcore_System_DateTime___constructor__2(&value, year, month, day, 14, 5, 6, 789,
                                                         3);
    return value;
}

static void assert_date(const datetime *value, cx_long year, cx_long month, cx_long day) {
    assert(cxcore_System_DateTime_Year___const_get(value) == year);
    assert(cxcore_System_DateTime_Month___const_get(value) == month);
    assert(cxcore_System_DateTime_Day___const_get(value) == day);
    assert(cxcore_System_DateTime_Hour___const_get(value) == 14);
    assert(cxcore_System_DateTime_Minute___const_get(value) == 5);
    assert(cxcore_System_DateTime_Second___const_get(value) == 6);
    assert(cxcore_System_DateTime_Millisecond___const_get(value) == 789);
    assert(value->_offset == 3);
}

int main(void) {
    datetime january31 = make_datetime(2024, 1, 31);
    datetime leapFebruary = cxcore_System_DateTime_AddMonths(&january31, 1);
    datetime march31 = make_datetime(2024, 3, 31);
    datetime february = cxcore_System_DateTime_AddMonths(&march31, -1);
    datetime yearOne = make_datetime(1, 1, 1);
    datetime yearTwo = cxcore_System_DateTime_AddMonths(&yearOne, 12);
    datetime nonLeapJanuary = make_datetime(2023, 1, 31);
    datetime nonLeapFebruary = cxcore_System_DateTime_AddMonths(&nonLeapJanuary, 1);

    assert_date(&leapFebruary, 2024, 2, 29);
    assert_date(&february, 2024, 2, 29);
    assert_date(&yearTwo, 2, 1, 1);
    assert_date(&nonLeapFebruary, 2023, 2, 28);
    assert(cxcore_System_DateTime_DaysInMonth(2, 2024) == 29);
    assert(cxcore_System_DateTime_DaysInMonth(2, 2023) == 28);
    return 0;
}

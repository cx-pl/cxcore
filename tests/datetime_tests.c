#include <assert.h>
#include "../src/cxcore.h"

typedef struct CX_ID_3(cxcore, System, DateTime) datetime;

static datetime make_datetime(cx_int year, cx_int month, cx_int day)
{
    datetime value;
    CX_ID_5(cxcore, System, DateTime, __constructor, _2)(
        &value, year, month, day, 14, 5, 6, 789, 3);
    return value;
}

static void assert_date(const datetime* value, cx_long year, cx_long month, cx_long day)
{
    assert(CX_ID_5(cxcore, System, DateTime, Year, __const_get)(value) == year);
    assert(CX_ID_5(cxcore, System, DateTime, Month, __const_get)(value) == month);
    assert(CX_ID_5(cxcore, System, DateTime, Day, __const_get)(value) == day);
    assert(CX_ID_5(cxcore, System, DateTime, Hour, __const_get)(value) == 14);
    assert(CX_ID_5(cxcore, System, DateTime, Minute, __const_get)(value) == 5);
    assert(CX_ID_5(cxcore, System, DateTime, Second, __const_get)(value) == 6);
    assert(CX_ID_5(cxcore, System, DateTime, Millisecond, __const_get)(value) == 789);
    assert(value->_offset == 3);
}

int main(void)
{
    datetime january31 = make_datetime(2024, 1, 31);
    datetime leapFebruary = CX_ID_4(cxcore, System, DateTime, AddMonths)(&january31, 1);
    datetime march31 = make_datetime(2024, 3, 31);
    datetime february = CX_ID_4(cxcore, System, DateTime, AddMonths)(&march31, -1);
    datetime yearOne = make_datetime(1, 1, 1);
    datetime yearTwo = CX_ID_4(cxcore, System, DateTime, AddMonths)(&yearOne, 12);
    datetime nonLeapJanuary = make_datetime(2023, 1, 31);
    datetime nonLeapFebruary = CX_ID_4(cxcore, System, DateTime, AddMonths)(&nonLeapJanuary, 1);

    assert_date(&leapFebruary, 2024, 2, 29);
    assert_date(&february, 2024, 2, 29);
    assert_date(&yearTwo, 2, 1, 1);
    assert_date(&nonLeapFebruary, 2023, 2, 28);
    assert(CX_ID_4(cxcore, System, DateTime, DaysInMonth)(2, 2024) == 29);
    assert(CX_ID_4(cxcore, System, DateTime, DaysInMonth)(2, 2023) == 28);
    return 0;
}

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "cxcore.h"
#include "decimal_vectors.h"

static void equal(cx_decimal value, const char *expected) {
    assert(cx_decimal_compare(value, cx_decimal_parse(expected)) == 0);
}
static void check_vectors(void) {
    for (size_t i = 0; i < sizeof(decimal_vectors) / sizeof(decimal_vectors[0]); ++i) {
        const struct decimal_vector *v = &decimal_vectors[i];
        cx_decimal a = cx_decimal_parse(v->left), b = cx_decimal_parse(v->right);
        struct cx_exception_frame frame;
        cx_exception_push(&frame);
        if (setjmp(frame.environment) == 0) {
            cx_decimal result;
            switch (v->operation) {
            case 0: result = cx_decimal_add(a, b); break;
            case 1: result = cx_decimal_subtract(a, b); break;
            case 2: result = cx_decimal_multiply(a, b); break;
            case 3: result = cx_decimal_divide(a, b); break;
            default: result = cx_decimal_remainder(a, b); break;
            }
            cx_exception_pop(&frame);
            if (v->error || cx_decimal_compare(result, cx_decimal_parse(v->result)) != 0) {
                char text[32];
                cx_decimal_format(result, text, sizeof(text));
                fprintf(stderr, "Decimal vector %zu: %s op%d %s: got %s, expected %s (error=%d)\n",
                    i, v->left, v->operation, v->right, text, v->result, v->error);
                assert(0);
            }
        } else {
            cx_exception_pop(&frame);
            cx_exception_clear();
            if (!v->error) { fprintf(stderr, "Unexpected decimal exception at vector %zu\n", i); assert(0); }
        }
    }
}
static void check_failure(int mode) {
    struct cx_exception_frame frame;
    cx_exception_push(&frame);
    if (setjmp(frame.environment) == 0) {
        switch (mode) {
        case 0: (void)cx_decimal_divide(cx_decimal_from_i64(1), cx_decimal_from_i64(0)); break;
        case 1: (void)cx_decimal_add(cx_decimal_parse("79228162514264337593543950335"), cx_decimal_from_i64(1)); break;
        case 2: (void)cx_decimal_to_i64(cx_decimal_parse("2147483648"), 2147483647); break;
        default: (void)cx_decimal_to_u64(cx_decimal_from_i64(-1), ULLONG_MAX); break;
        }
        assert(0);
    } else {
        cx_exception_pop(&frame);
        assert(cx_exception_current() != CX_NULL);
        cx_exception_clear();
    }
}
int main(void) {
    equal(cx_decimal_add(cx_decimal_parse("0.1"), cx_decimal_parse("0.2")), "0.3");
    equal(cx_decimal_divide(cx_decimal_from_i64(1), cx_decimal_from_i64(3)), "0.3333333333333333333333333333");
    equal(cx_decimal_divide(cx_decimal_from_i64(2), cx_decimal_from_i64(3)), "0.6666666666666666666666666667");
    equal(cx_decimal_multiply(cx_decimal_parse("0.0000000000000000000000000001"), cx_decimal_parse("0.5")), "0");
    equal(cx_decimal_multiply(cx_decimal_parse("0.0000000000000000000000000003"), cx_decimal_parse("0.5")), "0.0000000000000000000000000002");
    equal(cx_decimal_remainder(cx_decimal_parse("-7.5"), cx_decimal_from_i64(2)), "-1.5");
    equal(cx_decimal_from_i64(LLONG_MIN), "-9223372036854775808");
    equal(cx_decimal_from_u64(ULLONG_MAX), "18446744073709551615");
    assert(cx_decimal_to_i64(cx_decimal_parse("-12.99"), LLONG_MAX) == -12);
    assert(cx_decimal_to_i64(cx_decimal_from_i64(LLONG_MIN), LLONG_MAX) == LLONG_MIN);
    assert(cx_decimal_to_u64(cx_decimal_from_u64(ULLONG_MAX), ULLONG_MAX) == ULLONG_MAX);
    equal(cx_decimal_from_double(0.1, 15), "0.1");
    char buffer[32];
    cx_decimal_format(cx_decimal_parse("-0.0000000000000000000000000001"), buffer, sizeof(buffer));
    assert(strcmp(buffer, "-0.0000000000000000000000000001") == 0);
    cx_decimal_format(cx_decimal_parse("123.4500"), buffer, sizeof(buffer));
    assert(strcmp(buffer, "123.4500") == 0);
    cx_decimal value = cx_decimal_from_i64(5);
    equal(cx_decimal_increment(&value, 1, CX_TRUE), "5"); equal(value, "6");
    equal(cx_decimal_add_assign(&value, cx_decimal_parse("0.2")), "6.2");
    assert(cxcore_System_Decimal___typeinfo.Size == 16);
    for (int mode = 0; mode < 4; ++mode) check_failure(mode);
    check_vectors();
    return 0;
}

#include "cxcore.h"
#include <assert.h>
#include <fenv.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

#define M(name) cxcore_System_Math_ ## name
#define L(name) cxcore_System_Math_ ## name ## __2
#define F(name) cxcore_System_Math_ ## name ## __3
#define D(name) cxcore_System_Math_ ## name ## __4

static void constants(void) {
    assert(M(E) == 2.7182818284590452354);
    assert(M(Pi) == 3.14159265358979323846);
    assert(M(Tau) == 2.0 * M(Pi));
    assert(fabs(M(Sqrt2) * M(Sqrt2) - 2.0) < 1e-15);
    assert(fabs(M(Sqrt3) * M(Sqrt3) - 3.0) < 1e-15);
}

static void integers(void) {
    assert(M(Abs)(-123) == 123 && M(Abs)(0) == 0 && M(Abs)(INT_MAX) == INT_MAX);
    assert(L(Abs)(-9223372036854775807LL) == LLONG_MAX && L(Abs)(0) == 0);
    assert(M(Clamp)(INT_MIN, -5, 10) == -5);
    assert(M(Clamp)(INT_MAX, -5, 10) == 10);
    assert(M(Clamp)(3, -5, 10) == 3 && M(Clamp)(3, 7, 7) == 7);
    assert(L(Clamp)(LLONG_MIN, -5000000000LL, 5000000000LL) == -5000000000LL);
    assert(L(Clamp)(LLONG_MAX, -5000000000LL, 5000000000LL) == 5000000000LL);
    assert(L(Clamp)(123, LLONG_MIN, LLONG_MAX) == 123);
    assert(M(Floor)(INT_MIN) == INT_MIN && M(Ceiling)(INT_MAX) == INT_MAX);
    assert(L(Floor)(LLONG_MIN) == LLONG_MIN && L(Ceiling)(LLONG_MAX) == LLONG_MAX);
    assert(M(Min)(INT_MAX, INT_MIN) == INT_MIN && M(Min)(4, 4) == 4);
    assert(M(Max)(INT_MIN, INT_MAX) == INT_MAX && M(Max)(4, 4) == 4);
    assert(L(Min)(LLONG_MAX, LLONG_MIN) == LLONG_MIN);
    assert(L(Max)(LLONG_MIN, LLONG_MAX) == LLONG_MAX);
}

static void floating_point(void) {
    assert(F(Abs)(-1.5f) == 1.5f && D(Abs)(-1.5) == 1.5);
    assert(!signbit(F(Abs)(-0.0f)) && !signbit(D(Abs)(-0.0)));
    assert(F(Abs)(-INFINITY) == INFINITY && D(Abs)(-INFINITY) == INFINITY);
    assert(isnan(F(Abs)(NAN)) && isnan(D(Abs)(NAN)));
    assert(F(Clamp)(-INFINITY, -2.0f, 3.0f) == -2.0f);
    assert(D(Clamp)(INFINITY, -2.0, 3.0) == 3.0);
    assert(F(Clamp)(1.5f, -2.0f, 3.0f) == 1.5f && D(Clamp)(1.5, -2.0, 3.0) == 1.5);
    assert(F(Clamp)(5.0f, 3.0f, 3.0f) == 3.0f && D(Clamp)(-5.0, 3.0, 3.0) == 3.0);
    assert(isnan(F(Clamp)(NAN, 0.0f, 1.0f)) && isnan(D(Clamp)(NAN, 0.0, 1.0)));
    assert(F(Clamp)(5.0f, NAN, 10.0f) == 5.0f && D(Clamp)(20.0, NAN, 10.0) == 10.0);
    assert(signbit(F(Clamp)(-0.0f, 0.0f, 1.0f)) && signbit(D(Clamp)(-0.0, 0.0, 1.0)));
    assert(F(Floor)(-1.25f) == -2.0f && D(Floor)(1.75) == 1.0);
    assert(F(Ceiling)(1.25f) == 2.0f && D(Ceiling)(-1.75) == -1.0);
    assert(signbit(F(Floor)(-0.0f)) && signbit(D(Floor)(-0.0)));
    assert(signbit(F(Ceiling)(-0.0f)) && signbit(D(Ceiling)(-0.0)));
    assert(isnan(F(Floor)(NAN)) && isnan(D(Ceiling)(NAN)));
    assert(F(Ceiling)(INFINITY) == INFINITY && D(Floor)(-INFINITY) == -INFINITY);
    assert(F(Min)(-3.0f, 2.0f) == -3.0f && D(Min)(2.0, -3.0) == -3.0);
    assert(F(Max)(-3.0f, 2.0f) == 2.0f && D(Max)(2.0, -3.0) == 2.0);
    assert(isnan(F(Min)(NAN, 1.0f)) && isnan(F(Min)(1.0f, NAN)));
    assert(isnan(D(Min)(NAN, 1.0)) && isnan(D(Min)(1.0, NAN)));
    assert(isnan(F(Max)(NAN, 1.0f)) && isnan(F(Max)(1.0f, NAN)));
    assert(isnan(D(Max)(NAN, 1.0)) && isnan(D(Max)(1.0, NAN)));
    assert(signbit(F(Min)(-0.0f, 0.0f)) && signbit(F(Min)(0.0f, -0.0f)));
    assert(signbit(D(Min)(-0.0, 0.0)) && signbit(D(Min)(0.0, -0.0)));
    assert(!signbit(F(Max)(-0.0f, 0.0f)) && !signbit(F(Max)(0.0f, -0.0f)));
    assert(!signbit(D(Max)(-0.0, 0.0)) && !signbit(D(Max)(0.0, -0.0)));
}

static void rounding(void) {
    const double values[] = {0.0, -0.0, 0.5, -0.5, 1.5, -1.5, 2.5, -2.5, 3.5, -3.5, 0.499, -0.501};
    const double expected[] = {0.0, -0.0, 0.0, -0.0, 2.0, -2.0, 2.0, -2.0, 4.0, -4.0, 0.0, -1.0};
    const int modes[] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO};
    int original = fegetround();
    for (unsigned mode = 0; mode < sizeof(modes) / sizeof(modes[0]); ++mode) {
        assert(fesetround(modes[mode]) == 0);
        for (unsigned i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
            double actual = F(Round)(values[i]);
            float single = M(Round)((float)values[i]);
            assert(actual == expected[i] && single == (float)expected[i]);
            assert(!!signbit(actual) == !!signbit(expected[i]));
            assert(!!signbit(single) == !!signbit(expected[i]));
        }
        assert(fegetround() == modes[mode]);
    }
    assert(fesetround(original) == 0);
    assert(F(Round)(nextafter(0.5, 0.0)) == 0.0 && F(Round)(nextafter(0.5, 1.0)) == 1.0);
    assert(M(Round)(nextafterf(0.5f, 0.0f)) == 0.0f && M(Round)(nextafterf(0.5f, 1.0f)) == 1.0f);
    assert(M(Round)(8388609.0f) == 8388609.0f && F(Round)(4503599627370497.0) == 4503599627370497.0);
    assert(M(Round)(FLT_MAX) == FLT_MAX && F(Round)(DBL_MAX) == DBL_MAX);
    assert(L(Round)(1.125f, 2) == 1.12f && L(Round)(1.375f, 2) == 1.38f);
    assert(D(Round)(-1.125, 2) == -1.12 && D(Round)(-1.375, 2) == -1.38);
    assert(L(Round)(1.234567f, 6) == 1.234567f && D(Round)(1.234567890123456, 15) == 1.234567890123456);
    assert(L(Round)(-0.5f, 0) == 0.0f && signbit(L(Round)(-0.5f, 0)));
    assert(D(Round)(-0.0001, 2) == 0.0 && signbit(D(Round)(-0.0001, 2)));
    assert(L(Round)(FLT_MAX, 6) == FLT_MAX && D(Round)(DBL_MAX, 15) == DBL_MAX);
    assert(isnan(M(Round)(NAN)) && isnan(F(Round)(NAN)));
    assert(isnan(L(Round)(NAN, 6)) && isnan(D(Round)(NAN, 15)));
    assert(M(Round)(INFINITY) == INFINITY && F(Round)(-INFINITY) == -INFINITY);
    assert(L(Round)(-INFINITY, 0) == -INFINITY && D(Round)(INFINITY, 0) == INFINITY);
}

static void logarithms_and_reciprocals(void) {
    assert(fabsf(M(Log)((float)M(E)) - 1.0f) < 1e-6f && fabs(L(Log)(M(E)) - 1.0) < 1e-15);
    assert(M(Log)(1.0f) == 0.0f && L(Log)(1.0) == 0.0);
    assert(M(Log)(0.0f) == -INFINITY && L(Log)(0.0) == -INFINITY);
    assert(isnan(M(Log)(-1.0f)) && isnan(L(Log)(-1.0)));
    assert(M(Log)(INFINITY) == INFINITY && L(Log)(INFINITY) == INFINITY);
    assert(isnan(M(Log)(NAN)) && isnan(L(Log)(NAN)));
    assert(fabsf(F(Log)(8.0f, 2.0f) - 3.0f) < 1e-6f && fabs(D(Log)(8.0, 2.0) - 3.0) < 1e-15);
    assert(fabsf(F(Log)(4.0f, 0.5f) + 2.0f) < 1e-6f && fabs(D(Log)(4.0, 0.5) + 2.0) < 1e-15);
    const double invalid_bases[] = {-2.0, -INFINITY, 0.0, 1.0, INFINITY, NAN};
    for (unsigned i = 0; i < sizeof(invalid_bases) / sizeof(invalid_bases[0]); ++i) {
        assert(isnan(F(Log)(2.0f, (float)invalid_bases[i])));
        assert(isnan(D(Log)(2.0, invalid_bases[i])));
    }
    assert(F(Log)(1.0f, 0.0f) == 0.0f && signbit(F(Log)(1.0f, 0.0f)));
    assert(D(Log)(1.0, INFINITY) == 0.0 && !signbit(D(Log)(1.0, INFINITY)));
    assert(isnan(F(Log)(NAN, 2.0f)) && isnan(D(Log)(NAN, 2.0)));
    assert(M(ReciprocalEstimate)(4.0f) == 0.25f && L(ReciprocalEstimate)(-4.0) == -0.25);
    assert(M(ReciprocalSqrtEstimate)(16.0f) == 0.25f && L(ReciprocalSqrtEstimate)(16.0) == 0.25);
    assert(M(ReciprocalEstimate)(0.0f) == INFINITY && L(ReciprocalEstimate)(-0.0) == -INFINITY);
    assert(M(ReciprocalSqrtEstimate)(-0.0f) == -INFINITY && L(ReciprocalSqrtEstimate)(0.0) == INFINITY);
    assert(isnan(M(ReciprocalSqrtEstimate)(-1.0f)) && isnan(L(ReciprocalSqrtEstimate)(-1.0)));
    assert(M(ReciprocalEstimate)(INFINITY) == 0.0f && signbit(L(ReciprocalEstimate)(-INFINITY)));
    assert(M(ReciprocalSqrtEstimate)(INFINITY) == 0.0f && L(ReciprocalSqrtEstimate)(INFINITY) == 0.0);
    assert(isnan(M(ReciprocalEstimate)(NAN)) && isnan(L(ReciprocalSqrtEstimate)(NAN)));
}

static void invalid_arguments(int mode) {
    struct cx_exception_frame frame;
    cx_exception_push(&frame);
    if (setjmp(frame.environment) == 0) {
        switch (mode) {
        case 0: (void)M(Abs)(INT_MIN); break;
        case 1: (void)L(Abs)(LLONG_MIN); break;
        case 2: (void)M(Clamp)(0, 1, -1); break;
        case 3: (void)L(Clamp)(0, LLONG_MAX, LLONG_MIN); break;
        case 4: (void)F(Clamp)(NAN, INFINITY, -INFINITY); break;
        case 5: (void)D(Clamp)(NAN, INFINITY, -INFINITY); break;
        case 6: (void)L(Round)(1.0f, -1); break;
        case 7: (void)L(Round)(NAN, 7); break;
        case 8: (void)D(Round)(1.0, -1); break;
        default: (void)D(Round)(INFINITY, 16); break;
        }
        assert(!"Expected Math to throw a CX exception");
    } else {
        cx_exception_pop(&frame);
        struct cxcore_System_Exception *error = cx_exception_current();
        assert(error != CX_NULL);
        struct cxcore_System_String *message =
            cxcore_System_Exception_Message___const_get(error);
        assert(message != CX_NULL && message->_length > 5 && memcmp(message->_data, "Math.", 5) == 0);
        cx_exception_clear();
    }
}

int main(void) {
    constants();
    integers();
    floating_point();
    rounding();
    logarithms_and_reciprocals();
    for (int mode = 0; mode < 10; ++mode) invalid_arguments(mode);
    assert(!cx_exception_pending());
    return 0;
}

#include "cxcore.h"
#include <limits.h>
#include <math.h>

cx_double cxcore_System_Math_E = 2.7182818284590452354;
cx_double cxcore_System_Math_Pi = 3.14159265358979323846;
cx_double cxcore_System_Math_Tau = 6.283185307179586476925;
cx_double cxcore_System_Math_Sqrt2 = 1.41421356237309504880;
cx_double cxcore_System_Math_Sqrt3 = 1.73205080756887729353;

CX_STRING_DEF(cx_math_abs_overflow, "Math.Abs result is outside the signed integer range");
CX_STRING_DEF(cx_math_invalid_bounds, "Math.Clamp requires min <= max");
CX_STRING_DEF(cx_math_float_digits, "Math.Round float decimals must be between 0 and 6");
CX_STRING_DEF(cx_math_double_digits, "Math.Round double decimals must be between 0 and 15");
CX_STRING_DEF(cx_math_min_parameter, "min");
CX_STRING_DEF(cx_math_decimals_parameter, "decimals");

static CX_NORETURN void fail_overflow(const struct cxcore_System_String *message) {
    struct cxcore_System_OverflowException *error =
        cx_exception_allocate_durable(cxcore_System_OverflowException___typeinfo.Size);
    cxcore_System_OverflowException___constructor__2(error, message);
    cx_exception_throw((struct cxcore_System_Exception *)error, __FILE__, __LINE__);
}

static CX_NORETURN void fail_argument(
    const struct cxcore_System_String *message,
    const struct cxcore_System_String *parameter) {
    struct cxcore_System_ArgumentOutOfRangeException *error =
        cx_exception_allocate_durable(
            cxcore_System_ArgumentOutOfRangeException___typeinfo.Size);
    cxcore_System_ArgumentOutOfRangeException___constructor__3(
        error, message, parameter);
    cx_exception_throw((struct cxcore_System_Exception *)error, __FILE__, __LINE__);
}

cx_int cxcore_System_Math_Abs(cx_int value) {
    if (value == INT_MIN) fail_overflow(&cx_math_abs_overflow);
    return value < 0 ? -value : value;
}
cx_long cxcore_System_Math_Abs__2(cx_long value) {
    if (value == LLONG_MIN) fail_overflow(&cx_math_abs_overflow);
    return value < 0 ? -value : value;
}
cx_float cxcore_System_Math_Abs__3(cx_float value) {
    return fabsf(value);
}
cx_double cxcore_System_Math_Abs__4(cx_double value) {
    return fabs(value);
}

cx_int cxcore_System_Math_Clamp(cx_int value, cx_int min, cx_int max) {
    if (min > max) fail_argument(&cx_math_invalid_bounds, &cx_math_min_parameter);
    return value < min ? min : value > max ? max : value;
}
cx_long cxcore_System_Math_Clamp__2(cx_long value, cx_long min, cx_long max) {
    if (min > max) fail_argument(&cx_math_invalid_bounds, &cx_math_min_parameter);
    return value < min ? min : value > max ? max : value;
}
cx_float cxcore_System_Math_Clamp__3(cx_float value, cx_float min, cx_float max) {
    if (min > max) fail_argument(&cx_math_invalid_bounds, &cx_math_min_parameter);
    return value < min ? min : value > max ? max : value;
}
cx_double cxcore_System_Math_Clamp__4(cx_double value, cx_double min, cx_double max) {
    if (min > max) fail_argument(&cx_math_invalid_bounds, &cx_math_min_parameter);
    return value < min ? min : value > max ? max : value;
}

cx_int cxcore_System_Math_Floor(cx_int value) { return value; }
cx_long cxcore_System_Math_Floor__2(cx_long value) { return value; }
cx_float cxcore_System_Math_Floor__3(cx_float value) { return floorf(value); }
cx_double cxcore_System_Math_Floor__4(cx_double value) { return floor(value); }

cx_int cxcore_System_Math_Ceiling(cx_int value) { return value; }
cx_long cxcore_System_Math_Ceiling__2(cx_long value) { return value; }
cx_float cxcore_System_Math_Ceiling__3(cx_float value) { return ceilf(value); }
cx_double cxcore_System_Math_Ceiling__4(cx_double value) { return ceil(value); }

cx_int cxcore_System_Math_Min(cx_int value1, cx_int value2) {
    return value1 < value2 ? value1 : value2;
}
cx_long cxcore_System_Math_Min__2(cx_long value1, cx_long value2) {
    return value1 < value2 ? value1 : value2;
}
cx_float cxcore_System_Math_Min__3(cx_float value1, cx_float value2) {
    if (value1 == value2) return signbit(value1) ? value1 : value2;
    return isnan(value1) || value1 < value2 ? value1 : value2;
}
cx_double cxcore_System_Math_Min__4(cx_double value1, cx_double value2) {
    if (value1 == value2) return signbit(value1) ? value1 : value2;
    return isnan(value1) || value1 < value2 ? value1 : value2;
}

cx_int cxcore_System_Math_Max(cx_int value1, cx_int value2) {
    return value1 > value2 ? value1 : value2;
}
cx_long cxcore_System_Math_Max__2(cx_long value1, cx_long value2) {
    return value1 > value2 ? value1 : value2;
}
cx_float cxcore_System_Math_Max__3(cx_float value1, cx_float value2) {
    if (value1 == value2) return signbit(value1) ? value2 : value1;
    return isnan(value1) || value1 > value2 ? value1 : value2;
}
cx_double cxcore_System_Math_Max__4(cx_double value1, cx_double value2) {
    if (value1 == value2) return signbit(value1) ? value2 : value1;
    return isnan(value1) || value1 > value2 ? value1 : value2;
}

// C round/roundf choose ties away from zero, and nearbyint follows the native
// rounding mode. Select the even neighbor explicitly without changing that mode.
cx_float cxcore_System_Math_Round(cx_float value) {
    cx_float magnitude = fabsf(value);
    if (!(magnitude < 8388608.0f)) return value; // Integral values, infinities, NaN.
    cx_float whole = floorf(magnitude);
    cx_float fraction = magnitude - whole;
    if (fraction > 0.5f || (fraction == 0.5f && fmodf(whole, 2.0f) != 0.0f)) whole += 1.0f;
    return copysignf(whole, value);
}
cx_double cxcore_System_Math_Round__3(cx_double value) {
    cx_double magnitude = fabs(value);
    if (!(magnitude < 4503599627370496.0)) return value;
    cx_double whole = floor(magnitude);
    cx_double fraction = magnitude - whole;
    if (fraction > 0.5 || (fraction == 0.5 && fmod(whole, 2.0) != 0.0)) whole += 1.0;
    return copysign(whole, value);
}
cx_float cxcore_System_Math_Round__2(cx_float value, cx_int decimals) {
    static const cx_float powers[] = {1e0f, 1e1f, 1e2f, 1e3f, 1e4f, 1e5f, 1e6f};
    if (decimals < 0 || decimals > 6) fail_argument(&cx_math_float_digits, &cx_math_decimals_parameter);
    if (fabsf(value) >= 1e8f || !isfinite(value)) return value;
    cx_float scaled = value * powers[decimals];
    return cxcore_System_Math_Round(scaled) / powers[decimals];
}
cx_double cxcore_System_Math_Round__4(cx_double value, cx_int decimals) {
    static const cx_double powers[] = {
        1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15
    };
    if (decimals < 0 || decimals > 15) fail_argument(&cx_math_double_digits, &cx_math_decimals_parameter);
    if (fabs(value) >= 1e16 || !isfinite(value)) return value;
    cx_double scaled = value * powers[decimals];
    return cxcore_System_Math_Round__3(scaled) / powers[decimals];
}

cx_float cxcore_System_Math_Log(cx_float value) { return logf(value); }
cx_double cxcore_System_Math_Log__2(cx_double value) { return log(value); }
cx_float cxcore_System_Math_Log__3(cx_float value, cx_float newBase) {
    if (isnan(value)) return value;
    if (isnan(newBase)) return newBase;
    if (newBase == 1.0f || (value != 1.0f && (newBase == 0.0f || newBase == INFINITY))) return NAN;
    return logf(value) / logf(newBase);
}
cx_double cxcore_System_Math_Log__4(cx_double value, cx_double newBase) {
    if (isnan(value)) return value;
    if (isnan(newBase)) return newBase;
    if (newBase == 1.0 || (value != 1.0 && (newBase == 0.0 || newBase == INFINITY))) return NAN;
    return log(value) / log(newBase);
}

// Portable fallbacks also serve platforms without reciprocal estimate instructions.
cx_float cxcore_System_Math_ReciprocalEstimate(cx_float value) { return 1.0f / value; }
cx_double cxcore_System_Math_ReciprocalEstimate__2(cx_double value) { return 1.0 / value; }
cx_float cxcore_System_Math_ReciprocalSqrtEstimate(cx_float value) { return 1.0f / sqrtf(value); }
cx_double cxcore_System_Math_ReciprocalSqrtEstimate__2(cx_double value) { return 1.0 / sqrt(value); }

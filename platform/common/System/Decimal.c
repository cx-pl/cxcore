#include "cxnumeric.h"
#include "cxcore.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

// Wide integer intermediates keep arithmetic exact until the single final
// round-to-nearest, ties-to-even step. No binary floating point is used by
// decimal arithmetic, parsing, comparison, or integer conversions.
#define LIMBS 12
typedef struct { uint32_t word[LIMBS]; } wide;

CX_STRING_DEF(cx_decimal_overflow, "Decimal overflow");
CX_STRING_DEF(cx_decimal_zero_divisor, "Decimal division by zero");
CX_STRING_DEF(cx_decimal_invalid, "Invalid decimal value");

cx_decimal cxcore_System_Decimal_MaxValue = {UINT_MAX, UINT_MAX, UINT_MAX, 0};
cx_decimal cxcore_System_Decimal_MinValue = {UINT_MAX, UINT_MAX, UINT_MAX, 0x80000000u};

static CX_NORETURN void fail(const struct cxcore_System_String *message) {
    struct cxcore_System_Exception *error =
        cx_exception_allocate_durable((cx_uint)sizeof(*error));
    cxcore_System_Exception___constructor(error, message);
    cx_exception_throw(error, __FILE__, __LINE__);
}

static unsigned scale(cx_decimal value) {
    if ((value.flags & 0x7F00FFFFu) != 0 || ((value.flags >> 16) & 255u) > 28)
        fail(&cx_decimal_invalid);
    return (value.flags >> 16) & 255u;
}
static wide coefficient(cx_decimal value) {
    wide result = {{value.lo, value.mid, value.hi}};
    return result;
}
static int compare(wide a, wide b) {
    for (int i = LIMBS - 1; i >= 0; --i)
        if (a.word[i] != b.word[i]) return a.word[i] < b.word[i] ? -1 : 1;
    return 0;
}
static int zero(wide a) { return compare(a, (wide){{0}}) == 0; }
static int fits(wide a) {
    for (int i = 3; i < LIMBS; ++i) if (a.word[i] != 0) return 0;
    return 1;
}
static wide add(wide a, wide b) {
    uint64_t carry = 0;
    for (int i = 0; i < LIMBS; ++i) {
        uint64_t sum = (uint64_t)a.word[i] + b.word[i] + carry;
        a.word[i] = (uint32_t)sum;
        carry = sum >> 32;
    }
    if (carry) fail(&cx_decimal_overflow);
    return a;
}
static wide subtract(wide a, wide b) {
    uint64_t borrow = 0;
    for (int i = 0; i < LIMBS; ++i) {
        uint64_t sub = (uint64_t)b.word[i] + borrow;
        uint64_t original = a.word[i];
        a.word[i] = (uint32_t)(original - sub);
        borrow = original < sub;
    }
    return a;
}
static void multiply_small(wide *a, unsigned factor) {
    uint64_t carry = 0;
    for (int i = 0; i < LIMBS; ++i) {
        uint64_t product = (uint64_t)a->word[i] * factor + carry;
        a->word[i] = (uint32_t)product;
        carry = product >> 32;
    }
    if (carry) fail(&cx_decimal_overflow);
}
static void power10(wide *a, unsigned count) {
    while (count--) multiply_small(a, 10);
}
static unsigned divide10(wide *a) {
    uint64_t remainder = 0;
    for (int i = LIMBS - 1; i >= 0; --i) {
        uint64_t n = (remainder << 32) | a->word[i];
        a->word[i] = (uint32_t)(n / 10);
        remainder = n % 10;
    }
    return (unsigned)remainder;
}
static wide quotient(wide a, wide b, wide *remainder) {
    wide q = {{0}}, r = {{0}};
    if (zero(b)) fail(&cx_decimal_zero_divisor);
    for (int bit = LIMBS * 32 - 1; bit >= 0; --bit) {
        uint32_t carry = (a.word[bit / 32] >> (bit % 32)) & 1u;
        for (int i = 0; i < LIMBS; ++i) {
            uint32_t next = r.word[i] >> 31;
            r.word[i] = (r.word[i] << 1) | carry;
            carry = next;
        }
        if (compare(r, b) >= 0) {
            r = subtract(r, b);
            q.word[bit / 32] |= (uint32_t)1 << (bit % 32);
        }
    }
    *remainder = r;
    return q;
}

static int try_finish(wide n, unsigned digits, unsigned sign, int sticky, cx_decimal *result) {
    unsigned guard = 0;
    int dropped = 0;
    for (;;) {
        while (digits > 28 || !fits(n)) {
            if (digits == 0) return 0;
            sticky |= guard != 0;
            guard = divide10(&n);
            --digits;
            dropped = 1;
        }
        if (dropped && (guard > 5 || (guard == 5 && (sticky || (n.word[0] & 1u))))) {
            wide rounded = add(n, (wide){{1}});
            if (!fits(rounded)) {
                if (digits == 0) return 0;
                sticky |= guard != 0;
                guard = divide10(&n);
                --digits;
                continue;
            }
            n = rounded;
        }
        *result = (cx_decimal){n.word[0], n.word[1], n.word[2],
            (digits << 16) | (sign ? 0x80000000u : 0u)};
        return 1;
    }
}

static cx_decimal finish(wide n, unsigned digits, unsigned sign, int sticky) {
    cx_decimal result;
    if (!try_finish(n, digits, sign, sticky, &result)) fail(&cx_decimal_overflow);
    return result;
}

CX_API cx_bool cx_decimal_try_parse(const char *text, cx_decimal *result) {
    wide n = {{0}};
    int sign = 0, fractional = 0, found = 0, sticky = 0, started = 0;
    unsigned kept = 0;
    int64_t fractionDigits = 0, discarded = 0, exponent = 0;
    if (text == NULL || result == NULL) return CX_FALSE;
    if (*text == '-' || *text == '+') { sign = *text == '-'; ++text; }
    while ((*text >= '0' && *text <= '9') || *text == '.') {
        if (*text == '.') {
            if (fractional) return CX_FALSE;
            fractional = 1;
        } else {
            unsigned digit = (unsigned)(*text - '0');
            found = 1;
            if (fractional) ++fractionDigits;
            if (digit) started = 1;
            if (started) {
                if (kept < 100) {
                    multiply_small(&n, 10);
                    n = add(n, (wide){{digit}});
                    ++kept;
                } else { ++discarded; sticky |= digit != 0; }
            }
        }
        ++text;
    }
    if (*text == 'e' || *text == 'E') {
        int negative = 0;
        ++text;
        if (*text == '-' || *text == '+') { negative = *text == '-'; ++text; }
        if (*text < '0' || *text > '9') return CX_FALSE;
        while (*text >= '0' && *text <= '9') {
            if (exponent < 1000000000000LL) exponent = exponent * 10 + (*text - '0');
            ++text;
        }
        if (negative) exponent = -exponent;
    }
    if (*text != 0 || !found) return CX_FALSE;
    int64_t digits = fractionDigits - discarded - exponent;
    if (digits < 0) {
        if (!zero(n)) {
            if ((int64_t)kept - digits > 29) return CX_FALSE;
            power10(&n, (unsigned)-digits);
        }
        digits = 0;
    }
    if (digits > 150) {
        *result = (cx_decimal){0, 0, 0, (28u << 16) | (sign ? 0x80000000u : 0)};
        return CX_TRUE;
    }
    return (cx_bool)try_finish(n, (unsigned)digits, (unsigned)sign, sticky, result);
}

CX_API cx_decimal cx_decimal_from_u64(cx_ulong value) {
    return (cx_decimal){(cx_uint)value, (cx_uint)(value >> 32), 0, 0};
}
CX_API cx_decimal cx_decimal_from_i64(cx_long value) {
    cx_decimal result = cx_decimal_from_u64(value < 0 ? (cx_ulong)0 - (cx_ulong)value : (cx_ulong)value);
    if (value < 0) result.flags = 0x80000000u;
    return result;
}
CX_API cx_decimal cx_decimal_negate(cx_decimal value) {
    (void)scale(value);
    value.flags ^= 0x80000000u;
    return value;
}
static void align(cx_decimal a, cx_decimal b, wide *left, wide *right, unsigned *digits) {
    unsigned sa = scale(a), sb = scale(b);
    *digits = sa > sb ? sa : sb;
    *left = coefficient(a); *right = coefficient(b);
    power10(left, *digits - sa); power10(right, *digits - sb);
}
CX_API cx_decimal cx_decimal_add(cx_decimal a, cx_decimal b) {
    wide left, right, n;
    unsigned digits, sign = a.flags >> 31;
    align(a, b, &left, &right, &digits);
    if (((a.flags ^ b.flags) >> 31) == 0) n = add(left, right);
    else if (compare(left, right) >= 0) n = subtract(left, right);
    else { n = subtract(right, left); sign = b.flags >> 31; }
    return finish(n, digits, sign, 0);
}
CX_API cx_decimal cx_decimal_subtract(cx_decimal a, cx_decimal b) {
    return cx_decimal_add(a, cx_decimal_negate(b));
}
CX_API cx_decimal cx_decimal_multiply(cx_decimal a, cx_decimal b) {
    wide left = coefficient(a), right = coefficient(b), n = {{0}};
    unsigned digits = scale(a) + scale(b);
    for (int i = 0; i < 3; ++i) {
        uint64_t carry = 0;
        for (int j = 0; j < 3; ++j) {
            uint64_t product = (uint64_t)left.word[i] * right.word[j] + n.word[i + j] + carry;
            n.word[i + j] = (uint32_t)product;
            carry = product >> 32;
        }
        n.word[i + 3] = (uint32_t)carry;
    }
    return finish(n, digits, (a.flags ^ b.flags) >> 31, 0);
}
CX_API cx_decimal cx_decimal_divide(cx_decimal a, cx_decimal b) {
    unsigned sa = scale(a), sb = scale(b);
    wide left = coefficient(a), right = coefficient(b), remainder;
    power10(&left, 29 + sb - sa);
    wide n = quotient(left, right, &remainder);
    cx_decimal result = finish(n, 29, (a.flags ^ b.flags) >> 31, !zero(remainder));
    // Preserve the preferred input scale for exact results, removing only
    // unnecessary trailing zeroes introduced by division.
    unsigned minimum = sa > sb ? sa - sb : 0;
    n = coefficient(result);
    unsigned digits = scale(result);
    while (digits > minimum) {
        wide reduced = n;
        if (divide10(&reduced) != 0) break;
        n = reduced; --digits;
    }
    return finish(n, digits, result.flags >> 31, 0);
}
CX_API cx_decimal cx_decimal_remainder(cx_decimal a, cx_decimal b) {
    wide left, right, remainder;
    unsigned digits;
    align(a, b, &left, &right, &digits);
    (void)quotient(left, right, &remainder);
    return finish(remainder, digits, a.flags >> 31, 0);
}
CX_API cx_int cx_decimal_compare(cx_decimal a, cx_decimal b) {
    wide left, right;
    unsigned digits;
    align(a, b, &left, &right, &digits);
    if (zero(left) && zero(right)) return 0;
    int sa = (int)(a.flags >> 31), sb = (int)(b.flags >> 31);
    if (sa != sb) return sa ? -1 : 1;
    int result = compare(left, right);
    return sa ? -result : result;
}

CX_API cx_decimal cx_decimal_parse(const char *text) {
    wide n = {{0}};
    unsigned digits = 0, sign = 0;
    int fractional = 0, found = 0, exponent = 0, exponentSign = 1;
    if (*text == '-' || *text == '+') { sign = *text == '-'; ++text; }
    while ((*text >= '0' && *text <= '9') || *text == '.') {
        if (*text == '.') {
            if (fractional) fail(&cx_decimal_invalid);
            fractional = 1;
        } else {
            multiply_small(&n, 10);
            n = add(n, (wide){{(uint32_t)(*text - '0')}});
            if (fractional) ++digits;
            found = 1;
        }
        ++text;
    }
    if (*text == 'e' || *text == 'E') {
        ++text;
        if (*text == '-' || *text == '+') { exponentSign = *text == '-' ? -1 : 1; ++text; }
        if (*text < '0' || *text > '9') fail(&cx_decimal_invalid);
        while (*text >= '0' && *text <= '9') {
            if (exponent > 10000) fail(&cx_decimal_overflow);
            exponent = exponent * 10 + *text++ - '0';
        }
    }
    if (*text != 0 || !found) fail(&cx_decimal_invalid);
    int finalScale = (int)digits - exponentSign * exponent;
    if (finalScale < 0) { if (!zero(n)) power10(&n, (unsigned)-finalScale); finalScale = 0; }
    return finish(n, (unsigned)finalScale, sign, 0);
}

CX_API cx_double cx_decimal_to_double(cx_decimal value) {
    double n = ((double)value.hi * 4294967296.0 + value.mid) * 4294967296.0 + value.lo;
    n /= pow(10.0, (double)scale(value));
    return value.flags >> 31 ? -n : n;
}
CX_API cx_decimal cx_decimal_from_double(cx_double value, cx_int significantDigits) {
    char text[64];
    if (!isfinite(value)) fail(&cx_decimal_overflow);
    if (significantDigits != 7 && significantDigits != 15) fail(&cx_decimal_invalid);
    snprintf(text, sizeof(text), "%.*g", significantDigits, value);
    // snprintf follows the process locale; the decimal parser deliberately does not.
    for (char *p = text; *p; ++p) if (*p == ',') *p = '.';
    return cx_decimal_parse(text);
}
static wide truncate(cx_decimal value) {
    wide n = coefficient(value);
    unsigned digits = scale(value);
    while (digits--) (void)divide10(&n);
    return n;
}
CX_API cx_ulong cx_decimal_to_u64(cx_decimal value, cx_ulong maximum) {
    wide n = truncate(value);
    if (n.word[2] != 0 || !fits(n) || ((value.flags >> 31) && !zero(n))) fail(&cx_decimal_overflow);
    cx_ulong result = ((cx_ulong)n.word[1] << 32) | n.word[0];
    if (result > maximum) fail(&cx_decimal_overflow);
    return result;
}
CX_API cx_long cx_decimal_to_i64(cx_decimal value, cx_ulong maximum) {
    wide n = truncate(value);
    if (n.word[2] != 0 || !fits(n)) fail(&cx_decimal_overflow);
    cx_ulong result = ((cx_ulong)n.word[1] << 32) | n.word[0];
    if (value.flags >> 31) {
        if (result > maximum + 1) fail(&cx_decimal_overflow);
        if (result == 0x8000000000000000ULL) return LLONG_MIN;
        return -(cx_long)result;
    }
    if (result > maximum) fail(&cx_decimal_overflow);
    return (cx_long)result;
}
CX_API cx_uint cx_decimal_format(cx_decimal value, char *buffer, cx_uint capacity) {
    char reversed[29], text[32];
    wide n = coefficient(value);
    unsigned digits = scale(value), count = 0, length = 0;
    do { reversed[count++] = (char)('0' + divide10(&n)); } while (!zero(n));
    if ((value.flags >> 31) && (value.lo || value.mid || value.hi)) text[length++] = '-';
    if (count <= digits) {
        text[length++] = '0';
        if (digits) text[length++] = '.';
        for (unsigned i = count; i < digits; ++i) text[length++] = '0';
    }
    for (unsigned i = count; i > 0; --i) {
        text[length++] = reversed[i - 1];
        if (digits && i - 1 == digits) text[length++] = '.';
    }
    if (capacity > length) { memcpy(buffer, text, length); buffer[length] = 0; }
    return length;
}

CX_API cx_decimal cx_decimal_increment(cx_decimal *location, cx_int delta, cx_bool postfix) {
    cx_decimal old = *location;
    *location = cx_decimal_add(old, cx_decimal_from_i64(delta));
    return postfix ? old : *location;
}
#define ASSIGN(name) \
    CX_API cx_decimal cx_decimal_##name##_assign(cx_decimal *location, cx_decimal value) { \
        return *location = cx_decimal_##name(*location, value); \
    }
ASSIGN(add)
ASSIGN(subtract)
ASSIGN(multiply)
ASSIGN(divide)
ASSIGN(remainder)
#undef ASSIGN

struct cxcore_System_Nullable cxcore_System_Decimal_TryParse(
    const struct cxcore_System_String *text) {
    return cx_numeric_decimal(text);
}

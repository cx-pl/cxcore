#include "cxnumeric.h"
#include "../stdlib.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct cxcore_System_String string;
typedef struct cxcore_System_Nullable nullable;

static int space(unsigned char c) { return c == ' ' || (c >= '\t' && c <= '\r'); }
static int bounds(const string *text, const unsigned char **start, const unsigned char **end) {
    if (text == CX_NULL || text->_length == 0 || text->_data == CX_NULL) return 0;
    *start = text->_data; *end = *start + text->_length;
    while (*start < *end && space(**start)) ++*start;
    while (*end > *start && space((*end)[-1])) --*end;
    return *start < *end;
}
nullable cx_numeric_store(const void *value, cx_uint size) {
    cx_ptr storage = cxcore_System_Memory_Alloc(size);
    memcpy(storage, value, size);
    return (nullable){storage};
}
int cx_numeric_integer(const string *text, uint64_t positiveMax, uint64_t negativeMax, uint64_t *value, int *negative) {
    const unsigned char *p, *end;
    if (!bounds(text, &p, &end)) return 0;
    *negative = 0;
    if (*p == '-' || *p == '+') { *negative = *p == '-'; ++p; }
    if (p == end) return 0;
    uint64_t limit = *negative ? negativeMax : positiveMax, n = 0;
    while (p < end) {
        if (*p < '0' || *p > '9') return 0;
        unsigned digit = *p++ - '0';
        if (n > limit / 10 || (n == limit / 10 && digit > limit % 10)) return 0;
        n = n * 10 + digit;
    }
    *value = n;
    return 1;
}
// Validate the complete invariant grammar before passing a NUL-terminated copy
// to the platform's C-locale converter. Embedded NUL and hexadecimal inputs fail.
static char *number_text(const string *text) {
    const unsigned char *start, *end, *p;
    if (!bounds(text, &start, &end)) return CX_NULL;
    p = start;
    if (*p == '+' || *p == '-') ++p;
    int digits = 0;
    while (p < end && *p >= '0' && *p <= '9') { ++p; digits = 1; }
    if (p < end && *p == '.') {
        ++p;
        while (p < end && *p >= '0' && *p <= '9') { ++p; digits = 1; }
    }
    if (!digits) return CX_NULL;
    if (p < end && (*p == 'e' || *p == 'E')) {
        ++p;
        if (p < end && (*p == '+' || *p == '-')) ++p;
        const unsigned char *exponent = p;
        while (p < end && *p >= '0' && *p <= '9') ++p;
        if (p == exponent) return CX_NULL;
    }
    if (p != end || (size_t)(end - start) >= UINT_MAX) return CX_NULL;
    cx_uint length = (cx_uint)(end - start);
    char *copy = (char *)malloc((size_t)length + 1u);
    if (copy == NULL) return CX_NULL;
    memcpy(copy, start, length);
    copy[length] = 0;
    return copy;
}
nullable cx_numeric_double(const string *text) {
    char *copy = number_text(text);
    if (!copy) return (nullable){0};
    double value;
    int valid = cx_platform_parse_float(copy, 0, &value);
    free(copy);
    return valid ? cx_numeric_store(&value, sizeof(value)) : (nullable){0};
}
nullable cx_numeric_float(const string *text) {
    char *copy = number_text(text);
    if (!copy) return (nullable){0};
    double parsed;
    int valid = cx_platform_parse_float(copy, 1, &parsed);
    free(copy);
    if (!valid) return (nullable){0};
    cx_float value = (cx_float)parsed;
    return cx_numeric_store(&value, sizeof(value));
}
nullable cx_numeric_decimal(const string *text) {
    char *copy = number_text(text);
    if (!copy) return (nullable){0};
    cx_decimal value;
    int valid = cx_decimal_try_parse(copy, &value);
    free(copy);
    return valid ? cx_numeric_store(&value, sizeof(value)) : (nullable){0};
}

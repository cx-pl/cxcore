#include "../../../src/cxcore.h"
#include "../../stdlib.h"
#include "UuidHash.internal.h"
#include <limits.h>
#include <string.h>

typedef struct cxcore_System_Uuid uuid;
typedef struct cxcore_System_String string;
CX_STRING_DEF(uuid_error_message, "UUID generation failed or received invalid arguments");
static CX_NORETURN void uuid_error(void) {
    struct cxcore_System_Exception *error = cx_exception_allocate_durable((cx_uint)sizeof(*error));
    cxcore_System_Exception___constructor(error, &uuid_error_message);
    cx_exception_throw(error, __FILE__, __LINE__);
}

struct cxcore_System_Uuid
    cxcore_System_Uuid_Zero = {._a = 0, ._b = 0, ._c = 0, ._d = 0};

const uuid cxcore_System_Uuid_NamespaceDns = {0x6ba7b810,0x9dad11d1,0x80b400c0,0x4fd430c8};
const uuid cxcore_System_Uuid_NamespaceUrl = {0x6ba7b811,0x9dad11d1,0x80b400c0,0x4fd430c8};
const uuid cxcore_System_Uuid_NamespaceOid = {0x6ba7b812,0x9dad11d1,0x80b400c0,0x4fd430c8};
const uuid cxcore_System_Uuid_NamespaceX500 = {0x6ba7b814,0x9dad11d1,0x80b400c0,0x4fd430c8};

static uuid from_bytes(const unsigned char bytes[16], unsigned version) {
    uuid result = {uuid_read32(bytes,1),uuid_read32(bytes+4,1),uuid_read32(bytes+8,1),uuid_read32(bytes+12,1)};
    result._b = (result._b & 0xffff0fffu) | (version << 12);
    result._c = (result._c & 0x3fffffffu) | 0x80000000u;
    return result;
}
static void to_bytes(uuid value, unsigned char bytes[16]) {
    cx_uint words[4] = {value._a,value._b,value._c,value._d};
    for (unsigned i = 0; i < 16; ++i) bytes[i] = (unsigned char)(words[i/4] >> (24-8*(i%4)));
}
static uuid random_uuid(unsigned version) {
    unsigned char bytes[16];
    if (!cx_platform_random_bytes(bytes, sizeof(bytes))) uuid_error();
    return from_bytes(bytes, version);
}
static uuid named_uuid(uuid namespaceId, const string *name, int sha1) {
    if (!name || (name->_length && !name->_data)) uuid_error();
    unsigned char bytes[16], digest[20];
    to_bytes(namespaceId, bytes);
    cx_uuid_hash h = {{0x67452301,0xefcdab89,0x98badcfe,0x10325476,0xc3d2e1f0},0,0,sha1,{0}};
    uuid_hash_update(&h, bytes, 16);
    uuid_hash_update(&h, name->_data, name->_length);
    uuid_hash_finish(&h, digest);
    return from_bytes(digest, sha1 ? 5 : 3);
}
uuid cxcore_System_Uuid_NewUuidV3(uuid namespaceId, const string *name) { return named_uuid(namespaceId,name,0); }
uuid cxcore_System_Uuid_NewUuidV5(uuid namespaceId, const string *name) { return named_uuid(namespaceId,name,1); }
uuid cxcore_System_Uuid_NewUuidV4(void) { return random_uuid(4); }
uuid cxcore_System_Uuid_NewUuidV8(void) { return random_uuid(8); }

static uint64_t last_gregorian;
static unsigned char time_node[6];
static uint16_t time_sequence;
static int time_initialized;
static uuid time_uuid(unsigned version) {
    uint64_t time;
    if (!cx_platform_unix_time_100ns(&time) || time > 0x0fffffffffffffffULL - 122192928000000000ULL) uuid_error();
    time += 122192928000000000ULL;
    cx_platform_uuid_lock();
    if (!time_initialized) {
        unsigned char seed[8];
        if (!cx_platform_random_bytes(seed,sizeof(seed))) { cx_platform_uuid_unlock(); uuid_error(); }
        memcpy(time_node,seed,6); time_node[0] |= 1;
        time_sequence = (uint16_t)(((uint16_t)seed[6] << 8 | seed[7]) & 0x3fff);
        time_initialized = 1;
    }
    if (time <= last_gregorian) time = last_gregorian + 1;
    if (time > 0x0fffffffffffffffULL) { cx_platform_uuid_unlock(); uuid_error(); }
    last_gregorian = time;
    uuid result;
    if (version == 1) {
        result._a = (uint32_t)time;
        result._b = (uint32_t)(((time >> 32) & 0xffff) << 16) | 0x1000 | (uint32_t)(time >> 48);
    } else {
        result._a = (uint32_t)(time >> 28);
        result._b = (uint32_t)(((time >> 12) & 0xffff) << 16) | 0x6000 | (uint32_t)(time & 0xfff);
    }
    result._c = 0x80000000u | (uint32_t)time_sequence << 16 | (uint32_t)time_node[0] << 8 | time_node[1];
    result._d = uuid_read32(time_node+2,1);
    cx_platform_uuid_unlock();
    return result;
}
uuid cxcore_System_Uuid_NewUuidV1(void) { return time_uuid(1); }
uuid cxcore_System_Uuid_NewUuidV6(void) { return time_uuid(6); }

uuid cxcore_System_Uuid_NewUuidV2(cx_byte domain, cx_uint identifier) {
    uint64_t time;
    // Domain 0=person, 1=group, 2=organization. Other byte domains are application-defined.
    if (!cx_platform_unix_time_100ns(&time) || time > 0x0fffffffffffffffULL - 122192928000000000ULL) uuid_error();
    time += 122192928000000000ULL;
    uuid result = random_uuid(2);
    result._a = identifier;
    result._b = (uint32_t)(((time >> 32) & 0xffff) << 16) | 0x2000 | (uint32_t)(time >> 48);
    result._c = (result._c & 0xff00ffffu) | ((uint32_t)domain << 16) | 0x100u;
    // A fresh random multicast node avoids the 64-per-time-window sequence
    // limit imposed by replacing v1's low timestamp and sequence bytes.
    return result;
}

static uint64_t last_milliseconds, v7_random_b;
static uint16_t v7_random_a;
static int v7_initialized;
uuid cxcore_System_Uuid_NewUuidV7(void) {
    uint64_t time;
    unsigned char seed[10];
    if (!cx_platform_unix_time_100ns(&time) || !cx_platform_random_bytes(seed,sizeof(seed))) uuid_error();
    time /= 10000;
    cx_platform_uuid_lock();
    if (!v7_initialized || time > last_milliseconds) {
        v7_initialized = 1;
        last_milliseconds = time;
        v7_random_a = (uint16_t)(((uint16_t)seed[0] << 8 | seed[1]) & 0xfff);
        v7_random_b = ((uint64_t)uuid_read32(seed+2,1) << 32 | uuid_read32(seed+6,1)) & 0x3fffffffffffffffULL;
    } else if (++v7_random_b > 0x3fffffffffffffffULL) {
        v7_random_b = 0;
        if (++v7_random_a > 0xfff) { v7_random_a = 0; ++last_milliseconds; }
    }
    if (last_milliseconds > 0xffffffffffffULL) { cx_platform_uuid_unlock(); uuid_error(); }
    uuid result = {(uint32_t)(last_milliseconds >> 16),
        (uint32_t)(last_milliseconds << 16) | 0x7000 | v7_random_a,
        (uint32_t)(v7_random_b >> 32) | 0x80000000u, (uint32_t)v7_random_b};
    cx_platform_uuid_unlock();
    return result;
}
uuid cxcore_System_Uuid_NewUuidV8__2(cx_ulong customA, cx_ushort customB, cx_ulong customC) {
    if (customA > 0xffffffffffffULL || customB > 0xfff || customC > 0x3fffffffffffffffULL) uuid_error();
    return (uuid){(cx_uint)(customA >> 16), (cx_uint)(customA << 16) | 0x8000 | customB,
        (cx_uint)(customC >> 32) | 0x80000000u, (cx_uint)customC};
}
cx_uint cxcore_System_Uuid_Version___const_get(const uuid *self) { return (self->_b >> 12) & 15; }
string *cxcore_System_Uuid_ToString(const uuid *self) {
    static const char digits[] = "0123456789abcdef";
    unsigned char bytes[16];
    to_bytes(*self,bytes);
    char *text = cxcore_System_Memory_Alloc(36);
    unsigned position = 0;
    for (unsigned i = 0; i < 16; ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) text[position++] = '-';
        text[position++] = digits[bytes[i] >> 4]; text[position++] = digits[bytes[i] & 15];
    }
    string *result = cxcore_System_Memory_Alloc(sizeof(*result));
    cxcore_System_String___constructor__2(result,36,text);
    return result;
}

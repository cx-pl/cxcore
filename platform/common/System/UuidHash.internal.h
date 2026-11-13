#ifndef __CX_UUID_HASH_INTERNAL_H__
#define __CX_UUID_HASH_INTERNAL_H__

#include <stdint.h>
#include <string.h>

// MD5 and SHA-1 are used only for the standardized UUIDv3/v5 name mappings.
typedef struct {
    uint32_t state[5];
    uint64_t length;
    unsigned used;
    int sha1;
    unsigned char block[64];
} cx_uuid_hash;
static uint32_t uuid_rotate(uint32_t x, unsigned n) { return (x << n) | (x >> (32 - n)); }
static uint32_t uuid_read32(const unsigned char *p, int big) {
    return big ? ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]
        : ((uint32_t)p[3] << 24) | ((uint32_t)p[2] << 16) | ((uint32_t)p[1] << 8) | p[0];
}
static void uuid_hash_block(cx_uuid_hash *h) {
    uint32_t a = h->state[0], b = h->state[1], c = h->state[2], d = h->state[3];
    if (h->sha1) {
        uint32_t w[80], e = h->state[4];
        for (unsigned i = 0; i < 16; ++i) w[i] = uuid_read32(h->block + 4*i, 1);
        for (unsigned i = 16; i < 80; ++i) w[i] = uuid_rotate(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
        for (unsigned i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i < 20) { f = (b & c) | (~b & d); k = 0x5a827999; }
            else if (i < 40) { f = b ^ c ^ d; k = 0x6ed9eba1; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8f1bbcdc; }
            else { f = b ^ c ^ d; k = 0xca62c1d6; }
            uint32_t temp = uuid_rotate(a, 5) + f + e + k + w[i];
            e = d; d = c; c = uuid_rotate(b, 30); b = a; a = temp;
        }
        h->state[4] += e;
    } else {
        static const uint32_t k[64] = {
            0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
            0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,
            0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
            0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
            0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
            0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
            0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
            0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391
        };
        static const unsigned rotations[16] = {7,12,17,22, 5,9,14,20, 4,11,16,23, 6,10,15,21};
        uint32_t words[16];
        for (unsigned i = 0; i < 16; ++i) words[i] = uuid_read32(h->block + 4*i, 0);
        for (unsigned i = 0; i < 64; ++i) {
            uint32_t f; unsigned g;
            if (i < 16) { f = (b & c) | (~b & d); g = i; }
            else if (i < 32) { f = (d & b) | (~d & c); g = (5*i + 1) % 16; }
            else if (i < 48) { f = b ^ c ^ d; g = (3*i + 5) % 16; }
            else { f = c ^ (b | ~d); g = (7*i) % 16; }
            uint32_t temp = d;
            d = c; c = b;
            b += uuid_rotate(a + f + k[i] + words[g], rotations[(i / 16)*4 + i%4]);
            a = temp;
        }
    }
    h->state[0] += a; h->state[1] += b; h->state[2] += c; h->state[3] += d;
}
static void uuid_hash_update(cx_uuid_hash *h, const unsigned char *bytes, size_t length) {
    h->length += length;
    while (length) {
        size_t count = 64 - h->used;
        if (count > length) count = length;
        memcpy(h->block + h->used, bytes, count);
        h->used += (unsigned)count; bytes += count; length -= count;
        if (h->used == 64) { uuid_hash_block(h); h->used = 0; }
    }
}
static void uuid_hash_finish(cx_uuid_hash *h, unsigned char digest[20]) {
    uint64_t bits = h->length * 8;
    unsigned char pad[64] = {0x80}, encoded[8];
    for (unsigned i = 0; i < 8; ++i) encoded[i] = (unsigned char)(bits >> (h->sha1 ? 56-8*i : 8*i));
    uuid_hash_update(h, pad, h->used < 56 ? 56-h->used : 120-h->used);
    uuid_hash_update(h, encoded, 8);
    for (unsigned i = 0; i < (h->sha1 ? 20u : 16u); ++i)
        digest[i] = (unsigned char)(h->state[i/4] >> (h->sha1 ? 24-8*(i%4) : 8*(i%4)));
}

#endif // __CX_UUID_HASH_INTERNAL_H__

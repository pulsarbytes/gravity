/*
 * math.c - Definitions for math functions.
 */

#include <stdint.h>

#include <SDL2/SDL.h>

#include "../include/common.h"
#include "../include/structs.h"

#define ROTL64(x, y) ((x << y) | (x >> (64 - y)))

#define FMIX64(h)            \
    h ^= h >> 33;            \
    h *= 0xff51afd7ed558ccd; \
    h ^= h >> 33;            \
    h *= 0xc4ceb9fe1a85ec53; \
    h ^= h >> 33;

/*********************************************************************
 * uint64_t murmurhash3_64(const void *key, int len, uint32_t seed)
 *
 * Description:
 *   This function computes a 64-bit hash value for the given key using the
 *   MurmurHash3 algorithm.
 *
 * Parameters:
 *   key     - a pointer to the key data to be hashed
 *   len     - the length of the key data in bytes
 *   seed    - an initial seed value for the hash computation
 *
 * Returns:
 *   The 64-bit hash value for the given key.
 *
 * Note:
 *   The seed value can be used to initialize the hash computation,
 *   allowing multiple hash computations to be combined or to be
 *   repeated with the same results.
 *
 * References:
 *   MurmurHash3 algorithm: https://code.google.com/p/smhasher/wiki/MurmurHash3
 *   MurmurHash3 specification: https://sites.google.com/site/murmurhash/
 *   MurmurHash3 implementation: https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
 *
 * Example usage:
 *   uint64_t s = 2230559205683922026;
 *   uint64_t hash = murmurhash3_64(&s, sizeof(s), 0);
 *   uint64_t index = hash % 907;
 *   ...
 *********************************************************************/
// uint64_t murmurhash3_64(const void *key, int len, uint32_t seed)
// {
//     const uint8_t *data = (const uint8_t *)key;
//     const int nblocks = len / 8;
//     uint64_t h1 = seed;
//     uint64_t c1 = 0x87c37b91114253d5;
//     uint64_t c2 = 0x4cf5ad432745937f;
//     const uint64_t *blocks = (const uint64_t *)(data);
//     int i;
//     for (i = 0; i < nblocks; i++)
//     {
//         uint64_t k1 = blocks[i];
//         k1 *= c1;
//         k1 = ROTL64(k1, 31);
//         k1 *= c2;
//         h1 ^= k1;
//         h1 = ROTL64(h1, 27);
//         h1 += h1;
//         h1 = h1 * 5 + 0x52dce729;
//     }
//     const uint8_t *tail = (const uint8_t *)(data + nblocks * 8);
//     uint64_t k1 = 0;
//     switch (len & 7)
//     {
//     case 7:
//         k1 ^= (uint64_t)(tail[6]) << 48;
//     case 6:
//         k1 ^= (uint64_t)(tail[5]) << 40;
//     case 5:
//         k1 ^= (uint64_t)(tail[4]) << 32;
//     case 4:
//         k1 ^= (uint64_t)(tail[3]) << 24;
//     case 3:
//         k1 ^= (uint64_t)(tail[2]) << 16;
//     case 2:
//         k1 ^= (uint64_t)(tail[1]) << 8;
//     case 1:
//         k1 ^= (uint64_t)(tail[0]) << 0;
//         k1 *= c1;
//         k1 = ROTL64(k1, 31);
//         k1 *= c2;
//         h1 ^= k1;
//     };
//     h1 ^= len;
//     FMIX64(h1);
//     return h1;
// }

/**
 * Generates a hash value for a given double number `x`.
 *
 * @param x The double number to be hashed
 *
 * @return A 64-bit unsigned integer representing the hash value of `x`
 */
uint64_t double_hash(double x)
{
    uint64_t x_bits = *(uint64_t *)&x;
    uint64_t x_hash = x_bits ^ (x_bits >> 33);
    x_hash *= 0xffffffff;
    x_hash ^= x_hash >> 33;
    x_hash *= 0xffffffff;
    x_hash ^= x_hash >> 33;

    return x_hash;
}

/*
 * Hash function that maps two double numbers to a unique 64-bit integer.
 */
uint64_t pair_hash_order_sensitive(struct point_t position)
{
    uint64_t x_hash = double_hash(position.x);
    uint64_t y_hash = double_hash(position.y);
    uint64_t hash = x_hash ^ (y_hash + 0x9e3779b97f4a7c15ull + 1);

    return hash;
}

/*
 * Hash function that maps two double numbers to a unique 64-bit integer.
 */
uint64_t pair_hash_order_sensitive_2(struct point_t position)
{
    uint64_t x_hash = double_hash(position.x);
    uint64_t y_hash = double_hash(position.y);
    uint64_t hash = (x_hash + 0x9e3779b97f4a7c15ull) ^ y_hash;

    return hash;
}

/*
 * Hash function that maps a unique 64-bit integer to an int between 0 and modulo.
 * This int will be used as index in hash table.
 */
uint64_t unique_index(struct point_t position, int modulo, int entity_type)
{
    uint64_t index;

    if (entity_type == ENTITY_STAR)
        index = pair_hash_order_sensitive(position);
    else if (entity_type == ENTITY_GALAXY)
        index = pair_hash_order_sensitive_2(position);

    // uint64_t hash = murmurhash3_64(&index, sizeof(index), 0);

    return index % modulo;
}

/*
 * Check whether point p is in rectangular rect.
 */
int point_in_rect(struct point_t p, struct point_t rect[])
{
    int i, j;
    int sign = 0;
    int n = 4;

    for (i = 0, j = n - 1; i < n; j = i++)
    {
        double dx1 = p.x - rect[i].x;
        double dy1 = p.y - rect[i].y;
        double dx2 = rect[j].x - rect[i].x;
        double dy2 = rect[j].y - rect[i].y;
        double cross_prod = dx1 * dy2 - dy1 * dx2;

        if (i == 0)
            sign = cross_prod > 0 ? 1 : -1;
        else if ((cross_prod > 0) != (sign > 0))
            return 0; // point is outside the rectangle
    }
    return 1; // point is inside the rectangle
}
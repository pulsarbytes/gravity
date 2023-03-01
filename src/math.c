/*
 * math.c - Definitions for math functions.
 */

#include <stdint.h>

#include <SDL2/SDL.h>

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"

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
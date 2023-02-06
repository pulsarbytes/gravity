/*
 * physics.c - Definitions for physics functions.
 */

#include <stdlib.h>
#include <math.h>

#include <SDL2/SDL.h>

#include "../include/common.h"
#include "../include/structs.h"

extern struct vector_t velocity;

/*
 * Update velocity vector.
 */
void update_velocity(struct ship_t *ship)
{
    velocity.x = ship->position.x;
    velocity.y = ship->position.y;
    velocity.magnitude = sqrt((ship->vx * ship->vx) + (ship->vy * ship->vy));
    velocity.angle = atan2(ship->vy, ship->vx);
}

/*
 * Calculate orbital velocity for object orbiting
 * at distance <h> and angle <a> around object with radius <r>.
 *
 * Centripetal force:
 * Fc = m * v^2 / h
 * In this project, we assume that m = r^2
 *
 * Gravitational force:
 * Fg = G_CONSTANT * M * m / h^2
 * In this project, we assume that M = R^2, m = r^2
 *
 * Fc = Fg
 * r^2 * v^2 / h = G_CONSTANT * R^2 * r^2 / h^2
 * v = sqrt(G_CONSTANT * R^2 / h);
 */
void calc_orbital_velocity(float h, float a, float r, float *vx, float *vy)
{
    *vx = -COSMIC_CONSTANT * sqrt(G_CONSTANT * r * r / h) * sin(a * M_PI / 180); // negative for clockwise rotation
    *vy = COSMIC_CONSTANT * sqrt(G_CONSTANT * r * r / h) * cos(a * M_PI / 180);
}

/*
 * Transform a float to the nearest SECTION_SIZE point,
 * rounding up or down whichever is nearest.
 */
float find_nearest_section_axis(float n)
{
    float round_down = floorf(n / SECTION_SIZE) * SECTION_SIZE;
    float round_up = round_down + SECTION_SIZE;
    float diff_down = fabs(n - round_down);
    float diff_up = fabs(n - round_up);

    return diff_down < diff_up ? round_down : round_up;
}

/*
 * Hash function that takes a float number and maps it to a unique 64-bit integer.
 */

uint64_t float_hash(float x)
{
    uint32_t x_bits = *(uint32_t *)&x;
    uint64_t x_hash = x_bits * 0x9e3779b97f4a7c15ull;

    return x_hash;
}

/*
 * Hash function that maps two float numbers to a unique 64-bit integer.
 */
uint64_t float_pair_hash_order_sensitive(float x, float y)
{
    uint64_t x_hash = float_hash(x);
    uint64_t y_hash = float_hash(y);

    return x_hash + 0x9e3779b97f4a7c15 * y_hash;
}
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
 * at distance <h> and angle <a> degrees around object with radius <r>.
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
 * Transform a double to the nearest SECTION_SIZE point,
 * rounding up or down whichever is nearest.
 */
double find_nearest_section_axis(double n)
{
    double round_down = floorf(n / SECTION_SIZE) * SECTION_SIZE;
    double round_up = round_down + SECTION_SIZE;
    double diff_down = fabs(n - round_down);
    double diff_up = fabs(n - round_up);

    return diff_down < diff_up ? round_down : round_up;
}

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
    uint64_t x_hash = x_bits * 0x9e3779b97f4a7c15ull;

    return x_hash;
}

/*
 * Hash function that maps two double numbers to a unique 64-bit integer.
 */
uint64_t pair_hash_order_sensitive(struct position_t position)
{
    uint64_t x_hash = double_hash(position.x);
    uint64_t y_hash = double_hash(position.y);

    return x_hash + 0x9e3779b97f4a7c15 * y_hash;
}

/**
 * Calculates the density of a cloud pattern at a given point
 *
 * @x: x coordinate of the point
 * @y: y coordinate of the point
 * @densityFactor: scaling factor to adjust the overall density of the pattern
 *
 * Returns a value between 0 and 1 that represents the density of the cloud pattern at the given point.
 */
double cloudDensity(double x, double y, double densityFactor)
{
    double distanceFromCenter = sqrt((x - 100 / 2) * (x - 100 / 2) + (y - 100 / 2) * (y - 100 / 2));
    double falloff = 1.0 - distanceFromCenter / (100 / 2);
    falloff = pow(falloff, 4);
    return falloff * densityFactor;
}

/*
 * physics.c - Definitions for physics functions.
 */

#include <stdlib.h>
#include <math.h>

#include <SDL2/SDL.h>

#include "../include/common.h"
#include "../include/structs.h"

/*
 * Update velocity vector.
 */
void update_velocity(struct vector_t *velocity, struct ship_t *ship)
{
    velocity->x = ship->position.x;
    velocity->y = ship->position.y;
    velocity->magnitude = sqrt((ship->vx * ship->vx) + (ship->vy * ship->vy));
    velocity->angle = atan2(ship->vy, ship->vx);
}

/*
 * Calculate orbital velocity for object orbiting
 * at <distance> and <angle> degrees around object with <radius>.
 *
 * Centripetal force:
 * Fc = m * v^2 / distance
 * In this project, we assume that m = radius^2
 *
 * Gravitational force:
 * Fg = G_CONSTANT * M * m / distance^2
 * In this project, we assume that M = R^2, m = radius^2
 *
 * Fc = Fg
 * radius^2 * v^2 / distance = G_CONSTANT * R^2 * radius^2 / distance^2
 * v = sqrt(G_CONSTANT * R^2 / distance);
 */
void calc_orbital_velocity(float distance, float angle, float radius, float *vx, float *vy)
{
    *vx = -COSMIC_CONSTANT * sqrt(G_CONSTANT * radius * radius / distance) * sin(angle * M_PI / 180); // negative for clockwise rotation
    *vy = COSMIC_CONSTANT * sqrt(G_CONSTANT * radius * radius / distance) * cos(angle * M_PI / 180);
}

/*
 * Transform a double to the nearest section point,
 * rounding up or down whichever is nearest.
 */
double find_nearest_section_axis(double offset, int size)
{
    double round_down = floorf(offset / size) * size;
    double round_up = round_down + size;
    double diff_down = fabs(offset - round_down);
    double diff_up = fabs(offset - round_up);

    return diff_down < diff_up ? round_down : round_up;
}
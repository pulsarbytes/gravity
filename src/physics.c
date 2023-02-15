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
 * Transform a double to the nearest section point,
 * rounding up or down whichever is nearest.
 */
double find_nearest_section_axis(double n, int size)
{
    double round_down = floorf(n / size) * size;
    double round_up = round_down + size;
    double diff_down = fabs(n - round_down);
    double diff_up = fabs(n - round_up);

    return diff_down < diff_up ? round_down : round_up;
}
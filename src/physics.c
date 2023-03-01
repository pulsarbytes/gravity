/*
 * physics.c - Definitions for physics functions.
 */

#include <math.h>

#include <SDL2/SDL.h>

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"

// Function prototypes
void update_velocity(struct vector_t *velocity, struct ship_t *ship);
void calc_orbital_velocity(float distance, float angle, float radius, float *vx, float *vy);
void apply_gravity_to_ship(GameState *game_state, int thrust, NavigationState *nav_state, struct planet_t *planet, struct ship_t *ship, const struct camera_t *camera);

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
 * Apply planet gravity to ship.
 */
void apply_gravity_to_ship(GameState *game_state, int thrust, NavigationState *nav_state, struct planet_t *planet, struct ship_t *ship, const struct camera_t *camera)
{
    double delta_x = planet->position.x - ship->position.x;
    double delta_y = planet->position.y - ship->position.y;
    double distance = sqrt(delta_x * delta_x + delta_y * delta_y);
    float g_planet = 0;
    int is_star = planet->level == LEVEL_STAR;
    int collision_point = planet->radius;

    // Detect planet collision
    if (COLLISIONS_ON && distance <= collision_point + ship->radius)
    {
        game_state->landing_stage = STAGE_0; // This changes on next iteration (next planet). To-do: Must also link it to specific planet.
        g_planet = 0;

        if (is_star)
        {
            ship->vx = 0.0;
            ship->vy = 0.0;
        }
        else
        {
            ship->vx = planet->vx;
            ship->vy = planet->vy;
            ship->vx += planet->parent->vx;
            ship->vy += planet->parent->vy;
        }

        // Find landing angle
        if (ship->position.y == planet->position.y)
        {
            if (ship->position.x > planet->position.x)
            {
                ship->angle = 90;
                ship->position.x = planet->position.x + collision_point + ship->radius; // Fix ship position on collision surface
            }
            else
            {
                ship->angle = 270;
                ship->position.x = planet->position.x - collision_point - ship->radius; // Fix ship position on collision surface
            }
        }
        else if (ship->position.x == planet->position.x)
        {
            if (ship->position.y > planet->position.y)
            {
                ship->angle = 180;
                ship->position.y = planet->position.y + collision_point + ship->radius; // Fix ship position on collision surface
            }
            else
            {
                ship->angle = 0;
                ship->position.y = planet->position.y - collision_point - ship->radius; // Fix ship position on collision surface
            }
        }
        else
        {
            // 2nd quadrant
            if (ship->position.y > planet->position.y && ship->position.x > planet->position.x)
            {
                ship->angle = (asin(abs((int)(planet->position.x - ship->position.x)) / distance) * 180 / M_PI);
                ship->angle = 180 - ship->angle;
            }
            // 3rd quadrant
            else if (ship->position.y > planet->position.y && ship->position.x < planet->position.x)
            {
                ship->angle = (asin(abs((int)(planet->position.x - ship->position.x)) / distance) * 180 / M_PI);
                ship->angle = 180 + ship->angle;
            }
            // 4th quadrant
            else if (ship->position.y < planet->position.y && ship->position.x < planet->position.x)
            {
                ship->angle = (asin(abs((int)(planet->position.x - ship->position.x)) / distance) * 180 / M_PI);
                ship->angle = 360 - ship->angle;
            }
            // 1st quadrant
            else
            {
                ship->angle = asin(abs((int)(planet->position.x - ship->position.x)) / distance) * 180 / M_PI;
            }

            ship->position.x = ((ship->position.x - planet->position.x) * (collision_point + ship->radius) / distance) + planet->position.x; // Fix ship position on collision surface
            ship->position.y = ((ship->position.y - planet->position.y) * (collision_point + ship->radius) / distance) + planet->position.y; // Fix ship position on collision surface
        }

        // Apply thrust
        if (thrust)
        {
            ship->vx -= G_LAUNCH * delta_x / distance;
            ship->vy -= G_LAUNCH * delta_y / distance;
        }
    }
    // Ship inside cutoff
    else if (distance < planet->cutoff)
    {
        game_state->landing_stage = STAGE_OFF;
        g_planet = G_CONSTANT * planet->radius * planet->radius / (distance * distance);

        ship->vx += g_planet * delta_x / distance;
        ship->vy += g_planet * delta_y / distance;
    }

    // Update velocity
    update_velocity(&nav_state->velocity, ship);

    // Enforce speed limit if within star cutoff
    if (is_star && distance < planet->cutoff)
    {
        game_state->speed_limit = BASE_SPEED_LIMIT * planet->class;

        if (nav_state->velocity.magnitude > game_state->speed_limit)
        {
            ship->vx = game_state->speed_limit * ship->vx / nav_state->velocity.magnitude;
            ship->vy = game_state->speed_limit * ship->vy / nav_state->velocity.magnitude;

            // Update velocity
            update_velocity(&nav_state->velocity, ship);
        }
    }
}
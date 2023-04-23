/*
 * physics.c
 */

#include <math.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"
#include "../include/physics.h"

/**
 * Apply gravity and handle collision with a celestial body to update the state of the ship.
 *
 * @param game_state A pointer to the current GameState object.
 * @param input_state A pointer to the current InputState object.
 * @param nav_state A pointer to the current NavigationState object.
 * @param body A pointer to the CelestialBody struct representing the celestial body that the ship is interacting with.
 * @param ship A pointer to the Ship struct representing the ship whose state is being updated.
 * @param star_class An integer representing the class of the star the ship is interacting with.
 *
 * @return void
 */
void phys_apply_gravity_to_ship(GameState *game_state, const InputState *input_state, NavigationState *nav_state, CelestialBody *body, Ship *ship, unsigned short star_class)
{
    double delta_x = body->position.x - ship->position.x;
    double delta_y = body->position.y - ship->position.y;
    double distance = sqrt(delta_x * delta_x + delta_y * delta_y);
    float g_body;
    int is_star = body->level == LEVEL_STAR;
    int collision_point = body->radius;

    // Detect body collision
    if (COLLISIONS_ON && distance <= collision_point + ship->radius)
    {
        game_state->landing_stage = STAGE_0; // This changes on next iteration (next body). To-do: Must also link it to specific body.
        g_body = 0;

        if (is_star)
        {
            ship->vx = 0.0;
            ship->vy = 0.0;
        }
        else
        {
            ship->vx = body->vx;
            ship->vy = body->vy;
            ship->vx += body->parent->vx;
            ship->vy += body->parent->vy;
        }

        // Find landing angle
        if (ship->position.y == body->position.y)
        {
            if (ship->position.x > body->position.x)
            {
                ship->angle = 90;
                ship->position.x = body->position.x + collision_point + ship->radius; // Fix ship position on collision surface
            }
            else
            {
                ship->angle = 270;
                ship->position.x = body->position.x - collision_point - ship->radius; // Fix ship position on collision surface
            }
        }
        else if (ship->position.x == body->position.x)
        {
            if (ship->position.y > body->position.y)
            {
                ship->angle = 180;
                ship->position.y = body->position.y + collision_point + ship->radius; // Fix ship position on collision surface
            }
            else
            {
                ship->angle = 0;
                ship->position.y = body->position.y - collision_point - ship->radius; // Fix ship position on collision surface
            }
        }
        else
        {
            // 2nd quadrant
            if (ship->position.y > body->position.y && ship->position.x > body->position.x)
            {
                ship->angle = (asin(abs((int)(body->position.x - ship->position.x)) / distance) * 180 / M_PI);
                ship->angle = 180 - ship->angle;
            }
            // 3rd quadrant
            else if (ship->position.y > body->position.y && ship->position.x < body->position.x)
            {
                ship->angle = (asin(abs((int)(body->position.x - ship->position.x)) / distance) * 180 / M_PI);
                ship->angle = 180 + ship->angle;
            }
            // 4th quadrant
            else if (ship->position.y < body->position.y && ship->position.x < body->position.x)
            {
                ship->angle = (asin(abs((int)(body->position.x - ship->position.x)) / distance) * 180 / M_PI);
                ship->angle = 360 - ship->angle;
            }
            // 1st quadrant
            else
            {
                ship->angle = asin(abs((int)(body->position.x - ship->position.x)) / distance) * 180 / M_PI;
            }

            ship->position.x = ((ship->position.x - body->position.x) * (collision_point + ship->radius) / distance) + body->position.x; // Fix ship position on collision surface
            ship->position.y = ((ship->position.y - body->position.y) * (collision_point + ship->radius) / distance) + body->position.y; // Fix ship position on collision surface
        }

        // Apply thrust
        if (input_state->thrust_on)
        {
            ship->vx -= G_LAUNCH * delta_x / distance;
            ship->vy -= G_LAUNCH * delta_y / distance;
        }
    }
    // Ship inside cutoff
    else if (distance < body->cutoff)
    {
        game_state->landing_stage = STAGE_OFF;
        g_body = G_CONSTANT * body->radius * body->radius / (distance * distance);

        ship->vx += g_body * delta_x / distance;
        ship->vy += g_body * delta_y / distance;

        // Enforce star speed limit
        if (!input_state->autopilot_on)
        {
            game_state->speed_limit = BASE_SPEED_LIMIT + (star_class - 1) * (GALAXY_SPEED_LIMIT - BASE_SPEED_LIMIT) / 6;

            if (nav_state->velocity.magnitude >= game_state->speed_limit)
            {
                ship->vx = game_state->speed_limit * ship->vx / nav_state->velocity.magnitude;
                ship->vy = game_state->speed_limit * ship->vy / nav_state->velocity.magnitude;

                // Update velocity
                phys_update_velocity(&nav_state->velocity, ship);
            }
        }
    }
}

/**
 * Calculates the orbital velocity for an object orbiting at a certain distance and angle around an object with a given radius.
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
 *
 * @param distance The distance between the orbiting object and the object being orbited (in meters).
 * @param angle The angle (in degrees) at which the orbiting object is orbiting around the object.
 * @param radius The radius of the object being orbited (in meters).
 * @param vx A pointer to the horizontal component of the orbital velocity (in meters per second).
 * @param vy A pointer to the vertical component of the orbital velocity (in meters per second).
 *
 * @return void
 */
void phys_calculate_orbital_velocity(float distance, float angle, float radius, float *vx, float *vy)
{
    *vx = -COSMIC_CONSTANT * sqrt(G_CONSTANT * radius * radius / distance) * sin(angle * M_PI / 180); // negative for clockwise rotation
    *vy = COSMIC_CONSTANT * sqrt(G_CONSTANT * radius * radius / distance) * cos(angle * M_PI / 180);
}

/**
 * Updates the given velocity vector based on the given ship's position and velocity.
 *
 * @param velocity A pointer to the vector to be updated.
 * @param ship A pointer to the ship containing the position and velocity information.
 *
 * @return void
 */
void phys_update_velocity(Vector *velocity, const Ship *ship)
{
    velocity->x = ship->position.x;
    velocity->y = ship->position.y;
    velocity->magnitude = sqrt((ship->vx * ship->vx) + (ship->vy * ship->vy));
    velocity->angle = atan2(ship->vy, ship->vx);
}
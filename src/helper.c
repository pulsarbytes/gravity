/*
 * helper.c - Definitions for helper functions
 */

#include <stdlib.h>
#include <math.h>

#include <SDL2/SDL.h>

#include "../include/common.h"
#include "../include/structs.h"

/*
 * Clean up planets (recursive)
 */
static void cleanup_planets(struct planet_t *planet)
{
    int planets_size = sizeof(planet->planets) / sizeof(planet->planets[0]);
    int i = 0;

    for (i = 0; i < planets_size && planet->planets[i] != NULL; i++)
    {
        cleanup_planets(planet->planets[i]);
    }

    SDL_DestroyTexture(planet->texture);

    if (planet != NULL)
    {
        free(planet);
        planet = NULL;
    }
}

/*
 * Clean up resources
 */
void cleanup_resources(struct planet_t *planet, struct ship_t *ship)
{
    // Cleanup planets
    if (planet)
        cleanup_planets(planet);

    // Cleanup ship
    SDL_DestroyTexture(ship->projection->texture);
    SDL_DestroyTexture(ship->texture);
}

/*
 * Calculate orbital velocity for object orbiting
 * at height around object with radius.
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
float orbital_velocity(float height, int radius)
{
    float v;

    v = COSMIC_CONSTANT * sqrt(G_CONSTANT * radius * radius / height);

    return v;
}

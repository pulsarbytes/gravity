/*
 * helper.c - Definitions for helper functions.
 */

#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"
#include "../lib/pcg-c-basic-0.9/pcg_basic.h"

// Function prototypes
void cleanup_resources(GameState *game_state, NavigationState *nav_state, struct ship_t *ship);

// External function prototypes
void cleanup_galaxies(struct galaxy_entry *galaxies[]);
void cleanup_stars(struct star_entry *stars[]);

/*
 * Clean up resources.
 */
void cleanup_resources(GameState *game_state, NavigationState *nav_state, struct ship_t *ship)
{
    // Clean up galaxies
    cleanup_galaxies(nav_state->galaxies);

    // Clean up stars
    cleanup_stars(nav_state->stars);

    // Clean up ship
    SDL_DestroyTexture(ship->projection->texture);
    ship->projection->texture = NULL;
    SDL_DestroyTexture(ship->texture);
    ship->texture = NULL;

    // Clean up galaxies
    free(nav_state->current_galaxy);
    free(nav_state->buffer_galaxy);
    free(nav_state->previous_galaxy);

    // Clean up menu texttures
    for (int i = 0; i < MENU_BUTTON_COUNT; i++)
    {
        SDL_DestroyTexture(game_state->menu[i].texture);
    }

    // Clean up logo texture
    SDL_DestroyTexture(game_state->logo.texture);
}

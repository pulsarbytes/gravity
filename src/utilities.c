/*
 * utilities.c - Definitions for utilities functions.
 */

#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"
#include "../include/utilities.h"

/*
 * Clean up resources.
 */
void cleanup_resources(GameState *game_state, NavigationState *nav_state, Ship *ship)
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

/*
 * utilities.c
 */

#include <stdlib.h>

#include <SDL2/SDL.h>

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"
#include "../include/utilities.h"

/**
 * Frees the resources used by the game state, navigation state, bstars, and ship.
 *
 * @param game_state The current game state.
 * @param nav_state The current navigation state.
 * @param bstars The bstar array.
 * @param ship The current ship.
 *
 * @return void
 */
void utils_cleanup_resources(GameState *game_state, NavigationState *nav_state, Bstar *bstars, Ship *ship)
{
    // Clean up galaxies
    galaxies_clear_table(nav_state->galaxies);

    // Clean up stars
    stars_clear_table(nav_state->stars);

    // Clean up ship
    SDL_DestroyTexture(ship->projection->texture);
    ship->projection->texture = NULL;
    SDL_DestroyTexture(ship->texture);
    ship->texture = NULL;

    // Free memory
    free(nav_state->current_galaxy);
    free(nav_state->buffer_galaxy);
    free(nav_state->previous_galaxy);

    // Clean up menu textures
    for (int i = 0; i < MENU_BUTTON_COUNT; i++)
    {
        SDL_DestroyTexture(game_state->menu[i].texture);
    }

    // Clean up logo texture
    SDL_DestroyTexture(game_state->logo.texture);

    // Clean up bstars
    free(bstars);
}

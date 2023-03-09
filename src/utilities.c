/*
 * utilities.c
 */

#include <stdlib.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"
#include "../include/utilities.h"

/**
 * Adds thousand separators to an integer value and stores the result in a string.
 * If the resulting string is longer than `result_size` - 1 characters (to accommodate the null
 * terminator), the string will be truncated to fit within the buffer.
 *
 * @param num The integer value to format.
 * @param result A character array to store the formatted result.
 * @param result_size The size of the result buffer, including space for the null terminator.
 *
 * @return void
 */
void utils_add_thousand_separators(int num, char *result, size_t result_size)
{
    char num_str[32];
    snprintf(num_str, sizeof(num_str), "%d", num);

    int num_digits = snprintf(NULL, 0, "%d", num);
    int result_index = 0;
    int start_index = 0;

    // Handle negative sign, if present
    if (num_str[0] == '-')
    {
        if (result_index < result_size - 1)
            result[result_index++] = '-';

        start_index = 1;
    }

    // Add thousand separators
    for (int i = start_index; i < num_digits; i++)
    {
        if ((i - start_index) > 0 && (num_digits - i) % 3 == 0)
        {
            if (result_index < result_size - 1)
                result[result_index++] = ',';
        }
        if (result_index < result_size - 1)
            result[result_index++] = num_str[i];
    }

    result[result_index] = '\0';
}

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

    // Free allocated memory
    free(nav_state->current_galaxy);
    free(nav_state->buffer_galaxy);
    free(nav_state->previous_galaxy);
    free(nav_state->current_star);

    // Clean up menu textures
    for (int i = 0; i < MENU_BUTTON_COUNT; i++)
    {
        SDL_DestroyTexture(game_state->menu[i].text_texture);
    }

    // Clean up logo texture
    SDL_DestroyTexture(game_state->logo.text_texture);

    // Clean up bstars
    free(bstars);
}

/*
 * console.c - Function definitions for game console.
 */

#include <stdio.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"

// External variable definitions
extern TTF_Font *fonts[];
extern SDL_Renderer *renderer;
extern SDL_Color colors[];

void log_game_console(struct game_console_entry entries[], int index, double value);

/*
 * Calculate and log FPS to game console.
 */
void log_fps(struct game_console_entry entries[], unsigned int time_diff)
{
    static unsigned total_time = 0, current_fps = 0;

    total_time += time_diff;
    current_fps++;

    if (total_time >= 1000)
    {
        log_game_console(entries, FPS_INDEX, (float)current_fps);

        total_time = 0;
        current_fps = 0;
    }
}

/*
 * Save data to game_console_entries array.
 */
void log_game_console(struct game_console_entry entries[], int index, double value)
{
    char text[16];
    float rounded_value;

    if (index == SCALE_INDEX)
        rounded_value = value;
    else
        rounded_value = (int)value;

    sprintf(text, "%f", rounded_value);
    strcpy(entries[index].value, text);
}

/*
 * Update game console.
 */
void update_game_console(GameState *game_state, NavigationState nav_state)
{
    struct point_t position;

    if (game_state->state == NAVIGATE)
    {
        position.x = nav_state.navigate_offset.x;
        position.y = nav_state.navigate_offset.y;
    }
    else if (game_state->state == MAP)
    {
        position.x = nav_state.map_offset.x;
        position.y = nav_state.map_offset.y;
    }
    else if (game_state->state == UNIVERSE)
    {
        position.x = nav_state.universe_offset.x;
        position.y = nav_state.universe_offset.y;
    }

    log_game_console(game_state->game_console_entries, X_INDEX, position.x);
    log_game_console(game_state->game_console_entries, Y_INDEX, position.y);
    log_game_console(game_state->game_console_entries, SCALE_INDEX, game_state->game_scale);

    for (int i = 0; i < LOG_COUNT; i++)
    {
        game_state->game_console_entries[i].surface = TTF_RenderText_Solid(fonts[FONT_SIZE_14], game_state->game_console_entries[i].value, colors[COLOR_WHITE_255]);
        game_state->game_console_entries[i].texture = SDL_CreateTextureFromSurface(renderer, game_state->game_console_entries[i].surface);
        SDL_FreeSurface(game_state->game_console_entries[i].surface);
        game_state->game_console_entries[i].rect.x = 120;
        game_state->game_console_entries[i].rect.y = (i + 1) * 20;
        game_state->game_console_entries[i].rect.w = 100;
        game_state->game_console_entries[i].rect.h = 15;
        SDL_RenderCopy(renderer, game_state->game_console_entries[i].texture, NULL, &game_state->game_console_entries[i].rect);
        SDL_DestroyTexture(game_state->game_console_entries[i].texture);
        game_state->game_console_entries[i].texture = NULL;
    }
}
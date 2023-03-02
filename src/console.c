/*
 * console.c - Function definitions for console.
 */

#include <stdio.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"
#include "../include/console.h"

// External variable definitions
extern TTF_Font *fonts[];
extern SDL_Renderer *renderer;
extern SDL_Color colors[];

/*
 * Save data to console_entries array.
 */
void log_console(ConsoleEntry entries[], int index, double value)
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
 * Calculate and log FPS to console.
 */
void log_fps(ConsoleEntry entries[], unsigned int time_diff)
{
    static unsigned total_time = 0, current_fps = 0;

    total_time += time_diff;
    current_fps++;

    if (total_time >= 1000)
    {
        log_console(entries, FPS_INDEX, (float)current_fps);

        total_time = 0;
        current_fps = 0;
    }
}

/*
 * Update console.
 */
void update_console(GameState *game_state, const NavigationState *nav_state)
{
    Point position;

    if (game_state->state == NAVIGATE)
    {
        position.x = nav_state->navigate_offset.x;
        position.y = nav_state->navigate_offset.y;
    }
    else if (game_state->state == MAP)
    {
        position.x = nav_state->map_offset.x;
        position.y = nav_state->map_offset.y;
    }
    else if (game_state->state == UNIVERSE)
    {
        position.x = nav_state->universe_offset.x;
        position.y = nav_state->universe_offset.y;
    }

    log_console(game_state->console_entries, X_INDEX, position.x);
    log_console(game_state->console_entries, Y_INDEX, position.y);
    log_console(game_state->console_entries, SCALE_INDEX, game_state->game_scale);

    for (int i = 0; i < LOG_COUNT; i++)
    {
        game_state->console_entries[i].surface = TTF_RenderText_Solid(fonts[FONT_SIZE_14], game_state->console_entries[i].value, colors[COLOR_WHITE_255]);
        game_state->console_entries[i].texture = SDL_CreateTextureFromSurface(renderer, game_state->console_entries[i].surface);
        SDL_FreeSurface(game_state->console_entries[i].surface);
        game_state->console_entries[i].rect.x = 120;
        game_state->console_entries[i].rect.y = (i + 1) * 20;
        game_state->console_entries[i].rect.w = 100;
        game_state->console_entries[i].rect.h = 15;
        SDL_RenderCopy(renderer, game_state->console_entries[i].texture, NULL, &game_state->console_entries[i].rect);
        SDL_DestroyTexture(game_state->console_entries[i].texture);
        game_state->console_entries[i].texture = NULL;
    }
}
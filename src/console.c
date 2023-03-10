/*
 * console.c
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

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

/**
 * Log position to the console based on the current navigation state.
 *
 * @param game_state A pointer to the current game state.
 * @param nav_state The current navigation state containing the position offset.
 *
 * @return void
 */
void console_log_position(GameState *game_state, NavigationState nav_state)
{
    Point position;

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

    console_update_entry(game_state->console_entries, CONSOLE_X, position.x);
    console_update_entry(game_state->console_entries, CONSOLE_Y, position.y);
}

/**
 * Log the current frames per second (FPS) to the console.
 *
 * @param entries An array of console entries to update.
 * @param fps A pointer to the current FPS value.
 * @param last_time A pointer to the last time the FPS was updated.
 * @param frame_count A pointer to the current frame count.
 *
 * @return void
 */
void console_log_fps(ConsoleEntry entries[], unsigned int *fps, unsigned int *last_time, unsigned int *frame_count)
{
    unsigned int current_time = SDL_GetTicks();
    unsigned int time_diff = current_time - *last_time;

    // Only update FPS once per second
    if (time_diff >= 1000)
    {
        *fps = *frame_count;
        *frame_count = 0;
        *last_time = current_time;
    }
    else
        *frame_count += 1;

    console_update_entry(entries, CONSOLE_FPS, *fps);
}

/**
 * Render the console entries on the screen.
 *
 * @param entries An array of console entries to render.
 *
 * @return void
 */
void console_render(ConsoleEntry entries[], const Camera *camera)
{
    for (int i = 0; i < CONSOLE_COUNT; i++)
    {
        SDL_Surface *surface = TTF_RenderText_Solid(fonts[FONT_SIZE_14], entries[i].value, colors[COLOR_WHITE_255]);
        entries[i].text_texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        entries[i].texture_rect.x = 120;
        entries[i].texture_rect.y = camera->h - 50 - (i + 1) * 20; //(i + 1) * 20;
        entries[i].texture_rect.w = 100;
        entries[i].texture_rect.h = 15;
        SDL_RenderCopy(renderer, entries[i].text_texture, NULL, &entries[i].texture_rect);
        SDL_DestroyTexture(entries[i].text_texture);
        entries[i].text_texture = NULL;
    }
}

/**
 * Update a console entry with a new value.
 *
 * @param entries An array of `ConsoleEntry` structs representing the console entries to update.
 * @param index An integer representing the index of the entry to update.
 * @param value A double representing the new value to set for the entry.
 *
 * @return void
 */
void console_update_entry(ConsoleEntry entries[], int index, double value)
{
    char text[16];
    float rounded_value;

    if (index == CONSOLE_SCALE)
        rounded_value = value;
    else
        rounded_value = (int)value;

    sprintf(text, "%f", rounded_value);
    strcpy(entries[index].value, text);
}
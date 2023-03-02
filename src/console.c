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
 * Update entry in console_entries array.
 */
void console_update_entry(ConsoleEntry entries[], int index, double value)
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
 * Measure FPS.
 */
void console_measure_fps(unsigned int *fps, unsigned int *last_time, unsigned int *frame_count)
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
}

/*
 * Render console.
 */
void console_render(ConsoleEntry entries[])
{
    for (int i = 0; i < LOG_COUNT; i++)
    {
        entries[i].surface = TTF_RenderText_Solid(fonts[FONT_SIZE_14], entries[i].value, colors[COLOR_WHITE_255]);
        entries[i].texture = SDL_CreateTextureFromSurface(renderer, entries[i].surface);
        SDL_FreeSurface(entries[i].surface);
        entries[i].rect.x = 120;
        entries[i].rect.y = (i + 1) * 20;
        entries[i].rect.w = 100;
        entries[i].rect.h = 15;
        SDL_RenderCopy(renderer, entries[i].texture, NULL, &entries[i].rect);
        SDL_DestroyTexture(entries[i].texture);
        entries[i].texture = NULL;
    }
}
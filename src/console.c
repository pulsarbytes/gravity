/*
 * console.c - Function definitions for game console
 */

#include <stdio.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "../include/common.h"
#include "../include/structs.h"

extern struct game_console_entry game_console_entries[];
extern SDL_Renderer *renderer;
extern TTF_Font *font;
extern SDL_Color text_color;

void log_game_console(struct game_console_entry entries[], int index, float value);

/*
 * Calculate and log FPS to game console
 */
void log_fps(unsigned int time_diff)
{
    static unsigned total_time = 0, current_fps = 0;

    total_time += time_diff;
    current_fps++;

    if (total_time >= 1000)
    {
        log_game_console(game_console_entries, FPS_INDEX, (float)current_fps);

        total_time = 0;
        current_fps = 0;
    }
}

/*
 * Save data to game_console_entries array
 */
void log_game_console(struct game_console_entry entries[], int index, float value)
{
    char text[16];

    sprintf(text, "%f", value);
    strcpy(entries[index].value, text);
}

/*
 * Update game console
 */
void update_game_console(struct game_console_entry entries[])
{
    int i;

    for (i = 0; i < LOG_COUNT; i++)
    {
        entries[i].surface = TTF_RenderText_Solid(font, entries[i].value, text_color);
        entries[i].texture = SDL_CreateTextureFromSurface(renderer, entries[i].surface);
        SDL_FreeSurface(entries[i].surface);
        entries[i].rect.x = 120;
        entries[i].rect.y = (i + 1) * 20;
        entries[i].rect.w = 100;
        entries[i].rect.h = 15;
        SDL_RenderCopy(renderer, entries[i].texture, NULL, &entries[i].rect);
        SDL_DestroyTexture(entries[i].texture);
    }
}

/*
 * Destroy game console, free up resources
 */
void destroy_game_console(struct game_console_entry entries[])
{
    int i;

    for (i = 0; i < LOG_COUNT; i++)
    {
        SDL_FreeSurface(entries[i].surface);
        SDL_DestroyTexture(entries[i].texture);
    }
}

/*
 * Gravity - An infinite procedural 2d universe that models gravity and orbital motion.
 *
 * v1.4.4
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright (c) 2020 Yannis Maragos.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_timer.h>

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"

// Global variable definitions
TTF_Font *fonts[FONT_COUNT];
SDL_DisplayMode display_mode;
SDL_Renderer *renderer = NULL;
SDL_Color colors[COLOR_COUNT];

// External function prototypes
void console_draw_fps(unsigned int fps, const Camera *);
void console_measure_fps(GameState *, unsigned int *last_time, unsigned int *frame_count);
void controls_create_table(GameState *, const Camera *);
void controls_run_state(GameState *, InputState *, bool is_game_started, const NavigationState *, Bstar *bstars, Gstar *menustars, const Camera *);
void events_loop(GameState *, InputState *, GameEvents *, NavigationState *, const Camera *);
void events_set_cursor(GameState *, InputState *);
Ship game_create_ship(int radius, Point, long double scale);
void game_reset(GameState *, InputState *, GameEvents *, NavigationState *, Bstar *bstars, Ship *, Camera *, bool reset);
void game_run_map_state(GameState *, InputState *, GameEvents *, NavigationState *, Bstar *bstars, Ship *, Camera *);
void game_run_navigate_state(GameState *, InputState *, GameEvents *, NavigationState *, Bstar *bstars, Ship *, Camera *);
void game_run_universe_state(GameState *, InputState *, GameEvents *, NavigationState *, Ship *, Camera *);
void gfx_create_default_colors(void);
void menu_create(GameState *, NavigationState, Gstar *menustars);
void menu_run_state(GameState *, InputState *, bool is_game_started, const NavigationState *, Bstar *bstars, Gstar *menustars, Camera *);
void sdl_cleanup(SDL_Window *);
bool sdl_initialize(SDL_Window *);
bool sdl_ttf_load_fonts(SDL_Window *);
void utils_cleanup_resources(GameState *, InputState *, NavigationState *, Bstar *bstars, Ship *);

int main(int argc, char *argv[])
{
    // Check for valid GALAXY_SCALE
    if (GALAXY_SCALE > 100000 || GALAXY_SCALE < 1000)
    {
        fprintf(stderr, "Error: Invalid GALAXY_SCALE.\n");
        return 1;
    }

    // Initialize SDL
    SDL_Window *window = NULL;

    if (!sdl_initialize(window))
    {
        fprintf(stderr, "Error: could not initialize SDL.\n");
        return 1;
    }

    // Initialize SDL_ttf and load fonts
    if (!sdl_ttf_load_fonts(window))
    {
        fprintf(stderr, "Error: could not initialize SDL_ttf and load fonts.\n");
        sdl_cleanup(window);
        return 1;
    }

    gfx_create_default_colors();

    // Game variables
    GameState game_state;
    InputState input_state;
    GameEvents game_events;
    NavigationState nav_state;
    Camera camera;

    // Create ship
    Point zero_position = {.x = 0, .y = 0};
    Ship ship = game_create_ship(SHIP_RADIUS, zero_position, ZOOM_NAVIGATE);
    Ship ship_projection = game_create_ship(SHIP_PROJECTION_RADIUS, zero_position, ZOOM_NAVIGATE);
    ship.projection = &ship_projection;

    // Create background stars array
    int max_bstars = (int)(display_mode.w * display_mode.h * BSTARS_PER_SQUARE / BSTARS_SQUARE);
    Bstar *bstars = malloc(max_bstars * sizeof(Bstar));

    if (bstars == NULL)
    {
        fprintf(stderr, "Error: Could not allocate memory for bstars.\n");
        return 1;
    }

    // Initialize game
    game_reset(&game_state, &input_state, &game_events, &nav_state, bstars, &ship, &camera, false);

    // Create menu
    Gstar menustars[MAX_GSTARS];
    menu_create(&game_state, nav_state, menustars);

    // Create controls table
    controls_create_table(&game_state, &camera);

    // Set time keeping variables
    unsigned int start_time;
    unsigned int end_time;
    unsigned int last_time = SDL_GetTicks();
    unsigned int frame_count = 0;

    // Main loop
    while (game_state.state != QUIT)
    {
        start_time = SDL_GetTicks();

        // Process events
        events_loop(&game_state, &input_state, &game_events, &nav_state, &camera);

        // Set background color
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        // Clear the renderer
        SDL_RenderClear(renderer);

        switch (game_state.state)
        {
        case MENU:
            menu_run_state(&game_state, &input_state, game_events.is_game_started, &nav_state, bstars, menustars, &camera);
            break;
        case NAVIGATE:
            game_run_navigate_state(&game_state, &input_state, &game_events, &nav_state, bstars, &ship, &camera);
            break;
        case MAP:
            game_run_map_state(&game_state, &input_state, &game_events, &nav_state, bstars, &ship, &camera);
            break;
        case UNIVERSE:
            game_run_universe_state(&game_state, &input_state, &game_events, &nav_state, &ship, &camera);
            break;
        case NEW:
            game_reset(&game_state, &input_state, &game_events, &nav_state, bstars, &ship, &camera, true);
            break;
        case CONTROLS:
            controls_run_state(&game_state, &input_state, game_events.is_game_started, &nav_state, bstars, menustars, &camera);
            break;
        default:
            menu_run_state(&game_state, &input_state, game_events.is_game_started, &nav_state, bstars, menustars, &camera);
            break;
        }

        // Set mouse cursor
        events_set_cursor(&game_state, &input_state);

        // Draw FPS
        if (input_state.fps_on && FPS_ON)
        {
            console_measure_fps(&game_state, &last_time, &frame_count);
            console_draw_fps(game_state.fps, &camera);
        }

        // Switch buffers, display back buffer
        SDL_RenderPresent(renderer);

        // Get end time for this frame
        end_time = SDL_GetTicks();

        // Set frame rate
        if ((1000 / FPS) > end_time - start_time)
            SDL_Delay((1000 / FPS) - (end_time - start_time));
    }

    utils_cleanup_resources(&game_state, &input_state, &nav_state, bstars, &ship);

    // Close SDL
    sdl_cleanup(window);

    return 0;
}
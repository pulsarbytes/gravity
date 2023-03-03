/*
 * Gravity - An infinite procedural 2d universe that models gravity and orbital motion.
 *
 * v1.3.3
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
bool sdl_initialize(SDL_Window *);
bool sdl_ttf_load_fonts(SDL_Window *);
void gfx_create_default_colors(void);
void game_reset(GameState *, InputState *, GameEvents *, NavigationState *, Bstar *bstars, Ship *, Camera *, bool reset);
void menu_populate_menu_array(MenuButton menu[]);
void menu_create_logo(MenuButton *logo);
Ship game_create_ship(int radius, Point, long double scale);
Galaxy *galaxies_get_entry(GalaxyEntry *galaxies[], Point);
void stars_generate(GameState *, GameEvents *, NavigationState *, Bstar *bstars, Ship *, const Camera *);
void gfx_generate_menu_gstars(Galaxy *, Gstar menustars[]);
void events_loop(GameState *, InputState *, GameEvents *);
void menu_run_menu_state(GameState *, InputState *, int game_started, const NavigationState *, Bstar *bstars, Gstar menustars[], Camera *);
void game_run_navigate_state(GameState *, InputState *, GameEvents *, NavigationState *, Bstar *bstars, Ship *, Camera *);
void console_update_entry(ConsoleEntry entries[], int index, double value);
void console_measure_fps(unsigned int *fps, unsigned int *last_time, unsigned int *frame_count);
void game_run_map_state(GameState *, InputState *, GameEvents *, NavigationState *, Bstar *bstars, Ship *, Camera *);
void game_run_universe_state(GameState *, InputState *, GameEvents *, NavigationState *, Ship *, Camera *);
void console_render(ConsoleEntry entries[]);
void utils_cleanup_resources(GameState *, NavigationState *, Bstar *bstars, Ship *);
void sdl_cleanup(SDL_Window *);

int main(int argc, char *argv[])
{
    // Check for valid GALAXY_SCALE
    if (GALAXY_SCALE > 10000 || GALAXY_SCALE < 1000)
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
        return 1;
    }

    // Create colors
    gfx_create_default_colors();

    // Game variables
    GameState game_state;
    InputState input_state;
    GameEvents game_events;
    NavigationState nav_state;

    // Create ship
    Point zero_position = {.x = 0, .y = 0};
    Ship ship = game_create_ship(SHIP_RADIUS, zero_position, ZOOM_NAVIGATE);
    Ship ship_projection = game_create_ship(SHIP_PROJECTION_RADIUS, zero_position, ZOOM_NAVIGATE);
    ship.projection = &ship_projection;

    // Create camera
    Camera camera;

    // Create background stars
    int max_bstars = (int)(display_mode.w * display_mode.h * BSTARS_PER_SQUARE / BSTARS_SQUARE);
    Bstar *bstars = malloc(max_bstars * sizeof(Bstar));

    if (bstars == NULL)
    {
        fprintf(stderr, "Error: could not create bstars.\n");
        return 1;
    }

    // Initialize game module
    game_reset(&game_state, &input_state, &game_events, &nav_state, bstars, &ship, &camera, false);

    // Generate stars
    stars_generate(&game_state, &game_events, &nav_state, bstars, &ship, &camera);

    // Create menu
    menu_populate_menu_array(game_state.menu);

    // Create logo
    menu_create_logo(&game_state.logo);

    // Create galaxy for menu
    Point menu_galaxy_position = {.x = -140000, .y = -70000};
    Galaxy *menu_galaxy = galaxies_get_entry(nav_state.galaxies, menu_galaxy_position);
    Gstar menustars[MAX_GSTARS];
    gfx_generate_menu_gstars(menu_galaxy, menustars);

    // Set time keeping variables
    unsigned int start_time;
    unsigned int end_time;
    unsigned int fps = 0;
    unsigned int last_time = SDL_GetTicks();
    unsigned int frame_count = 0;

    // Main loop
    while (game_state.state != QUIT)
    {
        start_time = SDL_GetTicks();

        // Process events
        events_loop(&game_state, &input_state, &game_events);

        // Set background color
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        // Clear the renderer
        SDL_RenderClear(renderer);

        switch (game_state.state)
        {
        case MENU:
            menu_run_menu_state(&game_state, &input_state, game_events.game_started, &nav_state, bstars, menustars, &camera);
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
        default:
            menu_run_menu_state(&game_state, &input_state, game_events.game_started, &nav_state, bstars, menustars, &camera);
            break;
        }

        // Log position
        Point position;

        if (game_state.state == NAVIGATE)
        {
            position.x = nav_state.navigate_offset.x;
            position.y = nav_state.navigate_offset.y;
        }
        else if (game_state.state == MAP)
        {
            position.x = nav_state.map_offset.x;
            position.y = nav_state.map_offset.y;
        }
        else if (game_state.state == UNIVERSE)
        {
            position.x = nav_state.universe_offset.x;
            position.y = nav_state.universe_offset.y;
        }

        console_update_entry(game_state.console_entries, X_INDEX, position.x);
        console_update_entry(game_state.console_entries, Y_INDEX, position.y);

        // Log game scale
        console_update_entry(game_state.console_entries, SCALE_INDEX, game_state.game_scale);

        // Log FPS
        console_measure_fps(&fps, &last_time, &frame_count);
        console_update_entry(game_state.console_entries, FPS_INDEX, fps);

        // Render console
        if (input_state.console && CONSOLE_ON &&
            (game_state.state == NAVIGATE || game_state.state == MAP || game_state.state == UNIVERSE))
        {
            console_render(game_state.console_entries);
        }

        // Switch buffers, display back buffer
        SDL_RenderPresent(renderer);

        // Get end time for this frame
        end_time = SDL_GetTicks();

        // Set frame rate
        if ((1000 / FPS) > end_time - start_time)
            SDL_Delay((1000 / FPS) - (end_time - start_time));
    }

    // Cleanup resources
    utils_cleanup_resources(&game_state, &nav_state, bstars, &ship);

    // Close SDL
    sdl_cleanup(window);

    return 0;
}
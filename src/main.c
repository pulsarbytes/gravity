/*
 * Gravity - An infinite procedural 2d universe that models gravity and orbital motion.
 *
 * v1.3.2
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
int init_sdl(SDL_Window *);
void create_colors(void);
void create_menu(MenuButton menu[]);
void create_logo(MenuButton *logo);
Ship create_ship(int radius, Point, long double scale);
void galaxies_generate(GameEvents *, NavigationState *, Point);
Galaxy *galaxies_get_entry(GalaxyEntry *galaxies[], Point);
void create_bstars(NavigationState *, Bstar bstars[], const Camera *);
void stars_generate(GameState *, GameEvents *, NavigationState *, Bstar bstars[], Ship *, const Camera *);
void create_menu_galaxy_cloud(Galaxy *, Gstar menustars[]);
void events_loop(GameState *, InputState *, GameEvents *);
void onMenu(GameState *, InputState *, int game_started, const NavigationState *, Bstar bstars[], Gstar menustars[], Camera *);
void onNavigate(GameState *, InputState *, GameEvents *, NavigationState *, Bstar bstars[], Ship *, Camera *);
void console_update_entry(ConsoleEntry entries[], int index, double value);
void console_measure_fps(unsigned int *fps, unsigned int *last_time, unsigned int *frame_count);
void onMap(GameState *, InputState *, GameEvents *, NavigationState *, Bstar bstars[], Ship *, Camera *);
void onUniverse(GameState *, InputState *, GameEvents *, NavigationState *, Ship *, Camera *);
void reset_game(GameState *, InputState *, GameEvents *, NavigationState *, Ship *);
void console_render(ConsoleEntry entries[]);
void cleanup_resources(GameState *, NavigationState *, Ship *);
void close_sdl(SDL_Window *);

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

    if (!init_sdl(window))
    {
        fprintf(stderr, "Error: could not initialize SDL.\n");
        return 1;
    }

    // Create colors
    create_colors();

    // Initialize game state
    GameState game_state = {
        .state = MENU,
        .speed_limit = BASE_SPEED_LIMIT,
        .landing_stage = STAGE_OFF,
        .game_scale = ZOOM_NAVIGATE,
        .save_scale = OFF,
        .galaxy_region_size = GALAXY_REGION_SIZE};

    // Create menu
    create_menu(game_state.menu);

    // Create logo
    create_logo(&game_state.logo);

    // Initialize input state
    InputState input_state = {
        .left = OFF,
        .right = OFF,
        .up = OFF,
        .down = OFF,
        .thrust = OFF,
        .reverse = OFF,
        .camera_on = CAMERA_ON,
        .stop = OFF,
        .zoom_in = OFF,
        .zoom_out = OFF,
        .console = ON,
        .orbits_on = SHOW_ORBITS,
        .selected_button = 0};

    // Initialize game events
    GameEvents game_events = {
        .stars_start = ON,
        .galaxies_start = ON,
        .game_started = OFF,
        .map_enter = OFF,
        .map_exit = OFF,
        .map_center = OFF,
        .map_switch = OFF,
        .universe_enter = OFF,
        .universe_exit = OFF,
        .universe_center = OFF,
        .universe_switch = OFF,
        .exited_galaxy = OFF,
        .galaxy_found = OFF};

    // Initialize navigation state
    NavigationState nav_state;

    // Initialize stars hash table to NULL pointers
    for (int i = 0; i < MAX_STARS; i++)
    {
        nav_state.stars[i] = NULL;
    }

    // Initialize galaxies hash table to NULL pointers
    for (int i = 0; i < MAX_GALAXIES; i++)
    {
        nav_state.galaxies[i] = NULL;
    }

    // Allocate memory for galaxies
    nav_state.current_galaxy = (Galaxy *)malloc(sizeof(Galaxy));
    nav_state.buffer_galaxy = (Galaxy *)malloc(sizeof(Galaxy));
    nav_state.previous_galaxy = (Galaxy *)malloc(sizeof(Galaxy));

    // Galaxy coordinates
    // Retrieved from saved game or use default values if this is a new game
    nav_state.galaxy_offset.current_x = UNIVERSE_START_X;
    nav_state.galaxy_offset.current_y = UNIVERSE_START_Y;
    nav_state.galaxy_offset.buffer_x = UNIVERSE_START_X; // stores x of buffer galaxy
    nav_state.galaxy_offset.buffer_y = UNIVERSE_START_Y; // stores y of buffer galaxy

    // Initialize universe sections crossings
    nav_state.universe_cross_axis.x = nav_state.galaxy_offset.current_x;
    nav_state.universe_cross_axis.y = nav_state.galaxy_offset.current_y;

    // Navigation coordinates
    nav_state.navigate_offset.x = GALAXY_START_X;
    nav_state.navigate_offset.y = GALAXY_START_Y;

    // Map coordinates
    nav_state.map_offset.x = GALAXY_START_X;
    nav_state.map_offset.y = GALAXY_START_Y;

    // Universe coordinates
    nav_state.universe_offset.x = nav_state.galaxy_offset.current_x;
    nav_state.universe_offset.y = nav_state.galaxy_offset.current_y;

    // Create ship and ship projection
    Ship ship = create_ship(SHIP_RADIUS, nav_state.navigate_offset, game_state.game_scale);
    Point zero_position = {.x = 0, .y = 0};
    Ship ship_projection = create_ship(SHIP_PROJECTION_RADIUS, zero_position, game_state.game_scale);
    ship.projection = &ship_projection;

    // Initialize galaxy sections crossings
    nav_state.cross_axis.x = ship.position.x;
    nav_state.cross_axis.y = ship.position.y;

    // Generate galaxies
    Point initial_position = {
        .x = nav_state.galaxy_offset.current_x,
        .y = nav_state.galaxy_offset.current_y};
    galaxies_generate(&game_events, &nav_state, initial_position);

    // Get a copy of current galaxy from the hash table
    Point galaxy_position = {
        .x = nav_state.galaxy_offset.current_x,
        .y = nav_state.galaxy_offset.current_y};
    Galaxy *current_galaxy_copy = galaxies_get_entry(nav_state.galaxies, galaxy_position);

    // Copy current_galaxy_copy to current_galaxy
    memcpy(nav_state.current_galaxy, current_galaxy_copy, sizeof(Galaxy));

    // Copy current_galaxy to buffer_galaxy
    memcpy(nav_state.buffer_galaxy, nav_state.current_galaxy, sizeof(Galaxy));

    // Create camera, sync initial position with ship
    Camera camera = {
        .x = ship.position.x - (display_mode.w / 2),
        .y = ship.position.y - (display_mode.h / 2),
        .w = display_mode.w,
        .h = display_mode.h};

    // Create background stars
    int max_bstars = (int)(camera.w * camera.h * BSTARS_PER_SQUARE / BSTARS_SQUARE);
    Bstar bstars[max_bstars];

    for (int i = 0; i < max_bstars; i++)
    {
        bstars[i].final_star = 0;
    }

    create_bstars(&nav_state, bstars, &camera);

    // Generate stars
    stars_generate(&game_state, &game_events, &nav_state, bstars, &ship, &camera);

    // Create galaxy for menu
    Point menu_galaxy_position = {.x = -140000, .y = -70000};
    Galaxy *menu_galaxy = galaxies_get_entry(nav_state.galaxies, menu_galaxy_position);
    Gstar menustars[MAX_GSTARS];
    create_menu_galaxy_cloud(menu_galaxy, menustars);

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
            onMenu(&game_state, &input_state, game_events.game_started, &nav_state, bstars, menustars, &camera);
            break;
        case NAVIGATE:
            onNavigate(&game_state, &input_state, &game_events, &nav_state, bstars, &ship, &camera);
            console_update_entry(game_state.console_entries, V_INDEX, nav_state.velocity.magnitude);
            break;
        case MAP:
            onMap(&game_state, &input_state, &game_events, &nav_state, bstars, &ship, &camera);
            break;
        case UNIVERSE:
            onUniverse(&game_state, &input_state, &game_events, &nav_state, &ship, &camera);
            break;
        case NEW:
            reset_game(&game_state, &input_state, &game_events, &nav_state, &ship);
            stars_generate(&game_state, &game_events, &nav_state, bstars, &ship, &camera);

            for (int i = 0; i < max_bstars; i++)
            {
                bstars[i].final_star = 0;
            }

            create_bstars(&nav_state, bstars, &camera);
            break;
        default:
            onMenu(&game_state, &input_state, game_events.game_started, &nav_state, bstars, menustars, &camera);
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
    cleanup_resources(&game_state, &nav_state, &ship);

    // Close SDL
    close_sdl(window);

    return 0;
}

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
void create_menu(struct menu_button menu[]);
void create_logo(struct menu_button *logo);
struct ship_t create_ship(int radius, struct point_t position, long double scale);
void generate_galaxies(GameEvents *game_events, NavigationState *nav_state, struct point_t offset);
struct galaxy_t *get_galaxy(struct galaxy_entry *galaxies[], struct point_t);
void create_bstars(NavigationState *nav_state, struct bstar_t bstars[], const struct camera_t *camera);
void generate_stars(GameState *game_state, GameEvents *game_events, NavigationState *nav_state, struct bstar_t bstars[], struct ship_t *ship, const struct camera_t *camera);
void create_menu_galaxy_cloud(struct galaxy_t *, struct gstar_t menustars[]);
void poll_events(GameState *, InputState *, GameEvents *);
void onMenu(GameState *game_state, InputState *input_state, int game_started, NavigationState nav_state, struct bstar_t bstars[], struct gstar_t menustars[], struct camera_t *camera);
void onNavigate(GameState *game_state, InputState *input_state, GameEvents *game_events, NavigationState *nav_state, struct bstar_t bstars[], struct ship_t *ship, struct camera_t *camera);
void log_game_console(struct game_console_entry entries[], int index, double value);
void onMap(GameState *game_state, InputState *input_state, GameEvents *game_events, NavigationState *nav_state, struct bstar_t bstars[], struct ship_t *ship, struct camera_t *camera);
void onUniverse(GameState *game_state, InputState *input_state, GameEvents *game_events, NavigationState *nav_state, struct ship_t *ship, struct camera_t *camera);
void reset_game(GameState *game_state, InputState *input_state, GameEvents *game_events, NavigationState *nav_state, struct ship_t *ship);
void update_game_console(GameState *, NavigationState);
void log_fps(struct game_console_entry entries[], unsigned int time_diff);
void cleanup_resources(GameState *, NavigationState *, struct ship_t *);
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
    nav_state.current_galaxy = (struct galaxy_t *)malloc(sizeof(struct galaxy_t));
    nav_state.buffer_galaxy = (struct galaxy_t *)malloc(sizeof(struct galaxy_t));
    nav_state.previous_galaxy = (struct galaxy_t *)malloc(sizeof(struct galaxy_t));

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
    struct ship_t ship = create_ship(SHIP_RADIUS, nav_state.navigate_offset, game_state.game_scale);
    struct point_t zero_position = {.x = 0, .y = 0};
    struct ship_t ship_projection = create_ship(SHIP_PROJECTION_RADIUS, zero_position, game_state.game_scale);
    ship.projection = &ship_projection;

    // Initialize galaxy sections crossings
    nav_state.cross_axis.x = ship.position.x;
    nav_state.cross_axis.y = ship.position.y;

    // Generate galaxies
    struct point_t initial_position = {
        .x = nav_state.galaxy_offset.current_x,
        .y = nav_state.galaxy_offset.current_y};
    generate_galaxies(&game_events, &nav_state, initial_position);

    // Get a copy of current galaxy from the hash table
    struct point_t galaxy_position = {
        .x = nav_state.galaxy_offset.current_x,
        .y = nav_state.galaxy_offset.current_y};
    struct galaxy_t *current_galaxy_copy = get_galaxy(nav_state.galaxies, galaxy_position);

    // Copy current_galaxy_copy to current_galaxy
    memcpy(nav_state.current_galaxy, current_galaxy_copy, sizeof(struct galaxy_t));

    // Copy current_galaxy to buffer_galaxy
    memcpy(nav_state.buffer_galaxy, nav_state.current_galaxy, sizeof(struct galaxy_t));

    // Create camera, sync initial position with ship
    struct camera_t camera = {
        .x = ship.position.x - (display_mode.w / 2),
        .y = ship.position.y - (display_mode.h / 2),
        .w = display_mode.w,
        .h = display_mode.h};

    // Create background stars
    int max_bstars = (int)(camera.w * camera.h * BSTARS_PER_SQUARE / BSTARS_SQUARE);
    struct bstar_t bstars[max_bstars];

    for (int i = 0; i < max_bstars; i++)
    {
        bstars[i].final_star = 0;
    }

    create_bstars(&nav_state, bstars, &camera);

    // Generate stars
    generate_stars(&game_state, &game_events, &nav_state, bstars, &ship, &camera);

    // Create galaxy for menu
    struct point_t menu_galaxy_position = {.x = -140000, .y = -70000};
    struct galaxy_t *menu_galaxy = get_galaxy(nav_state.galaxies, menu_galaxy_position);
    struct gstar_t menustars[MAX_GSTARS];
    create_menu_galaxy_cloud(menu_galaxy, menustars);

    // Set time keeping variables
    unsigned int start_time;

    // Main loop
    while (game_state.state != QUIT)
    {
        start_time = SDL_GetTicks();

        // Process events
        poll_events(&game_state, &input_state, &game_events);

        // Set background color
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        // Clear the renderer
        SDL_RenderClear(renderer);

        switch (game_state.state)
        {
        case MENU:
            onMenu(&game_state, &input_state, game_events.game_started, nav_state, bstars, menustars, &camera);
            break;
        case NAVIGATE:
            onNavigate(&game_state, &input_state, &game_events, &nav_state, bstars, &ship, &camera);
            log_game_console(game_state.game_console_entries, V_INDEX, nav_state.velocity.magnitude);
            break;
        case MAP:
            onMap(&game_state, &input_state, &game_events, &nav_state, bstars, &ship, &camera);
            break;
        case UNIVERSE:
            onUniverse(&game_state, &input_state, &game_events, &nav_state, &ship, &camera);
            break;
        case NEW:
            reset_game(&game_state, &input_state, &game_events, &nav_state, &ship);
            generate_stars(&game_state, &game_events, &nav_state, bstars, &ship, &camera);

            for (int i = 0; i < max_bstars; i++)
            {
                bstars[i].final_star = 0;
            }

            create_bstars(&nav_state, bstars, &camera);
            break;
        default:
            onMenu(&game_state, &input_state, game_events.game_started, nav_state, bstars, menustars, &camera);
            break;
        }

        // Update game console
        if (input_state.console && CONSOLE_ON && (game_state.state == NAVIGATE || game_state.state == MAP || game_state.state == UNIVERSE))
        {
            update_game_console(&game_state, nav_state);
        }

        // Switch buffers, display back buffer
        SDL_RenderPresent(renderer);

        // Set FPS
        unsigned int end_time;

        if ((1000 / FPS) > ((end_time = SDL_GetTicks()) - start_time))
            SDL_Delay((1000 / FPS) - (end_time - start_time));

        // Log FPS
        unsigned int time_diff = end_time - start_time;
        log_fps(game_state.game_console_entries, time_diff);
    }

    // Cleanup resources
    cleanup_resources(&game_state, &nav_state, &ship);

    // Close SDL
    close_sdl(window);

    return 0;
}

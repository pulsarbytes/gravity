/*
 * Gravity - An infinite procedural 2d universe that models gravity and orbital motion.
 *
 * v1.3.1
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
#include <math.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_timer.h>
#include "../lib/pcg-c-basic-0.9/pcg_basic.h"

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"

// Global variable definitions
TTF_Font *fonts[FONT_COUNT];
SDL_DisplayMode display_mode;
SDL_Renderer *renderer = NULL;
SDL_Color colors[COLOR_COUNT];

// Function prototypes
void onMenu(GameState *, InputState *, int game_started, NavigationState, struct bstar_t bstars[], struct gstar_t menustars[], struct camera_t *);
void onNavigate(GameState *, InputState *, GameEvents *, NavigationState *, struct bstar_t bstars[], struct ship_t *, struct camera_t *);
void onMap(GameState *, InputState *, GameEvents *, NavigationState *, struct bstar_t bstars[], struct ship_t *, struct camera_t *);
void onUniverse(GameState *, InputState *, GameEvents *, NavigationState *, struct ship_t *, struct camera_t *);
int init_sdl(SDL_Window *);
void close_sdl(SDL_Window *);
void poll_events(GameState *, InputState *, GameEvents *);
void log_game_console(struct game_console_entry entries[], int index, double value);
void update_game_console(GameState *, NavigationState);
void log_fps(struct game_console_entry entries[], unsigned int time_diff);
void cleanup_resources(GameState *, NavigationState *, struct ship_t *);
void cleanup_stars(struct star_entry *stars[]);
void cleanup_galaxies(struct galaxy_entry *galaxies[]);
void calc_orbital_velocity(float distance, float angle, float radius, float *vx, float *vy);
void create_bstars(NavigationState *, struct bstar_t bstars[], const struct camera_t *);
void update_bstars(int state, int camera_on, NavigationState, struct bstar_t bstars[], const struct camera_t *, struct speed_t, double distance);
struct planet_t *create_star(NavigationState, struct point_t, int preview, long double scale);
uint64_t pair_hash_order_sensitive(struct point_t);
uint64_t pair_hash_order_sensitive_2(struct point_t);
void create_system(struct planet_t *, struct point_t, pcg32_random_t rng, long double scale);
struct ship_t create_ship(int radius, struct point_t, long double scale);
void update_system(GameState *, InputState, NavigationState *, struct planet_t *, struct ship_t *, const struct camera_t *);
void project_planet(GameState, NavigationState, struct planet_t *, const struct camera_t *);
void apply_gravity_to_ship(GameState *, int thrust, NavigationState *, struct planet_t *, struct ship_t *, const struct camera_t *);
void update_camera(struct camera_t *, struct point_t, long double scale);
void update_ship(GameState *, InputState, NavigationState, struct ship_t *, const struct camera_t *);
void project_ship(int state, InputState, NavigationState, struct ship_t *, const struct camera_t *, long double scale);
void project_galaxy(int state, NavigationState nav_state, struct galaxy_t *, const struct camera_t *, long double scale);
double find_nearest_section_axis(double offset, int size);
void generate_stars(GameState *, GameEvents *, NavigationState *, struct bstar_t bstars[], struct ship_t *, const struct camera_t *);
void put_star(struct star_entry *stars[], struct point_t, struct planet_t *);
int star_exists(struct star_entry *stars[], struct point_t);
void update_velocity(struct vector_t *velocity, struct ship_t *);
double nearest_star_distance(struct point_t, struct galaxy_t *, uint64_t initseq, int galaxy_density);
int get_star_class(float distance);
int get_planet_class(float width);
void zoom_star(struct planet_t *, long double scale);
void SDL_DrawCircle(SDL_Renderer *, const struct camera_t *, int xc, int yc, int radius, SDL_Color);
int in_camera(const struct camera_t *, double x, double y, float radius, long double scale);
void draw_screen_frame(struct camera_t *);
void generate_galaxies(GameEvents *, NavigationState *, struct point_t);
int galaxy_exists(struct galaxy_entry *galaxies[], struct point_t);
double nearest_galaxy_center_distance(struct point_t);
int get_galaxy_class(float distance);
struct galaxy_t *create_galaxy(struct point_t);
void put_galaxy(struct galaxy_entry *galaxies[], struct point_t, struct galaxy_t *);
void delete_galaxy(struct galaxy_entry *galaxies[], struct point_t);
void update_galaxy(NavigationState *, struct galaxy_t *, const struct camera_t *, int state, long double scale);
void SDL_DrawCircleApprox(SDL_Renderer *, const struct camera_t *, int x, int y, int r, SDL_Color);
void draw_section_lines(struct camera_t *, int section_size, SDL_Color, long double scale);
struct galaxy_t *get_galaxy(struct galaxy_entry *galaxies[], struct point_t);
struct galaxy_t *find_nearest_galaxy(NavigationState, struct point_t, int exclude);
double find_distance(double x1, double y1, double x2, double y2);
void create_galaxy_cloud(struct galaxy_t *, unsigned short);
void draw_galaxy_cloud(struct galaxy_t *, const struct camera_t *, int gstars_count, unsigned short high_definition, long double scale);
void update_gstars(struct galaxy_t *, struct point_t, const struct camera_t *, double distance, double limit);
void draw_speed_lines(float velocity, const struct camera_t *, struct speed_t);
void draw_speed_arc(struct ship_t *, const struct camera_t *, long double scale);
void delete_stars_outside_region(struct star_entry *stars[], double bx, double by, int region_size);
void generate_stars_preview(NavigationState *, const struct camera_t *, struct point_t *, int zoom_preview, long double scale);
int point_in_rect(struct point_t, struct point_t rect[]);
void create_menu(struct menu_button menu[]);
void create_logo(struct menu_button *);
void change_state(GameState *, GameEvents *, int new_state);
void update_menu(GameState *, int game_started);
void reset_game(GameState *, InputState *, GameEvents *, NavigationState *, struct ship_t *);
void create_menu_galaxy_cloud(struct galaxy_t *, struct gstar_t menustars[]);
void draw_menu_galaxy_cloud(const struct camera_t *, struct gstar_t menustars[]);
void create_colors(void);

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

void change_state(GameState *game_state, GameEvents *game_events, int new_state)
{
    game_state->state = new_state;

    if (game_state->state == NAVIGATE)
        game_events->game_started = ON;

    update_menu(game_state, game_events->game_started);
}

void reset_game(GameState *game_state, InputState *input_state, GameEvents *game_events, NavigationState *nav_state, struct ship_t *ship)
{
    // Game state
    game_state->landing_stage = STAGE_OFF;
    game_state->game_scale = ZOOM_NAVIGATE;
    game_state->save_scale = OFF;
    game_state->galaxy_region_size = GALAXY_REGION_SIZE;

    // Input state
    input_state->left = OFF;
    input_state->right = OFF;
    input_state->up = OFF;
    input_state->down = OFF;
    input_state->thrust = OFF;
    input_state->reverse = OFF;
    input_state->camera_on = CAMERA_ON;
    input_state->stop = OFF;
    input_state->zoom_in = OFF;
    input_state->zoom_out = OFF;
    input_state->console = ON;
    input_state->orbits_on = SHOW_ORBITS;
    input_state->selected_button = 0;

    // Game events
    game_events->stars_start = ON;
    game_events->galaxies_start = ON;
    game_events->game_started = ON;
    game_events->map_enter = OFF;
    game_events->map_exit = OFF;
    game_events->map_center = OFF;
    game_events->map_switch = OFF;
    game_events->universe_enter = OFF;
    game_events->universe_exit = OFF;
    game_events->universe_center = OFF;
    game_events->universe_switch = OFF;
    game_events->exited_galaxy = OFF;
    game_events->galaxy_found = OFF;

    // Navigation state & ship
    nav_state->galaxy_offset.current_x = UNIVERSE_START_X;
    nav_state->galaxy_offset.current_y = UNIVERSE_START_Y;
    nav_state->galaxy_offset.buffer_x = UNIVERSE_START_X;
    nav_state->galaxy_offset.buffer_y = UNIVERSE_START_Y;
    nav_state->universe_cross_axis.x = nav_state->galaxy_offset.current_x;
    nav_state->universe_cross_axis.y = nav_state->galaxy_offset.current_x;
    nav_state->navigate_offset.x = GALAXY_START_X;
    nav_state->navigate_offset.y = GALAXY_START_Y;
    nav_state->map_offset.x = GALAXY_START_X;
    nav_state->map_offset.y = GALAXY_START_Y;
    nav_state->universe_offset.x = nav_state->galaxy_offset.current_x;
    nav_state->universe_offset.y = nav_state->galaxy_offset.current_y;

    ship->position.x = GALAXY_START_X;
    ship->position.y = GALAXY_START_Y;
    ship->vx = 0;
    ship->vy = 0;
    ship->previous_position.x = 0;
    ship->previous_position.y = 0;
    ship->angle = 0;

    nav_state->cross_axis.x = ship->position.x;
    nav_state->cross_axis.y = ship->position.y;
    nav_state->velocity.magnitude = 0;
    nav_state->velocity.angle = 0;

    cleanup_stars(nav_state->stars);

    // Initialize stars hash table to NULL pointers
    for (int i = 0; i < MAX_STARS; i++)
    {
        nav_state->stars[i] = NULL;
    }

    cleanup_galaxies(nav_state->galaxies);

    // Initialize galaxies hash table to NULL pointers
    for (int i = 0; i < MAX_GALAXIES; i++)
    {
        nav_state->galaxies[i] = NULL;
    }

    // Generate galaxies
    struct point_t initial_position = {
        .x = nav_state->galaxy_offset.current_x,
        .y = nav_state->galaxy_offset.current_y};
    generate_galaxies(game_events, nav_state, initial_position);

    // Get a copy of current galaxy from the hash table
    struct point_t galaxy_position = {
        .x = nav_state->galaxy_offset.current_x,
        .y = nav_state->galaxy_offset.current_y};
    struct galaxy_t *current_galaxy_copy = get_galaxy(nav_state->galaxies, galaxy_position);

    // Copy current_galaxy_copy to current_galaxy
    memcpy(nav_state->current_galaxy, current_galaxy_copy, sizeof(struct galaxy_t));

    // Copy current_galaxy to buffer_galaxy
    memcpy(nav_state->buffer_galaxy, nav_state->current_galaxy, sizeof(struct galaxy_t));

    game_state->state = NAVIGATE;
}

void update_menu(GameState *game_state, int game_started)
{
    if (game_started)
    {
        // Update Start button
        game_state->menu[MENU_BUTTON_START].disabled = TRUE;
        SDL_Surface *start_surface = TTF_RenderText_Solid(fonts[FONT_SIZE_14], game_state->menu[MENU_BUTTON_START].text, colors[COLOR_WHITE_100]);
        SDL_DestroyTexture(game_state->menu[MENU_BUTTON_START].texture);
        SDL_Texture *start_texture = SDL_CreateTextureFromSurface(renderer, start_surface);
        game_state->menu[MENU_BUTTON_START].texture = start_texture;
        SDL_FreeSurface(start_surface);

        // Update Resume button
        game_state->menu[MENU_BUTTON_RESUME].disabled = FALSE;
        SDL_Surface *resume_surface = TTF_RenderText_Solid(fonts[FONT_SIZE_14], game_state->menu[MENU_BUTTON_RESUME].text, colors[COLOR_WHITE_255]);
        SDL_DestroyTexture(game_state->menu[MENU_BUTTON_RESUME].texture);
        SDL_Texture *resume_texture = SDL_CreateTextureFromSurface(renderer, resume_surface);
        game_state->menu[MENU_BUTTON_RESUME].texture = resume_texture;
        SDL_FreeSurface(resume_surface);

        // Update New button
        game_state->menu[MENU_BUTTON_NEW].disabled = FALSE;
        SDL_Surface *new_surface = TTF_RenderText_Solid(fonts[FONT_SIZE_14], game_state->menu[MENU_BUTTON_NEW].text, colors[COLOR_WHITE_255]);
        SDL_DestroyTexture(game_state->menu[MENU_BUTTON_NEW].texture);
        SDL_Texture *new_texture = SDL_CreateTextureFromSurface(renderer, new_surface);
        game_state->menu[MENU_BUTTON_NEW].texture = new_texture;
        SDL_FreeSurface(new_surface);
    }
}

void create_logo(struct menu_button *logo)
{
    strcpy(logo->text, "Gravity");

    // Set the position of the logo
    logo->rect.w = 300;
    logo->rect.h = 200;
    logo->rect.x = 50;
    logo->rect.y = 0;

    // Create a texture from the text
    SDL_Surface *logo_surface = TTF_RenderText_Solid(fonts[FONT_SIZE_36], logo->text, colors[COLOR_WHITE_255]);
    SDL_Texture *logo_texture = SDL_CreateTextureFromSurface(renderer, logo_surface);
    logo->texture = logo_texture;

    // Set the position of the text within the button
    logo->texture_rect.w = logo_surface->w;
    logo->texture_rect.h = logo_surface->h;
    logo->texture_rect.x = logo->rect.x + (logo->rect.w - logo->texture_rect.w) / 2;
    logo->texture_rect.y = logo->rect.y + (logo->rect.h - logo->texture_rect.h) / 2;

    SDL_FreeSurface(logo_surface);
}

void create_menu(struct menu_button menu[])
{
    // Start
    strcpy(menu[MENU_BUTTON_START].text, "Start");
    menu[MENU_BUTTON_START].state = NAVIGATE;
    menu[MENU_BUTTON_START].disabled = FALSE;

    // Resume
    strcpy(menu[MENU_BUTTON_RESUME].text, "Resume");
    menu[MENU_BUTTON_RESUME].state = RESUME;
    menu[MENU_BUTTON_RESUME].disabled = TRUE;

    // New
    strcpy(menu[MENU_BUTTON_NEW].text, "New Game");
    menu[MENU_BUTTON_NEW].state = NEW;
    menu[MENU_BUTTON_NEW].disabled = TRUE;

    // Exit
    strcpy(menu[MENU_BUTTON_EXIT].text, "Exit");
    menu[MENU_BUTTON_EXIT].state = QUIT;
    menu[MENU_BUTTON_EXIT].disabled = FALSE;

    for (int i = 0; i < MENU_BUTTON_COUNT; i++)
    {
        menu[i].rect.w = 300;
        menu[i].rect.h = 50;

        // Create a texture from the button text
        SDL_Surface *text_surface = TTF_RenderText_Solid(fonts[FONT_SIZE_14], menu[i].text, colors[COLOR_WHITE_255]);
        SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
        menu[i].texture = text_texture;
        menu[i].texture_rect.w = text_surface->w;
        menu[i].texture_rect.h = text_surface->h;

        SDL_FreeSurface(text_surface);
    }
}

void onMenu(GameState *game_state, InputState *input_state, int game_started, NavigationState nav_state, struct bstar_t bstars[], struct gstar_t menustars[], struct camera_t *camera)
{
    int num_buttons = 0;

    // Draw background stars
    struct speed_t speed = {.vx = 1000, .vy = 0};
    update_bstars(game_state->state, input_state->camera_on, nav_state, bstars, camera, speed, 0);

    // Draw logo
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderFillRect(renderer, &game_state->logo.rect);
    SDL_RenderCopy(renderer, game_state->logo.texture, NULL, &game_state->logo.texture_rect);

    // Draw menu
    for (int i = 0; i < MENU_BUTTON_COUNT; i++)
    {
        if (i == input_state->selected_button && game_state->menu[i].disabled)
        {
            do
            {
                input_state->selected_button = (input_state->selected_button + 1) % MENU_BUTTON_COUNT;
            } while (game_state->menu[input_state->selected_button].disabled);
        }

        if (game_started && i == MENU_BUTTON_START)
            continue;

        if (!game_started && i == MENU_BUTTON_RESUME)
            continue;

        if (!game_started && i == MENU_BUTTON_NEW)
            continue;

        if (i == input_state->selected_button)
        {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 40);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        }

        // Set the position of the button
        game_state->menu[i].rect.x = 50;
        game_state->menu[i].rect.y = 200 + 50 * num_buttons;

        SDL_RenderFillRect(renderer, &game_state->menu[i].rect);

        // Set the position of the text within the button
        game_state->menu[i].texture_rect.x = game_state->menu[i].rect.x + (game_state->menu[i].rect.w - game_state->menu[i].texture_rect.w) / 2;
        game_state->menu[i].texture_rect.y = game_state->menu[i].rect.y + (game_state->menu[i].rect.h - game_state->menu[i].texture_rect.h) / 2;

        // Render the text texture onto the button
        SDL_RenderCopy(renderer, game_state->menu[i].texture, NULL, &game_state->menu[i].texture_rect);

        num_buttons++;
    }

    // Draw galaxy
    draw_menu_galaxy_cloud(camera, menustars);

    // Draw speed lines
    struct speed_t lines_speed = {.vx = 100, .vy = 0};
    draw_speed_lines(1500, camera, lines_speed);
}

void onNavigate(GameState *game_state, InputState *input_state, GameEvents *game_events, NavigationState *nav_state, struct bstar_t bstars[], struct ship_t *ship, struct camera_t *camera)
{
    if (game_events->map_exit || game_events->universe_exit)
    {
        // Reset stars and galaxy to current position
        if (nav_state->current_galaxy->position.x != nav_state->buffer_galaxy->position.x ||
            nav_state->current_galaxy->position.y != nav_state->buffer_galaxy->position.y)
        {
            cleanup_stars(nav_state->stars);
            memcpy(nav_state->current_galaxy, nav_state->buffer_galaxy, sizeof(struct galaxy_t));

            // Reset galaxy_offset
            nav_state->galaxy_offset.current_x = nav_state->galaxy_offset.buffer_x;
            nav_state->galaxy_offset.current_y = nav_state->galaxy_offset.buffer_y;
        }

        // Generate new stars
        game_events->stars_start = ON;

        // Reset ship position
        ship->position.x = ship->previous_position.x;
        ship->position.y = ship->previous_position.y;

        // Reset region size
        game_state->galaxy_region_size = GALAXY_REGION_SIZE;

        // Delete stars that end up outside the region
        double bx = find_nearest_section_axis(ship->position.x, GALAXY_SECTION_SIZE);
        double by = find_nearest_section_axis(ship->position.y, GALAXY_SECTION_SIZE);

        delete_stars_outside_region(nav_state->stars, bx, by, game_state->galaxy_region_size);

        // Reset saved game_scale
        if (game_state->save_scale)
            game_state->game_scale = game_state->save_scale;
        else
            game_state->game_scale = ZOOM_NAVIGATE;

        game_state->save_scale = OFF;

        // Update camera
        if (input_state->camera_on)
            update_camera(camera, ship->position, game_state->game_scale);

        // Zoom in
        for (int s = 0; s < MAX_STARS; s++)
        {
            if (nav_state->stars[s] != NULL && nav_state->stars[s]->star != NULL)
                zoom_star(nav_state->stars[s]->star, game_state->game_scale);
        }
    }

    // Add small tolerance to account for floating-point precision errors
    const double epsilon = ZOOM_EPSILON;

    if (input_state->zoom_in)
    {
        if (game_state->game_scale + ZOOM_STEP <= ZOOM_MAX + epsilon)
        {
            // Reset scale
            game_state->game_scale += ZOOM_STEP;

            // Zoom in
            for (int s = 0; s < MAX_STARS; s++)
            {
                if (nav_state->stars[s] != NULL && nav_state->stars[s]->star != NULL)
                    zoom_star(nav_state->stars[s]->star, game_state->game_scale);
            }
        }

        input_state->zoom_in = OFF;
    }

    if (input_state->zoom_out)
    {
        if (game_state->game_scale - ZOOM_STEP >= ZOOM_NAVIGATE_MIN - epsilon)
        {
            // Reset scale
            game_state->game_scale -= ZOOM_STEP;

            // Zoom out
            for (int s = 0; s < MAX_STARS; s++)
            {
                if (nav_state->stars[s] != NULL && nav_state->stars[s]->star != NULL)
                    zoom_star(nav_state->stars[s]->star, game_state->game_scale);
            }
        }

        input_state->zoom_out = OFF;
    }

    // Generate stars
    if (input_state->camera_on)
        generate_stars(game_state, game_events, nav_state, bstars, ship, camera);

    // Update camera
    if (input_state->camera_on)
        update_camera(camera, nav_state->navigate_offset, game_state->game_scale);

    // Get distance from galaxy center
    double distance_current = find_distance(ship->position.x, ship->position.y, 0, 0);

    if (BSTARS_ON || GSTARS_ON || SPEED_LINES_ON)
    {
        struct speed_t speed = {.vx = ship->vx, .vy = ship->vy};

        // Draw galaxy cloud
        if (GSTARS_ON)
        {
            struct point_t ship_position_current = {.x = ship->position.x, .y = ship->position.y};
            static double limit_current;

            if (limit_current == 0.0)
                limit_current = 2 * nav_state->current_galaxy->radius * GALAXY_SCALE;

            if (game_events->galaxy_found)
            {
                limit_current = distance_current;
                game_events->galaxy_found = OFF;
            }

            update_gstars(nav_state->current_galaxy, ship_position_current, camera, distance_current, limit_current);

            if (game_events->exited_galaxy && nav_state->previous_galaxy != NULL && nav_state->previous_galaxy->initialized_hd)
            {
                // Convert ship position to universe coordinates
                struct point_t universe_position;
                universe_position.x = nav_state->current_galaxy->position.x + ship->position.x / GALAXY_SCALE;
                universe_position.y = nav_state->current_galaxy->position.y + ship->position.y / GALAXY_SCALE;

                // Convert universe coordinates to ship position relative to previous galaxy
                struct point_t ship_position_previous = {
                    .x = (universe_position.x - nav_state->previous_galaxy->position.x) * GALAXY_SCALE,
                    .y = (universe_position.y - nav_state->previous_galaxy->position.y) * GALAXY_SCALE};

                double distance_previous = find_distance(universe_position.x, universe_position.y, nav_state->previous_galaxy->position.x, nav_state->previous_galaxy->position.y);
                distance_previous *= GALAXY_SCALE;
                double limit_previous = 2 * nav_state->previous_galaxy->radius * GALAXY_SCALE;

                update_gstars(nav_state->previous_galaxy, ship_position_previous, camera, distance_previous, limit_previous);
            }
        }

        // Draw background stars
        if (BSTARS_ON)
            update_bstars(game_state->state, input_state->camera_on, *nav_state, bstars, camera, speed, distance_current);

        // Draw speed lines
        if (SPEED_LINES_ON && input_state->camera_on)
            draw_speed_lines(nav_state->velocity.magnitude, camera, speed);
    }

    // Draw speed tail
    if (nav_state->velocity.magnitude > GALAXY_SPEED_LIMIT)
        draw_speed_arc(ship, camera, game_state->game_scale);

    // Update system
    if ((!game_events->map_exit && !game_events->universe_exit && !input_state->zoom_in && !input_state->zoom_out) || game_state->game_scale > ZOOM_NAVIGATE_MIN)
    {
        for (int i = 0; i < MAX_STARS; i++)
        {
            if (nav_state->stars[i] != NULL)
            {
                // Each index can have many entries, loop through all of them
                struct star_entry *entry = nav_state->stars[i];

                while (entry != NULL)
                {
                    update_system(game_state, *input_state, nav_state, entry->star, ship, camera);
                    entry = entry->next;
                }
            }
        }
    }

    // Enforce speed limits
    if (distance_current < nav_state->current_galaxy->radius * GALAXY_SCALE)
    {
        if (nav_state->velocity.magnitude > GALAXY_SPEED_LIMIT)
        {
            ship->vx = GALAXY_SPEED_LIMIT * ship->vx / nav_state->velocity.magnitude;
            ship->vy = GALAXY_SPEED_LIMIT * ship->vy / nav_state->velocity.magnitude;
        }
    }
    else
    {
        if (nav_state->velocity.magnitude > UNIVERSE_SPEED_LIMIT)
        {
            ship->vx = UNIVERSE_SPEED_LIMIT * ship->vx / nav_state->velocity.magnitude;
            ship->vy = UNIVERSE_SPEED_LIMIT * ship->vy / nav_state->velocity.magnitude;
        }
    }

    // Update velocity
    update_velocity(&nav_state->velocity, ship);

    // Update ship
    update_ship(game_state, *input_state, *nav_state, ship, camera);

    // Update coordinates
    nav_state->navigate_offset.x = ship->position.x;
    nav_state->navigate_offset.y = ship->position.y;

    // Check for nearest galaxy, excluding current galaxy
    if (game_events->exited_galaxy && PROJECTIONS_ON)
    {
        // Convert offset to universe coordinates
        struct point_t universe_position;
        universe_position.x = nav_state->current_galaxy->position.x + ship->position.x / GALAXY_SCALE;
        universe_position.y = nav_state->current_galaxy->position.y + ship->position.y / GALAXY_SCALE;

        // Calculate camera position in universe scale
        struct camera_t universe_camera = {
            .x = nav_state->current_galaxy->position.x * GALAXY_SCALE + camera->x,
            .y = nav_state->current_galaxy->position.y * GALAXY_SCALE + camera->y,
            .w = camera->w,
            .h = camera->h};

        struct galaxy_t *nearest_galaxy = find_nearest_galaxy(*nav_state, universe_position, TRUE);

        if (nearest_galaxy != NULL &&
            (nearest_galaxy->position.x != nav_state->current_galaxy->position.x ||
             nearest_galaxy->position.y != nav_state->current_galaxy->position.y))
        {
            // Project nearest galaxy
            project_galaxy(MAP, *nav_state, nearest_galaxy, &universe_camera, game_state->game_scale);
        }

        // Project current galaxy
        project_galaxy(MAP, *nav_state, nav_state->current_galaxy, &universe_camera, game_state->game_scale);
    }

    // Create galaxy cloud
    if (!nav_state->current_galaxy->initialized_hd)
        create_galaxy_cloud(nav_state->current_galaxy, TRUE);

    // Draw screen frame
    draw_screen_frame(camera);

    if (game_events->map_exit)
        game_events->map_exit = OFF;

    if (game_events->universe_exit)
        game_events->universe_exit = OFF;
}

void onMap(GameState *game_state, InputState *input_state, GameEvents *game_events, NavigationState *nav_state, struct bstar_t bstars[], struct ship_t *ship, struct camera_t *camera)
{
    // Add small tolerance to account for floating-point precision errors
    const double epsilon = ZOOM_EPSILON;

    double zoom_step = ZOOM_STEP;

    if (game_events->universe_switch)
        cleanup_stars(nav_state->stars);

    // Reset region_size
    if (game_state->game_scale < ZOOM_MAP_REGION_SWITCH - epsilon)
    {
        if (game_events->universe_switch)
            game_state->galaxy_region_size = GALAXY_REGION_SIZE_MAX;

        // Generate new stars for new region size
        game_events->stars_start = ON;
    }

    if (game_events->map_enter)
    {
        // Save ship position
        ship->previous_position.x = ship->position.x;
        ship->previous_position.y = ship->position.y;
    }

    if (game_events->map_enter || game_events->map_center)
    {
        if (game_events->map_enter && !game_state->save_scale)
            game_state->save_scale = game_state->game_scale;

        if (!game_events->universe_switch)
        {
            game_state->game_scale = ZOOM_MAP;

            // Reset map_offset
            nav_state->map_offset.x = ship->previous_position.x;
            nav_state->map_offset.y = ship->previous_position.y;
        }

        // Zoom in
        for (int s = 0; s < MAX_STARS; s++)
        {
            if (nav_state->stars[s] != NULL && nav_state->stars[s]->star != NULL)
                zoom_star(nav_state->stars[s]->star, game_state->game_scale);
        }

        // Update camera
        update_camera(camera, nav_state->map_offset, game_state->game_scale);
    }

    if (game_events->map_center)
    {
        // Reset stars and galaxy to current position
        if (nav_state->current_galaxy->position.x != nav_state->buffer_galaxy->position.x ||
            nav_state->current_galaxy->position.y != nav_state->buffer_galaxy->position.y)
        {
            cleanup_stars(nav_state->stars);
            memcpy(nav_state->current_galaxy, nav_state->buffer_galaxy, sizeof(struct galaxy_t));

            // Reset galaxy_offset
            nav_state->galaxy_offset.current_x = nav_state->galaxy_offset.buffer_x;
            nav_state->galaxy_offset.current_y = nav_state->galaxy_offset.buffer_y;
        }

        // Reset ship position
        ship->position.x = ship->previous_position.x;
        ship->position.y = ship->previous_position.y;

        // Reset region size
        game_state->galaxy_region_size = GALAXY_REGION_SIZE;

        // Delete stars that end up outside the region
        if (game_state->game_scale - zoom_step <= ZOOM_MAP_REGION_SWITCH + epsilon)
        {
            double bx = find_nearest_section_axis(nav_state->map_offset.x, GALAXY_SECTION_SIZE);
            double by = find_nearest_section_axis(nav_state->map_offset.y, GALAXY_SECTION_SIZE);

            delete_stars_outside_region(nav_state->stars, bx, by, game_state->galaxy_region_size);
        }

        game_events->map_center = OFF;
    }

    // Zoom
    if (input_state->zoom_in)
    {
        if (game_state->game_scale <= ZOOM_MAP_REGION_SWITCH - epsilon)
            zoom_step /= 10;

        if (game_state->game_scale + zoom_step <= ZOOM_MAX + epsilon)
        {
            // Reset scale
            game_state->game_scale += zoom_step;

            // Reset region_size
            if (game_state->game_scale >= ZOOM_MAP_REGION_SWITCH - epsilon)
            {
                game_state->galaxy_region_size = GALAXY_REGION_SIZE;

                // Delete stars that end up outside the region
                if (game_state->game_scale <= ZOOM_MAP_REGION_SWITCH + zoom_step + epsilon)
                {
                    double bx = find_nearest_section_axis(nav_state->map_offset.x, GALAXY_SECTION_SIZE);
                    double by = find_nearest_section_axis(nav_state->map_offset.y, GALAXY_SECTION_SIZE);

                    delete_stars_outside_region(nav_state->stars, bx, by, game_state->galaxy_region_size);
                }
            }

            // Zoom in
            for (int s = 0; s < MAX_STARS; s++)
            {
                if (nav_state->stars[s] != NULL && nav_state->stars[s]->star != NULL)
                    zoom_star(nav_state->stars[s]->star, game_state->game_scale);
            }
        }

        input_state->zoom_in = OFF;
    }

    if (input_state->zoom_out)
    {
        if (game_state->game_scale <= ZOOM_MAP_REGION_SWITCH + epsilon)
            zoom_step = ZOOM_STEP / 10;

        if (game_state->game_scale - zoom_step >= ZOOM_MAP_MIN - epsilon)
        {
            // Reset scale
            game_state->game_scale -= zoom_step;

            // Reset region_size
            if (game_state->game_scale < ZOOM_MAP_REGION_SWITCH - epsilon)
            {
                game_state->galaxy_region_size = GALAXY_REGION_SIZE_MAX;

                // Generate new stars for new region size
                game_events->stars_start = ON;
            }

            // Switch to Universe mode
            if (game_state->game_scale <= ZOOM_MAP_SWITCH - epsilon)
            {
                game_events->map_exit = ON;
                game_events->map_switch = ON;
                game_events->universe_enter = ON;
                change_state(game_state, game_events, UNIVERSE);

                // Update universe_offset
                nav_state->universe_offset.x = nav_state->current_galaxy->position.x + nav_state->map_offset.x / GALAXY_SCALE;
                nav_state->universe_offset.y = nav_state->current_galaxy->position.y + nav_state->map_offset.y / GALAXY_SCALE;
            }

            // Zoom out
            for (int s = 0; s < MAX_STARS; s++)
            {
                if (nav_state->stars[s] != NULL && nav_state->stars[s]->star != NULL)
                    zoom_star(nav_state->stars[s]->star, game_state->game_scale);
            }
        }

        input_state->zoom_out = OFF;
    }

    // Generate stars
    generate_stars(game_state, game_events, nav_state, bstars, ship, camera);

    // Update camera
    update_camera(camera, nav_state->map_offset, game_state->game_scale);

    // Draw section lines
    int section_lines_grouping;

    if (game_state->game_scale < 0.01 - epsilon)
        section_lines_grouping = GALAXY_SECTION_SIZE * 10;
    else
        section_lines_grouping = GALAXY_SECTION_SIZE;

    draw_section_lines(camera, section_lines_grouping, colors[COLOR_ORANGE_32], game_state->game_scale);

    // Check for nearest galaxy, excluding current galaxy
    if (game_events->exited_galaxy && PROJECTIONS_ON)
    {
        // Convert offset to universe coordinates
        struct point_t universe_position;
        universe_position.x = nav_state->current_galaxy->position.x + nav_state->map_offset.x / GALAXY_SCALE;
        universe_position.y = nav_state->current_galaxy->position.y + nav_state->map_offset.y / GALAXY_SCALE;

        // Calculate camera position in universe scale
        struct camera_t universe_camera = {
            .x = nav_state->current_galaxy->position.x * GALAXY_SCALE + camera->x,
            .y = nav_state->current_galaxy->position.y * GALAXY_SCALE + camera->y,
            .w = camera->w,
            .h = camera->h};

        struct galaxy_t *nearest_galaxy = find_nearest_galaxy(*nav_state, universe_position, TRUE);

        if (nearest_galaxy != NULL &&
            (nearest_galaxy->position.x != nav_state->current_galaxy->position.x ||
             nearest_galaxy->position.y != nav_state->current_galaxy->position.y))
        {
            // Project nearest galaxy
            project_galaxy(MAP, *nav_state, nearest_galaxy, &universe_camera, game_state->game_scale);
        }

        // Project current galaxy
        project_galaxy(MAP, *nav_state, nav_state->current_galaxy, &universe_camera, game_state->game_scale);
    }

    // Create galaxy cloud
    if (!nav_state->current_galaxy->initialized_hd)
        create_galaxy_cloud(nav_state->current_galaxy, TRUE);

    // Move through map
    double rate_x = 0, rate_y = 0;

    if (input_state->right)
        rate_x = MAP_SPEED_MIN + (MAP_SPEED_MAX - MAP_SPEED_MIN) * (camera->w / 1000) / game_state->game_scale;
    else if (input_state->left)
        rate_x = -(MAP_SPEED_MIN + (MAP_SPEED_MAX - MAP_SPEED_MIN) * (camera->w / 1000) / game_state->game_scale);

    nav_state->map_offset.x += rate_x;

    if (input_state->down)
        rate_y = MAP_SPEED_MIN + (MAP_SPEED_MAX - MAP_SPEED_MIN) * (camera->w / 1000) / game_state->game_scale;
    else if (input_state->up)
        rate_y = -(MAP_SPEED_MIN + (MAP_SPEED_MAX - MAP_SPEED_MIN) * (camera->w / 1000) / game_state->game_scale);

    nav_state->map_offset.y += rate_y;

    // Update system
    if (!game_events->map_switch && !game_events->map_enter && !input_state->zoom_in && !input_state->zoom_out)
    {
        for (int i = 0; i < MAX_STARS; i++)
        {
            if (nav_state->stars[i] != NULL)
            {
                // Each index can have many entries, loop through all of them
                struct star_entry *entry = nav_state->stars[i];

                while (entry != NULL)
                {
                    update_system(game_state, *input_state, nav_state, entry->star, ship, camera);
                    entry = entry->next;
                }
            }
        }
    }

    // Draw ship projection
    ship->projection->rect.x = (ship->position.x - nav_state->map_offset.x) * game_state->game_scale + (camera->w / 2 - ship->projection->radius);
    ship->projection->rect.y = (ship->position.y - nav_state->map_offset.y) * game_state->game_scale + (camera->h / 2 - ship->projection->radius);
    ship->projection->angle = ship->angle;

    if (!game_events->map_switch && (ship->projection->rect.x + ship->projection->radius < 0 ||
                                     ship->projection->rect.x + ship->projection->radius > camera->w ||
                                     ship->projection->rect.y + ship->projection->radius < 0 ||
                                     ship->projection->rect.y + ship->projection->radius > camera->h))
    {
        project_ship(game_state->state, *input_state, *nav_state, ship, camera, game_state->game_scale);
    }
    else
        SDL_RenderCopyEx(renderer, ship->projection->texture, &ship->projection->main_img_rect, &ship->projection->rect, ship->projection->angle, &ship->projection->rotation_pt, SDL_FLIP_NONE);

    // Draw galaxy cutoff circle
    int cutoff = nav_state->current_galaxy->cutoff * GALAXY_SCALE * game_state->game_scale;
    int cx = -camera->x * game_state->game_scale;
    int cy = -camera->y * game_state->game_scale;
    SDL_DrawCircleApprox(renderer, camera, cx, cy, cutoff, colors[COLOR_CYAN_70]);

    // Draw cross at position
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);
    SDL_RenderDrawLine(renderer, (camera->w / 2) - 7, camera->h / 2, (camera->w / 2) + 7, camera->h / 2);
    SDL_RenderDrawLine(renderer, camera->w / 2, (camera->h / 2) - 7, camera->w / 2, (camera->h / 2) + 7);

    // Draw screen frame
    draw_screen_frame(camera);

    if (game_events->universe_switch)
        game_events->universe_switch = OFF;

    if (game_events->map_enter)
        game_events->map_enter = OFF;
}

void onUniverse(GameState *game_state, InputState *input_state, GameEvents *game_events, NavigationState *nav_state, struct ship_t *ship, struct camera_t *camera)
{
    static int stars_preview_start = ON;
    static int zoom_preview = OFF;
    static struct point_t cross_point = {0.0, 0.0};

    if (game_events->map_exit)
    {
        // Reset stars
        if (nav_state->current_galaxy->position.x != nav_state->buffer_galaxy->position.x ||
            nav_state->current_galaxy->position.y != nav_state->buffer_galaxy->position.y)
            cleanup_stars(nav_state->stars);

        // Reset galaxy_offset
        nav_state->galaxy_offset.current_x = nav_state->galaxy_offset.buffer_x;
        nav_state->galaxy_offset.current_y = nav_state->galaxy_offset.buffer_y;

        // Reset ship position
        ship->position.x = ship->previous_position.x;
        ship->position.y = ship->previous_position.y;

        // Reset region size
        game_state->galaxy_region_size = GALAXY_REGION_SIZE;

        // Delete stars that end up outside the region
        double bx = find_nearest_section_axis(ship->position.x, GALAXY_SECTION_SIZE);
        double by = find_nearest_section_axis(ship->position.y, GALAXY_SECTION_SIZE);

        delete_stars_outside_region(nav_state->stars, bx, by, game_state->galaxy_region_size);
    }

    if (game_events->universe_enter || game_events->universe_center)
    {
        // Save ship position
        ship->previous_position.x = ship->position.x;
        ship->previous_position.y = ship->position.y;

        // Initialize cross points for stars preview
        // Add GALAXY_SECTION_SIZE to map_offset so that stars generation is triggered on startup
        if (cross_point.x == 0.0)
            cross_point.x = find_nearest_section_axis(nav_state->map_offset.x + GALAXY_SECTION_SIZE, GALAXY_SECTION_SIZE);

        if (cross_point.y == 0.0)
            cross_point.y = find_nearest_section_axis(nav_state->map_offset.y + GALAXY_SECTION_SIZE, GALAXY_SECTION_SIZE);

        // Generate galaxies
        struct point_t offset = {.x = nav_state->galaxy_offset.current_x, .y = nav_state->galaxy_offset.current_y};
        generate_galaxies(game_events, nav_state, offset);

        if (game_events->universe_enter && !game_state->save_scale)
            game_state->save_scale = game_state->game_scale;

        if (!game_events->map_switch)
        {
            game_state->game_scale = ZOOM_UNIVERSE / GALAXY_SCALE;

            // Reset universe_offset
            nav_state->universe_offset.x = nav_state->galaxy_offset.current_x + ship->position.x / GALAXY_SCALE;
            nav_state->universe_offset.y = nav_state->galaxy_offset.current_y + ship->position.y / GALAXY_SCALE;
        }

        // Update camera
        update_camera(camera, nav_state->universe_offset, game_state->game_scale * GALAXY_SCALE);

        // Generate stars preview
        stars_preview_start = ON;

        if (game_events->universe_center)
            game_events->universe_center = OFF;
    }
    else
        // Generate galaxies
        generate_galaxies(game_events, nav_state, nav_state->universe_offset);

    // Add small tolerance to account for floating-point precision errors
    const double epsilon = ZOOM_EPSILON / GALAXY_SCALE;

    // Draw section lines
    int section_lines_grouping = UNIVERSE_SECTION_SIZE;

    if (GALAXY_SCALE == 10000)
    {
        if (game_state->game_scale >= 0.001 - epsilon)
            section_lines_grouping = UNIVERSE_SECTION_SIZE / 1000;
        else if (game_state->game_scale >= 0.0001 - epsilon)
            section_lines_grouping = UNIVERSE_SECTION_SIZE / 100;
        else if (game_state->game_scale >= 0.00001 - epsilon)
            section_lines_grouping = UNIVERSE_SECTION_SIZE / 10;
        else if (game_state->game_scale >= 0.000001 - epsilon)
            section_lines_grouping = UNIVERSE_SECTION_SIZE;
    }
    else if (GALAXY_SCALE == 1000)
    {
        if (game_state->game_scale >= 0.001 - epsilon)
            section_lines_grouping = UNIVERSE_SECTION_SIZE / 100;
        else if (game_state->game_scale >= 0.0001 - epsilon)
            section_lines_grouping = UNIVERSE_SECTION_SIZE / 10;
        else if (game_state->game_scale >= 0.00001 - epsilon)
            section_lines_grouping = UNIVERSE_SECTION_SIZE;
    }

    draw_section_lines(camera, section_lines_grouping, colors[COLOR_ORANGE_32], game_state->game_scale * GALAXY_SCALE);

    // Generate stars preview
    double zoom_universe_stars = ZOOM_UNIVERSE_STARS;

    switch (nav_state->current_galaxy->class)
    {
    case 1:
        zoom_universe_stars = 0.00005;
        break;

    default:
        zoom_universe_stars = ZOOM_UNIVERSE_STARS;
        break;
    }

    if (game_state->game_scale >= zoom_universe_stars - epsilon)
    {
        if (stars_preview_start)
        {
            // Update map_offset
            nav_state->map_offset.x = (nav_state->universe_offset.x - nav_state->current_galaxy->position.x) * GALAXY_SCALE;
            nav_state->map_offset.y = (nav_state->universe_offset.y - nav_state->current_galaxy->position.y) * GALAXY_SCALE;

            generate_stars_preview(nav_state, camera, &cross_point, zoom_preview, game_state->game_scale);
            stars_preview_start = OFF;
            zoom_preview = OFF;
        }

        for (int i = 0; i < MAX_STARS; i++)
        {
            if (nav_state->stars[i] != NULL)
            {
                // Each index can have many entries, loop through all of them
                struct star_entry *entry = nav_state->stars[i];

                while (entry != NULL)
                {
                    int x = (nav_state->current_galaxy->position.x - camera->x + entry->star->position.x / GALAXY_SCALE) * game_state->game_scale * GALAXY_SCALE;
                    int y = (nav_state->current_galaxy->position.y - camera->y + entry->star->position.y / GALAXY_SCALE) * game_state->game_scale * GALAXY_SCALE;
                    int opacity = entry->star->class * (255 / 6);
                    SDL_SetRenderDrawColor(renderer, entry->star->color.r, entry->star->color.g, entry->star->color.b, opacity);
                    SDL_RenderDrawPoint(renderer, x, y);

                    entry = entry->next;
                }
            }
        }
    }

    // Update galaxies
    if (!game_events->universe_enter)
    {
        for (int i = 0; i < MAX_GALAXIES; i++)
        {
            if (nav_state->galaxies[i] != NULL)
            {
                // Each index can have many entries, loop through all of them
                struct galaxy_entry *entry = nav_state->galaxies[i];

                while (entry != NULL)
                {
                    update_galaxy(nav_state, entry->galaxy, camera, game_state->state, game_state->game_scale);
                    entry = entry->next;
                }
            }
        }
    }

    // Move through map
    double rate_x = 0, rate_y = 0;
    double speed_universe_step = 700;

    if (game_state->game_scale >= 0.004 - epsilon)
        speed_universe_step = 10000;
    else if (game_state->game_scale >= 0.003 - epsilon)
        speed_universe_step = 3000;
    else if (game_state->game_scale >= 0.002 - epsilon)
        speed_universe_step = 1000;
    else if (game_state->game_scale >= 0.001 - epsilon)
        speed_universe_step = 800;
    else if (game_state->game_scale >= 0.00001 - epsilon)
        speed_universe_step = 700;

    if (input_state->right || input_state->left || input_state->down || input_state->up)
        stars_preview_start = ON;

    if (input_state->right)
        rate_x = UNIVERSE_SPEED_MIN + (UNIVERSE_SPEED_MAX - UNIVERSE_SPEED_MIN) * (camera->w / 1000) / (game_state->game_scale * speed_universe_step);
    else if (input_state->left)
        rate_x = -(UNIVERSE_SPEED_MIN + (UNIVERSE_SPEED_MAX - UNIVERSE_SPEED_MIN) * (camera->w / 1000) / (game_state->game_scale * speed_universe_step));

    rate_x /= (double)GALAXY_SCALE / 1000;
    nav_state->universe_offset.x += rate_x;

    if (input_state->down)
        rate_y = UNIVERSE_SPEED_MIN + (UNIVERSE_SPEED_MAX - UNIVERSE_SPEED_MIN) * (camera->w / 1000) / (game_state->game_scale * speed_universe_step);
    else if (input_state->up)
        rate_y = -(UNIVERSE_SPEED_MIN + (UNIVERSE_SPEED_MAX - UNIVERSE_SPEED_MIN) * (camera->w / 1000) / (game_state->game_scale * speed_universe_step));

    rate_y /= (double)GALAXY_SCALE / 1000;
    nav_state->universe_offset.y += rate_y;

    // Zoom
    double zoom_universe_step = ZOOM_UNIVERSE_STEP;

    if (input_state->zoom_in)
    {
        if (game_state->game_scale >= 0.001 - epsilon)
            zoom_universe_step = ZOOM_UNIVERSE_STEP;
        else if (game_state->game_scale >= 0.0001 - epsilon)
            zoom_universe_step = ZOOM_UNIVERSE_STEP / 10;
        else if (game_state->game_scale >= 0.00001 - epsilon)
            zoom_universe_step = ZOOM_UNIVERSE_STEP / 100;
        else if (game_state->game_scale > 0)
            zoom_universe_step = ZOOM_UNIVERSE_STEP / 1000;

        if (game_state->game_scale + zoom_universe_step <= ZOOM_MAP_SWITCH + epsilon)
        {
            // Reset scale
            game_state->game_scale += zoom_universe_step;

            // Switch to Map mode
            if (game_state->game_scale >= ZOOM_MAP_SWITCH - epsilon)
            {
                game_events->universe_switch = ON;
                game_events->map_enter = ON;
                change_state(game_state, game_events, MAP);

                cleanup_stars(nav_state->stars);

                // Update map_offset
                nav_state->map_offset.x = (nav_state->universe_offset.x - nav_state->current_galaxy->position.x) * GALAXY_SCALE;
                nav_state->map_offset.y = (nav_state->universe_offset.y - nav_state->current_galaxy->position.y) * GALAXY_SCALE;
            }
        }

        zoom_preview = ON;
        input_state->zoom_in = OFF;
        stars_preview_start = ON;
        cross_point.x += GALAXY_SCALE; // fake increment so that it triggers star preview generation
        cross_point.y += GALAXY_SCALE; // fake increment so that it triggers star preview generation
    }

    if (input_state->zoom_out)
    {
        if (game_state->game_scale <= 0.00001 + epsilon)
            zoom_universe_step = ZOOM_UNIVERSE_STEP / 1000;
        else if (game_state->game_scale <= 0.0001 + epsilon)
            zoom_universe_step = ZOOM_UNIVERSE_STEP / 100;
        else if (game_state->game_scale <= 0.001 + epsilon)
            zoom_universe_step = ZOOM_UNIVERSE_STEP / 10;

        if (game_state->game_scale - zoom_universe_step >= ZOOM_UNIVERSE_MIN / GALAXY_SCALE - epsilon)
        {
            // Reset scale
            game_state->game_scale -= zoom_universe_step;
        }

        cleanup_stars(nav_state->stars);

        zoom_preview = ON;
        input_state->zoom_out = OFF;
        stars_preview_start = ON;
        cross_point.x += GALAXY_SCALE; // fake increment so that it triggers star preview generation
        cross_point.y += GALAXY_SCALE; // fake increment so that it triggers star preview generation
    }

    // Update camera
    update_camera(camera, nav_state->universe_offset, game_state->game_scale * GALAXY_SCALE);

    // Draw ship projection
    ship->projection->rect.x = (nav_state->galaxy_offset.current_x + ship->position.x / GALAXY_SCALE - camera->x) * (game_state->game_scale * GALAXY_SCALE) - SHIP_PROJECTION_RADIUS;
    ship->projection->rect.y = (nav_state->galaxy_offset.current_y + ship->position.y / GALAXY_SCALE - camera->y) * (game_state->game_scale * GALAXY_SCALE) - SHIP_PROJECTION_RADIUS;
    ship->projection->angle = ship->angle;

    if (!game_events->universe_switch && (ship->projection->rect.x + ship->projection->radius < 0 ||
                                          ship->projection->rect.x + ship->projection->radius > camera->w ||
                                          ship->projection->rect.y + ship->projection->radius < 0 ||
                                          ship->projection->rect.y + ship->projection->radius > camera->h))
    {
        project_ship(game_state->state, *input_state, *nav_state, ship, camera, game_state->game_scale);
    }
    else
        SDL_RenderCopyEx(renderer, ship->projection->texture, &ship->projection->main_img_rect, &ship->projection->rect, ship->projection->angle, &ship->projection->rotation_pt, SDL_FLIP_NONE);

    // Draw cross at position
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);
    SDL_RenderDrawLine(renderer, (camera->w / 2) - 7, camera->h / 2, (camera->w / 2) + 7, camera->h / 2);
    SDL_RenderDrawLine(renderer, camera->w / 2, (camera->h / 2) - 7, camera->w / 2, (camera->h / 2) + 7);

    // Draw screen frame
    draw_screen_frame(camera);

    if (game_events->map_exit)
        game_events->map_exit = OFF;

    if (game_events->map_switch)
        game_events->map_switch = OFF;

    if (game_events->universe_enter)
        game_events->universe_enter = OFF;
}

/*
 * Probe region for stars and create them procedurally.
 * The region has intervals of size GALAXY_SECTION_SIZE.
 */
void generate_stars_preview(NavigationState *nav_state, const struct camera_t *camera, struct point_t *cross_point, int zoom_preview, long double scale)
{
    // Check how many sections fit in camera
    double section_size_scaled = GALAXY_SECTION_SIZE * scale;
    int sections_in_camera_x = (int)(camera->w / section_size_scaled);
    int sections_in_camera_y = (int)(camera->h / section_size_scaled);

    // Scale section_size with game_scale
    int section_size = GALAXY_SECTION_SIZE;
    const double epsilon = ZOOM_EPSILON / (10 * GALAXY_SCALE);
    int num_sections = 16;

    // Scale num_sections with galaxy class
    switch (nav_state->current_galaxy->class)
    {
    case 1:
        if (scale <= 0.0001 + epsilon)
            num_sections = 4;
        else if (scale <= 0.0004 + epsilon)
            num_sections = 2;
        else
            num_sections = 1;
        break;
    case 2:
        if (scale <= 0.00001 + epsilon)
            num_sections = 32;
        else if (scale <= 0.00004 + epsilon)
            num_sections = 12;
        else if (scale <= 0.00007 + epsilon)
            num_sections = 8;
        else if (scale <= 0.0001 + epsilon)
            num_sections = 4;
        else if (scale <= 0.0004 + epsilon)
            num_sections = 2;
        else
            num_sections = 1;
        break;
    case 3:
    case 4:
        if (scale <= 0.00001 + epsilon)
            num_sections = 24;
        else if (scale <= 0.00004 + epsilon)
            num_sections = 16;
        else if (scale <= 0.00007 + epsilon)
            num_sections = 8;
        else if (scale <= 0.0001 + epsilon)
            num_sections = 4;
        else if (scale <= 0.0004 + epsilon)
            num_sections = 2;
        else
            num_sections = 1;
        break;
    case 5:
    case 6:
        if (scale <= 0.00001 + epsilon)
            num_sections = 32;
        else if (scale <= 0.00004 + epsilon)
            num_sections = 16;
        else if (scale <= 0.00007 + epsilon)
            num_sections = 12;
        else if (scale <= 0.0001 + epsilon)
            num_sections = 6;
        else if (scale <= 0.0004 + epsilon)
            num_sections = 2;
        else
            num_sections = 1;
        break;
    }

    // Keep track of current nearest section axis coordinates
    section_size = num_sections * GALAXY_SECTION_SIZE;
    double bx = find_nearest_section_axis(nav_state->map_offset.x, section_size);
    double by = find_nearest_section_axis(nav_state->map_offset.y, section_size);

    // Check whether nearest section axis have changed
    if ((int)bx == (int)cross_point->x && (int)by == (int)cross_point->y)
        return;

    // Keep track of new axis
    if ((int)bx != (int)cross_point->x)
        cross_point->x = (int)bx;

    if ((int)by != (int)cross_point->y)
        cross_point->y = (int)by;

    // half_sections may lose precision due to int conversion.
    int half_sections_x = (int)(sections_in_camera_x / 2);
    int half_sections_y = (int)(sections_in_camera_y / 2);

    // Make sure that half_sections can be divided by <num_sections>
    while (half_sections_x % num_sections != 0)
        half_sections_x += 1;

    while (half_sections_y % num_sections != 0)
        half_sections_y += 1;

    double ix, iy;
    double left_boundary = bx - (half_sections_x * GALAXY_SECTION_SIZE);
    double right_boundary = bx + (half_sections_x * GALAXY_SECTION_SIZE);
    double top_boundary = by - (half_sections_y * GALAXY_SECTION_SIZE);
    double bottom_boundary = by + (half_sections_y * GALAXY_SECTION_SIZE);

    // Store previous boundaries
    static struct point_t boundaries_minus;
    static struct point_t boundaries_plus;
    static int initialized = OFF;

    // Define rect of previous boundaries
    struct point_t rect[4];

    if (initialized)
    {
        rect[0].x = boundaries_minus.x;
        rect[0].y = boundaries_plus.y;

        rect[1].x = boundaries_plus.x;
        rect[1].y = boundaries_plus.y;

        rect[2].x = boundaries_plus.x;
        rect[2].y = boundaries_minus.y;

        rect[3].x = boundaries_minus.x;
        rect[3].y = boundaries_minus.y;
    }

    // Use a local rng
    pcg32_random_t rng;

    // Density scaling parameter
    double a = nav_state->current_galaxy->radius * GALAXY_SCALE / 2.0f;

    // Set galaxy hash as initseq
    nav_state->initseq = pair_hash_order_sensitive_2(nav_state->current_galaxy->position);

    for (ix = left_boundary; ix < right_boundary; ix += section_size)
    {
        for (iy = top_boundary; iy < bottom_boundary; iy += section_size)
        {
            struct point_t position = {.x = ix, .y = iy};

            // If this point has been checked in previous function call,
            // check that point is not within previous boundaries
            if (initialized && !zoom_preview && scale <= 0.001 + epsilon)
            {
                if (point_in_rect(position, rect))
                    continue;
            }

            // Check that point is within galaxy radius
            double distance_from_center = sqrt(ix * ix + iy * iy);

            if (distance_from_center > (nav_state->current_galaxy->radius * GALAXY_SCALE))
                continue;

            // Create rng seed by combining x,y values
            uint64_t seed = pair_hash_order_sensitive(position);

            // Seed with a fixed constant
            pcg32_srandom_r(&rng, seed, nav_state->initseq);

            // Calculate density based on distance from center
            double density = (GALAXY_DENSITY / pow((distance_from_center / a + 1), 6));

            int has_star = abs(pcg32_random_r(&rng)) % 1000 < density;

            if (has_star)
            {
                // Check whether star exists in hash table
                if (star_exists(nav_state->stars, position))
                    continue;
                else
                {
                    // Create star
                    struct planet_t *star = create_star(*nav_state, position, TRUE, scale);

                    // Add star to hash table
                    put_star(nav_state->stars, position, star);
                }
            }
        }
    }

    // Store previous boundaries
    boundaries_minus.x = left_boundary;
    boundaries_minus.y = top_boundary;
    boundaries_plus.x = right_boundary;
    boundaries_plus.y = bottom_boundary;
    initialized = ON;

    // Delete stars that end up outside the region
    int region_size = sections_in_camera_x;
    delete_stars_outside_region(nav_state->stars, bx, by, region_size);
}

/*
 * Probe region for stars and create them procedurally.
 * The region has intervals of size GALAXY_SECTION_SIZE.
 * The function checks for galaxy boundaries and switches to a new galaxy if close enough.
 */
void generate_stars(GameState *game_state, GameEvents *game_events, NavigationState *nav_state, struct bstar_t bstars[], struct ship_t *ship, const struct camera_t *camera)
{
    struct point_t offset;

    if (game_state->state == NAVIGATE)
    {
        offset.x = nav_state->navigate_offset.x;
        offset.y = nav_state->navigate_offset.y;
    }
    else if (game_state->state == MAP)
    {
        offset.x = nav_state->map_offset.x;
        offset.y = nav_state->map_offset.y;
    }

    // Keep track of current nearest section axis coordinates
    double bx = find_nearest_section_axis(offset.x, GALAXY_SECTION_SIZE);
    double by = find_nearest_section_axis(offset.y, GALAXY_SECTION_SIZE);

    // Check if this is the first time calling this function
    if (!game_events->stars_start)
    {
        // Check whether nearest section axis have changed
        if (bx == nav_state->cross_axis.x && by == nav_state->cross_axis.y)
            return;

        // Keep track of new axis
        if (bx != nav_state->cross_axis.x)
            nav_state->cross_axis.x = bx;

        if (by != nav_state->cross_axis.y)
            nav_state->cross_axis.y = by;
    }

    // If exited galaxy, check for closest galaxy, including current galaxy
    if (sqrt(offset.x * offset.x + offset.y * offset.y) > nav_state->current_galaxy->cutoff * GALAXY_SCALE)
    {
        game_events->exited_galaxy = ON;

        // Convert offset to universe coordinates
        struct point_t universe_position;
        universe_position.x = nav_state->current_galaxy->position.x + offset.x / GALAXY_SCALE;
        universe_position.y = nav_state->current_galaxy->position.y + offset.y / GALAXY_SCALE;

        // Convert to cross section offset to query for new galaxies
        struct point_t cross_section_offset;
        cross_section_offset.x = find_nearest_section_axis(universe_position.x, UNIVERSE_SECTION_SIZE);
        cross_section_offset.y = find_nearest_section_axis(universe_position.y, UNIVERSE_SECTION_SIZE);
        generate_galaxies(game_events, nav_state, cross_section_offset);

        // Search for nearest galaxy to universe_position, including current galaxy
        struct galaxy_t *next_galaxy = find_nearest_galaxy(*nav_state, universe_position, FALSE);

        // Found a new galaxy
        if (next_galaxy != NULL &&
            (next_galaxy->position.x != nav_state->current_galaxy->position.x ||
             next_galaxy->position.y != nav_state->current_galaxy->position.y))
        {
            game_events->galaxy_found = ON;

            // Update previous_galaxy
            memcpy(nav_state->previous_galaxy, nav_state->current_galaxy, sizeof(struct galaxy_t));

            // Update current_galaxy
            memcpy(nav_state->current_galaxy, next_galaxy, sizeof(struct galaxy_t));

            // Get coordinates of current position relative to new galaxy
            double angle = atan2(universe_position.y - next_galaxy->position.y, universe_position.x - next_galaxy->position.x);
            double d = find_distance(universe_position.x, universe_position.y, next_galaxy->position.x, next_galaxy->position.y);
            double px = d * cos(angle) * GALAXY_SCALE;
            double py = d * sin(angle) * GALAXY_SCALE;

            // Update galaxy_offset
            nav_state->galaxy_offset.current_x = next_galaxy->position.x;
            nav_state->galaxy_offset.current_y = next_galaxy->position.y;

            if (game_state->state == NAVIGATE)
            {
                // Update ship coordinates
                ship->position.x = px;
                ship->position.y = py;

                // Update offset
                nav_state->navigate_offset.x = px;
                nav_state->navigate_offset.y = py;

                // Permanently in a new galaxy, update buffer in galaxy_offset
                nav_state->galaxy_offset.buffer_x = nav_state->galaxy_offset.current_x;
                nav_state->galaxy_offset.buffer_y = nav_state->galaxy_offset.current_y;

                // Update buffer_galaxy
                memcpy(nav_state->buffer_galaxy, nav_state->current_galaxy, sizeof(struct galaxy_t));

                // Create new background stars
                int max_bstars = (int)(camera->w * camera->h * BSTARS_PER_SQUARE / BSTARS_SQUARE);

                for (int i = 0; i < max_bstars; i++)
                {
                    bstars[i].final_star = 0;
                }

                create_bstars(nav_state, bstars, camera);
            }
            else if (game_state->state == MAP)
            {
                // Update offset
                nav_state->map_offset.x = px;
                nav_state->map_offset.y = py;

                // Update ship position so that it always points to original location
                // First find absolute coordinates for original ship position in universe scale
                double src_ship_position_x = nav_state->galaxy_offset.buffer_x + ship->previous_position.x / GALAXY_SCALE;
                double src_ship_position_y = nav_state->galaxy_offset.buffer_y + ship->previous_position.y / GALAXY_SCALE;
                // Then set new galaxy as center
                double src_ship_distance_x = src_ship_position_x - next_galaxy->position.x;
                double src_ship_distance_y = src_ship_position_y - next_galaxy->position.y;
                // Finally convert coordinates to galaxy scale and update ship position
                double dest_ship_position_x = src_ship_distance_x * GALAXY_SCALE;
                double dest_ship_position_y = src_ship_distance_y * GALAXY_SCALE;
                ship->position.x = dest_ship_position_x;
                ship->position.y = dest_ship_position_y;
            }

            // Delete stars from previous galaxy
            cleanup_stars(nav_state->stars);

            return;
        }
    }
    else
        game_events->exited_galaxy = OFF;

    // Define a region of galaxy_region_size * galaxy_region_size
    // bx,by are at the center of this area
    double ix, iy;
    double left_boundary = bx - ((game_state->galaxy_region_size / 2) * GALAXY_SECTION_SIZE);
    double right_boundary = bx + ((game_state->galaxy_region_size / 2) * GALAXY_SECTION_SIZE);
    double top_boundary = by - ((game_state->galaxy_region_size / 2) * GALAXY_SECTION_SIZE);
    double bottom_boundary = by + ((game_state->galaxy_region_size / 2) * GALAXY_SECTION_SIZE);

    // Add a buffer zone of <galaxy_region_size> sections beyond galaxy radius
    int radius_plus_buffer = (nav_state->current_galaxy->radius * GALAXY_SCALE) + game_state->galaxy_region_size * GALAXY_SECTION_SIZE;
    int in_horizontal_bounds = left_boundary > -radius_plus_buffer && right_boundary < radius_plus_buffer;
    int in_vertical_bounds = top_boundary > -radius_plus_buffer && bottom_boundary < radius_plus_buffer;

    // Use a local rng
    pcg32_random_t rng;

    // Density scaling parameter
    double a = nav_state->current_galaxy->radius * GALAXY_SCALE / 2.0f;

    // Set galaxy hash as initseq
    nav_state->initseq = pair_hash_order_sensitive_2(nav_state->current_galaxy->position);

    for (ix = left_boundary; ix < right_boundary && in_horizontal_bounds; ix += GALAXY_SECTION_SIZE)
    {
        for (iy = top_boundary; iy < bottom_boundary && in_vertical_bounds; iy += GALAXY_SECTION_SIZE)
        {
            // Check that point is within galaxy radius
            double distance_from_center = sqrt(ix * ix + iy * iy);

            if (distance_from_center > (nav_state->current_galaxy->radius * GALAXY_SCALE))
                continue;

            // Create rng seed by combining x,y values
            struct point_t position = {.x = ix, .y = iy};
            uint64_t seed = pair_hash_order_sensitive(position);

            // Seed with a fixed constant
            pcg32_srandom_r(&rng, seed, nav_state->initseq);

            // Calculate density based on distance from center
            double density = (GALAXY_DENSITY / pow((distance_from_center / a + 1), 6));

            int has_star = abs(pcg32_random_r(&rng)) % 1000 < density;

            if (has_star)
            {
                // Check whether star exists in hash table
                if (star_exists(nav_state->stars, position))
                    continue;
                else
                {
                    // Create star
                    struct planet_t *star = create_star(*nav_state, position, FALSE, game_state->game_scale);

                    // Add star to hash table
                    put_star(nav_state->stars, position, star);
                }
            }
        }
    }

    // Delete stars that end up outside the region
    delete_stars_outside_region(nav_state->stars, bx, by, game_state->galaxy_region_size);

    // First star generation complete
    game_events->stars_start = OFF;
}

/*
 * Probe region for galaxies and create them procedurally.
 * The region has intervals of size UNIVERSE_SECTION_SIZE.
 */
void generate_galaxies(GameEvents *game_events, NavigationState *nav_state, struct point_t offset)
{
    // Keep track of current nearest section axis coordinates
    double bx = find_nearest_section_axis(offset.x, UNIVERSE_SECTION_SIZE);
    double by = find_nearest_section_axis(offset.y, UNIVERSE_SECTION_SIZE);

    // Check if this is the first time calling this function
    if (!game_events->galaxies_start)
    {
        // Check whether nearest section axis have changed
        if (bx == nav_state->universe_cross_axis.x && by == nav_state->universe_cross_axis.y)
            return;

        // Keep track of new axis
        if (bx != nav_state->universe_cross_axis.x)
            nav_state->universe_cross_axis.x = bx;

        if (by != nav_state->universe_cross_axis.y)
            nav_state->universe_cross_axis.y = by;
    }

    // Define a region of UNIVERSE_REGION_SIZE * UNIVERSE_REGION_SIZE
    // bx,by are at the center of this area
    double ix, iy;
    double left_boundary = bx - ((UNIVERSE_REGION_SIZE / 2) * UNIVERSE_SECTION_SIZE);
    double right_boundary = bx + ((UNIVERSE_REGION_SIZE / 2) * UNIVERSE_SECTION_SIZE);
    double top_boundary = by - ((UNIVERSE_REGION_SIZE / 2) * UNIVERSE_SECTION_SIZE);
    double bottom_boundary = by + ((UNIVERSE_REGION_SIZE / 2) * UNIVERSE_SECTION_SIZE);

    // Add a buffer zone of <UNIVERSE_REGION_SIZE> sections beyond universe radius
    int radius_plus_buffer = UNIVERSE_X_LIMIT + UNIVERSE_REGION_SIZE * UNIVERSE_SECTION_SIZE;
    int in_horizontal_bounds = left_boundary > -radius_plus_buffer && right_boundary < radius_plus_buffer;
    int in_vertical_bounds = top_boundary > -radius_plus_buffer && bottom_boundary < radius_plus_buffer;

    // Use a local rng
    pcg32_random_t rng;

    for (ix = left_boundary; ix < right_boundary && in_horizontal_bounds; ix += UNIVERSE_SECTION_SIZE)
    {
        for (iy = top_boundary; iy < bottom_boundary && in_vertical_bounds; iy += UNIVERSE_SECTION_SIZE)
        {
            // Check that point is within universe radius
            if (sqrt(ix * ix + iy * iy) > UNIVERSE_X_LIMIT)
                continue;

            // Create rng seed by combining x,y values
            struct point_t position = {.x = ix, .y = iy};
            uint64_t seed = pair_hash_order_sensitive_2(position);

            // Seed with a fixed constant
            pcg32_srandom_r(&rng, seed, 1);

            int has_galaxy = abs(pcg32_random_r(&rng)) % 1000 < UNIVERSE_DENSITY;

            if (has_galaxy)
            {
                // Check whether galaxy exists in hash table
                if (galaxy_exists(nav_state->galaxies, position))
                    continue;
                else
                {
                    // Create galaxy
                    struct galaxy_t *galaxy = create_galaxy(position);

                    // Add galaxy to hash table
                    put_galaxy(nav_state->galaxies, position, galaxy);
                }
            }
        }
    }

    // Delete galaxies that end up outside the region
    for (int s = 0; s < MAX_GALAXIES; s++)
    {
        if (nav_state->galaxies[s] != NULL)
        {
            struct point_t position = {.x = nav_state->galaxies[s]->x, .y = nav_state->galaxies[s]->y};

            // Skip current galaxy, otherwise we lose track of where we are
            if (!game_events->galaxies_start)
            {
                if (position.x == nav_state->current_galaxy->position.x && position.y == nav_state->current_galaxy->position.y)
                    continue;
            }

            // Get distance from center of region
            double dx = position.x - bx;
            double dy = position.y - by;
            double distance = sqrt(dx * dx + dy * dy);
            double region_radius = sqrt((double)2 * ((UNIVERSE_REGION_SIZE + 1) / 2) * UNIVERSE_SECTION_SIZE * ((UNIVERSE_REGION_SIZE + 1) / 2) * UNIVERSE_SECTION_SIZE);

            // If galaxy outside region, delete it
            if (distance >= region_radius)
                delete_galaxy(nav_state->galaxies, position);
        }
    }

    // First galaxy generation complete
    game_events->galaxies_start = OFF;
}

void create_bstars(NavigationState *nav_state, struct bstar_t bstars[], const struct camera_t *camera)
{
    int i = 0, row, column, is_star;
    int end = FALSE;
    int max_bstars = (int)(camera->w * camera->h * BSTARS_PER_SQUARE / BSTARS_SQUARE);

    // Use a local rng
    pcg32_random_t rng;

    // Seed with a fixed constant
    srand(1200);

    for (row = 0; row < camera->h && !end; row++)
    {
        for (column = 0; column < camera->w && !end; column++)
        {
            // Create rng seed by combining x,y values
            struct point_t position = {.x = row, .y = column};
            uint64_t seed = pair_hash_order_sensitive(position);

            // Set galaxy hash as initseq
            nav_state->initseq = pair_hash_order_sensitive_2(nav_state->current_galaxy->position);

            // Seed with a fixed constant
            pcg32_srandom_r(&rng, seed, nav_state->initseq);

            is_star = abs(pcg32_random_r(&rng)) % BSTARS_SQUARE < BSTARS_PER_SQUARE;

            if (is_star)
            {
                struct bstar_t star;
                star.position.x = column;
                star.position.y = row;
                star.rect.x = star.position.x;
                star.rect.y = star.position.y;

                if (rand() % 12 < 1)
                {
                    star.rect.w = 2;
                    star.rect.h = 2;
                }
                else
                {
                    star.rect.w = 1;
                    star.rect.h = 1;
                }

                // Get a color between 15 - BSTARS_OPACITY
                star.opacity = ((rand() % (BSTARS_OPACITY + 1 - 15)) + 15);

                star.final_star = 1;
                bstars[i++] = star;
            }

            if (i >= max_bstars)
                end = TRUE;
        }
    }
}

/*
 * Move and draw background stars.
 */
void update_bstars(int state, int camera_on, NavigationState nav_state, struct bstar_t bstars[], const struct camera_t *camera, struct speed_t speed, double distance)
{
    int i = 0;
    int max_bstars = (int)(camera->w * camera->h * BSTARS_PER_SQUARE / BSTARS_SQUARE);
    float max_distance = 2 * nav_state.current_galaxy->radius * GALAXY_SCALE;

    while (i < max_bstars && bstars[i].final_star == 1)
    {
        if (camera_on || state == MENU)
        {
            float dx, dy;

            if (state == MENU)
            {
                dx = BSTARS_SPEED_FACTOR * speed.vx / FPS;
                dy = BSTARS_SPEED_FACTOR * speed.vy / FPS;
            }
            else
            {
                // Limit background stars speed
                if (nav_state.velocity.magnitude > GALAXY_SPEED_LIMIT)
                {
                    dx = BSTARS_SPEED_FACTOR * speed.vx / FPS;
                    dy = BSTARS_SPEED_FACTOR * speed.vy / FPS;
                }
                else
                {
                    dx = BSTARS_SPEED_FACTOR * (nav_state.velocity.magnitude / GALAXY_SPEED_LIMIT) * speed.vx / FPS;
                    dy = BSTARS_SPEED_FACTOR * (nav_state.velocity.magnitude / GALAXY_SPEED_LIMIT) * speed.vy / FPS;
                }
            }

            bstars[i].position.x -= dx;
            bstars[i].position.y -= dy;

            // Normalize within camera boundaries
            if (bstars[i].position.x > camera->w)
            {
                bstars[i].position.x = fmod(bstars[i].position.x, camera->w);
            }
            if (bstars[i].position.x < 0)
            {
                bstars[i].position.x += camera->w;
            }
            if (bstars[i].position.y > camera->h)
            {
                bstars[i].position.y = fmod(bstars[i].position.y, camera->h);
            }
            if (bstars[i].position.y < 0)
            {
                bstars[i].position.y += camera->h;
            }

            bstars[i].rect.x = (int)bstars[i].position.x;
            bstars[i].rect.y = (int)bstars[i].position.y;
        }

        float opacity;

        if (state == MENU)
            opacity = (double)(bstars[i].opacity * 1 / 2);
        else
        {
            // Fade out opacity as we move away from galaxy center
            opacity = (double)bstars[i].opacity * (1 - (distance / max_distance));

            // Opacity is 1/3 at center, 3/3 at max_distance
            opacity = (double)(opacity * ((3 - (2 - 2 * (distance / max_distance))) / 3));
        }

        if (opacity > 255)
            opacity = 255;
        else if (opacity < 0)
            opacity = 0;

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, (unsigned short)opacity);
        SDL_RenderFillRect(renderer, &bstars[i].rect);

        i++;
    }
}

/*
 * Move and draw galaxy cloud.
 */
void update_gstars(struct galaxy_t *galaxy, struct point_t ship_position, const struct camera_t *camera, double distance, double limit)
{
    int i = 0;
    float min_opacity_factor = 0.35;
    float max_opacity_factor = 0.45;
    float galaxy_radius = galaxy->radius * GALAXY_SCALE;

    // Calculate position in galaxy
    double delta_x = ship_position.x / (galaxy->cutoff * GALAXY_SCALE);
    double delta_y = ship_position.y / (galaxy->cutoff * GALAXY_SCALE);

    // Galaxy has double size when we are at center
    float scaling_factor = (float)galaxy->class / (2 + 2 * (1 - distance / galaxy_radius));

    while (i < MAX_GSTARS && galaxy->gstars_hd[i].final_star == 1)
    {
        int x = (galaxy->gstars_hd[i].position.x / (GALAXY_SCALE * GSTARS_SCALE)) / scaling_factor + camera->w / 2 - delta_x * (camera->w / 2);
        int y = (galaxy->gstars_hd[i].position.y / (GALAXY_SCALE * GSTARS_SCALE)) / scaling_factor + camera->h / 2 - delta_y * (camera->h / 2);

        float opacity;

        if (distance > limit)
        {
            opacity = 0;
        }
        else if (distance <= limit && distance > galaxy_radius)
        {
            // Fade in opacity as we move in towards galaxy radius
            opacity = (float)galaxy->gstars_hd[i].opacity * max_opacity_factor * (limit - distance) / (limit - galaxy_radius);
            opacity = opacity < 0 ? 0 : opacity;
        }
        else if (distance <= galaxy_radius)
        {
            // Fade out opacity as we move towards galaxy center
            float factor = 1.0 - distance / galaxy_radius;
            factor = factor < 0 ? 0 : factor;
            opacity = galaxy->gstars_hd[i].opacity * (max_opacity_factor - (max_opacity_factor - min_opacity_factor) * factor);
        }

        if (opacity > 255)
            opacity = 255;
        else if (opacity < 0)
            opacity = 0;

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, (unsigned short)opacity);
        SDL_RenderDrawPoint(renderer, x, y);

        i++;
    }
}

/*
 * Update camera position.
 */
void update_camera(struct camera_t *camera, struct point_t position, long double scale)
{
    camera->x = position.x - (camera->w / 2) / scale;
    camera->y = position.y - (camera->h / 2) / scale;
}

/*
 * Create a ship.
 */
struct ship_t create_ship(int radius, struct point_t position, long double scale)
{
    struct ship_t ship;

    ship.image = "../assets/sprites/ship.png";
    ship.radius = radius;
    ship.position.x = (int)position.x;
    ship.position.y = (int)position.y;
    ship.previous_position.x = 0;
    ship.previous_position.y = 0;
    ship.vx = 0.0;
    ship.vy = 0.0;
    ship.angle = 0.0;
    SDL_Surface *surface = IMG_Load(ship.image);
    ship.texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    ship.rect.x = ship.position.x * scale - ship.radius;
    ship.rect.y = ship.position.y * scale - ship.radius;
    ship.rect.w = 2 * ship.radius;
    ship.rect.h = 2 * ship.radius;
    ship.main_img_rect.x = 0; // start clipping at x of texture
    ship.main_img_rect.y = 0; // start clipping at y of texture
    ship.main_img_rect.w = 162;
    ship.main_img_rect.h = 162;
    ship.thrust_img_rect.x = 256; // start clipping at x of texture
    ship.thrust_img_rect.y = 0;   // start clipping at y of texture
    ship.thrust_img_rect.w = 162;
    ship.thrust_img_rect.h = 162;
    ship.reverse_img_rect.x = 428; // start clipping at x of texture
    ship.reverse_img_rect.y = 0;   // start clipping at y of texture
    ship.reverse_img_rect.w = 162;
    ship.reverse_img_rect.h = 162;

    // Point around which ship will be rotated (relative to destination rect)
    ship.rotation_pt.x = ship.radius;
    ship.rotation_pt.y = ship.radius;

    return ship;
}

/*
 * Create a galaxy.
 */
struct galaxy_t *create_galaxy(struct point_t position)
{
    // Find distance to nearest galaxy
    double distance = nearest_galaxy_center_distance(position);

    // Get galaxy class
    int class = get_galaxy_class(distance);

    // Use a local rng
    pcg32_random_t rng;

    // Create rng seed by combining x,y values
    uint64_t seed = pair_hash_order_sensitive_2(position);

    // Seed with a fixed constant
    pcg32_srandom_r(&rng, seed, 1);

    float radius;

    switch (class)
    {
    case GALAXY_CLASS_1:
        radius = abs(pcg32_random_r(&rng)) % GALAXY_CLASS_1_RADIUS_MAX + GALAXY_CLASS_1_RADIUS_MIN;
        break;
    case GALAXY_CLASS_2:
        radius = abs(pcg32_random_r(&rng)) % GALAXY_CLASS_2_RADIUS_MAX + GALAXY_CLASS_2_RADIUS_MIN;
        break;
    case GALAXY_CLASS_3:
        radius = abs(pcg32_random_r(&rng)) % GALAXY_CLASS_3_RADIUS_MAX + GALAXY_CLASS_3_RADIUS_MIN;
        break;
    case GALAXY_CLASS_4:
        radius = abs(pcg32_random_r(&rng)) % GALAXY_CLASS_4_RADIUS_MAX + GALAXY_CLASS_4_RADIUS_MIN;
        break;
    case GALAXY_CLASS_5:
        radius = abs(pcg32_random_r(&rng)) % GALAXY_CLASS_5_RADIUS_MAX + GALAXY_CLASS_5_RADIUS_MIN;
        break;
    case GALAXY_CLASS_6:
        radius = abs(pcg32_random_r(&rng)) % GALAXY_CLASS_6_RADIUS_MAX + GALAXY_CLASS_6_RADIUS_MIN;
        break;
    default:
        radius = abs(pcg32_random_r(&rng)) % GALAXY_CLASS_1_RADIUS_MAX + GALAXY_CLASS_1_RADIUS_MIN;
        break;
    }

    // Create galaxy
    struct galaxy_t *galaxy = (struct galaxy_t *)malloc(sizeof(struct galaxy_t));

    // Get unique galaxy index
    uint64_t index = pair_hash_order_sensitive_2(position);

    galaxy->initialized = 0;
    galaxy->initialized_hd = 0;
    sprintf(galaxy->name, "%s-%lu", "G", index);
    galaxy->class = get_galaxy_class(distance);
    galaxy->radius = radius;
    galaxy->cutoff = UNIVERSE_SECTION_SIZE * class / 2;
    galaxy->position.x = position.x;
    galaxy->position.y = position.y;
    galaxy->color.r = colors[COLOR_WHITE_255].r;
    galaxy->color.g = colors[COLOR_WHITE_255].g;
    galaxy->color.b = colors[COLOR_WHITE_255].b;

    for (int i = 0; i < MAX_GSTARS; i++)
    {
        galaxy->gstars[i].final_star = 0;
        galaxy->gstars_hd[i].final_star = 0;
    }

    return galaxy;
}

/*
 * Create a star.
 */
struct planet_t *create_star(NavigationState nav_state, struct point_t position, int preview, long double scale)
{
    // Find distance to nearest star
    double distance = nearest_star_distance(position, nav_state.current_galaxy, nav_state.initseq, GALAXY_DENSITY);

    // Get star class
    int class = get_star_class(distance);

    // Use a local rng
    pcg32_random_t rng;

    // Create rng seed by combining x,y values
    uint64_t seed = pair_hash_order_sensitive(position);

    // Seed with a fixed constant
    pcg32_srandom_r(&rng, seed, nav_state.initseq);

    float radius;

    switch (class)
    {
    case STAR_CLASS_1:
        radius = abs(pcg32_random_r(&rng)) % STAR_CLASS_1_RADIUS_MAX + STAR_CLASS_1_RADIUS_MIN;
        break;
    case STAR_CLASS_2:
        radius = abs(pcg32_random_r(&rng)) % STAR_CLASS_2_RADIUS_MAX + STAR_CLASS_2_RADIUS_MIN;
        break;
    case STAR_CLASS_3:
        radius = abs(pcg32_random_r(&rng)) % STAR_CLASS_3_RADIUS_MAX + STAR_CLASS_3_RADIUS_MIN;
        break;
    case STAR_CLASS_4:
        radius = abs(pcg32_random_r(&rng)) % STAR_CLASS_4_RADIUS_MAX + STAR_CLASS_4_RADIUS_MIN;
        break;
    case STAR_CLASS_5:
        radius = abs(pcg32_random_r(&rng)) % STAR_CLASS_5_RADIUS_MAX + STAR_CLASS_5_RADIUS_MIN;
        break;
    case STAR_CLASS_6:
        radius = abs(pcg32_random_r(&rng)) % STAR_CLASS_6_RADIUS_MAX + STAR_CLASS_6_RADIUS_MIN;
        break;
    default:
        radius = abs(pcg32_random_r(&rng)) % STAR_CLASS_1_RADIUS_MAX + STAR_CLASS_1_RADIUS_MIN;
        break;
    }

    // Create star
    struct planet_t *star = (struct planet_t *)malloc(sizeof(struct planet_t));

    // Get unique star index
    uint64_t index = pair_hash_order_sensitive(position);

    star->initialized = 0;
    sprintf(star->name, "%s-%lu", "S", index);
    star->image = "../assets/images/sol.png";
    star->class = get_star_class(distance);
    star->radius = radius;
    star->cutoff = GALAXY_SECTION_SIZE * class / 2;
    star->position.x = position.x;
    star->position.y = position.y;
    star->vx = 0.0;
    star->vy = 0.0;
    star->dx = 0.0;
    star->dy = 0.0;

    if (preview)
    {
        star->texture = NULL;
    }
    else
    {
        SDL_Surface *surface = IMG_Load(star->image);
        star->texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }

    star->rect.x = (star->position.x - star->radius) * scale;
    star->rect.y = (star->position.y - star->radius) * scale;
    star->rect.w = 2 * star->radius * scale;
    star->rect.h = 2 * star->radius * scale;
    star->color.r = colors[COLOR_WHITE_255].r;
    star->color.g = colors[COLOR_WHITE_255].g;
    star->color.b = colors[COLOR_WHITE_255].b;
    star->planets[0] = NULL;
    star->parent = NULL;
    star->level = LEVEL_STAR;

    return star;
}

/*
 * Create a system (recursive). Takes a pointer to a star or planet
 * and populates it with children planets.
 */
void create_system(struct planet_t *planet, struct point_t position, pcg32_random_t rng, long double scale)
{
    if (planet->level == LEVEL_STAR && planet->initialized == 1)
        return;

    if (planet->level >= LEVEL_MOON)
        return;

    int max_planets = (planet->level == LEVEL_STAR) ? MAX_PLANETS : MAX_MOONS;

    if (max_planets == 0)
        return;

    if (planet->level == LEVEL_STAR)
    {
        int orbit_range_min = STAR_CLASS_1_ORBIT_MIN;
        int orbit_range_max = STAR_CLASS_1_ORBIT_MAX;
        float radius_max = STAR_CLASS_1_PLANET_RADIUS_MAX;

        switch (planet->class)
        {
        case STAR_CLASS_1:
            orbit_range_min = STAR_CLASS_1_ORBIT_MIN;
            orbit_range_max = STAR_CLASS_1_ORBIT_MAX;
            radius_max = STAR_CLASS_1_PLANET_RADIUS_MAX;
            break;
        case STAR_CLASS_2:
            orbit_range_min = STAR_CLASS_2_ORBIT_MIN;
            orbit_range_max = STAR_CLASS_2_ORBIT_MAX;
            radius_max = STAR_CLASS_2_PLANET_RADIUS_MAX;
            break;
        case STAR_CLASS_3:
            orbit_range_min = STAR_CLASS_3_ORBIT_MIN;
            orbit_range_max = STAR_CLASS_3_ORBIT_MAX;
            radius_max = STAR_CLASS_3_PLANET_RADIUS_MAX;
            break;
        case STAR_CLASS_4:
            orbit_range_min = STAR_CLASS_4_ORBIT_MIN;
            orbit_range_max = STAR_CLASS_4_ORBIT_MAX;
            radius_max = STAR_CLASS_4_PLANET_RADIUS_MAX;
            break;
        case STAR_CLASS_5:
            orbit_range_min = STAR_CLASS_5_ORBIT_MIN;
            orbit_range_max = STAR_CLASS_5_ORBIT_MAX;
            radius_max = STAR_CLASS_5_PLANET_RADIUS_MAX;
            break;
        case STAR_CLASS_6:
            orbit_range_min = STAR_CLASS_6_ORBIT_MIN;
            orbit_range_max = STAR_CLASS_6_ORBIT_MAX;
            radius_max = STAR_CLASS_6_PLANET_RADIUS_MAX;
            break;
        default:
            orbit_range_min = STAR_CLASS_1_ORBIT_MIN;
            orbit_range_max = STAR_CLASS_1_ORBIT_MAX;
            radius_max = STAR_CLASS_1_PLANET_RADIUS_MAX;
            break;
        }

        float width = 0;
        int i = 0;

        // Keep track of previous orbit so that we increment orbits
        float previous_orbit = 0;

        while (i < max_planets && width < planet->cutoff - 2 * planet->radius)
        {
            // Orbit is calculated between surfaces, not centers
            // Round some values to get rid of floating-point inaccuracies
            float _orbit_width = fmod(abs(pcg32_random_r(&rng)), orbit_range_max * planet->radius) + orbit_range_min * planet->radius;
            float orbit_width = 0;

            // Increment next orbit
            while (1)
            {
                orbit_width += _orbit_width;

                if (orbit_width >= previous_orbit)
                    break;
            }

            previous_orbit = orbit_width;
            float radius = fmod(orbit_width, radius_max) + PLANET_RADIUS_MIN;

            // Add planet
            if (width + orbit_width + 2 * radius < planet->cutoff - 2 * planet->radius)
            {
                width += orbit_width + 2 * radius;

                struct planet_t *_planet = (struct planet_t *)malloc(sizeof(struct planet_t));

                strcpy(_planet->name, planet->name);                              // Copy star name to planet name
                sprintf(_planet->name + strlen(_planet->name), "-%s-%d", "P", i); // Append to planet name
                _planet->image = "../assets/images/earth.png";
                _planet->class = get_planet_class(orbit_width);
                _planet->color.r = colors[COLOR_SKY_BLUE_255].r;
                _planet->color.g = colors[COLOR_SKY_BLUE_255].g;
                _planet->color.b = colors[COLOR_SKY_BLUE_255].b;
                _planet->level = LEVEL_PLANET;
                _planet->radius = radius;
                _planet->cutoff = orbit_width / 2;

                // Calculate orbital velocity
                float angle = fmod(abs(pcg32_random_r(&rng)), 360);
                float vx, vy;
                float total_width = width + planet->radius - _planet->radius; // center to center
                calc_orbital_velocity(total_width, angle, planet->radius, &vx, &vy);

                _planet->position.x = planet->position.x + total_width * cos(angle * M_PI / 180);
                _planet->position.y = planet->position.y + total_width * sin(angle * M_PI / 180);
                _planet->vx = vx;
                _planet->vy = vy;
                _planet->dx = 0.0;
                _planet->dy = 0.0;
                SDL_Surface *surface = IMG_Load(_planet->image);
                _planet->texture = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_FreeSurface(surface);
                _planet->rect.x = (_planet->position.x - _planet->radius) * scale;
                _planet->rect.y = (_planet->position.y - _planet->radius) * scale;
                _planet->rect.w = 2 * _planet->radius * scale;
                _planet->rect.h = 2 * _planet->radius * scale;
                _planet->planets[0] = NULL;
                _planet->parent = planet;

                planet->planets[i] = _planet;
                planet->planets[i + 1] = NULL;
                i++;

                create_system(_planet, position, rng, scale);
            }
            else
                break;
        }

        // Set star as initialized
        planet->initialized = 1;
    }

    // Moons
    else if (planet->level == LEVEL_PLANET)
    {
        int orbit_range_min = PLANET_CLASS_1_ORBIT_MIN;
        int orbit_range_max = PLANET_CLASS_1_ORBIT_MAX;
        float radius_max = PLANET_CLASS_1_MOON_RADIUS_MAX;
        float planet_cutoff_limit;

        switch (planet->class)
        {
        case PLANET_CLASS_1:
            orbit_range_min = PLANET_CLASS_1_ORBIT_MIN;
            orbit_range_max = PLANET_CLASS_1_ORBIT_MAX;
            radius_max = PLANET_CLASS_1_MOON_RADIUS_MAX;
            planet_cutoff_limit = planet->cutoff / 2;
            max_planets = (int)planet->cutoff % (max_planets - 4); // 0 - <max - 4>
            break;
        case PLANET_CLASS_2:
            orbit_range_min = PLANET_CLASS_2_ORBIT_MIN;
            orbit_range_max = PLANET_CLASS_2_ORBIT_MAX;
            radius_max = PLANET_CLASS_2_MOON_RADIUS_MAX;
            planet_cutoff_limit = planet->cutoff / 2;
            max_planets = (int)planet->cutoff % (max_planets - 3); // 0 - <max - 3>
            break;
        case PLANET_CLASS_3:
            orbit_range_min = PLANET_CLASS_3_ORBIT_MIN;
            orbit_range_max = PLANET_CLASS_3_ORBIT_MAX;
            radius_max = PLANET_CLASS_3_MOON_RADIUS_MAX;
            planet_cutoff_limit = planet->cutoff / 2;
            max_planets = (int)planet->cutoff % (max_planets - 2); // 0 - <max - 2>
            break;
        case PLANET_CLASS_4:
            orbit_range_min = PLANET_CLASS_4_ORBIT_MIN;
            orbit_range_max = PLANET_CLASS_4_ORBIT_MAX;
            radius_max = PLANET_CLASS_4_MOON_RADIUS_MAX;
            planet_cutoff_limit = planet->cutoff / 2;
            max_planets = (int)planet->cutoff % (max_planets - 2); // 0 - <max - 2>
            break;
        case PLANET_CLASS_5:
            orbit_range_min = PLANET_CLASS_5_ORBIT_MIN;
            orbit_range_max = PLANET_CLASS_5_ORBIT_MAX;
            radius_max = PLANET_CLASS_5_MOON_RADIUS_MAX;
            planet_cutoff_limit = planet->cutoff / 3;
            max_planets = (int)planet->cutoff % (max_planets - 1); // 0 - <max - 1>
            break;
        case PLANET_CLASS_6:
            orbit_range_min = PLANET_CLASS_6_ORBIT_MIN;
            orbit_range_max = PLANET_CLASS_6_ORBIT_MAX;
            radius_max = PLANET_CLASS_6_MOON_RADIUS_MAX;
            planet_cutoff_limit = planet->cutoff / 3;
            max_planets = (int)planet->cutoff % max_planets; // 0 - <max>
            break;
        default:
            orbit_range_min = PLANET_CLASS_1_ORBIT_MIN;
            orbit_range_max = PLANET_CLASS_1_ORBIT_MAX;
            radius_max = PLANET_CLASS_1_MOON_RADIUS_MAX;
            planet_cutoff_limit = planet->cutoff / 2;
            max_planets = (int)planet->cutoff % (max_planets - 4); // 0 - <max - 4>
            break;
        }

        float width = 0;
        int i = 0;

        while (i < max_planets && width < planet->cutoff - 2 * planet->radius)
        {
            // Orbit is calculated between surfaces, not centers
            float _orbit_width = fmod(abs(pcg32_random_r(&rng)), orbit_range_max * planet->radius) + orbit_range_min * planet->radius;
            float orbit_width = 0;

            // The first orbit should not be closer than <planet_cutoff_limit>
            while (1)
            {
                orbit_width += _orbit_width;

                if (orbit_width >= planet_cutoff_limit)
                    break;
            }

            // A moon can not be larger than class radius_max or 1 / 3 of planet radius
            float radius = fmod(orbit_width, fmin(radius_max, planet->radius / 3)) + MOON_RADIUS_MIN;

            // Add moon
            if (width + orbit_width + 2 * radius < planet->cutoff - 2 * planet->radius)
            {
                width += orbit_width + 2 * radius;

                struct planet_t *_planet = (struct planet_t *)malloc(sizeof(struct planet_t));

                strcpy(_planet->name, planet->name);                              // Copy planet name to moon name
                sprintf(_planet->name + strlen(_planet->name), "-%s-%d", "M", i); // Append to moon name
                _planet->image = "../assets/images/moon.png";
                _planet->class = 0;
                _planet->color.r = colors[COLOR_GAINSBORO_255].r;
                _planet->color.g = colors[COLOR_GAINSBORO_255].g;
                _planet->color.b = colors[COLOR_GAINSBORO_255].b;
                _planet->level = LEVEL_MOON;
                _planet->radius = radius;

                // Calculate orbital velocity
                float angle = fmod(abs(pcg32_random_r(&rng)), 360);
                float vx, vy;
                float total_width = width + planet->radius - _planet->radius;
                calc_orbital_velocity(total_width, angle, planet->radius, &vx, &vy);

                _planet->position.x = planet->position.x + total_width * cos(angle * M_PI / 180);
                _planet->position.y = planet->position.y + total_width * sin(angle * M_PI / 180);
                _planet->vx = vx;
                _planet->vy = vy;
                _planet->dx = 0.0;
                _planet->dy = 0.0;
                SDL_Surface *surface = IMG_Load(_planet->image);
                _planet->texture = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_FreeSurface(surface);
                _planet->rect.x = (_planet->position.x - _planet->radius) * scale;
                _planet->rect.y = (_planet->position.y - _planet->radius) * scale;
                _planet->rect.w = 2 * _planet->radius * scale;
                _planet->rect.h = 2 * _planet->radius * scale;
                _planet->planets[0] = NULL;
                _planet->parent = planet;

                planet->planets[i] = _planet;
                planet->planets[i + 1] = NULL;
                i++;
            }
            else
                break;
        }
    }
}

/*
 * Update and draw galaxy.
 */
void update_galaxy(NavigationState *nav_state, struct galaxy_t *galaxy, const struct camera_t *camera, int state, long double scale)
{
    // Get galaxy distance from position
    double delta_x = galaxy->position.x - nav_state->universe_offset.x;
    double delta_y = galaxy->position.y - nav_state->universe_offset.y;
    double distance = sqrt(delta_x * delta_x + delta_y * delta_y);

    // Draw cutoff circle
    if (distance < galaxy->cutoff)
    {
        // Reset stars and update current_galaxy
        if (strcmp(nav_state->current_galaxy->name, galaxy->name) != 0)
        {
            cleanup_stars(nav_state->stars);
            memcpy(nav_state->current_galaxy, galaxy, sizeof(struct galaxy_t));
        }

        int cutoff = galaxy->cutoff * scale * GALAXY_SCALE;
        int rx = (galaxy->position.x - camera->x) * scale * GALAXY_SCALE;
        int ry = (galaxy->position.y - camera->y) * scale * GALAXY_SCALE;

        SDL_DrawCircle(renderer, camera, rx, ry, cutoff, colors[COLOR_CYAN_70]);

        // Create gstars_hd
        if (!galaxy->initialized_hd)
            create_galaxy_cloud(galaxy, TRUE);

        double zoom_universe_stars = ZOOM_UNIVERSE_STARS;

        switch (nav_state->current_galaxy->class)
        {
        case 1:
            zoom_universe_stars = 0.00005;
            break;

        default:
            zoom_universe_stars = ZOOM_UNIVERSE_STARS;
            break;
        }

        const double epsilon = ZOOM_EPSILON / GALAXY_SCALE;

        if (scale < zoom_universe_stars + epsilon)
            draw_galaxy_cloud(galaxy, camera, galaxy->initialized_hd, TRUE, scale);
    }
    else
    {
        // Draw galaxy cloud
        if (in_camera(camera, galaxy->position.x, galaxy->position.y, galaxy->radius, scale * GALAXY_SCALE))
        {
            if (!galaxy->initialized)
                create_galaxy_cloud(galaxy, FALSE);

            draw_galaxy_cloud(galaxy, camera, galaxy->initialized, FALSE, scale);
        }
        // Draw galaxy projection
        else if (PROJECTIONS_ON)
        {
            // Show projections only if game scale < 50 * ZOOM_UNIVERSE_MIN
            if (scale / (ZOOM_UNIVERSE_MIN / GALAXY_SCALE) < 50)
                project_galaxy(state, *nav_state, galaxy, camera, scale * GALAXY_SCALE);
        }
    }
}

/*
 * Create, update, draw star system and apply gravity to planets and ship (recursive).
 */
void update_system(GameState *game_state, InputState input_state, NavigationState *nav_state, struct planet_t *planet, struct ship_t *ship, const struct camera_t *camera)
{
    struct point_t position;

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

    // Update planets & moons
    if (planet->level != LEVEL_STAR)
    {
        double distance;
        float orbit_opacity;

        if (game_state->state == NAVIGATE)
        {
            // Update planet position
            planet->position.x += planet->parent->dx;
            planet->position.y += planet->parent->dy;

            // Find distance from parent
            double delta_x = planet->parent->position.x - planet->position.x;
            double delta_y = planet->parent->position.y - planet->position.y;
            distance = sqrt(delta_x * delta_x + delta_y * delta_y);

            // Determine speed and position shift
            if (distance > (planet->parent->radius + planet->radius))
            {
                float g_planet = G_CONSTANT * planet->parent->radius * planet->parent->radius / (distance * distance);

                planet->vx += g_planet * delta_x / distance;
                planet->vy += g_planet * delta_y / distance;
                planet->dx = planet->vx / FPS;
                planet->dy = planet->vy / FPS;
            }

            // Update planet position
            planet->position.x += planet->vx / FPS;
            planet->position.y += planet->vy / FPS;

            orbit_opacity = 45;
        }
        else if (game_state->state == MAP)
        {
            // Find distance from parent
            double delta_x = planet->parent->position.x - planet->position.x;
            double delta_y = planet->parent->position.y - planet->position.y;
            distance = sqrt(delta_x * delta_x + delta_y * delta_y);

            orbit_opacity = 32;
        }

        // Draw orbit
        if (input_state.orbits_on)
        {
            int radius = distance * game_state->game_scale;
            int _x = (planet->parent->position.x - camera->x) * game_state->game_scale;
            int _y = (planet->parent->position.y - camera->y) * game_state->game_scale;
            SDL_Color orbit_color = {
                colors[COLOR_WHITE_255].r,
                colors[COLOR_WHITE_255].g,
                colors[COLOR_WHITE_255].b,
                orbit_opacity};

            SDL_DrawCircle(renderer, camera, _x, _y, radius, orbit_color);
        }

        // Update moons
        int max_planets = MAX_MOONS;

        for (int i = 0; i < max_planets && planet->planets[i] != NULL; i++)
        {
            update_system(game_state, input_state, nav_state, planet->planets[i], ship, camera);
        }
    }
    else if (planet->level == LEVEL_STAR)
    {
        // Get star distance from position
        double delta_x_star = planet->position.x - position.x;
        double delta_y_star = planet->position.y - position.y;
        double distance_star = sqrt(delta_x_star * delta_x_star + delta_y_star * delta_y_star);

        if (game_state->state == MAP)
        {
            if (distance_star < planet->cutoff && SOLAR_SYSTEMS_ON)
            {
                // Create system
                if (!planet->initialized && SOLAR_SYSTEMS_ON)
                {
                    struct point_t star_position = {.x = planet->position.x, .y = planet->position.y};

                    // Use a local rng
                    pcg32_random_t rng;

                    // Create rng seed by combining x,y values
                    uint64_t seed = pair_hash_order_sensitive(star_position);

                    // Seed with a fixed constant
                    pcg32_srandom_r(&rng, seed, nav_state->initseq);

                    create_system(planet, star_position, rng, game_state->game_scale);
                }

                // Update planets
                int max_planets = MAX_PLANETS;

                for (int i = 0; i < max_planets && planet->planets[i] != NULL; i++)
                {
                    update_system(game_state, input_state, nav_state, planet->planets[i], ship, camera);
                }

                if (input_state.orbits_on)
                {
                    // Draw cutoff area circles
                    int _r = planet->class * GALAXY_SECTION_SIZE / 2;
                    int radius = _r * game_state->game_scale;
                    int _x = (planet->position.x - camera->x) * game_state->game_scale;
                    int _y = (planet->position.y - camera->y) * game_state->game_scale;

                    SDL_DrawCircle(renderer, camera, _x, _y, radius - 1, colors[COLOR_MAGENTA_40]);
                    SDL_DrawCircle(renderer, camera, _x, _y, radius - 2, colors[COLOR_MAGENTA_40]);
                    SDL_DrawCircle(renderer, camera, _x, _y, radius - 3, colors[COLOR_MAGENTA_40]);
                }
            }
        }
        else if (game_state->state == NAVIGATE)
        {
            if (distance_star < planet->cutoff && SOLAR_SYSTEMS_ON)
            {
                // Create system
                if (!planet->initialized)
                {
                    struct point_t star_position = {.x = planet->position.x, .y = planet->position.y};

                    // Use a local rng
                    pcg32_random_t rng;

                    // Create rng seed by combining x,y values
                    uint64_t seed = pair_hash_order_sensitive(star_position);

                    // Seed with a fixed constant
                    pcg32_srandom_r(&rng, seed, nav_state->initseq);

                    create_system(planet, star_position, rng, game_state->game_scale);
                }

                // Update planets
                int max_planets = MAX_PLANETS;

                for (int i = 0; i < max_planets && planet->planets[i] != NULL; i++)
                {
                    update_system(game_state, input_state, nav_state, planet->planets[i], ship, camera);
                }
            }

            // Draw cutoff area circle
            if (input_state.orbits_on && distance_star < 2 * planet->cutoff)
            {
                int cutoff = planet->cutoff * game_state->game_scale;
                int _x = (planet->position.x - camera->x) * game_state->game_scale;
                int _y = (planet->position.y - camera->y) * game_state->game_scale;

                SDL_DrawCircle(renderer, camera, _x, _y, cutoff, colors[COLOR_MAGENTA_70]);
            }
        }
    }

    // Draw planet
    if (in_camera(camera, planet->position.x, planet->position.y, planet->radius, game_state->game_scale))
    {
        planet->rect.x = (int)(planet->position.x - planet->radius - camera->x) * game_state->game_scale;
        planet->rect.y = (int)(planet->position.y - planet->radius - camera->y) * game_state->game_scale;

        SDL_RenderCopy(renderer, planet->texture, NULL, &planet->rect);
    }
    // Draw planet projection
    else if (PROJECT_PLANETS_ON)
    {
        if (planet->level == LEVEL_MOON)
        {
            double delta_x = planet->parent->position.x - position.x;
            double delta_y = planet->parent->position.y - position.y;
            double distance = sqrt(delta_x * delta_x + delta_y * delta_y);

            if (distance < 2 * planet->parent->cutoff)
                project_planet(*game_state, *nav_state, planet, camera);
        }
        else
            project_planet(*game_state, *nav_state, planet, camera);
    }

    // Update ship speed due to gravity
    if (game_state->state == NAVIGATE && SHIP_GRAVITY_ON)
        apply_gravity_to_ship(game_state, input_state.thrust, nav_state, planet, ship, camera);
}

/*
 * Apply planet gravity to ship.
 */
void apply_gravity_to_ship(GameState *game_state, int thrust, NavigationState *nav_state, struct planet_t *planet, struct ship_t *ship, const struct camera_t *camera)
{
    double delta_x = planet->position.x - ship->position.x;
    double delta_y = planet->position.y - ship->position.y;
    double distance = sqrt(delta_x * delta_x + delta_y * delta_y);
    float g_planet = 0;
    int is_star = planet->level == LEVEL_STAR;
    int collision_point = planet->radius;

    // Detect planet collision
    if (COLLISIONS_ON && distance <= collision_point + ship->radius)
    {
        game_state->landing_stage = STAGE_0; // This changes on next iteration (next planet). To-do: Must also link it to specific planet.
        g_planet = 0;

        if (is_star)
        {
            ship->vx = 0.0;
            ship->vy = 0.0;
        }
        else
        {
            ship->vx = planet->vx;
            ship->vy = planet->vy;
            ship->vx += planet->parent->vx;
            ship->vy += planet->parent->vy;
        }

        // Find landing angle
        if (ship->position.y == planet->position.y)
        {
            if (ship->position.x > planet->position.x)
            {
                ship->angle = 90;
                ship->position.x = planet->position.x + collision_point + ship->radius; // Fix ship position on collision surface
            }
            else
            {
                ship->angle = 270;
                ship->position.x = planet->position.x - collision_point - ship->radius; // Fix ship position on collision surface
            }
        }
        else if (ship->position.x == planet->position.x)
        {
            if (ship->position.y > planet->position.y)
            {
                ship->angle = 180;
                ship->position.y = planet->position.y + collision_point + ship->radius; // Fix ship position on collision surface
            }
            else
            {
                ship->angle = 0;
                ship->position.y = planet->position.y - collision_point - ship->radius; // Fix ship position on collision surface
            }
        }
        else
        {
            // 2nd quadrant
            if (ship->position.y > planet->position.y && ship->position.x > planet->position.x)
            {
                ship->angle = (asin(abs((int)(planet->position.x - ship->position.x)) / distance) * 180 / M_PI);
                ship->angle = 180 - ship->angle;
            }
            // 3rd quadrant
            else if (ship->position.y > planet->position.y && ship->position.x < planet->position.x)
            {
                ship->angle = (asin(abs((int)(planet->position.x - ship->position.x)) / distance) * 180 / M_PI);
                ship->angle = 180 + ship->angle;
            }
            // 4th quadrant
            else if (ship->position.y < planet->position.y && ship->position.x < planet->position.x)
            {
                ship->angle = (asin(abs((int)(planet->position.x - ship->position.x)) / distance) * 180 / M_PI);
                ship->angle = 360 - ship->angle;
            }
            // 1st quadrant
            else
            {
                ship->angle = asin(abs((int)(planet->position.x - ship->position.x)) / distance) * 180 / M_PI;
            }

            ship->position.x = ((ship->position.x - planet->position.x) * (collision_point + ship->radius) / distance) + planet->position.x; // Fix ship position on collision surface
            ship->position.y = ((ship->position.y - planet->position.y) * (collision_point + ship->radius) / distance) + planet->position.y; // Fix ship position on collision surface
        }

        // Apply thrust
        if (thrust)
        {
            ship->vx -= G_LAUNCH * delta_x / distance;
            ship->vy -= G_LAUNCH * delta_y / distance;
        }
    }
    // Ship inside cutoff
    else if (distance < planet->cutoff)
    {
        game_state->landing_stage = STAGE_OFF;
        g_planet = G_CONSTANT * planet->radius * planet->radius / (distance * distance);

        ship->vx += g_planet * delta_x / distance;
        ship->vy += g_planet * delta_y / distance;
    }

    // Update velocity
    update_velocity(&nav_state->velocity, ship);

    // Enforce speed limit if within star cutoff
    if (is_star && distance < planet->cutoff)
    {
        game_state->speed_limit = BASE_SPEED_LIMIT * planet->class;

        if (nav_state->velocity.magnitude > game_state->speed_limit)
        {
            ship->vx = game_state->speed_limit * ship->vx / nav_state->velocity.magnitude;
            ship->vy = game_state->speed_limit * ship->vy / nav_state->velocity.magnitude;

            // Update velocity
            update_velocity(&nav_state->velocity, ship);
        }
    }
}

/*
 * Update ship position, listen for key controls and draw ship.
 */
void update_ship(GameState *game_state, InputState input_state, NavigationState nav_state, struct ship_t *ship, const struct camera_t *camera)
{
    float radians;

    // Update ship angle
    if (input_state.right && !input_state.left && game_state->landing_stage == STAGE_OFF)
        ship->angle += 3;

    if (input_state.left && !input_state.right && game_state->landing_stage == STAGE_OFF)
        ship->angle -= 3;

    if (ship->angle > 360)
        ship->angle -= 360;

    // Apply thrust
    if (input_state.thrust)
    {
        game_state->landing_stage = STAGE_OFF;
        radians = ship->angle * M_PI / 180;

        ship->vx += G_THRUST * sin(radians);
        ship->vy -= G_THRUST * cos(radians);
    }

    // Apply reverse
    if (input_state.reverse)
    {
        radians = ship->angle * M_PI / 180;

        ship->vx -= G_THRUST * sin(radians);
        ship->vy += G_THRUST * cos(radians);
    }

    // Stop ship
    if (input_state.stop)
    {
        ship->vx = 0;
        ship->vy = 0;
    }

    // Update ship position
    ship->position.x += (float)ship->vx / FPS;
    ship->position.y += (float)ship->vy / FPS;

    if (input_state.camera_on)
    {
        // Static rect position at center of screen fixes flickering caused by float-to-int inaccuracies
        ship->rect.x = (camera->w / 2) - ship->radius;
        ship->rect.y = (camera->h / 2) - ship->radius;
    }
    else
    {
        // Dynamic rect position based on ship position
        ship->rect.x = (int)((ship->position.x - camera->x) * game_state->game_scale - ship->radius);
        ship->rect.y = (int)((ship->position.y - camera->y) * game_state->game_scale - ship->radius);
    }

    // Draw ship if in camera
    if (in_camera(camera, ship->position.x, ship->position.y, ship->radius, game_state->game_scale))
    {
        SDL_RenderCopyEx(renderer, ship->texture, &ship->main_img_rect, &ship->rect, ship->angle, &ship->rotation_pt, SDL_FLIP_NONE);
    }
    // Draw ship projection
    else if (PROJECTIONS_ON)
        project_ship(NAVIGATE, input_state, nav_state, ship, camera, game_state->game_scale);

    // Draw ship thrust
    if (input_state.thrust)
        SDL_RenderCopyEx(renderer, ship->texture, &ship->thrust_img_rect, &ship->rect, ship->angle, &ship->rotation_pt, SDL_FLIP_NONE);

    // Draw reverse thrust
    if (input_state.reverse)
        SDL_RenderCopyEx(renderer, ship->texture, &ship->reverse_img_rect, &ship->rect, ship->angle, &ship->rotation_pt, SDL_FLIP_NONE);
}
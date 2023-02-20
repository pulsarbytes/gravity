/*
 * Gravity - An infinite procedural 2d universe that models gravity and orbital motion.
 *
 * v1.2.0
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

#include "../include/common.h"
#include "../include/structs.h"
#include "../lib/pcg-c-basic-0.9/pcg_basic.h"

// External variable definitions
SDL_Window *window = NULL;
SDL_DisplayMode display_mode;
SDL_Renderer *renderer = NULL;
TTF_Font *font = NULL;
SDL_Color text_color;

// Global variables
const float g_launch = 0.7 * G_CONSTANT;
const float g_thrust = 1 * G_CONSTANT;
static int stars_start = 1;
static int galaxies_start = 1;
static int speed_limit = BASE_SPEED_LIMIT;
static int landing_stage = STAGE_OFF;
struct vector_t velocity;
int state = NAVIGATE;
float game_scale = ZOOM_NAVIGATE;
static float save_scale = 0;

// Keep track of keyboard inputs
int left = OFF;
int right = OFF;
int up = OFF;
int down = OFF;
int thrust = OFF;
int reverse = OFF;
int camera_on = CAMERA_ON;
int stop = OFF;
int zoom_in = OFF;
int zoom_out = OFF;
int console = ON;
int orbits_on = SHOW_ORBITS;
int galaxy_region_size = GALAXY_REGION_SIZE;

// Keep track of events
int map_enter = OFF;
int map_exit = OFF;
int map_center = OFF;
int universe_enter = OFF;
int universe_exit = OFF;
int universe_center = OFF;
int exited_galaxy = OFF;
int galaxy_found = OFF;

// Keep track of nearest axis coordinates
struct point_t cross_axis;
struct point_t universe_cross_axis;

// Array for game console entries
struct game_console_entry game_console_entries[LOG_COUNT];

// Hash table for stars
struct star_entry *stars[MAX_STARS];

// Hash table for galaxies
struct galaxy_entry *galaxies[MAX_GALAXIES];

struct galaxy_t *current_galaxy;
struct galaxy_t *buffer_galaxy;
struct galaxy_t *previous_galaxy;
static struct point_t previous_ship_position = {.x = 0, .y = 0};

// Output sequence for the RNG of stars.
// Changes for every new current_galaxy.
uint64_t initseq;

// Function prototypes
void onMenu(void);
void onNavigate(struct bstar_t bstars[], struct ship_t *, struct camera_t *, struct point_t *, struct point_state *);
void onMap(struct bstar_t bstars[], struct ship_t *, struct camera_t *, struct point_t *, struct point_state *);
void onUniverse(struct ship_t *, struct camera_t *, struct point_t *, struct point_state *);
void onPause(struct bstar_t bstars[], struct ship_t *, const struct camera_t *, unsigned int start_time);
int init_sdl(void);
void close_sdl(void);
void poll_events(int *quit);
void log_game_console(struct game_console_entry entries[], int index, float value);
void update_game_console(struct game_console_entry entries[]);
void log_fps(unsigned int time_diff);
void cleanup_resources(struct ship_t *, struct galaxy_t *, struct galaxy_t *, struct galaxy_t *);
void cleanup_stars(void);
void calc_orbital_velocity(float height, float angle, float radius, float *vx, float *vy);
void create_bstars(struct bstar_t bstars[], int max_bstars);
void update_bstars(struct bstar_t bstars[], const struct camera_t *, struct speed_t, double distance);
struct planet_t *create_star(struct point_t);
uint64_t pair_hash_order_sensitive(struct point_t);
uint64_t pair_hash_order_sensitive_2(struct point_t);
void create_system(struct planet_t *, struct point_t, pcg32_random_t rng);
struct ship_t create_ship(int radius, struct point_t);
void update_system(struct planet_t *, struct ship_t *, const struct camera_t *, struct point_t, int state);
void project_planet(struct planet_t *, const struct camera_t *, int state);
void apply_gravity_to_ship(struct planet_t *, struct ship_t *, const struct camera_t *);
void update_camera(struct camera_t *, struct point_t);
void update_ship(struct ship_t *, const struct camera_t *);
void project_ship(struct ship_t *, const struct camera_t *, int state);
double find_nearest_section_axis(double n, int size);
void generate_stars(struct bstar_t bstars[], struct ship_t *, struct point_t *, struct point_state *, int state);
void put_star(struct point_t, struct planet_t *);
struct planet_t *get_star(struct point_t);
int star_exists(struct point_t);
void delete_star(struct point_t);
void update_velocity(struct ship_t *ship);
float nearest_star_distance(struct point_t, struct galaxy_t *, uint64_t initseq);
int get_star_class(float n);
int get_planet_class(float n);
void zoom_star(struct planet_t *);
void SDL_DrawCircle(SDL_Renderer *, const struct camera_t *, int xc, int yc, int radius, SDL_Color);
int in_camera(const struct camera_t *, double x, double y, float radius);
void draw_screen_frame(struct camera_t *);
void generate_galaxies(struct point_t);
int galaxy_exists(struct point_t);
float nearest_galaxy_center_distance(struct point_t);
int get_galaxy_class(float n);
struct galaxy_t *create_galaxy(struct point_t);
void put_galaxy(struct point_t, struct galaxy_t *);
void delete_galaxy(struct point_t);
void update_galaxy(struct galaxy_t *, const struct camera_t *, struct point_t);
void SDL_DrawCircleApprox(SDL_Renderer *, const struct camera_t *, int x, int y, int r, SDL_Color);
void draw_section_lines(struct camera_t *, int section_size, SDL_Color);
void project_galaxy(struct galaxy_t *, const struct camera_t *, int state);
struct galaxy_t *get_galaxy(struct point_t);
struct galaxy_t *find_nearest_galaxy(struct point_t, struct galaxy_t *, int exclude);
double find_distance(double x1, double y1, double x2, double y2);
void create_galaxy_cloud(struct galaxy_t *);
void draw_galaxy_cloud(struct galaxy_t *, const struct camera_t *, int gstars_count);
void update_gstars(struct galaxy_t *, struct point_t, const struct camera_t *, double distance, double limit);
void update_speed_lines(const struct camera_t *, struct speed_t);
void draw_speed_arc(struct ship_t *ship, const struct camera_t *camera);

int main(int argc, char *argv[])
{
    int quit = 0;

    // Initialize SDL
    if (!init_sdl())
    {
        fprintf(stderr, "Error: could not initialize SDL.\n");
        return 1;
    }

    // Universe coordinates
    // Retrieved from saved game or use default values if this is a new game
    struct point_state universe_offset = {.current_x = UNIVERSE_START_X, .current_y = UNIVERSE_START_Y, .previous_x = UNIVERSE_START_X, .previous_y = UNIVERSE_START_Y};

    // Navigation coordinates
    struct point_t navigate_offset = {.x = GALAXY_START_X, .y = GALAXY_START_Y};

    // Map coordinates
    struct point_t map_offset = {.x = GALAXY_START_X, .y = GALAXY_START_Y};

    // Universe scroll coordinates
    struct point_t scroll_offset = {.x = universe_offset.current_x, .y = universe_offset.current_y};

    // Create ship and ship projection
    struct ship_t ship = create_ship(SHIP_RADIUS, navigate_offset);
    struct point_t zero_position = {.x = 0, .y = 0};
    struct ship_t ship_projection = create_ship(SHIP_PROJECTION_RADIUS, zero_position);
    ship.projection = &ship_projection;

    // Create camera, sync initial position with ship
    struct camera_t camera = {
        .x = ship.position.x - (display_mode.w / 2),
        .y = ship.position.y - (display_mode.h / 2),
        .w = display_mode.w,
        .h = display_mode.h};

    // Generate galaxies
    struct point_t initial_position = {.x = universe_offset.current_x, .y = universe_offset.current_y};
    generate_galaxies(initial_position);

    // Allocate memory for galaxies
    current_galaxy = (struct galaxy_t *)malloc(sizeof(struct galaxy_t));
    buffer_galaxy = (struct galaxy_t *)malloc(sizeof(struct galaxy_t));
    previous_galaxy = (struct galaxy_t *)malloc(sizeof(struct galaxy_t));

    // Get a copy of current galaxy from the hash table
    struct point_t galaxy_position = {.x = universe_offset.current_x, .y = universe_offset.current_y};
    struct galaxy_t *current_galaxy_copy = get_galaxy(galaxy_position);

    // Use memcpy to copy the contents of current_galaxy_copy to current_galaxy
    memcpy(current_galaxy, current_galaxy_copy, sizeof(struct galaxy_t));

    // Create background stars
    int max_bstars = (int)(display_mode.w * display_mode.h * BSTARS_PER_SQUARE / BSTARS_SQUARE);
    struct bstar_t bstars[max_bstars];

    for (int i = 0; i < max_bstars; i++)
    {
        bstars[i].final_star = 0;
    }

    create_bstars(bstars, max_bstars);

    // Generate stars
    generate_stars(bstars, &ship, &navigate_offset, &universe_offset, state);

    // Put ship in orbit around a star
    if (START_IN_ORBIT)
    {
        struct point_t start_position = {.x = GALAXY_START_X, .y = GALAXY_START_Y};

        if (star_exists(start_position))
        {
            struct planet_t *star = get_star(start_position);

            float d = 3 * (star->radius);
            ship.position.x = start_position.x + d;
            ship.position.y = start_position.y - d;
            calc_orbital_velocity(sqrt(d * d + d * d), -45, star->radius, &ship.vx, &ship.vy);
        }
    }

    // Initialize universe sections crossings
    universe_cross_axis.x = universe_offset.current_x;
    universe_cross_axis.y = universe_offset.current_x;

    // Initialize galaxy sections crossings
    cross_axis.x = ship.position.x;
    cross_axis.y = ship.position.y;

    // Set time keeping variables
    unsigned int start_time;

    // Main loop
    while (!quit)
    {
        start_time = SDL_GetTicks();

        // Process events
        poll_events(&quit);

        // Set background color
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        // Clear the renderer
        SDL_RenderClear(renderer);

        switch (state)
        {
        case MENU:
            onMenu();
            break;
        case NAVIGATE:
            onNavigate(bstars, &ship, &camera, &navigate_offset, &universe_offset);
            break;
        case MAP:
            onMap(bstars, &ship, &camera, &map_offset, &universe_offset);
            break;
        case UNIVERSE:
            onUniverse(&ship, &camera, &scroll_offset, &universe_offset);
            break;
        case PAUSE:
            onPause(bstars, &ship, &camera, start_time);
            continue;
        default:
            onMenu();
            break;
        }

        // Switch buffers, display back buffer
        SDL_RenderPresent(renderer);

        // Set FPS
        unsigned int end_time;

        if ((1000 / FPS) > ((end_time = SDL_GetTicks()) - start_time))
            SDL_Delay((1000 / FPS) - (end_time - start_time));

        // Log FPS
        log_fps(end_time - start_time);
    }

    // Cleanup resources
    cleanup_resources(&ship, current_galaxy, buffer_galaxy, previous_galaxy);

    // Close SDL
    close_sdl();

    return 0;
}

void onMenu(void)
{
    // printf("\nonMenu");
}

void onNavigate(struct bstar_t bstars[], struct ship_t *ship, struct camera_t *camera, struct point_t *navigate_offset, struct point_state *universe_offset)
{
    if (map_exit)
    {
        // Reset stars
        if (current_galaxy->position.x != buffer_galaxy->position.x && current_galaxy->position.y != buffer_galaxy->position.y)
            cleanup_stars();

        // Reset current_galaxy from buffer_galaxy
        memcpy(current_galaxy, buffer_galaxy, sizeof(struct galaxy_t));

        // Reset universe_offset
        universe_offset->current_x = universe_offset->previous_x;
        universe_offset->current_y = universe_offset->previous_y;

        // Reset ship position
        ship->position.x = previous_ship_position.x;
        ship->position.y = previous_ship_position.y;
    }

    if (map_exit || universe_exit)
    {
        // Reset saved game_scale
        if (save_scale)
            game_scale = save_scale;
        else
            game_scale = ZOOM_NAVIGATE;

        save_scale = 0;

        // Update camera
        if (camera_on)
            update_camera(camera, ship->position);

        // Zoom in
        for (int s = 0; s < MAX_STARS; s++)
        {
            if (stars[s] != NULL && stars[s]->star != NULL)
                zoom_star(stars[s]->star);
        }
    }

    // Add small tolerance to account for floating-point precision errors
    const float epsilon = 0.0001;

    if (zoom_in)
    {
        if (game_scale + ZOOM_STEP <= ZOOM_MAX + epsilon)
        {
            // Reset scale
            game_scale += ZOOM_STEP;

            // Zoom in
            for (int s = 0; s < MAX_STARS; s++)
            {
                if (stars[s] != NULL && stars[s]->star != NULL)
                    zoom_star(stars[s]->star);
            }
        }

        zoom_in = OFF;
    }

    if (zoom_out)
    {
        if (game_scale - ZOOM_STEP >= ZOOM_NAVIGATE_MIN - epsilon)
        {
            // Reset scale
            game_scale -= ZOOM_STEP;

            // Zoom out
            for (int s = 0; s < MAX_STARS; s++)
            {
                if (stars[s] != NULL && stars[s]->star != NULL)
                    zoom_star(stars[s]->star);
            }
        }

        zoom_out = OFF;
    }

    // Generate stars
    if (camera_on)
        generate_stars(bstars, ship, navigate_offset, universe_offset, NAVIGATE);

    // Update camera
    if (camera_on)
        update_camera(camera, *navigate_offset);

    if (BSTARS_ON || GSTARS_ON || SPEED_LINES_ON)
    {
        struct speed_t speed = {.vx = ship->vx, .vy = ship->vy};
        double distance_current = find_distance(ship->position.x, ship->position.y, 0, 0);

        // Draw galaxy cloud
        if (GSTARS_ON)
        {
            struct point_t ship_position_current = {.x = ship->position.x, .y = ship->position.y};
            static double limit_current;

            if (limit_current == 0.0)
                limit_current = 2 * current_galaxy->radius * GALAXY_SCALE;

            if (galaxy_found)
            {
                limit_current = distance_current;
                galaxy_found = OFF;
            }

            update_gstars(current_galaxy, ship_position_current, camera, distance_current, limit_current);

            if (exited_galaxy && previous_galaxy != NULL && previous_galaxy->initialized)
            {
                // Convert ship position to universe coordinates
                struct point_t universe_position;
                universe_position.x = current_galaxy->position.x + ship->position.x / GALAXY_SCALE;
                universe_position.y = current_galaxy->position.y + ship->position.y / GALAXY_SCALE;

                // Convert universe coordinates to ship position relative to previous galaxy
                struct point_t ship_position_previous = {
                    .x = (universe_position.x - previous_galaxy->position.x) * GALAXY_SCALE,
                    .y = (universe_position.y - previous_galaxy->position.y) * GALAXY_SCALE};

                double distance_previous = find_distance(universe_position.x, universe_position.y, previous_galaxy->position.x, previous_galaxy->position.y);
                distance_previous *= GALAXY_SCALE;
                double limit_previous = 2 * previous_galaxy->radius * GALAXY_SCALE;

                update_gstars(previous_galaxy, ship_position_previous, camera, distance_previous, limit_previous);
            }
        }

        // Draw background stars
        if (BSTARS_ON)
            update_bstars(bstars, camera, speed, distance_current);

        // Draw speed lines
        if (SPEED_LINES_ON && camera_on)
            update_speed_lines(camera, speed);
    }

    // Draw speed tail
    if (velocity.magnitude > GALAXY_SPEED_LIMIT)
        draw_speed_arc(ship, camera);

    // Update system
    if ((!map_exit && !universe_exit && !zoom_in && !zoom_out) || game_scale > ZOOM_NAVIGATE_MIN)
    {
        for (int i = 0; i < MAX_STARS; i++)
        {
            if (stars[i] != NULL)
            {
                // Each index can have many entries, loop through all of them
                struct star_entry *entry = stars[i];

                while (entry != NULL)
                {
                    update_system(entry->star, ship, camera, *navigate_offset, NAVIGATE);
                    entry = entry->next;
                }
            }
        }
    }

    // Enforce speed limits
    if (!exited_galaxy)
    {
        if (velocity.magnitude > GALAXY_SPEED_LIMIT)
        {
            ship->vx = GALAXY_SPEED_LIMIT * ship->vx / velocity.magnitude;
            ship->vy = GALAXY_SPEED_LIMIT * ship->vy / velocity.magnitude;
        }
    }
    else
    {
        if (velocity.magnitude > UNIVERSE_SPEED_LIMIT)
        {
            ship->vx = UNIVERSE_SPEED_LIMIT * ship->vx / velocity.magnitude;
            ship->vy = UNIVERSE_SPEED_LIMIT * ship->vy / velocity.magnitude;
        }
    }

    // Update velocity
    update_velocity(ship);

    // Update ship
    update_ship(ship, camera);

    // Update coordinates
    navigate_offset->x = ship->position.x;
    navigate_offset->y = ship->position.y;

    // Check for nearest galaxy, excluding current galaxy
    if (exited_galaxy && PROJECTIONS_ON)
    {
        // Convert offset to universe coordinates
        struct point_t universe_position;
        universe_position.x = current_galaxy->position.x + ship->position.x / GALAXY_SCALE;
        universe_position.y = current_galaxy->position.y + ship->position.y / GALAXY_SCALE;

        // Calculate camera position in universe scale
        struct camera_t universe_camera = {
            .x = current_galaxy->position.x * GALAXY_SCALE + camera->x,
            .y = current_galaxy->position.y * GALAXY_SCALE + camera->y,
            .w = camera->w,
            .h = camera->h};

        struct galaxy_t *nearest_galaxy = find_nearest_galaxy(universe_position, current_galaxy, TRUE);

        if (nearest_galaxy != NULL && nearest_galaxy->position.x != current_galaxy->position.x && nearest_galaxy->position.y != current_galaxy->position.y)
        {
            // Project nearest galaxy
            project_galaxy(nearest_galaxy, &universe_camera, MAP);
        }

        // Project current galaxy
        project_galaxy(current_galaxy, &universe_camera, MAP);
    }

    // Create galaxy cloud
    if (!current_galaxy->initialized)
        create_galaxy_cloud(current_galaxy);

    // Draw screen frame
    draw_screen_frame(camera);

    // Update game console
    if (console && CONSOLE_ON)
    {
        // Log velocity magnitude (relative to 0,0)
        log_game_console(game_console_entries, V_INDEX, velocity.magnitude);

        // Log coordinates (relative to (0,0))
        log_game_console(game_console_entries, X_INDEX, navigate_offset->x);
        log_game_console(game_console_entries, Y_INDEX, navigate_offset->y);

        // Log game scale
        log_game_console(game_console_entries, SCALE_INDEX, game_scale);

        update_game_console(game_console_entries);
    }

    if (map_exit)
        map_exit = OFF;

    if (universe_exit)
        universe_exit = OFF;
}

void onMap(struct bstar_t bstars[], struct ship_t *ship, struct camera_t *camera, struct point_t *map_offset, struct point_state *universe_offset)
{
    if (map_enter)
    {
        // Save current_galaxy
        memcpy(buffer_galaxy, current_galaxy, sizeof(struct galaxy_t));

        // Save universe_offset
        universe_offset->previous_x = universe_offset->current_x;
        universe_offset->previous_y = universe_offset->current_y;

        // Save ship position
        previous_ship_position.x = ship->position.x;
        previous_ship_position.y = ship->position.y;
    }

    if (map_center)
    {
        // Reset stars
        if (current_galaxy->position.x != buffer_galaxy->position.x && current_galaxy->position.y != buffer_galaxy->position.y)
            cleanup_stars();

        // Reset buffer_galaxy to current_galaxy
        memcpy(current_galaxy, buffer_galaxy, sizeof(struct galaxy_t));

        // Reset universe_offset
        universe_offset->current_x = universe_offset->previous_x;
        universe_offset->current_y = universe_offset->previous_y;

        // Reset ship position
        ship->position.x = previous_ship_position.x;
        ship->position.y = previous_ship_position.y;
    }

    if (map_enter || map_center)
    {
        if (map_enter && !save_scale)
            save_scale = game_scale;

        game_scale = ZOOM_MAP;

        // Zoom in
        for (int s = 0; s < MAX_STARS; s++)
        {
            if (stars[s] != NULL && stars[s]->star != NULL)
                zoom_star(stars[s]->star);
        }

        // Reset ship position
        map_offset->x = previous_ship_position.x;
        map_offset->y = previous_ship_position.y;

        // Update camera
        update_camera(camera, *map_offset);

        if (map_center)
            map_center = OFF;
    }

    // Add small tolerance to account for floating-point precision errors
    const float epsilon = 0.0001;
    float zoom_step = ZOOM_STEP;

    if (zoom_in)
    {
        if (game_scale + epsilon < 0.01)
            zoom_step = 0.001;

        if (game_scale + zoom_step <= ZOOM_MAX + epsilon)
        {
            // Reset scale
            game_scale += zoom_step;

            // Reset region_size
            if (game_scale + epsilon >= 0.01)
                galaxy_region_size = GALAXY_REGION_SIZE;

            // Zoom in
            for (int s = 0; s < MAX_STARS; s++)
            {
                if (stars[s] != NULL && stars[s]->star != NULL)
                    zoom_star(stars[s]->star);
            }
        }

        zoom_in = OFF;
    }

    if (zoom_out)
    {
        if (game_scale - epsilon <= 0.01)
            zoom_step = 0.001;

        if (game_scale - zoom_step >= ZOOM_MAP_MIN - epsilon)
        {
            // Reset scale
            game_scale -= zoom_step;

            // Reset region_size
            if (game_scale + epsilon < 0.01)
            {
                galaxy_region_size = GALAXY_REGION_SIZE_MAX;

                // Generate new stars for new region size
                stars_start = 1;
            }

            // Zoom out
            for (int s = 0; s < MAX_STARS; s++)
            {
                if (stars[s] != NULL && stars[s]->star != NULL)
                    zoom_star(stars[s]->star);
            }
        }

        zoom_out = OFF;
    }

    // Generate stars
    generate_stars(bstars, ship, map_offset, universe_offset, MAP);

    // Update camera
    update_camera(camera, *map_offset);

    // Check for nearest galaxy, excluding current galaxy
    if (exited_galaxy && PROJECTIONS_ON)
    {
        // Convert offset to universe coordinates
        struct point_t universe_position;
        universe_position.x = current_galaxy->position.x + map_offset->x / GALAXY_SCALE;
        universe_position.y = current_galaxy->position.y + map_offset->y / GALAXY_SCALE;

        // Calculate camera position in universe scale
        struct camera_t universe_camera = {
            .x = current_galaxy->position.x * GALAXY_SCALE + camera->x,
            .y = current_galaxy->position.y * GALAXY_SCALE + camera->y,
            .w = camera->w,
            .h = camera->h};

        struct galaxy_t *nearest_galaxy = find_nearest_galaxy(universe_position, current_galaxy, TRUE);

        if (nearest_galaxy != NULL && nearest_galaxy->position.x != current_galaxy->position.x && nearest_galaxy->position.y != current_galaxy->position.y)
        {
            // Project nearest galaxy
            project_galaxy(nearest_galaxy, &universe_camera, MAP);
        }

        // Project current galaxy
        project_galaxy(current_galaxy, &universe_camera, MAP);
    }

    // Create galaxy cloud
    if (!current_galaxy->initialized)
        create_galaxy_cloud(current_galaxy);

    // Move through map
    float rate_x = 0;

    if (right)
        rate_x = MAP_SPEED_MIN + (MAP_SPEED_MAX - MAP_SPEED_MIN) * (camera->w / 1000) / game_scale;
    else if (left)
        rate_x = -(MAP_SPEED_MIN + (MAP_SPEED_MAX - MAP_SPEED_MIN) * (camera->w / 1000) / game_scale);

    map_offset->x += rate_x;

    float rate_y = 0;

    if (down)
        rate_y = MAP_SPEED_MIN + (MAP_SPEED_MAX - MAP_SPEED_MIN) * (camera->w / 1000) / game_scale;
    else if (up)
        rate_y = -(MAP_SPEED_MIN + (MAP_SPEED_MAX - MAP_SPEED_MIN) * (camera->w / 1000) / game_scale);

    map_offset->y += rate_y;

    // Update system
    if ((!map_enter && !zoom_in && !zoom_out) || game_scale > 0)
    {
        for (int i = 0; i < MAX_STARS; i++)
        {
            if (stars[i] != NULL)
            {
                // Each index can have many entries, loop through all of them
                struct star_entry *entry = stars[i];

                while (entry != NULL)
                {
                    update_system(entry->star, ship, camera, *map_offset, MAP);
                    entry = entry->next;
                }
            }
        }
    }

    // Draw ship projection
    ship->projection->rect.x = (ship->position.x - map_offset->x) * game_scale + (camera->w / 2 - ship->projection->radius);
    ship->projection->rect.y = (ship->position.y - map_offset->y) * game_scale + (camera->h / 2 - ship->projection->radius);
    ship->projection->angle = ship->angle;

    if (ship->projection->rect.x + ship->projection->radius < 0 ||
        ship->projection->rect.x + ship->projection->radius > camera->w ||
        ship->projection->rect.y + ship->projection->radius < 0 ||
        ship->projection->rect.y + ship->projection->radius > camera->h)
    {
        project_ship(ship, camera, MAP);
    }
    else
        SDL_RenderCopyEx(renderer, ship->projection->texture, &ship->projection->main_img_rect, &ship->projection->rect, ship->projection->angle, &ship->projection->rotation_pt, SDL_FLIP_NONE);

    // Draw section lines
    SDL_Color color = {255, 165, 0, 32};
    draw_section_lines(camera, GALAXY_SECTION_SIZE, color);

    // Draw galaxy radius circle
    int radius = current_galaxy->radius * GALAXY_SCALE * game_scale;
    int cx = -camera->x * game_scale;
    int cy = -camera->y * game_scale;
    SDL_Color radius_color = {0, 255, 255, 70};
    SDL_DrawCircleApprox(renderer, camera, cx, cy, radius, radius_color);

    // Draw cross at position
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);
    SDL_RenderDrawLine(renderer, (camera->w / 2) - 7, camera->h / 2, (camera->w / 2) + 7, camera->h / 2);
    SDL_RenderDrawLine(renderer, camera->w / 2, (camera->h / 2) - 7, camera->w / 2, (camera->h / 2) + 7);

    // Draw screen frame
    draw_screen_frame(camera);

    // Update game console
    if (console && CONSOLE_ON)
    {
        // Log coordinates (relative to (0,0))
        log_game_console(game_console_entries, X_INDEX, map_offset->x);
        log_game_console(game_console_entries, Y_INDEX, map_offset->y);

        // Log game scale
        log_game_console(game_console_entries, SCALE_INDEX, game_scale);

        update_game_console(game_console_entries);
    }

    if (map_enter)
        map_enter = OFF;
}

void onUniverse(struct ship_t *ship, struct camera_t *camera, struct point_t *scroll_offset, struct point_state *universe_offset)
{
    if (map_exit)
    {
        // Reset stars
        if (current_galaxy->position.x != buffer_galaxy->position.x && current_galaxy->position.y != buffer_galaxy->position.y)
            cleanup_stars();

        // Reset current_galaxy from buffer_galaxy
        memcpy(current_galaxy, buffer_galaxy, sizeof(struct galaxy_t));

        // Reset universe_offset
        universe_offset->current_x = universe_offset->previous_x;
        universe_offset->current_y = universe_offset->previous_y;

        // Reset ship position
        ship->position.x = previous_ship_position.x;
        ship->position.y = previous_ship_position.y;
    }

    if (universe_enter || universe_center)
    {
        // Generate galaxies
        struct point_t offset = {.x = universe_offset->current_x, .y = universe_offset->current_y};
        generate_galaxies(offset);

        if (universe_enter && !save_scale)
            save_scale = game_scale;

        game_scale = ZOOM_UNIVERSE;

        // Reset position in universe
        scroll_offset->x = universe_offset->current_x + ship->position.x / GALAXY_SCALE;
        scroll_offset->y = universe_offset->current_y + ship->position.y / GALAXY_SCALE;

        // Update camera
        update_camera(camera, *scroll_offset);

        if (universe_center)
            universe_center = OFF;
    }
    else
        // Generate galaxies
        generate_galaxies(*scroll_offset);

    if (!universe_enter || game_scale > 0)
    {
        // Update universe
        for (int i = 0; i < MAX_GALAXIES; i++)
        {
            if (galaxies[i] != NULL)
            {
                // Each index can have many entries, loop through all of them
                struct galaxy_entry *entry = galaxies[i];

                while (entry != NULL)
                {
                    update_galaxy(entry->galaxy, camera, *scroll_offset);
                    entry = entry->next;
                }
            }
        }
    }

    // Move through map
    float rate_x = 0;

    if (right)
        rate_x = MAP_SPEED_MIN + (MAP_SPEED_MAX - MAP_SPEED_MIN) * (camera->w / 1000) / game_scale;
    else if (left)
        rate_x = -(MAP_SPEED_MIN + (MAP_SPEED_MAX - MAP_SPEED_MIN) * (camera->w / 1000) / game_scale);

    scroll_offset->x += rate_x;

    float rate_y = 0;

    if (down)
        rate_y = MAP_SPEED_MIN + (MAP_SPEED_MAX - MAP_SPEED_MIN) * (camera->w / 1000) / game_scale;
    else if (up)
        rate_y = -(MAP_SPEED_MIN + (MAP_SPEED_MAX - MAP_SPEED_MIN) * (camera->w / 1000) / game_scale);

    scroll_offset->y += rate_y;

    // Add small tolerance to account for floating-point precision errors
    const float epsilon = 0.0001;

    if (zoom_in)
    {
        if (game_scale + ZOOM_STEP <= ZOOM_MAX + epsilon)
        {
            // Reset scale
            game_scale += ZOOM_STEP;
        }

        zoom_in = OFF;
    }

    if (zoom_out)
    {
        if (game_scale - ZOOM_STEP >= ZOOM_MAP_MIN - epsilon)
        {
            // Reset scale
            game_scale -= ZOOM_STEP;
        }

        zoom_out = OFF;
    }

    // Update camera
    update_camera(camera, *scroll_offset);

    // Draw ship projection
    ship->projection->rect.x = (universe_offset->current_x + ship->position.x / GALAXY_SCALE - camera->x) * game_scale - SHIP_PROJECTION_RADIUS;
    ship->projection->rect.y = (universe_offset->current_y + ship->position.y / GALAXY_SCALE - camera->y) * game_scale - SHIP_PROJECTION_RADIUS;
    ship->projection->angle = ship->angle;

    if (ship->projection->rect.x + ship->projection->radius < 0 ||
        ship->projection->rect.x + ship->projection->radius > camera->w ||
        ship->projection->rect.y + ship->projection->radius < 0 ||
        ship->projection->rect.y + ship->projection->radius > camera->h)
    {
        project_ship(ship, camera, UNIVERSE);
    }
    else
        SDL_RenderCopyEx(renderer, ship->projection->texture, &ship->projection->main_img_rect, &ship->projection->rect, ship->projection->angle, &ship->projection->rotation_pt, SDL_FLIP_NONE);

    // Draw section lines
    SDL_Color color = {255, 255, 255, 25};
    draw_section_lines(camera, UNIVERSE_SECTION_SIZE, color);

    // Draw cross at position
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);
    SDL_RenderDrawLine(renderer, (camera->w / 2) - 7, camera->h / 2, (camera->w / 2) + 7, camera->h / 2);
    SDL_RenderDrawLine(renderer, camera->w / 2, (camera->h / 2) - 7, camera->w / 2, (camera->h / 2) + 7);

    // Draw screen frame
    draw_screen_frame(camera);

    // Update game console
    if (console && CONSOLE_ON)
    {
        // Log coordinates (relative to (0,0))
        log_game_console(game_console_entries, X_INDEX, scroll_offset->x);
        log_game_console(game_console_entries, Y_INDEX, scroll_offset->y);

        // Log game scale
        log_game_console(game_console_entries, SCALE_INDEX, game_scale);

        update_game_console(game_console_entries);
    }

    if (map_exit)
        map_exit = OFF;

    if (universe_enter)
        universe_enter = OFF;
}

void onPause(struct bstar_t bstars[], struct ship_t *ship, const struct camera_t *camera, unsigned int start_time)
{
    // Update game console
    if (console)
        update_game_console(game_console_entries);

    // Draw background stars
    if (BSTARS_ON)
    {
        struct speed_t speed = {.vx = 0, .vy = 0};
        double distance = find_distance(ship->position.x, ship->position.y, 0, 0);
        update_bstars(bstars, camera, speed, distance);
    }

    // Switch buffers, display back buffer
    SDL_RenderPresent(renderer);

    // Set FPS
    unsigned int end_time;

    if ((1000 / FPS) > ((end_time = SDL_GetTicks()) - start_time))
        SDL_Delay((1000 / FPS) - (end_time - start_time));

    // Log FPS
    log_fps(end_time - start_time);
}

/*
 * Probe region for stars and create them procedurally.
 * The region has intervals of size GALAXY_SECTION_SIZE.
 * The function checks for galaxy boundaries and switches to a new galaxy if close enough.
 */
void generate_stars(struct bstar_t bstars[], struct ship_t *ship, struct point_t *offset, struct point_state *universe_offset, int state)
{
    // Keep track of current nearest section axis coordinates
    double bx = find_nearest_section_axis(offset->x, GALAXY_SECTION_SIZE);
    double by = find_nearest_section_axis(offset->y, GALAXY_SECTION_SIZE);

    // Check if this is the first time calling this function
    if (!stars_start)
    {
        // Check whether nearest section axis have changed
        if (bx == cross_axis.x && by == cross_axis.y)
            return;

        // Keep track of new axis
        if (bx != cross_axis.x)
            cross_axis.x = bx;

        if (by != cross_axis.y)
            cross_axis.y = by;
    }

    // Define a region of galaxy_region_size * galaxy_region_size
    // bx,by are at the center of this area
    double ix, iy;
    double left_boundary = bx - ((galaxy_region_size / 2) * GALAXY_SECTION_SIZE);
    double right_boundary = bx + ((galaxy_region_size / 2) * GALAXY_SECTION_SIZE);
    double top_boundary = by - ((galaxy_region_size / 2) * GALAXY_SECTION_SIZE);
    double bottom_boundary = by + ((galaxy_region_size / 2) * GALAXY_SECTION_SIZE);

    // Add a buffer zone of <galaxy_region_size> sections beyond galaxy radius
    int radius_plus_buffer = (current_galaxy->radius * GALAXY_SCALE) + galaxy_region_size * GALAXY_SECTION_SIZE;
    int in_horizontal_bounds = left_boundary > -radius_plus_buffer && right_boundary < radius_plus_buffer;
    int in_vertical_bounds = top_boundary > -radius_plus_buffer && bottom_boundary < radius_plus_buffer;

    // If exited galaxy, check for closest galaxy, including current galaxy
    if (sqrt(offset->x * offset->x + offset->y * offset->y) > current_galaxy->radius * GALAXY_SCALE)
    {
        exited_galaxy = ON;

        // Convert offset to universe coordinates
        struct point_t universe_position;
        universe_position.x = current_galaxy->position.x + offset->x / GALAXY_SCALE;
        universe_position.y = current_galaxy->position.y + offset->y / GALAXY_SCALE;

        // Convert to cross section offset to query for new galaxies
        struct point_t cross_section_offset;
        cross_section_offset.x = find_nearest_section_axis(universe_position.x, UNIVERSE_SECTION_SIZE);
        cross_section_offset.y = find_nearest_section_axis(universe_position.y, UNIVERSE_SECTION_SIZE);
        generate_galaxies(cross_section_offset);

        // Search for nearest galaxy to universe_position, including current galaxy
        struct galaxy_t *next_galaxy = find_nearest_galaxy(universe_position, current_galaxy, FALSE);

        // Found a new galaxy
        if (next_galaxy != NULL && next_galaxy->position.x != current_galaxy->position.x && next_galaxy->position.y != current_galaxy->position.y)
        {
            galaxy_found = ON;

            // Update previous_galaxy
            memcpy(previous_galaxy, current_galaxy, sizeof(struct galaxy_t));

            // Update current_galaxy
            memcpy(current_galaxy, next_galaxy, sizeof(struct galaxy_t));

            // Get coordinates of current position relative to new galaxy
            double angle = atan2(universe_position.y - next_galaxy->position.y, universe_position.x - next_galaxy->position.x);
            double d = find_distance(universe_position.x, universe_position.y, next_galaxy->position.x, next_galaxy->position.y);
            double px = d * cos(angle) * GALAXY_SCALE;
            double py = d * sin(angle) * GALAXY_SCALE;

            // Update universe_offset
            universe_offset->current_x = next_galaxy->position.x;
            universe_offset->current_y = next_galaxy->position.y;

            if (state == NAVIGATE)
            {
                // Update ship coordinates
                ship->position.x = px;
                ship->position.y = py;

                // Update offset
                offset->x = px;
                offset->y = py;

                // Permanently in a new galaxy, don't need previous universe_offset anymore
                universe_offset->previous_x = universe_offset->current_x;
                universe_offset->previous_y = universe_offset->current_y;

                // Create new background stars
                int max_bstars = (int)(display_mode.w * display_mode.h * BSTARS_PER_SQUARE / BSTARS_SQUARE);

                for (int i = 0; i < max_bstars; i++)
                {
                    bstars[i].final_star = 0;
                }

                create_bstars(bstars, max_bstars);
            }
            else if (state == MAP)
            {
                // Update offset
                offset->x = px;
                offset->y = py;

                // Update ship position so that it always points to original location
                // First find absolute coordinates for original ship position in universe scale
                double src_ship_position_x = universe_offset->previous_x + previous_ship_position.x / GALAXY_SCALE;
                double src_ship_position_y = universe_offset->previous_y + previous_ship_position.y / GALAXY_SCALE;
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
            cleanup_stars();

            return;
        }
    }
    else
        exited_galaxy = OFF;

    // Use a local rng
    pcg32_random_t rng;

    // Density scaling parameter
    double a = current_galaxy->radius * GALAXY_SCALE / 2.0f;

    for (ix = left_boundary; ix < right_boundary && in_horizontal_bounds; ix += GALAXY_SECTION_SIZE)
    {
        for (iy = top_boundary; iy < bottom_boundary && in_vertical_bounds; iy += GALAXY_SECTION_SIZE)
        {
            // Check that point is within galaxy radius
            double distance_from_center = sqrt(ix * ix + iy * iy);

            if (distance_from_center > (current_galaxy->radius * GALAXY_SCALE))
                continue;

            // Create rng seed by combining x,y values
            struct point_t position = {.x = ix, .y = iy};
            uint64_t seed = pair_hash_order_sensitive(position);

            // Set galaxy hash as initseq
            initseq = pair_hash_order_sensitive_2(current_galaxy->position);

            // Seed with a fixed constant
            pcg32_srandom_r(&rng, seed, initseq);

            // Calculate density based on distance from center
            float density = (GALAXY_DENSITY / pow((distance_from_center / a + 1), 2));

            int has_star = abs(pcg32_random_r(&rng)) % 1000 < density;

            if (has_star)
            {
                // Check whether star exists in hash table
                if (star_exists(position))
                    continue;
                else
                {
                    // Create star
                    struct planet_t *star = create_star(position);

                    // Add star to hash table
                    put_star(position, star);
                }
            }
        }
    }

    // Delete stars that end up outside the region
    for (int s = 0; s < MAX_STARS; s++)
    {
        struct star_entry *entry = stars[s];

        while (entry != NULL)
        {
            struct point_t position = {.x = entry->x, .y = entry->y};

            // Get distance from center of region
            double dx = position.x - bx;
            double dy = position.y - by;
            double distance = sqrt(dx * dx + dy * dy);
            double region_radius = sqrt((double)2 * ((galaxy_region_size + 1) / 2) * GALAXY_SECTION_SIZE * ((galaxy_region_size + 1) / 2) * GALAXY_SECTION_SIZE);

            // If star outside region, delete it
            if (distance >= region_radius)
                delete_star(position);

            entry = entry->next;
        }
    }

    // First star generation complete
    stars_start = 0;
}

/*
 * Probe region for galaxies and create them procedurally.
 * The region has intervals of size UNIVERSE_SECTION_SIZE.
 */
void generate_galaxies(struct point_t offset)
{
    // Keep track of current nearest section axis coordinates
    double bx = find_nearest_section_axis(offset.x, UNIVERSE_SECTION_SIZE);
    double by = find_nearest_section_axis(offset.y, UNIVERSE_SECTION_SIZE);

    // Check if this is the first time calling this function
    if (!galaxies_start)
    {
        // Check whether nearest section axis have changed
        if (bx == universe_cross_axis.x && by == universe_cross_axis.y)
            return;

        // Keep track of new axis
        if (bx != universe_cross_axis.x)
            universe_cross_axis.x = bx;

        if (by != universe_cross_axis.y)
            universe_cross_axis.y = by;
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
                if (galaxy_exists(position))
                    continue;
                else
                {
                    // Create galaxy
                    struct galaxy_t *galaxy = create_galaxy(position);

                    // Add galaxy to hash table
                    put_galaxy(position, galaxy);
                }
            }
        }
    }

    // Delete galaxies that end up outside the region
    for (int s = 0; s < MAX_GALAXIES; s++)
    {
        if (galaxies[s] != NULL)
        {
            struct point_t position = {.x = galaxies[s]->x, .y = galaxies[s]->y};

            // Skip current galaxy, otherwise we lose track of where we are
            if (!galaxies_start)
            {
                if (position.x == current_galaxy->position.x && position.y == current_galaxy->position.y)
                    continue;
            }

            // Get distance from center of region
            double dx = position.x - bx;
            double dy = position.y - by;
            double distance = sqrt(dx * dx + dy * dy);
            double region_radius = sqrt((double)2 * ((UNIVERSE_REGION_SIZE + 1) / 2) * UNIVERSE_SECTION_SIZE * ((UNIVERSE_REGION_SIZE + 1) / 2) * UNIVERSE_SECTION_SIZE);

            // If galaxy outside region, delete it
            if (distance >= region_radius)
                delete_galaxy(position);
        }
    }

    // First galaxy generation complete
    galaxies_start = 0;
}

void create_bstars(struct bstar_t bstars[], int max_bstars)
{
    int i = 0, row, column, is_star;
    int end = FALSE;

    // Use a local rng
    pcg32_random_t rng;

    // Seed with a fixed constant
    srand(1200);

    for (row = 0; row < display_mode.h && !end; row++)
    {
        for (column = 0; column < display_mode.w && !end; column++)
        {
            // Create rng seed by combining x,y values
            struct point_t position = {.x = row, .y = column};
            uint64_t seed = pair_hash_order_sensitive(position);

            // Set galaxy hash as initseq
            initseq = pair_hash_order_sensitive_2(current_galaxy->position);

            // Seed with a fixed constant
            pcg32_srandom_r(&rng, seed, initseq);

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

                // Get a color between 15 - 165
                star.opacity = ((rand() % 166) + 15);

                star.final_star = 1;
                bstars[i++] = star;
            }

            if (i >= max_bstars)
                end = TRUE;
        }
    }
}

/*
 * Move and draw speed lines.
 */
void update_speed_lines(const struct camera_t *camera, struct speed_t speed)
{
    // Check if velocity magnitude is zero
    if (velocity.magnitude == 0)
        return;

    // Define constants
    SDL_Color color = {255, 255, 255};
    const int num_lines = SPEED_LINES_NUM;
    const int max_length = 100; // Final max length will be 4/6 of this length
    const int line_distance = 120;
    const int base_speed = BASE_SPEED_LIMIT;
    const float max_speed = 2.5 * base_speed;
    const int speed_limit = 6 * BASE_SPEED_LIMIT;
    const float opacity_exponent = 1.5;

    // Calculate opacity based on velocity
    float opacity = 0.0;
    float base_opacity = 60.0;

    if (velocity.magnitude < 2.0 * base_speed)
    {
        opacity = base_opacity * (1.0 - exp(-3.0 * (velocity.magnitude / base_speed)));
    }
    else if (velocity.magnitude >= 2.0 * base_speed && velocity.magnitude < 3.0 * base_speed)
    {
        float opacity_ratio = (velocity.magnitude - 2.0 * base_speed) / (base_speed);
        opacity = 60.0 * exp(-1.0 * opacity_ratio);
        opacity = fmax(opacity, 30.0);
    }
    else
    {
        opacity = 30.0;
    }

    int final_opacity = (int)round(opacity); // Round the opacity to the nearest integer

    // Calculate the normalized velocity vector
    float velocity_x = speed.vx / velocity.magnitude;
    float velocity_y = speed.vy / velocity.magnitude;

    static float line_start_x[SPEED_LINES_NUM][SPEED_LINES_NUM], line_start_y[SPEED_LINES_NUM][SPEED_LINES_NUM];
    static int initialized = FALSE;
    if (!initialized)
    {
        // Initialize starting positions of lines
        float start_x = -(num_lines / 2) * line_distance;
        float start_y = -(num_lines / 2) * line_distance;
        for (int row = 0; row < num_lines; row++)
        {
            for (int col = 0; col < num_lines; col++)
            {
                if (row % 2 == 0)
                {
                    line_start_x[row][col] = camera->w / 2 + line_distance / 2 + start_x + col * line_distance;
                }
                else
                    line_start_x[row][col] = camera->w / 2 + line_distance + start_x + col * line_distance;

                line_start_y[row][col] = camera->h / 2 + line_distance + start_y + row * line_distance;
            }
        }
        initialized = TRUE;
    }

    for (int row = 0; row < num_lines; row++)
    {
        for (int col = 0; col < num_lines; col++)
        {
            float x = line_start_x[row][col];
            float y = line_start_y[row][col];

            // Calculate the starting position
            float start_x = x - velocity_x;
            float start_y = y - velocity_y;

            // Calculate the ending position based on the magnitude of the velocity vector
            float speed_ray_length;

            if (velocity.magnitude >= speed_limit)
            {
                speed_ray_length = (float)4 * max_length / 6;
            }
            else
            {
                if (velocity.magnitude < 2 * base_speed)
                    speed_ray_length = 1;
                else
                    speed_ray_length = max_length * (velocity.magnitude - 2 * base_speed) / speed_limit;
            }

            if (speed_ray_length > max_length)
                speed_ray_length = max_length;

            float end_x = x + velocity_x * speed_ray_length;
            float end_y = y + velocity_y * speed_ray_length;

            // Calculate opacity based on start point distance from center
            float dist_x = start_x - camera->w / 2;
            float dist_y = start_y - camera->h / 2;
            float max_distance = sqrt(2 * (SPEED_LINES_NUM / 2) * line_distance * (SPEED_LINES_NUM / 2) * line_distance);
            float opacity_factor = 1 - (sqrt(dist_x * dist_x + dist_y * dist_y) / max_distance);
            int scaled_opacity = (int)final_opacity * pow(opacity_factor, opacity_exponent);

            if (scaled_opacity < 0)
                scaled_opacity = 0;
            else if (scaled_opacity > base_opacity)
                scaled_opacity = base_opacity;

            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, scaled_opacity);

            // Draw the speed line
            SDL_RenderDrawLine(renderer, (int)start_x, (int)start_y, (int)end_x, (int)end_y);

            // Update the starting position of the line based on the velocity magnitude
            float delta_x;
            float delta_y;

            if (velocity.magnitude > speed_limit)
            {
                delta_x = max_speed * velocity_x / FPS;
                delta_y = max_speed * velocity_y / FPS;
            }
            else
            {
                delta_x = max_speed * velocity_x * (velocity.magnitude / speed_limit) / FPS;
                delta_y = max_speed * velocity_y * (velocity.magnitude / speed_limit) / FPS;
            }

            // Move the line in the opposite direction
            line_start_x[row][col] -= delta_x;
            line_start_y[row][col] -= delta_y;

            // Wrap around to the other side of the screen if the line goes off-screen
            if (line_start_x[row][col] < camera->w / 2 - (num_lines / 2) * line_distance)
                line_start_x[row][col] += line_distance * num_lines;
            if (line_start_y[row][col] < camera->h / 2 - (num_lines / 2) * line_distance)
                line_start_y[row][col] += line_distance * num_lines;
            if (line_start_x[row][col] >= camera->w / 2 + (num_lines / 2) * line_distance)
                line_start_x[row][col] -= line_distance * num_lines;
            if (line_start_y[row][col] >= camera->h / 2 + (num_lines / 2) * line_distance)
                line_start_y[row][col] -= line_distance * num_lines;
        }
    }
}

/*
 * Move and draw background stars.
 */
void update_bstars(struct bstar_t bstars[], const struct camera_t *camera, struct speed_t speed, double distance)
{
    int i = 0;
    int max_bstars = (int)(display_mode.w * display_mode.h * BSTARS_PER_SQUARE / BSTARS_SQUARE);

    while (i < max_bstars && bstars[i].final_star == 1)
    {
        if (camera_on)
        {
            // Don't move background stars faster than BSTARS_MAX_SPEED
            if (velocity.magnitude > BSTARS_MAX_SPEED)
            {
                bstars[i].position.x -= BSTARS_SPEED_FACTOR * (BSTARS_MAX_SPEED * speed.vx / velocity.magnitude) / FPS;
                bstars[i].position.y -= BSTARS_SPEED_FACTOR * (BSTARS_MAX_SPEED * speed.vy / velocity.magnitude) / FPS;
            }
            else
            {
                bstars[i].position.x -= BSTARS_SPEED_FACTOR * speed.vx / FPS;
                bstars[i].position.y -= BSTARS_SPEED_FACTOR * speed.vy / FPS;
            }

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

            bstars[i].rect.x = (int)(bstars[i].position.x);
            bstars[i].rect.y = (int)(bstars[i].position.y);
        }

        // Update opacity with game_scale and fade as we move away from galaxy center.
        float max_distance = 1.5 * current_galaxy->radius;
        float opacity = (double)bstars[i].opacity * (1 - pow(1 - game_scale, 2)) * (1 - (distance / (max_distance * GALAXY_SCALE)));

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
    float min_opacity_factor = 0.4;
    float max_opacity_factor = 0.6;
    float scaling_factor = galaxy->class / 2;
    float galaxy_radius = galaxy->radius * GALAXY_SCALE;

    while (i < MAX_GSTARS && galaxy->gstars[i].final_star == 1)
    {
        // Calculate position in galaxy
        float delta_x = ship_position.x / (galaxy->cutoff * GALAXY_SCALE);
        float delta_y = ship_position.y / (galaxy->cutoff * GALAXY_SCALE);

        int x = (galaxy->gstars[i].position.x / GSTARS_SCALE) / scaling_factor + camera->w / 2 - delta_x * (camera->w / 2);
        int y = (galaxy->gstars[i].position.y / GSTARS_SCALE) / scaling_factor + camera->h / 2 - delta_y * (camera->h / 2);

        float opacity;

        if (distance > limit)
        {
            opacity = 0;
        }
        else if (distance <= limit && distance > galaxy_radius)
        {
            // Fade in opacity as we move in towards galaxy radius
            opacity = (float)galaxy->gstars[i].opacity * max_opacity_factor * (limit - distance) / (limit - galaxy_radius);
            opacity = opacity < 0 ? 0 : opacity;
        }
        else if (distance <= galaxy_radius)
        {
            // Fade out opacity as we move towards galaxy center
            opacity = max_opacity_factor * (float)galaxy->gstars[i].opacity * (min_opacity_factor + max_opacity_factor * (distance / galaxy_radius));
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
void update_camera(struct camera_t *camera, struct point_t position)
{
    camera->x = position.x - (camera->w / 2) / game_scale;
    camera->y = position.y - (camera->h / 2) / game_scale;
}

/*
 * Create a ship.
 */
struct ship_t create_ship(int radius, struct point_t position)
{
    struct ship_t ship;

    ship.image = "../assets/sprites/ship.png";
    ship.radius = radius;
    ship.position.x = (int)position.x;
    ship.position.y = (int)position.y;
    ship.vx = 0.0;
    ship.vy = 0.0;
    ship.angle = 0.0;
    SDL_Surface *surface = IMG_Load(ship.image);
    ship.texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    ship.rect.x = ship.position.x * game_scale - ship.radius;
    ship.rect.y = ship.position.y * game_scale - ship.radius;
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
    float distance = nearest_galaxy_center_distance(position);

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
    sprintf(galaxy->name, "%s-%lu", "G", index);
    galaxy->class = get_galaxy_class(distance);
    galaxy->radius = radius;
    galaxy->cutoff = UNIVERSE_SECTION_SIZE * class / 2;
    galaxy->position.x = position.x;
    galaxy->position.y = position.y;
    galaxy->color.r = 255;
    galaxy->color.g = 255;
    galaxy->color.b = 255;

    for (int i = 0; i < MAX_GSTARS; i++)
    {
        galaxy->gstars[i].final_star = 0;
    }

    return galaxy;
}

/*
 * Create a star.
 */
struct planet_t *create_star(struct point_t position)
{
    // Find distance to nearest star
    float distance = nearest_star_distance(position, current_galaxy, initseq);

    // Get star class
    int class = get_star_class(distance);

    // Use a local rng
    pcg32_random_t rng;

    // Create rng seed by combining x,y values
    uint64_t seed = pair_hash_order_sensitive(position);

    // Seed with a fixed constant
    pcg32_srandom_r(&rng, seed, initseq);

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
    SDL_Surface *surface = IMG_Load(star->image);
    star->texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    star->rect.x = (star->position.x - star->radius) * game_scale;
    star->rect.y = (star->position.y - star->radius) * game_scale;
    star->rect.w = 2 * star->radius * game_scale;
    star->rect.h = 2 * star->radius * game_scale;
    star->color.r = 255;
    star->color.g = 255;
    star->color.b = 0;
    star->planets[0] = NULL;
    star->parent = NULL;
    star->level = LEVEL_STAR;

    return star;
}

/*
 * Create a system (recursive). Takes a pointer to a star or planet
 * and populates it with children planets.
 */
void create_system(struct planet_t *planet, struct point_t position, pcg32_random_t rng)
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
                _planet->color.r = 135;
                _planet->color.g = 206;
                _planet->color.b = 235;
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
                _planet->rect.x = (_planet->position.x - _planet->radius) * game_scale;
                _planet->rect.y = (_planet->position.y - _planet->radius) * game_scale;
                _planet->rect.w = 2 * _planet->radius * game_scale;
                _planet->rect.h = 2 * _planet->radius * game_scale;
                _planet->planets[0] = NULL;
                _planet->parent = planet;

                planet->planets[i] = _planet;
                planet->planets[i + 1] = NULL;
                i++;

                create_system(_planet, position, rng);
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
                _planet->color.r = 220;
                _planet->color.g = 220;
                _planet->color.b = 220;
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
                _planet->rect.x = (_planet->position.x - _planet->radius) * game_scale;
                _planet->rect.y = (_planet->position.y - _planet->radius) * game_scale;
                _planet->rect.w = 2 * _planet->radius * game_scale;
                _planet->rect.h = 2 * _planet->radius * game_scale;
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
void update_galaxy(struct galaxy_t *galaxy, const struct camera_t *camera, struct point_t position)
{
    // Get galaxy distance from position
    float delta_x = galaxy->position.x - position.x;
    float delta_y = galaxy->position.y - position.y;
    float distance = sqrt(delta_x * delta_x + delta_y * delta_y);

    // Draw radius circle
    if (distance < galaxy->cutoff)
    {
        int radius = galaxy->radius * game_scale;
        int rx = (galaxy->position.x - camera->x) * game_scale;
        int ry = (galaxy->position.y - camera->y) * game_scale;
        SDL_Color radius_color = {255, 0, 255, 80};
        SDL_DrawCircle(renderer, camera, rx, ry, radius, radius_color);
    }

    // Draw galaxy cloud
    if (in_camera(camera, galaxy->position.x, galaxy->position.y, galaxy->radius))
    {
        if (!galaxy->initialized)
            create_galaxy_cloud(galaxy);

        draw_galaxy_cloud(galaxy, camera, galaxy->initialized);
    }
    // Draw galaxy projection
    else if (PROJECTIONS_ON)
        project_galaxy(galaxy, camera, state);
}

/*
 * Create, update, draw star system and apply gravity to planets and ship (recursive).
 */
void update_system(struct planet_t *planet, struct ship_t *ship, const struct camera_t *camera, struct point_t position, int state)
{
    // Update planets & moons
    if (planet->level != LEVEL_STAR)
    {
        float distance;
        float orbit_opacity;

        if (state == NAVIGATE)
        {
            // Update planet position
            planet->position.x += planet->parent->dx;
            planet->position.y += planet->parent->dy;

            // Find distance from parent
            float delta_x = planet->parent->position.x - planet->position.x;
            float delta_y = planet->parent->position.y - planet->position.y;
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
        else if (state == MAP)
        {
            // Find distance from parent
            float delta_x = planet->parent->position.x - planet->position.x;
            float delta_y = planet->parent->position.y - planet->position.y;
            distance = sqrt(delta_x * delta_x + delta_y * delta_y);

            orbit_opacity = 32;
        }

        // Draw orbit
        if (orbits_on)
        {
            int radius = distance * game_scale;
            int _x = (planet->parent->position.x - camera->x) * game_scale;
            int _y = (planet->parent->position.y - camera->y) * game_scale;
            SDL_Color orbit_color = {255, 255, 255, orbit_opacity};

            SDL_DrawCircle(renderer, camera, _x, _y, radius, orbit_color);
        }

        // Update moons
        int max_planets = MAX_MOONS;

        for (int i = 0; i < max_planets && planet->planets[i] != NULL; i++)
        {
            update_system(planet->planets[i], ship, camera, position, state);
        }
    }
    else if (planet->level == LEVEL_STAR)
    {
        // Get star distance from position
        float delta_x_star = planet->position.x - position.x;
        float delta_y_star = planet->position.y - position.y;
        float distance_star = sqrt(delta_x_star * delta_x_star + delta_y_star * delta_y_star);

        if (state == MAP)
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
                    pcg32_srandom_r(&rng, seed, initseq);

                    create_system(planet, star_position, rng);
                }

                // Update planets
                int max_planets = MAX_PLANETS;

                for (int i = 0; i < max_planets && planet->planets[i] != NULL; i++)
                {
                    update_system(planet->planets[i], ship, camera, position, state);
                }

                if (orbits_on)
                {
                    // Draw cutoff area circles
                    int _r = planet->class * GALAXY_SECTION_SIZE / 2;
                    int radius = _r * game_scale;
                    int _x = (planet->position.x - camera->x) * game_scale;
                    int _y = (planet->position.y - camera->y) * game_scale;
                    SDL_Color cutoff_color = {255, 0, 255, 40};

                    SDL_DrawCircle(renderer, camera, _x, _y, radius - 1, cutoff_color);
                    SDL_DrawCircle(renderer, camera, _x, _y, radius - 2, cutoff_color);
                    SDL_DrawCircle(renderer, camera, _x, _y, radius - 3, cutoff_color);
                }
            }
        }
        else if (state == NAVIGATE)
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
                    pcg32_srandom_r(&rng, seed, initseq);

                    create_system(planet, star_position, rng);
                }

                // Update planets
                int max_planets = MAX_PLANETS;

                for (int i = 0; i < max_planets && planet->planets[i] != NULL; i++)
                {
                    update_system(planet->planets[i], ship, camera, position, state);
                }
            }

            // Draw cutoff area circle
            if (orbits_on && distance_star < 2 * planet->cutoff)
            {
                int radius = planet->cutoff * game_scale;
                int _x = (planet->position.x - camera->x) * game_scale;
                int _y = (planet->position.y - camera->y) * game_scale;
                SDL_Color cutoff_color = {255, 0, 255, 70};

                SDL_DrawCircle(renderer, camera, _x, _y, radius, cutoff_color);
            }
        }
    }

    // Draw planet
    if (in_camera(camera, planet->position.x, planet->position.y, planet->radius))
    {
        planet->rect.x = (int)(planet->position.x - planet->radius - camera->x) * game_scale;
        planet->rect.y = (int)(planet->position.y - planet->radius - camera->y) * game_scale;

        SDL_RenderCopy(renderer, planet->texture, NULL, &planet->rect);
    }
    // Draw planet projection
    else if (PROJECT_PLANETS_ON)
    {
        if (planet->level == LEVEL_MOON)
        {
            float delta_x = planet->parent->position.x - position.x;
            float delta_y = planet->parent->position.y - position.y;
            float distance = sqrt(delta_x * delta_x + delta_y * delta_y);

            if (distance < 2 * planet->parent->cutoff)
                project_planet(planet, camera, state);
        }
        else
            project_planet(planet, camera, state);
    }

    // Update ship speed due to gravity
    if (state == NAVIGATE && SHIP_GRAVITY_ON)
        apply_gravity_to_ship(planet, ship, camera);
}

/*
 * Apply planet gravity to ship.
 */
void apply_gravity_to_ship(struct planet_t *planet, struct ship_t *ship, const struct camera_t *camera)
{
    float delta_x = planet->position.x - ship->position.x;
    float delta_y = planet->position.y - ship->position.y;
    float distance = sqrt(delta_x * delta_x + delta_y * delta_y);
    float g_planet = 0;
    int is_star = planet->level == LEVEL_STAR;
    int collision_point = planet->radius;

    // Detect planet collision
    if (COLLISIONS_ON && distance <= collision_point + ship->radius)
    {
        landing_stage = STAGE_0;
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
            ship->vx -= g_launch * delta_x / distance;
            ship->vy -= g_launch * delta_y / distance;
        }
    }
    // Ship inside cutoff
    else if (distance < planet->cutoff)
    {
        landing_stage = STAGE_OFF;
        g_planet = G_CONSTANT * planet->radius * planet->radius / (distance * distance);

        ship->vx += g_planet * delta_x / distance;
        ship->vy += g_planet * delta_y / distance;
    }

    // Update velocity
    update_velocity(ship);

    // Enforce speed limit if within star cutoff
    if (is_star && distance < planet->cutoff)
    {
        speed_limit = BASE_SPEED_LIMIT * planet->class;

        if (velocity.magnitude > speed_limit)
        {
            ship->vx = speed_limit * ship->vx / velocity.magnitude;
            ship->vy = speed_limit * ship->vy / velocity.magnitude;

            // Update velocity
            update_velocity(ship);
        }
    }
}

/*
 * Update ship position, listen for key controls and draw ship.
 */
void update_ship(struct ship_t *ship, const struct camera_t *camera)
{
    float radians;

    // Update ship angle
    if (right && !left && landing_stage == STAGE_OFF)
        ship->angle += 3;

    if (left && !right && landing_stage == STAGE_OFF)
        ship->angle -= 3;

    if (ship->angle > 360)
        ship->angle -= 360;

    // Apply thrust
    if (thrust)
    {
        landing_stage = STAGE_OFF;
        radians = ship->angle * M_PI / 180;

        ship->vx += g_thrust * sin(radians);
        ship->vy -= g_thrust * cos(radians);
    }

    // Apply reverse
    if (reverse)
    {
        radians = ship->angle * M_PI / 180;

        ship->vx -= g_thrust * sin(radians);
        ship->vy += g_thrust * cos(radians);
    }

    // Stop ship
    if (stop)
    {
        ship->vx = 0;
        ship->vy = 0;
    }

    // Update ship position
    ship->position.x += (float)ship->vx / FPS;
    ship->position.y += (float)ship->vy / FPS;

    if (camera_on)
    {
        // Static rect position at center of screen fixes flickering caused by float-to-int inaccuracies
        ship->rect.x = (camera->w / 2) - ship->radius;
        ship->rect.y = (camera->h / 2) - ship->radius;
    }
    else
    {
        // Dynamic rect position based on ship position
        ship->rect.x = (int)((ship->position.x - camera->x) * game_scale - ship->radius);
        ship->rect.y = (int)((ship->position.y - camera->y) * game_scale - ship->radius);
    }

    // Draw ship if in camera
    if (in_camera(camera, ship->position.x, ship->position.y, ship->radius))
    {
        SDL_RenderCopyEx(renderer, ship->texture, &ship->main_img_rect, &ship->rect, ship->angle, &ship->rotation_pt, SDL_FLIP_NONE);
    }
    // Draw ship projection
    else if (PROJECTIONS_ON)
        project_ship(ship, camera, NAVIGATE);

    // Draw ship thrust
    if (thrust)
        SDL_RenderCopyEx(renderer, ship->texture, &ship->thrust_img_rect, &ship->rect, ship->angle, &ship->rotation_pt, SDL_FLIP_NONE);

    // Draw reverse thrust
    if (reverse)
        SDL_RenderCopyEx(renderer, ship->texture, &ship->reverse_img_rect, &ship->rect, ship->angle, &ship->rotation_pt, SDL_FLIP_NONE);
}
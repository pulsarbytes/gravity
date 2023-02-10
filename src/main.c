/*
 * Gravity - An infinite procedural 2d universe that models gravity and orbital motion.
 *
 * v1.1.0
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
static int start = 1;
int state = NAVIGATE;
float game_scale = ZOOM_NAVIGATE;
float save_scale = 1;
int speed_limit = BASE_SPEED_LIMIT;
int landing_stage = STAGE_OFF;
struct vector_t velocity;

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
int map_enter = OFF;
int map_exit = OFF;
int map_center = OFF;

// Keep track of current nearest axis coordinates
struct position_t cross_axis;

// Array for game console entries
struct game_console_entry game_console_entries[LOG_COUNT];

// Hash table for stars
struct star_entry *stars[MAX_STARS];

// Function prototypes
void onMenu(void);
void onNavigate(struct bgstar_t bgstars[], int bgstars_count, struct ship_t *, struct camera_t *, struct position_t *);
void onMap(struct bgstar_t bgstars[], int bgstars_count, struct ship_t *, struct camera_t *, struct position_t *);
void onPause(struct bgstar_t bgstars[], int bgstars_count, struct ship_t *, const struct camera_t *, unsigned int start_time);
int init_sdl(void);
void close_sdl(void);
void poll_events(int *quit);
void log_game_console(struct game_console_entry entries[], int index, float value);
void update_game_console(struct game_console_entry entries[]);
void log_fps(unsigned int time_diff);
void cleanup_resources(struct ship_t *);
void calc_orbital_velocity(float height, float angle, float radius, float *vx, float *vy);
int create_bgstars(struct bgstar_t bgstars[], int max_bgstars);
void update_bgstars(struct bgstar_t bgstars[], int stars_count, float vx, float vy, const struct camera_t *);
struct planet_t *create_star(struct position_t);
uint64_t float_pair_hash_order_sensitive(struct position_t);
void create_system(struct planet_t *, struct position_t);
struct ship_t create_ship(int radius, struct position_t);
void update_system(struct planet_t *, struct ship_t *, const struct camera_t *, struct position_t, int state);
void project_planet(struct planet_t *, const struct camera_t *, int state);
void update_projection_coordinates(void *, int entity_type, const struct camera_t *, int state);
void apply_gravity_to_ship(struct planet_t *, struct ship_t *, const struct camera_t *);
void update_camera(struct camera_t *, struct ship_t *);
void update_map_camera(struct camera_t *camera, struct position_t);
void update_ship(struct ship_t *, const struct camera_t *);
void project_ship(struct ship_t *, const struct camera_t *, int state);
float find_nearest_section_axis(float n);
void generate_stars(struct position_t);
void put_star(struct position_t, struct planet_t *);
struct planet_t *get_star(struct position_t position);
int star_exists(struct position_t);
void delete_star(struct position_t);
uint64_t float_hash(float x);
void update_velocity(struct ship_t *ship);
float nearest_star_distance(struct position_t position);
int get_star_class(float n);
int get_planet_class(float n);
void zoom_star(struct planet_t *planet);
void SDL_DrawCircle(SDL_Renderer *renderer, int xc, int yc, int radius, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

int main(int argc, char *argv[])
{
    int quit = 0;

    // Initialize SDL
    if (!init_sdl())
    {
        fprintf(stderr, "Error: could not initialize SDL.\n");
        return 1;
    }

    // Create background stars
    int max_bgstars = (int)(display_mode.w * display_mode.h * BGSTARS_PER_SQUARE / BGSTARS_SQUARE);
    max_bgstars *= 1.3; // Add 30% more space for safety
    struct bgstar_t bgstars[max_bgstars];
    int bgstars_count = create_bgstars(bgstars, max_bgstars);

    // Navigation coordinates
    struct position_t offset = {.x = STARTING_X, .y = STARTING_Y};

    // Map coordinates
    struct position_t map_offset = {.x = STARTING_X, .y = STARTING_Y};

    // Create ship and ship projection
    struct ship_t ship = create_ship(SHIP_RADIUS, offset);
    struct position_t zero_position = {.x = 0, .y = 0};
    struct ship_t ship_projection = create_ship(SHIP_PROJECTION_RADIUS, zero_position);
    ship.projection = &ship_projection;

    // Generate stars
    generate_stars(offset);

    // Put ship in orbit around a star
    if (START_IN_ORBIT)
    {
        struct position_t start_position = {.x = STARTING_X, .y = STARTING_Y};

        if (star_exists(start_position))
        {
            struct planet_t *star = get_star(start_position);

            float d = 3 * (star->radius);
            ship.position.x = start_position.x + d;
            ship.position.y = start_position.y - d;
            calc_orbital_velocity(sqrt(d * d + d * d), -45, star->radius, &ship.vx, &ship.vy);
        }
    }

    // Create camera, sync initial position with ship
    struct camera_t camera = {
        .x = ship.position.x - (display_mode.w / 2),
        .y = ship.position.y - (display_mode.h / 2),
        .w = display_mode.w,
        .h = display_mode.h};

    // Initialize sections crossings
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
            onNavigate(bgstars, bgstars_count, &ship, &camera, &offset);
            break;
        case MAP:
            onMap(bgstars, bgstars_count, &ship, &camera, &map_offset);
            break;
        case PAUSE:
            onPause(bgstars, bgstars_count, &ship, &camera, start_time);
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
    cleanup_resources(&ship);

    // Close SDL
    close_sdl();

    return 0;
}

void onMenu(void)
{
    // printf("\nonMenu");
}

void onNavigate(struct bgstar_t bgstars[], int bgstars_count, struct ship_t *ship, struct camera_t *camera, struct position_t *offset)
{
    if (map_exit)
    {
        // Reset previous game_scale
        game_scale = save_scale;

        // Update camera
        if (camera_on)
            update_camera(camera, ship);

        // Zoom in
        for (int s = 0; s < MAX_STARS; s++)
        {
            if (stars[s] != NULL && stars[s]->star != NULL)
                zoom_star(stars[s]->star);
        }
    }

    // Update velocity
    update_velocity(ship);

    // Draw background stars
    if (BGSTARS_ON)
        update_bgstars(bgstars, bgstars_count, ship->vx, ship->vy, camera);

    if (camera_on)
    {
        // Generate stars
        generate_stars(*offset);
    }

    if ((!map_exit && !zoom_in && !zoom_out) || game_scale > ZOOM_NAVIGATE_MIN)
    {
        struct position_t position = {.x = ship->position.x, .y = ship->position.y};

        // Update system
        for (int i = 0; i < MAX_STARS; i++)
        {
            if (stars[i] != NULL)
                update_system(stars[i]->star, ship, camera, position, NAVIGATE);
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

    // Update ship
    update_ship(ship, camera);

    // Update coordinates
    offset->x = ship->position.x;
    offset->y = ship->position.y;

    // Update camera
    if (camera_on)
        update_camera(camera, ship);

    // Draw Map frame
    SDL_Rect top_frame;
    top_frame.x = 0;
    top_frame.y = 0;
    top_frame.w = camera->w;
    top_frame.h = 2 * PROJECTION_RADIUS;
    SDL_Rect left_frame;
    left_frame.x = 0;
    left_frame.y = 2 * PROJECTION_RADIUS;
    left_frame.w = 2 * PROJECTION_RADIUS;
    left_frame.h = camera->h - 4 * PROJECTION_RADIUS;
    SDL_Rect bottom_frame;
    bottom_frame.x = 0;
    bottom_frame.y = camera->h - 2 * PROJECTION_RADIUS;
    bottom_frame.w = camera->w;
    bottom_frame.h = 2 * PROJECTION_RADIUS;
    SDL_Rect right_frame;
    right_frame.x = camera->w - 2 * PROJECTION_RADIUS;
    right_frame.y = 2 * PROJECTION_RADIUS;
    right_frame.w = 2 * PROJECTION_RADIUS;
    right_frame.h = camera->h - 4 * PROJECTION_RADIUS;

    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 22);
    SDL_RenderFillRect(renderer, &top_frame);
    SDL_RenderFillRect(renderer, &left_frame);
    SDL_RenderFillRect(renderer, &bottom_frame);
    SDL_RenderFillRect(renderer, &right_frame);

    // Update game console
    if (console && CONSOLE_ON)
    {
        // Log velocity magnitude (relative to 0,0)
        log_game_console(game_console_entries, V_INDEX, velocity.magnitude);

        // Log coordinates (relative to (0,0))
        log_game_console(game_console_entries, X_INDEX, offset->x);
        log_game_console(game_console_entries, Y_INDEX, offset->y);

        // Log game scale
        log_game_console(game_console_entries, SCALE_INDEX, game_scale);

        update_game_console(game_console_entries);
    }

    if (map_exit)
        map_exit = OFF;
}

void onMap(struct bgstar_t bgstars[], int bgstars_count, struct ship_t *ship, struct camera_t *camera, struct position_t *offset)
{
    if (map_enter || map_center)
    {
        if (map_enter)
            save_scale = game_scale;

        game_scale = ZOOM_MAP;

        // Zoom in
        for (int s = 0; s < MAX_STARS; s++)
        {
            if (stars[s] != NULL && stars[s]->star != NULL)
                zoom_star(stars[s]->star);
        }

        // Reset map to ship position
        offset->x = ship->position.x;
        offset->y = ship->position.y;

        // Update map camera
        update_map_camera(camera, *offset);

        if (map_center)
            map_center = OFF;
    }

    // Generate stars
    generate_stars(*offset);

    if ((!map_enter && !zoom_in && !zoom_out) || game_scale > 0)
    {
        struct position_t position = {.x = offset->x, .y = offset->y};

        // Update system
        for (int i = 0; i < MAX_STARS; i++)
        {
            if (stars[i] != NULL)
                update_system(stars[i]->star, ship, camera, position, MAP);
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
        if (game_scale - ZOOM_STEP >= ZOOM_MAP_MIN - epsilon)
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

    // Move through map
    float rate_x = 0;

    if (right)
        rate_x = MAP_SPEED_MIN + (MAP_SPEED_MAX - MAP_SPEED_MIN) * (camera->w / 1000) / game_scale;
    else if (left)
        rate_x = -(MAP_SPEED_MIN + (MAP_SPEED_MAX - MAP_SPEED_MIN) * (camera->w / 1000) / game_scale);

    offset->x += rate_x;

    float rate_y = 0;

    if (down)
        rate_y = MAP_SPEED_MIN + (MAP_SPEED_MAX - MAP_SPEED_MIN) * (camera->w / 1000) / game_scale;
    else if (up)
        rate_y = -(MAP_SPEED_MIN + (MAP_SPEED_MAX - MAP_SPEED_MIN) * (camera->w / 1000) / game_scale);

    offset->y += rate_y;

    // Draw background stars
    if (BGSTARS_ON)
        update_bgstars(bgstars, bgstars_count, rate_x, rate_y, camera);

    // Update map camera
    update_map_camera(camera, *offset);

    // Draw ship projection
    ship->projection->rect.x = (ship->position.x - offset->x) * game_scale + (camera->w / 2 - ship->projection->radius);
    ship->projection->rect.y = (ship->position.y - offset->y) * game_scale + (camera->h / 2 - ship->projection->radius);
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
    float bx = find_nearest_section_axis(camera->x);
    float by = find_nearest_section_axis(camera->y);
    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 32);

    for (int ix = bx; ix <= bx + camera->w / game_scale; ix = ix + SECTION_SIZE)
    {
        SDL_RenderDrawLine(renderer, (ix - camera->x) * game_scale, 0, (ix - camera->x) * game_scale, camera->h);
    }

    for (int iy = by; iy <= by + camera->h / game_scale; iy = iy + SECTION_SIZE)
    {
        SDL_RenderDrawLine(renderer, 0, (iy - camera->y) * game_scale, camera->w, (iy - camera->y) * game_scale);
    }

    // Draw cross at position
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);
    SDL_RenderDrawLine(renderer, (camera->w / 2) - 7, camera->h / 2, (camera->w / 2) + 7, camera->h / 2);
    SDL_RenderDrawLine(renderer, camera->w / 2, (camera->h / 2) - 7, camera->w / 2, (camera->h / 2) + 7);

    // Draw Map frame
    SDL_Rect top_frame;
    top_frame.x = 0;
    top_frame.y = 0;
    top_frame.w = camera->w;
    top_frame.h = 2 * PROJECTION_RADIUS;
    SDL_Rect left_frame;
    left_frame.x = 0;
    left_frame.y = 2 * PROJECTION_RADIUS;
    left_frame.w = 2 * PROJECTION_RADIUS;
    left_frame.h = camera->h - 4 * PROJECTION_RADIUS;
    SDL_Rect bottom_frame;
    bottom_frame.x = 0;
    bottom_frame.y = camera->h - 2 * PROJECTION_RADIUS;
    bottom_frame.w = camera->w;
    bottom_frame.h = 2 * PROJECTION_RADIUS;
    SDL_Rect right_frame;
    right_frame.x = camera->w - 2 * PROJECTION_RADIUS;
    right_frame.y = 2 * PROJECTION_RADIUS;
    right_frame.w = 2 * PROJECTION_RADIUS;
    right_frame.h = camera->h - 4 * PROJECTION_RADIUS;

    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 22);
    SDL_RenderFillRect(renderer, &top_frame);
    SDL_RenderFillRect(renderer, &left_frame);
    SDL_RenderFillRect(renderer, &bottom_frame);
    SDL_RenderFillRect(renderer, &right_frame);

    // Update game console
    if (console && CONSOLE_ON)
    {
        // Log coordinates (relative to (0,0))
        log_game_console(game_console_entries, X_INDEX, offset->x);
        log_game_console(game_console_entries, Y_INDEX, offset->y);

        // Log game scale
        log_game_console(game_console_entries, SCALE_INDEX, game_scale);

        update_game_console(game_console_entries);
    }

    if (map_enter)
        map_enter = OFF;
}

void onPause(struct bgstar_t bgstars[], int bgstars_count, struct ship_t *ship, const struct camera_t *camera, unsigned int start_time)
{
    // Update game console
    if (console)
        update_game_console(game_console_entries);

    // Draw background stars
    if (BGSTARS_ON)
        update_bgstars(bgstars, bgstars_count, 0, 0, camera);

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
 * The region has intervals of size SECTION_SIZE.
 */
void generate_stars(struct position_t offset)
{
    // Keep track of current nearest section axis coordinates
    float bx = find_nearest_section_axis(offset.x);
    float by = find_nearest_section_axis(offset.y);

    // Check if this is the first time calling this function
    if (!start)
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

    // Define a region of REGION_SIZE x REGION_SIZE
    // bx,by are at the center of this area
    float ix, iy;
    float left_boundary = bx - ((REGION_SIZE / 2) * SECTION_SIZE);
    float right_boundary = bx + ((REGION_SIZE / 2) * SECTION_SIZE);
    float top_boundary = by - ((REGION_SIZE / 2) * SECTION_SIZE);
    float bottom_boundary = by + ((REGION_SIZE / 2) * SECTION_SIZE);

    // Use a local rng
    pcg32_random_t rng;

    for (ix = left_boundary; ix < right_boundary; ix += SECTION_SIZE)
    {
        for (iy = top_boundary; iy < bottom_boundary; iy += SECTION_SIZE)
        {
            // Seed with a fixed constant
            pcg32_srandom_r(&rng, float_hash(ix), float_hash(iy));

            int has_star = abs(pcg32_random_r(&rng)) % 1000 < REGION_DENSITY;

            if (has_star)
            {
                struct position_t position = {.x = ix, .y = iy};

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
        if (stars[s] != NULL)
        {
            struct position_t position = {.x = stars[s]->x, .y = stars[s]->y};

            // Skip star at (STARTING_X, STARTING_Y)
            if (position.x == STARTING_X && position.y == STARTING_Y)
                continue;

            // Get distance from center of region
            float dx = position.x - bx;
            float dy = position.y - by;
            float distance = sqrt(dx * dx + dy * dy);
            float region_radius = sqrt((double)2 * ((REGION_SIZE + 1) / 2) * SECTION_SIZE * ((REGION_SIZE + 1) / 2) * SECTION_SIZE);

            // If star outside region, delete it
            if (distance >= region_radius)
                delete_star(position);
        }
    }

    // First star generation complete
    start = 0;
}

/*
 * Create background stars.
 */
int create_bgstars(struct bgstar_t bgstars[], int max_bgstars)
{
    int i = 0, row, column, is_star;
    int end = FALSE;

    // Seed with a fixed constant
    srand(1200);

    for (row = 0; row < display_mode.h && !end; row++)
    {
        for (column = 0; column < display_mode.w && !end; column++)
        {
            is_star = rand() % BGSTARS_SQUARE < BGSTARS_PER_SQUARE;

            if (is_star)
            {
                struct bgstar_t star;
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

                // Get a color between 10 - 165 and increase exponentially with game_scale
                star.opacity = ((rand() % 166) + 10); // * (1 - pow(1 - game_scale, 2));
                bgstars[i++] = star;
            }

            if (i >= max_bgstars)
                end = TRUE;
        }
    }

    return i;
}

/*
 * Move and draw background stars.
 */
void update_bgstars(struct bgstar_t bgstars[], int stars_count, float vx, float vy, const struct camera_t *camera)
{
    for (int i = 0; i < stars_count; i++)
    {
        if (camera_on)
        {
            // Don't move background stars faster than BGSTARS_MAX_SPEED
            if (velocity.magnitude > BGSTARS_MAX_SPEED)
            {
                bgstars[i].position.x -= 0.2 * (BGSTARS_MAX_SPEED * vx / velocity.magnitude) / FPS;
                bgstars[i].position.y -= 0.2 * (BGSTARS_MAX_SPEED * vy / velocity.magnitude) / FPS;
            }
            else
            {
                bgstars[i].position.x -= 0.2 * vx / FPS;
                bgstars[i].position.y -= 0.2 * vy / FPS;
            }

            // Normalize within camera boundaries
            if (bgstars[i].position.x > camera->w)
            {
                bgstars[i].position.x = fmod(bgstars[i].position.x, camera->w);
            }
            if (bgstars[i].position.x < 0)
            {
                bgstars[i].position.x += camera->w;
            }
            if (bgstars[i].position.y > camera->h)
            {
                bgstars[i].position.y = fmod(bgstars[i].position.y, camera->h);
            }
            if (bgstars[i].position.y < 0)
            {
                bgstars[i].position.y += camera->h;
            }

            bgstars[i].rect.x = (int)(bgstars[i].position.x);
            bgstars[i].rect.y = (int)(bgstars[i].position.y);
        }

        // Update opacity with game_scale
        float opacity = (double)bgstars[i].opacity * (1 - pow(1 - game_scale, 2));

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, (unsigned short)opacity);
        SDL_RenderFillRect(renderer, &bgstars[i].rect);
    }
}

/*
 * Update camera position.
 */
void update_camera(struct camera_t *camera, struct ship_t *ship)
{
    camera->x = ship->position.x - (camera->w / 2) / game_scale;
    camera->y = ship->position.y - (camera->h / 2) / game_scale;
}

/*
 * Update map camera position.
 */
void update_map_camera(struct camera_t *camera, struct position_t offset)
{
    camera->x = offset.x - (camera->w / 2) / game_scale;
    camera->y = offset.y - (camera->h / 2) / game_scale;
}

/*
 * Create a ship.
 */
struct ship_t create_ship(int radius, struct position_t position)
{
    struct ship_t ship;

    ship.image = "../assets/sprites/ship.png";
    ship.radius = radius;
    ship.position.x = (int)position.x;
    ship.position.y = (int)position.y;
    ship.vx = 0.0;
    ship.vy = 0.0;
    ship.angle = 0;
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
 * Create a star.
 */
struct planet_t *create_star(struct position_t position)
{
    // Find distance to nearest star
    float distance = nearest_star_distance(position);

    // Get star class
    int class = get_star_class(distance);

    // Use a local rng
    pcg32_random_t rng;

    // Seed with a fixed constant
    pcg32_srandom_r(&rng, float_hash(position.x), float_hash(position.y));

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
    uint64_t index = float_pair_hash_order_sensitive(position);

    star->initialized = 0;
    sprintf(star->name, "%s-%lu", "S", index);
    star->image = "../assets/images/sol.png";
    star->class = get_star_class(distance);
    star->radius = radius;
    star->cutoff = class * (SECTION_SIZE / 2);
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
void create_system(struct planet_t *planet, struct position_t position)
{
    if (planet->level == LEVEL_STAR && planet->initialized == 1)
        return;

    if (planet->level >= LEVEL_MOON)
        return;

    int max_planets = (planet->level == LEVEL_STAR) ? MAX_PLANETS : MAX_MOONS;

    if (max_planets == 0)
        return;

    // Use a local rng
    pcg32_random_t rng;

    // Seed with a fixed constant
    pcg32_srandom_r(&rng, float_hash(position.x), float_hash(position.y));

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
            // We round some values to get rid of floating-point inaccuracies
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

                create_system(_planet, position);
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
 * Create, update, draw system and apply gravity to planets and ship (recursive).
 */
void update_system(struct planet_t *planet, struct ship_t *ship, const struct camera_t *camera, struct position_t position, int state)
{
    // Update planets & moons
    if (planet->level != LEVEL_STAR)
    {
        if (state == NAVIGATE)
        {
            float delta_x = 0.0;
            float delta_y = 0.0;
            float distance;
            float g_planet = G_CONSTANT;
            float dx = planet->parent->dx;
            float dy = planet->parent->dy;

            // Update planet position
            planet->position.x += planet->parent->dx;
            planet->position.y += planet->parent->dy;

            // Find distance from parent
            delta_x = planet->parent->position.x - planet->position.x;
            delta_y = planet->parent->position.y - planet->position.y;
            distance = sqrt(delta_x * delta_x + delta_y * delta_y);

            // Determine speed and position shift
            if (distance > (planet->parent->radius + planet->radius))
            {
                g_planet = G_CONSTANT * (float)(planet->parent->radius * planet->parent->radius) / (distance * distance);

                planet->vx += g_planet * delta_x / distance;
                planet->vy += g_planet * delta_y / distance;

                dx = (float)planet->vx / FPS;
                dy = (float)planet->vy / FPS;

                planet->dx = dx;
                planet->dy = dy;
            }

            // Update planet position
            planet->position.x += dx;
            planet->position.y += dy;
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
        float delta_x_star = 0.0;
        float delta_y_star = 0.0;
        delta_x_star = planet->position.x - position.x;
        delta_y_star = planet->position.y - position.y;
        float distance_star = sqrt(delta_x_star * delta_x_star + delta_y_star * delta_y_star);

        if (state == MAP)
        {
            if (distance_star < planet->cutoff && SOLAR_SYSTEMS_ON)
            {
                // Create system
                if (!planet->initialized && SOLAR_SYSTEMS_ON)
                {
                    struct position_t star_position = {.x = planet->position.x, .y = planet->position.y};
                    create_system(planet, star_position);
                }

                // Update planets
                int max_planets = MAX_PLANETS;

                for (int i = 0; i < max_planets && planet->planets[i] != NULL; i++)
                {
                    update_system(planet->planets[i], ship, camera, position, state);
                }

                // Draw cutoff area circles
                int _r = planet->class * SECTION_SIZE / 2;
                int radius = _r * game_scale;
                int _x = (planet->position.x - camera->x) * game_scale;
                int _y = (planet->position.y - camera->y) * game_scale;

                SDL_DrawCircle(renderer, _x, _y, radius - 1, 0, 255, 255, 20);
                SDL_DrawCircle(renderer, _x, _y, radius - 2, 0, 255, 255, 20);
                SDL_DrawCircle(renderer, _x, _y, radius - 3, 0, 255, 255, 20);
                SDL_DrawCircle(renderer, _x, _y, radius - 4, 0, 255, 255, 20);
            }
        }
        else if (state == NAVIGATE)
        {
            if (distance_star < planet->cutoff && SOLAR_SYSTEMS_ON)
            {
                // Create system
                if (!planet->initialized)
                {
                    struct position_t star_position = {.x = planet->position.x, .y = planet->position.y};
                    create_system(planet, star_position);
                }

                // Update planets
                int max_planets = MAX_PLANETS;

                for (int i = 0; i < max_planets && planet->planets[i] != NULL; i++)
                {
                    update_system(planet->planets[i], ship, camera, position, state);
                }
            }
        }
    }

    // Draw if in camera
    if (planet->position.x - planet->radius <= camera->x + camera->w / game_scale && planet->position.x + planet->radius > camera->x &&
        planet->position.y - planet->radius <= camera->y + camera->h / game_scale && planet->position.y + planet->radius > camera->y)
    {
        planet->rect.x = (int)(planet->position.x - planet->radius - camera->x) * game_scale;
        planet->rect.y = (int)(planet->position.y - planet->radius - camera->y) * game_scale;

        SDL_RenderCopy(renderer, planet->texture, NULL, &planet->rect);
    }
    // Draw planet projection
    else if (PROJECTIONS_ON)
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
    float delta_x = 0.0;
    float delta_y = 0.0;
    float distance;
    float g_planet = 0;
    int is_star = planet->level == LEVEL_STAR;
    int collision_point = planet->radius;

    delta_x = planet->position.x - ship->position.x;
    delta_y = planet->position.y - ship->position.y;
    distance = sqrt(delta_x * delta_x + delta_y * delta_y);

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
        g_planet = G_CONSTANT * (float)(planet->radius * planet->radius) / (distance * distance);

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
        radians = M_PI * ship->angle / 180;

        ship->vx += g_thrust * sin(radians);
        ship->vy -= g_thrust * cos(radians);
    }

    // Apply reverse
    if (reverse)
    {
        radians = M_PI * ship->angle / 180;

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
    if (camera_on || (!camera_on && (ship->position.x <= camera->x + camera->w / game_scale && ship->position.x > camera->x &&
                                     ship->position.y <= camera->y + camera->h / game_scale && ship->position.y > camera->y)))
    {
        SDL_RenderCopyEx(renderer, ship->texture, &ship->main_img_rect, &ship->rect, ship->angle, &ship->rotation_pt, SDL_FLIP_NONE);
    }
    // Draw ship projection
    else if (PROJECTIONS_ON)
    {
        project_ship(ship, camera, NAVIGATE);
    }

    // Draw ship thrust
    if (thrust)
        SDL_RenderCopyEx(renderer, ship->texture, &ship->thrust_img_rect, &ship->rect, ship->angle, &ship->rotation_pt, SDL_FLIP_NONE);

    // Draw reverse thrust
    if (reverse)
        SDL_RenderCopyEx(renderer, ship->texture, &ship->reverse_img_rect, &ship->rect, ship->angle, &ship->rotation_pt, SDL_FLIP_NONE);
}
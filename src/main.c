/*
 * Gravity - A basic 2d game engine in C that models gravity and orbital motion
 *
 * v1.0.1
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
 * Copyright (c) 2020 Yannis Maragos
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

enum
{
    LEVEL_STAR = 1,
    LEVEL_PLANET,
    LEVEL_MOON
};

static enum {
    STAGE_OFF = -1, // Not landed
    STAGE_0         // Landed
} landing_stage = STAGE_OFF;

// External variable definitions
SDL_Window *window = NULL;
SDL_DisplayMode display_mode;
SDL_Renderer *renderer = NULL;
TTF_Font *font = NULL;
SDL_Color text_color;

static float velocity;
const float g_launch = 0.7 * G_CONSTANT;
const float g_thrust = 1 * G_CONSTANT;

// Keep track of keyboard inputs
int left = OFF;
int right = OFF;
int thrust = OFF;
int console = ON;

// Array for game console entries
struct game_console_entry game_console_entries[LOG_COUNT];

// Function prototypes
int init_sdl(void);
void close_sdl(void);
void poll_events(int *quit);
void log_game_console(struct game_console_entry entries[], int index, float value);
void update_game_console(struct game_console_entry entries[]);
void destroy_game_console(struct game_console_entry entries[]);
void log_fps(unsigned int time_diff);
void cleanup_resources(struct planet_t *, struct ship_t *);
float orbital_velocity(float height, int radius);
int create_bgstars(struct bgstar_t bgstars[], int max_bgstars, struct ship_t *);
void update_bgstars(struct bgstar_t bgstars[], int stars_count, struct ship_t *, const struct camera_t *);
struct planet_t *create_star(void);
void create_system(struct planet_t *);
struct ship_t create_ship(int radius, int x, int y);
void update_planets(struct planet_t *, struct ship_t *, const struct camera_t *);
void project_planet(struct planet_t *, const struct camera_t *);
void apply_gravity_to_ship(struct planet_t *, struct ship_t *, const struct camera_t *);
void update_camera(struct camera_t *, struct ship_t *);
void update_ship(struct ship_t *ship, struct ship_t *projection, const struct camera_t *);
void project_ship(struct ship_t *ship, struct ship_t *projection, const struct camera_t *);

int main(int argc, char *argv[])
{
    int quit = 0;

    // Global coordinates
    float x_coord = 0;
    float y_coord = 0;

    // Initialize SDL
    if (!init_sdl())
    {
        fprintf(stderr, "Error: could not initialize SDL.\n");
        return 1;
    }

    // Create ship
    struct ship_t ship = create_ship(SHIP_RADIUS, SHIP_STARTING_X, SHIP_STARTING_Y);

    // Create ship projection
    struct ship_t ship_projection = create_ship(SHIP_PROJECTION_RADIUS, 0, 0);

    // Create camera, sync initial position with ship
    struct camera_t camera = {
        .x = ship.position.x - (display_mode.w / 2),
        .y = ship.position.y - (display_mode.h / 2),
        .w = display_mode.w,
        .h = display_mode.h};

    // Create a star and a system
    struct planet_t *star = create_star();
    create_system(star);

    // Put ship in orbit
    if (SHIP_IN_ORBIT)
        ship.vx = orbital_velocity(abs((int)star->position.y - (int)ship.position.y), star->radius);

    // Create stars background
    int max_bgstars = (int)(display_mode.w * display_mode.h * STARS_PER_SQUARE / STARS_SQUARE);
    max_bgstars *= 1.3; // Add 30% more space for safety
    struct bgstar_t bgstars[max_bgstars];
    int bgstars_count = create_bgstars(bgstars, max_bgstars, &ship);

    // Set time keeping variables
    unsigned int start_time, end_time;

    // Animation loop
    while (!quit)
    {
        start_time = SDL_GetTicks();

        // Process events
        poll_events(&quit);

        // Set background color
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        // Clear the renderer
        SDL_RenderClear(renderer);

        // Draw background stars
        update_bgstars(bgstars, bgstars_count, &ship, &camera);

        // Updates planets in solar system recursively
        update_planets(star, &ship, &camera);

        // Update camera
        if (CAMERA_ON)
            update_camera(&camera, &ship);

        // Update ship
        update_ship(&ship, &ship_projection, &camera);

        // Update coordinates
        x_coord = ship.position.x;
        y_coord = ship.position.y;

        if (CONSOLE_ON)
        {
            // Log coordinates (relative to star)
            log_game_console(game_console_entries, X_INDEX, x_coord);
            log_game_console(game_console_entries, Y_INDEX, y_coord);

            // Log velocity (relative to star)
            log_game_console(game_console_entries, V_INDEX, velocity);

            // Update game console
            if (console)
                update_game_console(game_console_entries);
        }

        // Switch buffers, display back buffer
        SDL_RenderPresent(renderer);

        // Set FPS
        if ((1000 / FPS) > ((end_time = SDL_GetTicks()) - start_time))
            SDL_Delay((1000 / FPS) - (end_time - start_time));

        // Log FPS
        log_fps(end_time - start_time);
    }

    if (CONSOLE_ON)
    {
        // Destroy game console
        destroy_game_console(game_console_entries);
    }

    // Cleanup resources
    cleanup_resources(star, &ship);

    // Close SDL
    close_sdl();

    return 0;
}

/*
 * Create background stars
 */
int create_bgstars(struct bgstar_t bgstars[], int max_bgstars, struct ship_t *ship)
{
    int i = 0, row, column, is_star;
    int end = FALSE;

    for (row = 0; row < display_mode.h && !end; row++)
    {
        for (column = 0; column < display_mode.w && !end; column++)
        {
            is_star = rand() % STARS_SQUARE < STARS_PER_SQUARE;

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

                star.opacity = (rand() % (246 - 30)) + 10; // Skip 0-9 and go up to 225
                bgstars[i++] = star;
            }

            if (i >= max_bgstars)
                end = TRUE;
        }
    }

    return i;
}

/*
 * Move and draw stars background
 */
void update_bgstars(struct bgstar_t bgstars[], int stars_count, struct ship_t *ship, const struct camera_t *camera)
{
    for (int i = 0; i < stars_count; i++)
    {
        if (CAMERA_ON)
        {
            bgstars[i].position.x -= 0.2 * ship->vx / FPS;
            bgstars[i].position.y -= 0.2 * ship->vy / FPS;

            bgstars[i].rect.x = (int)(bgstars[i].position.x + (camera->w / 2));
            bgstars[i].rect.y = (int)(bgstars[i].position.y + (camera->h / 2));

            // Right boundary
            if (bgstars[i].position.x > ship->position.x - camera->x)
                bgstars[i].position.x -= camera->w;
            // Left boundary
            else if (bgstars[i].position.x < camera->x - ship->position.x)
                bgstars[i].position.x += camera->w;

            // Top boundary
            if (bgstars[i].position.y > ship->position.y - camera->y)
                bgstars[i].position.y -= camera->h;
            // Bottom boundary
            else if (bgstars[i].position.y < camera->y - ship->position.y)
                bgstars[i].position.y += camera->h;
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, bgstars[i].opacity);
        SDL_RenderFillRect(renderer, &bgstars[i].rect);
    }
}

/*
 * Update camera position
 */
void update_camera(struct camera_t *camera, struct ship_t *ship)
{
    // Update camera position
    camera->x = ship->position.x - camera->w / 2;
    camera->y = ship->position.y - camera->h / 2;
}

/*
 * Create a ship
 */
struct ship_t create_ship(int radius, int x, int y)
{
    struct ship_t ship;

    ship.image = "../assets/sprites/ship.png";
    ship.radius = radius;
    ship.position.x = x;
    ship.position.y = y;
    ship.vx = 0;
    ship.vy = 0;
    ship.angle = 0;
    SDL_Surface *surface = IMG_Load(ship.image);
    ship.texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    ship.rect.x = ship.position.x - ship.radius;
    ship.rect.y = ship.position.y - ship.radius;
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

    // Point around which ship will be rotated (relative to destination rect)
    ship.rotation_pt.x = ship.radius;
    ship.rotation_pt.y = ship.radius;

    return ship;
}

/*
 * Create a star
 */
struct planet_t *create_star(void)
{
    struct planet_t *star = (struct planet_t *)malloc(sizeof(struct planet_t));

    strcpy(star->name, "Star");
    star->image = "../assets/images/sol.png";
    star->radius = 250;
    star->position.x = 0.0;
    star->position.y = 0.0;
    star->vx = 0.0;
    star->vy = 0.0;
    star->dx = 0.0;
    star->dy = 0.0;
    SDL_Surface *surface = IMG_Load(star->image);
    star->texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    star->rect.x = star->position.x - star->radius;
    star->rect.y = star->position.y - star->radius;
    star->rect.w = 2 * star->radius;
    star->rect.h = 2 * star->radius;
    star->color.r = 255;
    star->color.g = 255;
    star->color.b = 0;
    star->planets[0] = NULL;
    star->parent = NULL;
    star->level = LEVEL_STAR;

    return star;
}

/*
 * Create a system (recursive). Takes a pointer to an initialized planet
 * and populates it with children planets
 */
void create_system(struct planet_t *planet)
{
    if (planet->level >= LEVEL_MOON)
        return;

    int max_planets = (planet->level == LEVEL_STAR) ? MAX_PLANETS : MAX_MOONS;

    // Count actual planets to be created, since some may be skipped if too close to star or planets
    int j = 0;

    for (int i = 0; i < max_planets; i++)
    {
        int planet_distance_star;
        int moon_distance_planet;
        int radius;

        // Don't add planets if they end up too close to star or to other planets
        if (planet->level == LEVEL_STAR)
        {
            // Check star distance
            planet_distance_star = (i + 1) * PLANET_DISTANCE; // To-do: variable planet distance
            radius = 100;

            if (planet_distance_star < (3 * planet->radius) + radius) // To-do: add constant
                continue;

            // Check previous planet distance
            if (j > 0)
            {
                float previous_planet_distance_star = abs(planet->position.y - (planet->planets[j - 1])->position.y);
                float planet_distance_planet = abs(planet_distance_star - previous_planet_distance_star);

                if (planet_distance_planet < (4 * radius)) // To-do: add constant
                    continue;
            }
        }

        // Don't add moons if they end up too close to star or to other planets
        if (planet->level == LEVEL_PLANET)
        {
            moon_distance_planet = (i + 1) * MOON_DISTANCE; // To-do: variable moon distance
            int planet_distance_star = abs(planet->position.y - planet->parent->position.y);

            // Check star distance
            if (moon_distance_planet > (planet_distance_star / 2))
                continue;

            // Check previous planet distance
            if (j > 0)
            {
                if (moon_distance_planet > (PLANET_DISTANCE / 2)) // To-do: find actual distance from previous planet
                    continue;
            }

            // Check next planet distance
            if (j < max_planets - 1)
            {
                if (moon_distance_planet > (PLANET_DISTANCE / 2)) // To-do: find actual distance from next planet
                    continue;
            }

            radius = 50;
        }

        struct planet_t *_planet = (struct planet_t *)malloc(sizeof(struct planet_t));

        if (planet->level == LEVEL_STAR)
        {
            char *name = "P";
            sprintf(_planet->name, "%s%d", name, i);
            _planet->image = "../assets/images/earth.png";
            _planet->position.y = (planet->position.y) - planet_distance_star;
            _planet->color.r = 135;
            _planet->color.g = 206;
            _planet->color.b = 235;
            _planet->level = LEVEL_PLANET;
        }
        else if (planet->level == LEVEL_PLANET)
        {
            char parent_name[MAX_PLANET_NAME / 2];
            strncpy(parent_name, planet->name, sizeof(parent_name));
            char *name = "M";
            sprintf(_planet->name, "%s %s%d", parent_name, name, i);
            _planet->image = "../assets/images/moon.png";
            _planet->position.y = planet->position.y - moon_distance_planet;
            _planet->color.r = 220;
            _planet->color.g = 220;
            _planet->color.b = 220;
            _planet->level = LEVEL_MOON;
        }

        _planet->radius = radius;
        _planet->position.x = 0.0;
        _planet->vx = orbital_velocity(abs((int)planet->position.y - (int)_planet->position.y), planet->radius); // Initial velocity
        _planet->vy = 0.0;
        _planet->dx = 0.0;
        _planet->dy = 0.0;
        SDL_Surface *surface = IMG_Load(_planet->image);
        _planet->texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        _planet->rect.x = _planet->position.x - _planet->radius;
        _planet->rect.y = _planet->position.y - _planet->radius;
        _planet->rect.w = 2 * _planet->radius;
        _planet->rect.h = 2 * _planet->radius;
        _planet->planets[0] = NULL;
        _planet->parent = planet;

        planet->planets[j] = _planet;
        planet->planets[j + 1] = NULL;

        j++;

        create_system(_planet);
    }
}

/*
 * Update planets positions and draw planets (recursive)
 */
void update_planets(struct planet_t *planet, struct ship_t *ship, const struct camera_t *camera)
{
    int is_star = planet->level == LEVEL_STAR;
    float distance_star = 0.0;

    if (!is_star)
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

        // Determine velocity and position shift
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

    // If star, get ship distance from star
    if (is_star)
    {
        float delta_x_star = 0.0;
        float delta_y_star = 0.0;
        delta_x_star = planet->position.x - ship->position.x;
        delta_y_star = planet->position.y - ship->position.y;
        distance_star = sqrt(delta_x_star * delta_x_star + delta_y_star * delta_y_star);
    }

    // Don't update if it's a star and ship is outside STAR_CUTOFF
    if (!is_star || (is_star && distance_star < STAR_CUTOFF * planet->radius))
    {
        int max_planets = (planet->level == LEVEL_STAR) ? MAX_PLANETS : MAX_MOONS;

        // Update planets
        for (int i = 0; i < max_planets && planet->planets[i] != NULL; i++)
        {
            update_planets(planet->planets[i], ship, camera);
        }
    }

    // Draw planet if in camera
    if (planet->position.x - planet->radius <= camera->x + camera->w && planet->position.x + planet->radius > camera->x &&
        planet->position.y - planet->radius <= camera->y + camera->h && planet->position.y + planet->radius > camera->y)
    {
        planet->rect.x = (int)(planet->position.x - planet->radius - camera->x);
        planet->rect.y = (int)(planet->position.y - planet->radius - camera->y);

        SDL_RenderCopy(renderer, planet->texture, NULL, &planet->rect);
    }
    // Draw planet projection
    else
        project_planet(planet, camera);

    // Update ship speed due to gravity
    apply_gravity_to_ship(planet, ship, camera);
}

/*
 * Draw planet projection on axis
 */
void project_planet(struct planet_t *planet, const struct camera_t *camera)
{
    float delta_x, delta_y, point;

    // Find screen quadrant for planet exit from screen; 0, 0 is screen center, negative y is up
    delta_x = planet->position.x - (camera->x + (camera->w / 2));
    delta_y = planet->position.y - (camera->y + (camera->h / 2));

    // 1st quadrant (clockwise)
    if (delta_x >= 0 && delta_y < 0)
    {
        point = (int)(((camera->h / 2) - PROJECTION_OFFSET) * delta_x / (-delta_y));

        // Top
        if (point <= (camera->w / 2) - PROJECTION_OFFSET)
        {
            planet->projection.x = (camera->w / 2) + point;
            planet->projection.y = PROJECTION_OFFSET;
        }
        // Right
        else
        {
            if (delta_x > 0)
                point = (int)(((camera->h / 2) - PROJECTION_OFFSET) - ((camera->w / 2) - PROJECTION_OFFSET) * (-delta_y) / delta_x);
            else
                point = (int)(((camera->h / 2) - PROJECTION_OFFSET) - ((camera->w / 2) - PROJECTION_OFFSET));

            planet->projection.x = camera->w - PROJECTION_OFFSET;
            planet->projection.y = point + PROJECTION_OFFSET;
        }
    }
    // 2nd quadrant
    else if (delta_x >= 0 && delta_y >= 0)
    {
        if (delta_x > 0)
            point = (int)(((camera->w / 2) - PROJECTION_OFFSET) * delta_y / delta_x);
        else
            point = (int)((camera->w / 2) - PROJECTION_OFFSET);

        // Right
        if (point <= (camera->h / 2) - PROJECTION_OFFSET)
        {
            planet->projection.x = camera->w - PROJECTION_OFFSET;
            planet->projection.y = (camera->h / 2) + point;
        }
        // Bottom
        else
        {
            if (delta_y > 0)
                point = (int)(((camera->h / 2) - PROJECTION_OFFSET) * delta_x / delta_y);
            else
                point = (int)(((camera->h / 2) - PROJECTION_OFFSET));

            planet->projection.x = (camera->w / 2) + point;
            planet->projection.y = camera->h - PROJECTION_OFFSET;
        }
    }
    // 3rd quadrant
    else if (delta_x < 0 && delta_y >= 0)
    {
        if (delta_y > 0)
            point = (int)(((camera->h / 2) - PROJECTION_OFFSET) * (-delta_x) / delta_y);
        else
            point = (int)(((camera->h / 2) - PROJECTION_OFFSET));

        // Bottom
        if (point <= (camera->w / 2) - PROJECTION_OFFSET)
        {
            planet->projection.x = (camera->w / 2) - point;
            planet->projection.y = camera->h - PROJECTION_OFFSET;
        }
        // Left
        else
        {
            point = (int)(((camera->h / 2) - PROJECTION_OFFSET) - ((camera->w / 2) - PROJECTION_OFFSET) * delta_y / (-delta_x));
            planet->projection.x = PROJECTION_OFFSET;
            planet->projection.y = camera->h - point - PROJECTION_OFFSET;
        }
    }
    // 4th quadrant
    else if (delta_x < 0 && delta_y < 0)
    {
        point = (int)(((camera->w / 2) - PROJECTION_OFFSET) * (-delta_y) / (-delta_x));

        // Left
        if (point <= (camera->h / 2) - PROJECTION_OFFSET)
        {
            planet->projection.x = PROJECTION_OFFSET;
            planet->projection.y = (camera->h / 2) - point;
        }
        // Top
        else
        {
            point = (int)(((camera->w / 2) - PROJECTION_OFFSET) - ((camera->h / 2) - PROJECTION_OFFSET) * (-delta_x) / (-delta_y));
            planet->projection.x = point + PROJECTION_OFFSET;
            planet->projection.y = PROJECTION_OFFSET;
        }
    }

    planet->projection.w = 5;
    planet->projection.h = 5;
    SDL_SetRenderDrawColor(renderer, planet->color.r, planet->color.g, planet->color.b, 255);
    SDL_RenderFillRect(renderer, &planet->projection);
}

/*
 * Apply planet gravity to ship and update speed and velocity
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
    if (distance <= collision_point + ship->radius)
    {
        landing_stage = STAGE_0;
        g_planet = 0;

        if (is_star)
        {
            ship->vx = 0;
            ship->vy = 0;
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
    // Ship inside PLANET_CUTOFF
    else if ((is_star && distance < STAR_CUTOFF * planet->radius) ||
             (!is_star && distance < PLANET_CUTOFF * planet->radius))
    {
        g_planet = G_CONSTANT * (float)(planet->radius * planet->radius) / (distance * distance);

        ship->vx += g_planet * delta_x / distance;
        ship->vy += g_planet * delta_y / distance;
    }

    // Enforce speed limit if within STAR_CUTOFF
    if (!is_star || (is_star && distance < STAR_CUTOFF * planet->radius))
    {
        if (velocity > SPEED_LIMIT)
        {
            ship->vx = SPEED_LIMIT * ship->vx / velocity;
            ship->vy = SPEED_LIMIT * ship->vy / velocity;
        }
    }

    // Update velocity
    velocity = sqrt((ship->vx * ship->vx) + (ship->vy * ship->vy));
}

/*
 * Update ship position, listen for key controls and draw ship
 */
void update_ship(struct ship_t *ship, struct ship_t *ship_projection, const struct camera_t *camera)
{
    float radians;

    // Update ship angle
    if (right && !left && landing_stage == STAGE_OFF)
        ship->angle += 3;

    if (left && !right && landing_stage == STAGE_OFF)
        ship->angle -= 3;

    if (ship->angle > 360)
        ship->angle -= 360;

    // Apply ship thrust
    if (thrust)
    {
        landing_stage = STAGE_OFF;
        radians = M_PI * ship->angle / 180;

        ship->vx += g_thrust * sin(radians);
        ship->vy -= g_thrust * cos(radians);
    }

    // Update ship position
    ship->position.x += (float)ship->vx / FPS;
    ship->position.y += (float)ship->vy / FPS;

    if (CAMERA_ON)
    {
        // Static rect position at center of screen fixes flickering caused by float-to-int inaccuracies
        ship->rect.x = (camera->w / 2) - ship->radius;
        ship->rect.y = (camera->h / 2) - ship->radius;
    }
    else
    {
        // Dynamic rect position based on ship position
        ship->rect.x = (int)(ship->position.x - ship->radius - camera->x);
        ship->rect.y = (int)(ship->position.y - ship->radius - camera->y);
    }

    // Draw ship if in camera
    if (CAMERA_ON || (!CAMERA_ON && (ship->position.x <= camera->x + camera->w && ship->position.x > camera->x &&
                                     ship->position.y <= camera->y + camera->h && ship->position.y > camera->y)))
    {
        SDL_RenderCopyEx(renderer, ship->texture, &ship->main_img_rect, &ship->rect, ship->angle, &ship->rotation_pt, SDL_FLIP_NONE);
    }
    // Draw ship projection
    else
        project_ship(ship, ship_projection, camera);

    // Draw ship thrust
    if (thrust)
        SDL_RenderCopyEx(renderer, ship->texture, &ship->thrust_img_rect, &ship->rect, ship->angle, &ship->rotation_pt, SDL_FLIP_NONE);
}

/*
 * Draw ship projection on axis
 */
void project_ship(struct ship_t *ship, struct ship_t *projection, const struct camera_t *camera)
{
    float delta_x, delta_y, point;

    // Find screen quadrant for ship exit from screen; 0, 0 is screen center, negative y is up
    delta_x = ship->position.x - (camera->x + (camera->w / 2));
    delta_y = ship->position.y - (camera->y + (camera->h / 2));

    // Mirror ship angle
    projection->angle = ship->angle;

    // 1st quadrant (clockwise)
    if (delta_x >= 0 && delta_y < 0)
    {
        point = (int)(((camera->h / 2) - PROJECTION_OFFSET) * delta_x / (-delta_y));

        // Top
        if (point <= (camera->w / 2) - PROJECTION_OFFSET)
        {
            projection->rect.x = (camera->w / 2) + point - PROJECTION_OFFSET;
            projection->rect.y = 0;
        }
        // Right
        else
        {
            if (delta_x > 0)
                point = (int)(((camera->h / 2) - PROJECTION_OFFSET) - ((camera->w / 2) - PROJECTION_OFFSET) * (-delta_y) / delta_x);
            else
                point = (int)(((camera->h / 2) - PROJECTION_OFFSET) - ((camera->w / 2) - PROJECTION_OFFSET));

            projection->rect.x = camera->w - (2 * PROJECTION_OFFSET);
            projection->rect.y = point;
        }
    }
    // 2nd quadrant
    else if (delta_x >= 0 && delta_y >= 0)
    {
        if (delta_x > 0)
            point = (int)(((camera->w / 2) - PROJECTION_OFFSET) * delta_y / delta_x);
        else
            point = (int)(((camera->w / 2) - PROJECTION_OFFSET));

        // Right
        if (point <= (camera->h / 2) - PROJECTION_OFFSET)
        {
            projection->rect.x = camera->w - (2 * PROJECTION_OFFSET);
            projection->rect.y = (camera->h / 2) + point - PROJECTION_OFFSET;
        }
        // Bottom
        else
        {
            if (delta_y > 0)
                point = (int)(((camera->h / 2) - PROJECTION_OFFSET) * delta_x / delta_y);
            else
                point = (int)(((camera->h / 2) - PROJECTION_OFFSET));

            projection->rect.x = (camera->w / 2) + point - PROJECTION_OFFSET;
            projection->rect.y = camera->h - (2 * PROJECTION_OFFSET);
        }
    }
    // 3rd quadrant
    else if (delta_x < 0 && delta_y >= 0)
    {
        if (delta_y > 0)
            point = (int)(((camera->h / 2) - PROJECTION_OFFSET) * (-delta_x) / delta_y);
        else
            point = (int)(((camera->h / 2) - PROJECTION_OFFSET));

        // Bottom
        if (point <= (camera->w / 2) - PROJECTION_OFFSET)
        {
            projection->rect.x = (camera->w / 2) - point - PROJECTION_OFFSET;
            projection->rect.y = camera->h - (2 * PROJECTION_OFFSET);
        }
        // Left
        else
        {
            point = (int)(((camera->h / 2) - PROJECTION_OFFSET) - ((camera->w / 2) - PROJECTION_OFFSET) * delta_y / (-delta_x));
            projection->rect.x = 0;
            projection->rect.y = camera->h - point - (2 * PROJECTION_OFFSET);
        }
    }
    // 4th quadrant
    else if (delta_x < 0 && delta_y < 0)
    {
        point = (int)(((camera->w / 2) - PROJECTION_OFFSET) * (-delta_y) / (-delta_x));

        // Left
        if (point <= (camera->h / 2) - PROJECTION_OFFSET)
        {
            projection->rect.x = 0;
            projection->rect.y = (camera->h / 2) - point - PROJECTION_OFFSET;
        }
        // Top
        else
        {
            point = (int)(((camera->w / 2) - PROJECTION_OFFSET) - ((camera->h / 2) - PROJECTION_OFFSET) * (-delta_x) / (-delta_y));
            projection->rect.x = point;
            projection->rect.y = 0;
        }
    }

    // Draw ship projection
    SDL_RenderCopyEx(renderer, projection->texture, &projection->main_img_rect, &projection->rect, projection->angle, &projection->rotation_pt, SDL_FLIP_NONE);

    // Draw projection thrust
    if (thrust)
        SDL_RenderCopyEx(renderer, projection->texture, &projection->thrust_img_rect, &projection->rect, projection->angle, &projection->rotation_pt, SDL_FLIP_NONE);
}
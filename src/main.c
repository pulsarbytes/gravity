/*
 * Gravity - A basic 2d game engine in C that models gravity and orbital motion
 *
 * v1.0.0
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

#define FPS 60
#define STARS_SQUARE 10000
#define STARS_PER_SQUARE 5
#define SHIP_RADIUS 17
#define SHIP_STARTING_X 0
#define SHIP_STARTING_Y -700
#define STAR_CUTOFF 60
#define PLANET_CUTOFF 10
#define LANDING_CUTOFF 3
#define SPEED_LIMIT 300
#define APPROACH_LIMIT 100
#define PROJECTION_OFFSET 10
#define CAMERA_ON 1
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
void cleanup_resources(struct planet_t *planet, struct ship_t *ship);
float orbital_velocity(float height, int radius);
int create_bgstars(struct bgstar_t bgstars[], int max_bgstars, struct ship_t *);
void update_bgstars(struct bgstar_t bgstars[], int stars_count, struct ship_t *, const struct camera_t *);
struct planet_t create_solar_system(void);
struct ship_t create_ship(void);
void update_planets(struct planet_t *planet, struct planet_t *parent, struct ship_t *, const struct camera_t *);
void project_planet(struct planet_t *, const struct camera_t *);
void update_ship_velocity(struct planet_t *planet, struct planet_t *parent, struct ship_t *, const struct camera_t *);
void update_camera(struct camera_t *, struct ship_t *);
void update_ship(struct ship_t *, const struct camera_t *);

int main(int argc, char *argv[])
{
    int quit = 0;

    // Global coordinates
    int x_coord = 0;
    int y_coord = 0;

    // Initialize SDL
    if (!init_sdl())
    {
        fprintf(stderr, "Error: could not initialize SDL.\n");
        return 1;
    }

    // Create ship
    struct ship_t ship = create_ship();

    // Create camera, sync initial position with ship
    struct camera_t camera = {
        .x = ship.position.x - (display_mode.w / 2),
        .y = ship.position.y - (display_mode.h / 2),
        .w = display_mode.w,
        .h = display_mode.h};

    // Create solar system
    struct planet_t sol = create_solar_system();

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
        if (CAMERA_ON)
        {
            update_bgstars(bgstars, bgstars_count, &ship, &camera);
        }

        // Updates planets in solar system recursively
        update_planets(&sol, NULL, &ship, &camera);

        // Update camera
        if (CAMERA_ON)
        {
            update_camera(&camera, &ship);
        }

        // Update ship
        update_ship(&ship, &camera);

        // Update coordinates
        x_coord = ship.position.x;
        y_coord = ship.position.y;

        // Log coordinates (relative to sol)
        log_game_console(game_console_entries, X_INDEX, x_coord);
        log_game_console(game_console_entries, Y_INDEX, y_coord);

        // Log velocity (relative to sol)
        log_game_console(game_console_entries, V_INDEX, velocity);

        // Update game console
        if (console)
        {
            update_game_console(game_console_entries);
        }

        // Switch buffers, display back buffer
        SDL_RenderPresent(renderer);

        // Set FPS
        if ((1000 / FPS) > ((end_time = SDL_GetTicks()) - start_time))
        {
            SDL_Delay((1000 / FPS) - (end_time - start_time));
        }

        // Log FPS
        log_fps(end_time - start_time);
    }

    // Destroy game console
    destroy_game_console(game_console_entries);

    // Cleanup resources
    cleanup_resources(&sol, &ship);

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
                star.position.x = column + ship->position.x;
                star.position.y = row + ship->position.y;
                star.rect.x = 0;
                star.rect.y = 0;

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
            {
                end = TRUE;
            }
        }
    }

    return i;
}

/*
 * Move and draw stars background
 */
void update_bgstars(struct bgstar_t bgstars[], int stars_count, struct ship_t *ship, const struct camera_t *camera)
{
    int i;

    for (i = 0; i < stars_count; i++)
    {
        bgstars[i].position.x -= 0.2 * ship->vx / FPS;
        bgstars[i].position.y -= 0.2 * ship->vy / FPS;

        bgstars[i].rect.x = (int)(bgstars[i].position.x + (camera->w / 2));
        bgstars[i].rect.y = (int)(bgstars[i].position.y + (camera->h / 2));

        // Right boundary
        if (bgstars[i].position.x > ship->position.x - camera->x)
        {
            bgstars[i].position.x -= camera->w;
        }
        // Left boundary
        else if (bgstars[i].position.x < camera->x - ship->position.x)
        {
            bgstars[i].position.x += camera->w;
        }

        // Top boundary
        if (bgstars[i].position.y > ship->position.y - camera->y)
        {
            bgstars[i].position.y -= camera->h;
        }
        // Bottom boundary
        else if (bgstars[i].position.y < camera->y - ship->position.y)
        {
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
struct ship_t create_ship(void)
{
    struct ship_t ship;

    ship.image = "../assets/sprites/ship.png";
    ship.radius = SHIP_RADIUS;
    ship.position.x = SHIP_STARTING_X;
    ship.position.y = SHIP_STARTING_Y;
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
 * Create a solar system
 */
struct planet_t create_solar_system(void)
{
    // Sol
    struct planet_t sol;

    sol.name = "Sol";
    sol.image = "../assets/images/sol.png";
    sol.radius = 250;
    sol.position.x = 0;
    sol.position.y = 0;
    sol.vx = 0;
    sol.vy = 0;
    sol.dx = 0;
    sol.dy = 0;
    SDL_Surface *sol_surface = IMG_Load(sol.image);
    sol.texture = SDL_CreateTextureFromSurface(renderer, sol_surface);
    SDL_FreeSurface(sol_surface);
    sol.rect.x = sol.position.x - sol.radius;
    sol.rect.y = sol.position.y - sol.radius;
    sol.rect.w = 2 * sol.radius;
    sol.rect.h = 2 * sol.radius;
    sol.color.r = 255;
    sol.color.g = 255;
    sol.color.b = 0;
    sol.moons[0] = NULL;

    // Mercury
    struct planet_t *mercury = NULL;
    mercury = (struct planet_t *)malloc(sizeof(struct planet_t));

    if (mercury != NULL)
    {
        mercury->name = "Mercury";
        mercury->image = "../assets/images/mercury.png";
        mercury->radius = 60;
        mercury->position.x = sol.position.x;
        mercury->position.y = sol.position.y - 1500;
        mercury->vx = orbital_velocity(abs((int)sol.position.y - (int)mercury->position.y), sol.radius); // Initial velocity
        mercury->vy = 0;
        mercury->dx = 0;
        mercury->dy = 0;
        SDL_Surface *mercury_surface = IMG_Load(mercury->image);
        mercury->texture = SDL_CreateTextureFromSurface(renderer, mercury_surface);
        SDL_FreeSurface(mercury_surface);
        mercury->rect.x = mercury->position.x - mercury->radius;
        mercury->rect.y = mercury->position.y - mercury->radius;
        mercury->rect.w = 2 * mercury->radius;
        mercury->rect.h = 2 * mercury->radius;
        mercury->color.r = 192;
        mercury->color.g = 192;
        mercury->color.b = 192;
        mercury->moons[0] = NULL;

        sol.moons[0] = mercury;
        sol.moons[1] = NULL;
    }

    // Venus
    struct planet_t *venus = NULL;
    venus = (struct planet_t *)malloc(sizeof(struct planet_t));

    if (venus != NULL)
    {
        venus->name = "Venus";
        venus->image = "../assets/images/venus.png";
        venus->radius = 100;
        venus->position.x = sol.position.x;
        venus->position.y = sol.position.y - 3000;
        venus->vx = orbital_velocity(abs((int)sol.position.y - (int)venus->position.y), sol.radius); // Initial velocity
        venus->vy = 0;
        venus->dx = 0;
        venus->dy = 0;
        SDL_Surface *venus_surface = IMG_Load(venus->image);
        venus->texture = SDL_CreateTextureFromSurface(renderer, venus_surface);
        SDL_FreeSurface(venus_surface);
        venus->rect.x = venus->position.x - venus->radius;
        venus->rect.y = venus->position.y - venus->radius;
        venus->rect.w = 2 * venus->radius;
        venus->rect.h = 2 * venus->radius;
        venus->color.r = 215;
        venus->color.g = 140;
        venus->color.b = 0;
        venus->moons[0] = NULL;

        sol.moons[1] = venus;
        sol.moons[2] = NULL;
    }

    // Earth
    struct planet_t *earth = NULL;
    earth = (struct planet_t *)malloc(sizeof(struct planet_t));

    if (earth != NULL)
    {
        earth->name = "Earth";
        earth->image = "../assets/images/earth.png";
        earth->radius = 100;
        earth->position.x = sol.position.x;
        earth->position.y = sol.position.y - 4500;
        earth->vx = orbital_velocity(abs((int)sol.position.y - (int)earth->position.y), sol.radius); // Initial velocity
        earth->vy = 0;
        earth->dx = 0;
        earth->dy = 0;
        SDL_Surface *earth_surface = IMG_Load(earth->image);
        earth->texture = SDL_CreateTextureFromSurface(renderer, earth_surface);
        SDL_FreeSurface(earth_surface);
        earth->rect.x = earth->position.x - earth->radius;
        earth->rect.y = earth->position.y - earth->radius;
        earth->rect.w = 2 * earth->radius;
        earth->rect.h = 2 * earth->radius;
        earth->color.r = 135;
        earth->color.g = 206;
        earth->color.b = 235;
        earth->moons[0] = NULL;

        sol.moons[2] = earth;
        sol.moons[3] = NULL;

        // Moon
        struct planet_t *moon = NULL;
        moon = (struct planet_t *)malloc(sizeof(struct planet_t));

        if (moon != NULL)
        {
            moon->name = "Moon";
            moon->image = "../assets/images/moon.png";
            moon->radius = 50;
            moon->position.x = earth->position.x;
            moon->position.y = earth->position.y - 1200;
            moon->vx = orbital_velocity(abs((int)earth->position.y - (int)moon->position.y), earth->radius); // Initial velocity
            moon->vy = 0;
            moon->dx = 0;
            moon->dy = 0;
            SDL_Surface *moon_surface = IMG_Load(moon->image);
            moon->texture = SDL_CreateTextureFromSurface(renderer, moon_surface);
            SDL_FreeSurface(moon_surface);
            moon->rect.x = moon->position.x - moon->radius;
            moon->rect.y = moon->position.y - moon->radius;
            moon->rect.w = 2 * moon->radius;
            moon->rect.h = 2 * moon->radius;
            moon->color.r = 220;
            moon->color.g = 220;
            moon->color.b = 220;
            moon->moons[0] = NULL;

            earth->moons[0] = moon;
            earth->moons[1] = NULL;
        }
    }

    // Mars
    struct planet_t *mars = NULL;
    mars = (struct planet_t *)malloc(sizeof(struct planet_t));

    if (mars != NULL)
    {
        mars->name = "Mars";
        mars->image = "../assets/images/mars.png";
        mars->radius = 70;
        mars->position.x = sol.position.x;
        mars->position.y = sol.position.y - 6000;
        mars->vx = orbital_velocity(abs((int)sol.position.y - (int)mars->position.y), sol.radius); // Initial velocity
        mars->vy = 0;
        mars->dx = 0;
        mars->dy = 0;
        SDL_Surface *mars_surface = IMG_Load(mars->image);
        mars->texture = SDL_CreateTextureFromSurface(renderer, mars_surface);
        SDL_FreeSurface(mars_surface);
        mars->rect.x = mars->position.x - mars->radius;
        mars->rect.y = mars->position.y - mars->radius;
        mars->rect.w = 2 * mars->radius;
        mars->rect.h = 2 * mars->radius;
        mars->color.r = 255;
        mars->color.g = 69;
        mars->color.b = 0;
        mars->moons[0] = NULL;

        sol.moons[3] = mars;
        sol.moons[4] = NULL;
    }

    // Jupiter
    struct planet_t *jupiter = NULL;
    jupiter = (struct planet_t *)malloc(sizeof(struct planet_t));

    if (jupiter != NULL)
    {
        jupiter->name = "Jupiter";
        jupiter->image = "../assets/images/jupiter.png";
        jupiter->radius = 160;
        jupiter->position.x = sol.position.x;
        jupiter->position.y = sol.position.y - 7800;
        jupiter->vx = orbital_velocity(abs((int)sol.position.y - (int)jupiter->position.y), sol.radius); // Initial velocity
        jupiter->vy = 0;
        jupiter->dx = 0;
        jupiter->dy = 0;
        SDL_Surface *jupiter_surface = IMG_Load(jupiter->image);
        jupiter->texture = SDL_CreateTextureFromSurface(renderer, jupiter_surface);
        SDL_FreeSurface(jupiter_surface);
        jupiter->rect.x = jupiter->position.x - jupiter->radius;
        jupiter->rect.y = jupiter->position.y - jupiter->radius;
        jupiter->rect.w = 2 * jupiter->radius;
        jupiter->rect.h = 2 * jupiter->radius;
        jupiter->color.r = 244;
        jupiter->color.g = 164;
        jupiter->color.b = 96;
        jupiter->moons[0] = NULL;

        sol.moons[4] = jupiter;
        sol.moons[5] = NULL;
    }

    return sol;
}

/*
 * Update planets positions and draw planets (recursive)
 */
void update_planets(struct planet_t *planet, struct planet_t *parent, struct ship_t *ship, const struct camera_t *camera)
{
    if (parent != NULL)
    {
        float delta_x = 0.0;
        float delta_y = 0.0;
        float distance;
        float g_planet = G_CONSTANT;
        float dx = parent->dx;
        float dy = parent->dy;

        // Update planet position
        planet->position.x += parent->dx;
        planet->position.y += parent->dy;

        // Find distance from parent
        delta_x = parent->position.x - planet->position.x;
        delta_y = parent->position.y - planet->position.y;
        distance = sqrt(delta_x * delta_x + delta_y * delta_y);

        // Determine velocity and position shift
        if (distance > (parent->radius + planet->radius))
        {
            g_planet = G_CONSTANT * (float)(parent->radius * parent->radius) / (distance * distance);

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

    int i = 0;

    for (i = 0; i < MAX_MOONS && planet->moons[i] != NULL; i++)
    {
        update_planets(planet->moons[i], planet, ship, camera);
    }

    planet->rect.x = (int)(planet->position.x - planet->radius - camera->x);
    planet->rect.y = (int)(planet->position.y - planet->radius - camera->y);

    // Draw planet if in camera
    if (planet->position.x - planet->radius <= camera->x + camera->w && planet->position.x + planet->radius > camera->x &&
        planet->position.y - planet->radius <= camera->y + camera->h && planet->position.y + planet->radius > camera->y)
    {
        SDL_RenderCopy(renderer, planet->texture, NULL, &planet->rect);
    }
    // Draw planet projection
    else
    {
        project_planet(planet, camera);
    }

    // Update ship velocity
    update_ship_velocity(planet, parent, ship, camera);
}

/*
 * Draw planet projection on axis
 */
void project_planet(struct planet_t *planet, const struct camera_t *camera)
{
    float delta_x, delta_y, point;

    delta_x = planet->position.x - (camera->x + ((camera->w / 2) - PROJECTION_OFFSET));
    delta_y = planet->position.y - (camera->y + ((camera->h / 2) - PROJECTION_OFFSET));

    if (delta_x > 0 && delta_y < 0)
    {
        point = (int)(((camera->h / 2) - PROJECTION_OFFSET) * delta_x / (-delta_y));

        if (point <= (camera->w / 2) - PROJECTION_OFFSET)
        {
            planet->projection.x = (camera->w / 2) + point;
            planet->projection.y = PROJECTION_OFFSET;
        }
        else
        {
            point = (int)(((camera->h / 2) - PROJECTION_OFFSET) - ((camera->w / 2) - PROJECTION_OFFSET) * (-delta_y) / delta_x);
            planet->projection.x = camera->w - PROJECTION_OFFSET;
            planet->projection.y = point + PROJECTION_OFFSET;
        }
    }
    else if (delta_x > 0 && delta_y > 0)
    {
        point = (int)(((camera->w / 2) - PROJECTION_OFFSET) * delta_y / delta_x);

        if (point <= (camera->h / 2) - PROJECTION_OFFSET)
        {
            planet->projection.x = camera->w - PROJECTION_OFFSET;
            planet->projection.y = (camera->h / 2) + point;
        }
        else
        {
            point = (int)(((camera->h / 2) - PROJECTION_OFFSET) * delta_x / delta_y);
            planet->projection.x = (camera->w / 2) + point;
            planet->projection.y = camera->h - PROJECTION_OFFSET;
        }
    }
    else if (delta_x < 0 && delta_y > 0)
    {
        point = (int)(((camera->h / 2) - PROJECTION_OFFSET) * (-delta_x) / delta_y);

        if (point <= (camera->w / 2) - PROJECTION_OFFSET)
        {
            planet->projection.x = (camera->w / 2) - point;
            planet->projection.y = camera->h - PROJECTION_OFFSET;
        }
        else
        {
            point = (int)(((camera->h / 2) - PROJECTION_OFFSET) - ((camera->w / 2) - PROJECTION_OFFSET) * delta_y / (-delta_x));
            planet->projection.x = PROJECTION_OFFSET;
            planet->projection.y = camera->h - PROJECTION_OFFSET - point;
        }
    }
    else if (delta_x < 0 && delta_y < 0)
    {
        point = (int)(((camera->w / 2) - PROJECTION_OFFSET) * (-delta_y) / (-delta_x));

        if (point <= (camera->h / 2) - PROJECTION_OFFSET)
        {
            planet->projection.x = PROJECTION_OFFSET;
            planet->projection.y = (camera->h / 2) - point;
        }
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
 * Update ship velocity
 */
void update_ship_velocity(struct planet_t *planet, struct planet_t *parent, struct ship_t *ship, const struct camera_t *camera)
{
    float delta_x = 0.0;
    float delta_y = 0.0;
    float distance;
    float g_planet = 0;
    int is_star = parent == NULL;
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
            ship->vx += parent->vx;
            ship->vy += parent->vy;
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

    velocity = sqrt((ship->vx * ship->vx) + (ship->vy * ship->vy));
}

/*
 * Update ship position and draw ship
 */
void update_ship(struct ship_t *ship, const struct camera_t *camera)
{
    float radians;

    // Update ship angle
    if (right && !left && landing_stage == STAGE_OFF)
    {
        ship->angle += 3;
    }

    if (left && !right && landing_stage == STAGE_OFF)
    {
        ship->angle -= 3;
    }

    if (ship->angle > 360)
    {
        ship->angle -= 360;
    }

    // Apply ship thrust
    if (thrust)
    {
        landing_stage = STAGE_OFF;
        radians = M_PI * ship->angle / 180;

        ship->vx += g_thrust * sin(radians);
        ship->vy -= g_thrust * cos(radians);
    }

    // Speed limit
    if (velocity > SPEED_LIMIT)
    {
        ship->vx = SPEED_LIMIT * ship->vx / velocity;
        ship->vy = SPEED_LIMIT * ship->vy / velocity;
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

    // Draw ship
    SDL_RenderCopyEx(renderer, ship->texture, &ship->main_img_rect, &ship->rect, ship->angle, &ship->rotation_pt, SDL_FLIP_NONE);

    // Draw ship thrust
    if (thrust)
    {
        SDL_RenderCopyEx(renderer, ship->texture, &ship->thrust_img_rect, &ship->rect, ship->angle, &ship->rotation_pt, SDL_FLIP_NONE);
    }
}

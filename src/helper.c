/*
 * helper.c - Definitions for helper functions.
 */

#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"
#include "../lib/pcg-c-basic-0.9/pcg_basic.h"

// External variable definitions
extern SDL_Renderer *renderer;
extern SDL_Color colors[];

// Function prototypes
static void cleanup_planets(struct planet_t *planet);
void cleanup_galaxies(struct galaxy_entry *galaxies[]);
void cleanup_stars(struct star_entry *stars[]);
void cleanup_resources(GameState *game_state, NavigationState *nav_state, struct ship_t *ship);
void put_galaxy(struct galaxy_entry *galaxies[], struct point_t position, struct galaxy_t *galaxy);
int galaxy_exists(struct galaxy_entry *galaxies[], struct point_t position);
struct galaxy_t *get_galaxy(struct galaxy_entry *galaxies[], struct point_t position);
void delete_galaxy(struct galaxy_entry *galaxies[], struct point_t position);
void put_star(struct star_entry *stars[], struct point_t position, struct planet_t *star);
int star_exists(struct star_entry *stars[], struct point_t position);
void delete_star(struct star_entry *stars[], struct point_t position);
void update_projection_coordinates(NavigationState nav_state, void *ptr, int entity_type, const struct camera_t *camera, int state, long double scale);
void project_ship(int state, InputState input_state, NavigationState nav_state, struct ship_t *ship, const struct camera_t *camera, long double scale);
void project_planet(GameState game_state, NavigationState nav_state, struct planet_t *planet, const struct camera_t *camera);
void project_galaxy(int state, NavigationState nav_state, struct galaxy_t *galaxy, const struct camera_t *camera, long double scale);
int calculate_projection_opacity(double distance, int region_size, int section_size);
double nearest_galaxy_center_distance(struct point_t position);
double find_distance(double x1, double y1, double x2, double y2);
struct galaxy_t *find_nearest_galaxy(NavigationState nav_state, struct point_t position, int exclude);
double nearest_star_distance(struct point_t position, struct galaxy_t *current_galaxy, uint64_t initseq, int galaxy_density);
int get_galaxy_class(float distance);
int get_star_class(float distance);
int get_planet_class(float width);
bool point_eq(struct point_t a, struct point_t b);
bool point_in_array(struct point_t p, struct point_t arr[], int len);
void zoom_star(struct planet_t *planet, long double scale);
int in_camera_relative(const struct camera_t *camera, int x, int y);
int in_camera(const struct camera_t *camera, double x, double y, float radius, long double scale);
void draw_screen_frame(struct camera_t *camera);
bool line_intersects_viewport(const struct camera_t *camera, double x1, double y1, double x2, double y2);
void draw_section_lines(struct camera_t *camera, int section_size, SDL_Color color, long double scale);
void create_menu_galaxy_cloud(struct galaxy_t *galaxy, struct gstar_t menustars[]);
void draw_menu_galaxy_cloud(const struct camera_t *camera, struct gstar_t menustars[]);
void create_galaxy_cloud(struct galaxy_t *galaxy, unsigned short high_definition);
void draw_galaxy_cloud(struct galaxy_t *galaxy, const struct camera_t *camera, int gstars_count, unsigned short high_definition, long double scale);
void draw_speed_arc(struct ship_t *ship, const struct camera_t *camera, long double scale);
void delete_stars_outside_region(struct star_entry *stars[], double bx, double by, int region_size);
void draw_speed_lines(float velocity, const struct camera_t *camera, struct speed_t speed);
void create_colors(void);

// External function prototypes
uint64_t pair_hash_order_sensitive(struct point_t);
uint64_t pair_hash_order_sensitive_2(struct point_t);
uint64_t double_hash(double x);
double find_nearest_section_axis(double offset, int size);
uint64_t unique_index(struct point_t, int modulo, int entity_type);

/*
 * Clean up planets (recursive).
 */
static void cleanup_planets(struct planet_t *planet)
{
    int planets_size = sizeof(planet->planets) / sizeof(planet->planets[0]);
    int i;

    for (i = 0; i < planets_size && planet->planets[i] != NULL; i++)
    {
        cleanup_planets(planet->planets[i]);
    }

    SDL_DestroyTexture(planet->texture);
    planet->texture = NULL;
}

/*
 * Clean up galaxies.
 */
void cleanup_galaxies(struct galaxy_entry *galaxies[])
{
    // Loop through hash table
    for (int s = 0; s < MAX_GALAXIES; s++)
    {
        struct galaxy_entry *entry = galaxies[s];

        while (entry != NULL)
        {
            struct point_t position = {.x = entry->x, .y = entry->y};
            delete_galaxy(galaxies, position);
            entry = entry->next;
        }
    }
}

/*
 * Delete all stars from hash table.
 */
void cleanup_stars(struct star_entry *stars[])
{
    // Loop through hash table
    for (int s = 0; s < MAX_STARS; s++)
    {
        struct star_entry *entry = stars[s];

        while (entry != NULL)
        {
            struct point_t position = {.x = entry->x, .y = entry->y};
            delete_star(stars, position);
            entry = entry->next;
        }
    }
}

/*
 * Clean up resources.
 */
void cleanup_resources(GameState *game_state, NavigationState *nav_state, struct ship_t *ship)
{
    // Clean up galaxies
    cleanup_galaxies(nav_state->galaxies);

    // Clean up stars
    cleanup_stars(nav_state->stars);

    // Clean up ship
    SDL_DestroyTexture(ship->projection->texture);
    ship->projection->texture = NULL;
    SDL_DestroyTexture(ship->texture);
    ship->texture = NULL;

    // Clean up galaxies
    free(nav_state->current_galaxy);
    free(nav_state->buffer_galaxy);
    free(nav_state->previous_galaxy);

    // Clean up menu texttures
    for (int i = 0; i < MENU_BUTTON_COUNT; i++)
    {
        SDL_DestroyTexture(game_state->menu[i].texture);
    }

    // Clean up logo texture
    SDL_DestroyTexture(game_state->logo.texture);
}

/*
 * Insert a new galaxy entry in galaxies hash table.
 */
void put_galaxy(struct galaxy_entry *galaxies[], struct point_t position, struct galaxy_t *galaxy)
{
    // Generate unique index for hash table
    uint64_t index = unique_index(position, MAX_GALAXIES, ENTITY_GALAXY);

    struct galaxy_entry *entry = (struct galaxy_entry *)malloc(sizeof(struct galaxy_entry));
    entry->x = position.x;
    entry->y = position.y;
    entry->galaxy = galaxy;
    entry->next = galaxies[index];
    galaxies[index] = entry;
}

/*
 * Check whether a galaxy entry exists in the galaxies hash table.
 */
int galaxy_exists(struct galaxy_entry *galaxies[], struct point_t position)
{
    // Generate unique index for hash table
    uint64_t index = unique_index(position, MAX_GALAXIES, ENTITY_GALAXY);

    struct galaxy_entry *entry = galaxies[index];

    while (entry != NULL)
    {
        if (entry->galaxy == NULL)
            return false;

        if (entry->x == position.x && entry->y == position.y)
            return true;

        entry = entry->next;
    }

    return false;
}

/*
 * Get a galaxy entry from the galaxies hash table.
 */
struct galaxy_t *get_galaxy(struct galaxy_entry *galaxies[], struct point_t position)
{
    // Generate unique index for hash table
    uint64_t index = unique_index(position, MAX_GALAXIES, ENTITY_GALAXY);

    struct galaxy_entry *entry = galaxies[index];

    while (entry != NULL)
    {
        if (entry->x == position.x && entry->y == position.y)
            return entry->galaxy;

        entry = entry->next;
    }

    return NULL;
}

/*
 * Delete a galaxy entry from the galaxies hash table.
 */
void delete_galaxy(struct galaxy_entry *galaxies[], struct point_t position)
{
    // Generate unique index for hash table
    uint64_t index = unique_index(position, MAX_GALAXIES, ENTITY_GALAXY);

    struct galaxy_entry *previous = NULL;
    struct galaxy_entry *entry = galaxies[index];

    while (entry != NULL)
    {
        if (entry->x == position.x && entry->y == position.y)
        {
            // Clean up galaxy
            free(entry->galaxy);
            entry->galaxy = NULL;

            if (previous == NULL)
                galaxies[index] = entry->next;
            else
                previous->next = entry->next;

            free(entry);

            return;
        }

        previous = entry;
        entry = entry->next;
    }
}

/*
 * Insert a new star entry in stars hash table.
 */
void put_star(struct star_entry *stars[], struct point_t position, struct planet_t *star)
{
    // Generate unique index for hash table
    uint64_t index = unique_index(position, MAX_STARS, ENTITY_STAR);

    struct star_entry *entry = (struct star_entry *)malloc(sizeof(struct star_entry));
    entry->x = position.x;
    entry->y = position.y;
    entry->star = star;
    entry->next = stars[index];
    stars[index] = entry;
}

/*
 * Check whether a star entry exists in the stars hash table.
 */
int star_exists(struct star_entry *stars[], struct point_t position)
{
    // Generate unique index for hash table
    uint64_t index = unique_index(position, MAX_STARS, ENTITY_STAR);

    struct star_entry *entry = stars[index];

    while (entry != NULL)
    {
        if (entry->star == NULL)
            return false;

        if (entry->x == position.x && entry->y == position.y)
            return true;

        entry = entry->next;
    }

    return false;
}

/*
 * Delete a star entry from the stars hash table.
 */
void delete_star(struct star_entry *stars[], struct point_t position)
{
    // Generate unique index for hash table
    uint64_t index = unique_index(position, MAX_STARS, ENTITY_STAR);

    struct star_entry *previous = NULL;
    struct star_entry *entry = stars[index];

    while (entry != NULL)
    {
        if (entry->x == position.x && entry->y == position.y)
        {
            // Clean up planets
            if (entry->star != NULL && entry->star->planets[0] != NULL)
                cleanup_planets(entry->star);

            // Clean up star
            if (entry->star->texture != NULL)
                SDL_DestroyTexture(entry->star->texture);

            entry->star->texture = NULL;
            free(entry->star);
            entry->star = NULL;

            if (previous == NULL)
                stars[index] = entry->next;
            else
                previous->next = entry->next;

            free(entry);

            return;
        }

        previous = entry;
        entry = entry->next;
    }
}

/*
 * Update projection position.
 * Projection rect top-left point is where the object projection crosses a quadrant line.
 * Projection rect can be centered by moving left or up by <offset>.
 */
void update_projection_coordinates(NavigationState nav_state, void *ptr, int entity_type, const struct camera_t *camera, int state, long double scale)
{
    struct galaxy_t *galaxy = NULL;
    struct planet_t *planet = NULL;
    struct ship_t *ship = NULL;
    double delta_x, delta_y, point;
    int offset;

    switch (entity_type)
    {
    case ENTITY_GALAXY:
        galaxy = (struct galaxy_t *)ptr;
        offset = PROJECTION_RADIUS;
        break;
    case ENTITY_PLANET:
        planet = (struct planet_t *)ptr;
        offset = PROJECTION_RADIUS;
        break;
    case ENTITY_SHIP:
        ship = (struct ship_t *)ptr;
        offset = 3 * PROJECTION_RADIUS + SHIP_PROJECTION_RADIUS;
        break;
    }

    if (state == UNIVERSE)
        scale = scale * GALAXY_SCALE;

    long double camera_w = camera->w / scale;
    long double camera_h = camera->h / scale;

    // Find screen quadrant for object exit from screen; 0, 0 is screen center, negative y is up
    if (galaxy)
    {
        if (state == NAVIGATE || state == MAP)
        {
            delta_x = galaxy->position.x * GALAXY_SCALE - camera->x - (camera_w / 2);
            delta_y = galaxy->position.y * GALAXY_SCALE - camera->y - (camera_h / 2);
        }
        else if (state == UNIVERSE)
        {
            delta_x = galaxy->position.x - camera->x - (camera_w / 2) * GALAXY_SCALE;
            delta_y = galaxy->position.y - camera->y - (camera_h / 2) * GALAXY_SCALE;
        }
    }
    else if (planet)
    {
        delta_x = planet->position.x - camera->x - (camera_w / 2);
        delta_y = planet->position.y - camera->y - (camera_h / 2);
    }
    else if (ship)
    {
        if (state == NAVIGATE || state == MAP)
        {
            // If in other galaxy, project position at buffer galaxy
            if (state == MAP &&
                (nav_state.current_galaxy->position.x != nav_state.buffer_galaxy->position.x || nav_state.current_galaxy->position.y != nav_state.buffer_galaxy->position.y))
            {
                // Convert camera to Universe coordinates
                long double camera_x = nav_state.current_galaxy->position.x + (nav_state.map_offset.x / GALAXY_SCALE) - (camera->w / 2);
                long double camera_y = nav_state.current_galaxy->position.y + (nav_state.map_offset.y / GALAXY_SCALE) - (camera->h / 2);

                delta_x = nav_state.buffer_galaxy->position.x + (ship->position.x / GALAXY_SCALE) - camera_x - (camera->w / 2);
                delta_y = nav_state.buffer_galaxy->position.y + (ship->position.y / GALAXY_SCALE) - camera_y - (camera->h / 2);
            }
            else
            {
                delta_x = ship->position.x - camera->x - (camera_w / 2);
                delta_y = ship->position.y - camera->y - (camera_h / 2);
            }
        }
        else if (state == UNIVERSE)
        {
            delta_x = (nav_state.buffer_galaxy->position.x + ship->position.x / GALAXY_SCALE - camera->x) - (camera_w / 2);
            delta_y = (nav_state.buffer_galaxy->position.y + ship->position.y / GALAXY_SCALE - camera->y) - (camera_h / 2);
        }
    }

    // 1st quadrant (clockwise)
    if (delta_x >= 0 && delta_y < 0)
    {
        point = (camera_h / 2) * delta_x / -delta_y;

        // Top
        if (point <= camera_w / 2)
        {
            if (galaxy)
            {
                galaxy->projection.x = ((camera_w / 2) + point) * scale - offset * (point / (camera_w / 2) + 1);
                galaxy->projection.y = 0;
            }
            else if (planet)
            {
                planet->projection.x = ((camera_w / 2) + point) * scale - offset * (point / (camera_w / 2) + 1);
                planet->projection.y = 0;
            }
            else if (ship)
            {
                ship->projection->rect.x = ((camera_w / 2) + point) * scale - offset * (point / (camera_w / 2) + 1) + 3 * PROJECTION_RADIUS;
                ship->projection->rect.y = 3 * PROJECTION_RADIUS;
            }
        }
        // Right
        else
        {
            if (delta_x >= 0)
                point = (camera_h / 2) - (camera_w / 2) * -delta_y / delta_x;
            else
                point = (camera_h / 2) - (camera_w / 2);

            if (galaxy)
            {
                galaxy->projection.x = camera_w * scale - (2 * offset);
                galaxy->projection.y = point * scale - offset * (point / (camera_h / 2));
            }
            else if (planet)
            {
                planet->projection.x = camera_w * scale - (2 * offset);
                planet->projection.y = point * scale - offset * (point / (camera_h / 2));
            }
            else if (ship)
            {
                ship->projection->rect.x = camera_w * scale - (2 * offset) + 3 * PROJECTION_RADIUS;
                ship->projection->rect.y = point * scale - offset * (point / (camera_h / 2)) + 3 * PROJECTION_RADIUS;
            }
        }
    }
    // 2nd quadrant
    else if (delta_x >= 0 && delta_y >= 0)
    {
        if (delta_x >= 0)
            point = (camera_w / 2) * delta_y / delta_x;
        else
            point = camera_w / 2;

        // Right
        if (point <= camera_h / 2)
        {
            if (galaxy)
            {
                galaxy->projection.x = camera_w * scale - (2 * offset);
                galaxy->projection.y = ((camera_h / 2) + point) * scale - offset * (point / (camera_h / 2) + 1);
            }
            else if (planet)
            {
                planet->projection.x = camera_w * scale - (2 * offset);
                planet->projection.y = ((camera_h / 2) + point) * scale - offset * (point / (camera_h / 2) + 1);
            }
            else if (ship)
            {
                ship->projection->rect.x = camera_w * scale - (2 * offset) + 3 * PROJECTION_RADIUS;
                ship->projection->rect.y = ((camera_h / 2) + point) * scale - offset * (point / (camera_h / 2) + 1) + 3 * PROJECTION_RADIUS;
            }
        }
        // Bottom
        else
        {
            if (delta_y > 0)
                point = (camera_h / 2) * delta_x / delta_y;
            else
                point = camera_h / 2;

            if (galaxy)
            {
                galaxy->projection.x = ((camera_w / 2) + point) * scale - offset * (point / (camera_w / 2) + 1);
                galaxy->projection.y = camera_h * scale - (2 * offset);
            }
            else if (planet)
            {
                planet->projection.x = ((camera_w / 2) + point) * scale - offset * (point / (camera_w / 2) + 1);
                planet->projection.y = camera_h * scale - (2 * offset);
            }
            else if (ship)
            {
                ship->projection->rect.x = ((camera_w / 2) + point) * scale - offset * (point / (camera_w / 2) + 1) + 3 * PROJECTION_RADIUS;
                ship->projection->rect.y = camera_h * scale - (2 * offset) + 3 * PROJECTION_RADIUS;
            }
        }
    }
    // 3rd quadrant
    else if (delta_x < 0 && delta_y >= 0)
    {
        if (delta_y >= 0)
            point = (camera_h / 2) * -delta_x / delta_y;
        else
            point = camera_h / 2;

        // Bottom
        if (point <= camera_w / 2)
        {
            if (galaxy)
            {
                galaxy->projection.x = ((camera_w / 2) - point) * scale - offset * (((camera_w / 2) - point) / (camera_w / 2));
                galaxy->projection.y = camera_h * scale - (2 * offset);
            }
            else if (planet)
            {
                planet->projection.x = ((camera_w / 2) - point) * scale - offset * (((camera_w / 2) - point) / (camera_w / 2));
                planet->projection.y = camera_h * scale - (2 * offset);
            }
            else if (ship)
            {
                ship->projection->rect.x = ((camera_w / 2) - point) * scale - offset * (((camera_w / 2) - point) / (camera_w / 2)) + 3 * PROJECTION_RADIUS;
                ship->projection->rect.y = camera_h * scale - (2 * offset) + 3 * PROJECTION_RADIUS;
            }
        }
        // Left
        else
        {
            point = (camera_h / 2) - (camera_w / 2) * delta_y / -delta_x;

            if (galaxy)
            {
                galaxy->projection.x = 0;
                galaxy->projection.y = (camera_h - point) * scale - offset * (((camera_h / 2) - point) / (camera_h / 2) + 1);
            }
            else if (planet)
            {
                planet->projection.x = 0;
                planet->projection.y = (camera_h - point) * scale - offset * (((camera_h / 2) - point) / (camera_h / 2) + 1);
            }
            else if (ship)
            {
                ship->projection->rect.x = 3 * PROJECTION_RADIUS;
                ship->projection->rect.y = (camera_h - point) * scale - offset * (((camera_h / 2) - point) / (camera_h / 2) + 1) + 3 * PROJECTION_RADIUS;
            }
        }
    }
    // 4th quadrant
    else if (delta_x < 0 && delta_y < 0)
    {
        point = (camera_w / 2) * -delta_y / -delta_x;

        // Left
        if (point <= camera_h / 2)
        {
            if (galaxy)
            {
                galaxy->projection.x = 0;
                galaxy->projection.y = ((camera_h / 2) - point) * scale - offset * (((camera_h / 2) - point) / (camera_h / 2));
            }
            else if (planet)
            {
                planet->projection.x = 0;
                planet->projection.y = ((camera_h / 2) - point) * scale - offset * (((camera_h / 2) - point) / (camera_h / 2));
            }
            else if (ship)
            {
                ship->projection->rect.x = 3 * PROJECTION_RADIUS;
                ship->projection->rect.y = ((camera_h / 2) - point) * scale - offset * (((camera_h / 2) - point) / (camera_h / 2)) + 3 * PROJECTION_RADIUS;
            }
        }
        // Top
        else
        {
            point = (camera_w / 2) - (camera_h / 2) * -delta_x / -delta_y;

            if (galaxy)
            {
                galaxy->projection.x = point * scale - offset * (point / (camera_w / 2));
                galaxy->projection.y = 0;
            }
            else if (planet)
            {
                planet->projection.x = point * scale - offset * (point / (camera_w / 2));
                planet->projection.y = 0;
            }
            else if (ship)
            {
                ship->projection->rect.x = point * scale - offset * (point / (camera_w / 2)) + 3 * PROJECTION_RADIUS;
                ship->projection->rect.y = 3 * PROJECTION_RADIUS;
            }
        }
    }
}

/*
 * Draw ship projection on axis.
 */
void project_ship(int state, InputState input_state, NavigationState nav_state, struct ship_t *ship, const struct camera_t *camera, long double scale)
{
    update_projection_coordinates(nav_state, ship, ENTITY_SHIP, camera, state, scale);

    // Mirror ship angle
    ship->projection->angle = ship->angle;

    // Draw ship projection
    SDL_RenderCopyEx(renderer, ship->projection->texture, &ship->projection->main_img_rect, &ship->projection->rect, ship->projection->angle, &ship->projection->rotation_pt, SDL_FLIP_NONE);

    // Draw projection thrust
    if (state == NAVIGATE && input_state.thrust)
        SDL_RenderCopyEx(renderer, ship->projection->texture, &ship->projection->thrust_img_rect, &ship->projection->rect, ship->projection->angle, &ship->projection->rotation_pt, SDL_FLIP_NONE);

    // Draw projection reverse
    if (state == NAVIGATE && input_state.reverse)
        SDL_RenderCopyEx(renderer, ship->projection->texture, &ship->projection->reverse_img_rect, &ship->projection->rect, ship->projection->angle, &ship->projection->rotation_pt, SDL_FLIP_NONE);
}

/*
 * Draw planet projection on axis.
 */
void project_planet(GameState game_state, NavigationState nav_state, struct planet_t *planet, const struct camera_t *camera)
{
    update_projection_coordinates(nav_state, planet, ENTITY_PLANET, camera, game_state.state, game_state.game_scale);

    planet->projection.w = 2 * PROJECTION_RADIUS;
    planet->projection.h = 2 * PROJECTION_RADIUS;

    double x = camera->x + (camera->w / 2) / game_state.game_scale;
    double y = camera->y + (camera->h / 2) / game_state.game_scale;
    double distance = sqrt(pow(fabs(x - planet->position.x), 2) + pow(fabs(y - planet->position.y), 2));
    int opacity = colors[COLOR_YELLOW_255].a;
    SDL_Color color;

    if (planet->level == LEVEL_STAR)
    {
        if (game_state.state == NAVIGATE)
            opacity = calculate_projection_opacity(distance, GALAXY_REGION_SIZE, GALAXY_SECTION_SIZE);
        else if (game_state.state == MAP)
            opacity = calculate_projection_opacity(distance, game_state.galaxy_region_size, GALAXY_SECTION_SIZE);

        color.r = colors[COLOR_YELLOW_255].r;
        color.g = colors[COLOR_YELLOW_255].g;
        color.b = colors[COLOR_YELLOW_255].b;
    }
    else
        color = planet->color;

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, opacity);
    SDL_RenderFillRect(renderer, &planet->projection);
}

/*
 * Draw galaxy projection on axis.
 */
void project_galaxy(int state, NavigationState nav_state, struct galaxy_t *galaxy, const struct camera_t *camera, long double scale)
{
    int scaling_factor = 1;

    if (state == NAVIGATE || state == MAP)
        scaling_factor = GALAXY_SCALE;

    update_projection_coordinates(nav_state, galaxy, ENTITY_GALAXY, camera, state, scale);

    galaxy->projection.w = 2 * PROJECTION_RADIUS;
    galaxy->projection.h = 2 * PROJECTION_RADIUS;
    double x = camera->x + (camera->w / 2) / scale;
    double y = camera->y + (camera->h / 2) / scale;
    double delta_x = fabs(x - galaxy->position.x * scaling_factor) / scaling_factor;
    double delta_y = fabs(y - galaxy->position.y * scaling_factor) / scaling_factor;

    double distance = sqrt(pow(delta_x, 2) + pow(delta_y, 2));
    int opacity = calculate_projection_opacity(distance, UNIVERSE_REGION_SIZE, UNIVERSE_SECTION_SIZE);

    SDL_SetRenderDrawColor(renderer, galaxy->color.r, galaxy->color.g, galaxy->color.b, opacity);
    SDL_RenderFillRect(renderer, &galaxy->projection);
}

/*
 * Calculate projection opacity according to object distance.
 * Opacity fades to 128 for <near_sections> and then fades linearly to 0.
 */
int calculate_projection_opacity(double distance, int region_size, int section_size)
{
    const int near_sections = 4;
    int a = (int)distance / section_size;
    int opacity;

    if (a <= 1)
        opacity = 255;
    else if (a > 1 && a <= near_sections)
    {
        // Linear fade from 255 to 100
        opacity = 100 + (255 - 100) * (near_sections - a) / (near_sections - 1);
    }
    else if (a > near_sections && a <= 10)
    {
        // Linear fade from 100 to 40
        opacity = 40 + (100 - 40) * (10 - a) / (10 - near_sections);
    }
    else if (a > 10 && a <= region_size)
    {
        // Linear fade from 40 to 0
        opacity = 0 + (40 - 0) * (region_size - a) / (region_size - 10);
    }
    else
        opacity = 0;

    return opacity;
}

/*
 * Find distance to nearest galaxy.
 */
double nearest_galaxy_center_distance(struct point_t position)
{
    // We use 6 * UNIVERSE_SECTION_SIZE as max, since a CLASS_6 galaxy needs 6 + 1 empty sections
    // We search inner circumferences of points first and work towards outward circumferences
    // If we find a galaxy, the function returns.

    // Keep track of checked points
    struct point_t checked_points[196];
    int num_checked_points = 0;

    // Use a local rng
    pcg32_random_t rng;

    for (int i = 1; i <= 6; i++)
    {
        for (double ix = position.x - i * UNIVERSE_SECTION_SIZE; ix <= position.x + i * UNIVERSE_SECTION_SIZE; ix += UNIVERSE_SECTION_SIZE)
        {
            for (double iy = position.y - i * UNIVERSE_SECTION_SIZE; iy <= position.y + i * UNIVERSE_SECTION_SIZE; iy += UNIVERSE_SECTION_SIZE)
            {
                if (ix == position.x && iy == position.y)
                    continue;

                struct point_t p = {ix, iy};

                if (point_in_array(p, checked_points, num_checked_points))
                    continue;

                checked_points[num_checked_points++] = p;

                // Create rng seed by combining x,y values
                uint64_t seed = pair_hash_order_sensitive_2(p);

                // Seed with a fixed constant
                pcg32_srandom_r(&rng, seed, 1);

                int has_galaxy = abs(pcg32_random_r(&rng)) % 1000 < UNIVERSE_DENSITY;

                if (has_galaxy)
                {
                    double distance = sqrt(pow(ix - position.x, 2) + pow(iy - position.y, 2));

                    return distance;
                }
            }
        }
    }

    return 7 * UNIVERSE_SECTION_SIZE;
}

/*
 * Find distance between two points.
 */
double find_distance(double x1, double y1, double x2, double y2)
{
    return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

/*
 * Find nearest galaxy to position, excluding or not <galaxy>.
 * The funtion finds the galaxy in the galaxies hash table
 * whose circumference is closest to the position.
 */
struct galaxy_t *find_nearest_galaxy(NavigationState nav_state, struct point_t position, int exclude)
{
    struct galaxy_t *closest = NULL;
    double closest_distance = INFINITY;
    int sections = 10;

    for (int i = 0; i < MAX_GALAXIES; i++)
    {
        struct galaxy_entry *entry = nav_state.galaxies[i];

        while (entry != NULL)
        {
            // Exlude current galaxy
            if (exclude)
            {
                if (entry->galaxy->position.x == nav_state.current_galaxy->position.x &&
                    entry->galaxy->position.y == nav_state.current_galaxy->position.y)
                {
                    entry = entry->next;
                    continue;
                }
            }

            double cx = entry->galaxy->position.x;
            double cy = entry->galaxy->position.y;
            double r = entry->galaxy->radius;
            double d = find_distance(position.x, position.y, cx, cy);

            if (d <= r * GALAXY_SCALE + sections * UNIVERSE_SECTION_SIZE)
            {
                double angle = atan2(position.y - cy, position.x - cx);
                double px = cx + r * cos(angle);
                double py = cy + r * sin(angle);
                double pd = find_distance(position.x, position.y, px, py);

                if (pd < closest_distance)
                {
                    closest = entry->galaxy;
                    closest_distance = pd;
                }
            }

            entry = entry->next;
        }
    }

    return closest;
}

/*
 * Find distance to nearest star.
 */
double nearest_star_distance(struct point_t position, struct galaxy_t *current_galaxy, uint64_t initseq, int galaxy_density)
{
    // We use 6 * GALAXY_SECTION_SIZE as max, since a CLASS_6 star needs 6 + 1 empty sections
    // We search inner circumferences of points first and work towards outward circumferences
    // If we find a star, the function returns.

    // Keep track of checked points
    struct point_t checked_points[196];
    int num_checked_points = 0;

    // Use a local rng
    pcg32_random_t rng;

    // Density scaling parameter
    double a = current_galaxy->radius * GALAXY_SCALE / 2.0f;

    for (int i = 1; i <= 6; i++)
    {
        for (double ix = position.x - i * GALAXY_SECTION_SIZE; ix <= position.x + i * GALAXY_SECTION_SIZE; ix += GALAXY_SECTION_SIZE)
        {
            for (double iy = position.y - i * GALAXY_SECTION_SIZE; iy <= position.y + i * GALAXY_SECTION_SIZE; iy += GALAXY_SECTION_SIZE)
            {
                if (ix == position.x && iy == position.y)
                    continue;

                struct point_t p = {ix, iy};

                if (point_in_array(p, checked_points, num_checked_points))
                    continue;

                checked_points[num_checked_points++] = p;

                // Create rng seed by combining x,y values
                uint64_t seed = pair_hash_order_sensitive(p);

                // Seed with a fixed constant
                pcg32_srandom_r(&rng, seed, initseq);

                // Calculate density based on distance from center
                double distance_from_center = sqrt((ix - position.x) * (ix - position.x) + (iy - position.y) * (iy - position.y));
                double density = (galaxy_density / pow((distance_from_center / a + 1), 6));

                int has_star = abs(pcg32_random_r(&rng)) % 1000 < density;

                if (has_star)
                {
                    double distance = sqrt(pow(ix - position.x, 2) + pow(iy - position.y, 2));

                    return distance;
                }
            }
        }
    }

    return 7 * GALAXY_SECTION_SIZE;
}

/*
 * Find galaxy class.
 * <distance> is number of empty sections.
 */
int get_galaxy_class(float distance)
{
    if (distance < 3 * UNIVERSE_SECTION_SIZE)
        return GALAXY_CLASS_1;
    else if (distance < 4 * UNIVERSE_SECTION_SIZE)
        return GALAXY_CLASS_2;
    else if (distance < 5 * UNIVERSE_SECTION_SIZE)
        return GALAXY_CLASS_3;
    else if (distance < 6 * UNIVERSE_SECTION_SIZE)
        return GALAXY_CLASS_4;
    else if (distance < 7 * UNIVERSE_SECTION_SIZE)
        return GALAXY_CLASS_5;
    else if (distance >= 7 * UNIVERSE_SECTION_SIZE)
        return GALAXY_CLASS_6;
    else
        return GALAXY_CLASS_1;
}

/*
 * Find star class.
 * <distance> is number of empty sections.
 */
int get_star_class(float distance)
{
    if (distance < 2 * GALAXY_SECTION_SIZE)
        return STAR_CLASS_1;
    else if (distance < 3 * GALAXY_SECTION_SIZE)
        return STAR_CLASS_2;
    else if (distance < 4 * GALAXY_SECTION_SIZE)
        return STAR_CLASS_3;
    else if (distance < 5 * GALAXY_SECTION_SIZE)
        return STAR_CLASS_4;
    else if (distance < 6 * GALAXY_SECTION_SIZE)
        return STAR_CLASS_5;
    else if (distance >= 6 * GALAXY_SECTION_SIZE)
        return STAR_CLASS_6;
    else
        return STAR_CLASS_1;
}

/*
 * Find planet class.
 * <width> is orbit width.
 */
int get_planet_class(float width)
{
    if (width < GALAXY_SECTION_SIZE / 20) // < 500
        return PLANET_CLASS_1;
    else if (width < GALAXY_SECTION_SIZE / 10) // < 1000
        return PLANET_CLASS_2;
    else if (width < GALAXY_SECTION_SIZE / 6.67) // < 1500
        return PLANET_CLASS_3;
    else if (width < GALAXY_SECTION_SIZE / 5) // < 2000
        return PLANET_CLASS_4;
    else if (width < 6 * GALAXY_SECTION_SIZE / 4) // < 2500
        return PLANET_CLASS_5;
    else if (width >= 6 * GALAXY_SECTION_SIZE) // >= 2500
        return PLANET_CLASS_6;
    else
        return PLANET_CLASS_1;
}

/*
 * Compares two points.
 */
bool point_eq(struct point_t a, struct point_t b)
{
    return a.x == b.x && a.y == b.y;
}

/*
 * Checks whether a point exists in an array.
 */
bool point_in_array(struct point_t p, struct point_t arr[], int len)
{
    for (int i = 0; i < len; ++i)
    {
        if (point_eq(p, arr[i]))
            return true;
    }

    return false;
}

/*
 * Zoom in/out a star system (recursive).
 */
void zoom_star(struct planet_t *planet, long double scale)
{
    planet->rect.x = (planet->position.x - planet->radius) * scale;
    planet->rect.y = (planet->position.y - planet->radius) * scale;
    planet->rect.w = 2 * planet->radius * scale;
    planet->rect.h = 2 * planet->radius * scale;

    // Zoom children
    if (planet->level <= LEVEL_PLANET && planet->planets != NULL && planet->planets[0] != NULL)
    {
        int max_planets = (planet->level == LEVEL_STAR) ? MAX_PLANETS : MAX_MOONS;

        for (int i = 0; i < max_planets && planet->planets[i] != NULL; i++)
        {
            zoom_star(planet->planets[i], scale);
        }
    }
}

/*
 * Check whether relative point is in camera.
 * x, y are relative to camera->x, camera->y.
 */
int in_camera_relative(const struct camera_t *camera, int x, int y)
{
    return x >= 0 && x < camera->w && y >= 0 && y < camera->h;
}

/*
 * Check whether object with center (x,y) and radius r is in camera.
 * x, y are absolute galaxy scale coordinates. Radius is galaxy scale.
 */
int in_camera(const struct camera_t *camera, double x, double y, float radius, long double scale)
{
    return x + radius >= camera->x && x - radius - camera->x < camera->w / scale &&
           y + radius >= camera->y && y - radius - camera->y < camera->h / scale;
}

/*
 * Draw screen frame.
 */
void draw_screen_frame(struct camera_t *camera)
{
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
}

/**
 * This function checks if the line segment specified by (x1,y1) and (x2,y2) intersects with the viewport defined by `camera`.
 *
 * @param camera Pointer to a camera structure that defines the viewport
 * @param x1 x-coordinate of the first end point of the line segment
 * @param y1 y-coordinate of the first end point of the line segment
 * @param x2 x-coordinate of the second end point of the line segment
 * @param y2 y-coordinate of the second end point of the line segment
 *
 * @return: Returns true if the line segment intersects with the viewport, and false otherwise
 */
bool line_intersects_viewport(const struct camera_t *camera, double x1, double y1, double x2, double y2)
{
    double left = 0;
    double right = camera->w;
    double top = 0;
    double bottom = camera->h;

    // Check if both endpoints of the line are inside the viewport.
    if (in_camera_relative(camera, x1, y1) || in_camera_relative(camera, x2, y2))
        return true;

    // Check if the line intersects the left edge of the viewport.
    if (x1 < left && x2 >= left)
    {
        double y = y1 + (y2 - y1) * (left - x1) / (x2 - x1);
        if (y >= top && y <= bottom)
        {
            return true;
        }
    }

    // Check if the line intersects the right edge of the viewport.
    if (x1 > right && x2 <= right)
    {
        double y = y1 + (y2 - y1) * (right - x1) / (x2 - x1);
        if (y >= top && y <= bottom)
        {
            return true;
        }
    }

    // Check if the line intersects the top edge of the viewport.
    if (y1 < top && y2 >= top)
    {
        double x = x1 + (x2 - x1) * (top - y1) / (y2 - y1);
        if (x >= left && x <= right)
        {
            return true;
        }
    }

    // Check if the line intersects the bottom edge of the viewport.
    if (y1 > bottom && y2 <= bottom)
    {
        double x = x1 + (x2 - x1) * (bottom - y1) / (y2 - y1);
        if (x >= left && x <= right)
        {
            return true;
        }
    }

    return false;
}

void draw_section_lines(struct camera_t *camera, int section_size, SDL_Color color, long double scale)
{
    double bx = find_nearest_section_axis(camera->x, section_size);
    double by = find_nearest_section_axis(camera->y, section_size);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    for (int ix = bx; ix <= bx + camera->w / scale; ix = ix + section_size)
    {
        SDL_RenderDrawLine(renderer, (ix - camera->x) * scale, 0, (ix - camera->x) * scale, camera->h);
    }

    for (int iy = by; iy <= by + camera->h / scale; iy = iy + section_size)
    {
        SDL_RenderDrawLine(renderer, 0, (iy - camera->y) * scale, camera->w, (iy - camera->y) * scale);
    }
}

void create_menu_galaxy_cloud(struct galaxy_t *galaxy, struct gstar_t menustars[])
{
    float radius = galaxy->radius;
    double full_size_radius = radius * GALAXY_SCALE;
    full_size_radius -= fmod(full_size_radius, GALAXY_SECTION_SIZE); // zero out any digits below 10,000
    double full_size_diameter = full_size_radius * 2;

    // Check whether MAX_GSTARS_ROW * GALAXY_SECTION_SIZE fit in full_size_diameter
    // If they don't fit, we must group sections together
    int grouped_sections = 1;
    double total_sections = full_size_diameter / (grouped_sections * GALAXY_SECTION_SIZE);

    // Allow <array_factor> times more than array size as galaxy_density is low
    // Increase array_factor to show more stars
    int array_factor = 12;

    while (total_sections > MAX_GSTARS_ROW * array_factor)
    {
        grouped_sections++;
        total_sections = full_size_diameter / (grouped_sections * GALAXY_SECTION_SIZE);
    }

    int section_size = grouped_sections * GALAXY_SECTION_SIZE;
    double ix, iy;
    int i = 0;

    // Use a local rng
    pcg32_random_t rng;

    // Set galaxy hash as initseq
    uint64_t initseq = pair_hash_order_sensitive_2(galaxy->position);

    // Density scaling parameter
    double a = galaxy->radius * GALAXY_SCALE / 2.0f;

    for (ix = -full_size_radius; ix <= full_size_radius; ix += section_size)
    {
        for (iy = -full_size_radius; iy <= full_size_radius; iy += section_size)
        {
            // Calculate the distance from the center of the galaxy
            double distance_from_center = sqrt(ix * ix + iy * iy);

            // Check that point is within galaxy radius
            if (distance_from_center > full_size_radius)
                continue;

            // Create rng seed by combining x,y values
            struct point_t position = {.x = ix, .y = iy};
            uint64_t seed = pair_hash_order_sensitive(position);

            // Seed with a fixed constant
            pcg32_srandom_r(&rng, seed, initseq);

            // Calculate density based on distance from center
            double density = (MENU_GALAXY_CLOUD_DENSITY / pow((distance_from_center / a + 1), 6));

            int has_star = abs(pcg32_random_r(&rng)) % 1000 < density;

            if (has_star)
            {
                struct gstar_t star;
                star.position.x = ix;
                star.position.y = iy;

                double distance = nearest_star_distance(position, galaxy, initseq, MENU_GALAXY_CLOUD_DENSITY);
                int class = get_star_class(distance);
                float class_opacity_max = class * (255 / 6) + 20; // There are only a few Class 6 galaxies, increase max value by 20
                class_opacity_max = class_opacity_max > 255 ? 255 : class_opacity_max;
                float class_opacity_min = class_opacity_max - (255 / 6);
                int opacity = (abs(pcg32_random_r(&rng)) % (int)class_opacity_max + (int)class_opacity_min);
                star.opacity = opacity * (1 - pow(distance_from_center / (galaxy->radius * GALAXY_SCALE), 3));
                star.opacity = star.opacity < 0 ? 0 : star.opacity;

                star.final_star = 1;
                menustars[i++] = star;
            }
        }
    }
}

void draw_menu_galaxy_cloud(const struct camera_t *camera, struct gstar_t menustars[])
{
    int i = 0;

    while (i < MAX_GSTARS && menustars[i].final_star == 1)
    {
        int x, y;

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, menustars[i].opacity);

        x = camera->w - camera->w / 4 + (menustars[i].position.x / GALAXY_SCALE) * 0.15;
        y = camera->h / 3 + (menustars[i].position.y / GALAXY_SCALE) * 0.15;

        SDL_RenderDrawPoint(renderer, x, y);

        i++;
    }
}

void create_galaxy_cloud(struct galaxy_t *galaxy, unsigned short high_definition)
{
    float radius = galaxy->radius;
    double full_size_radius = radius * GALAXY_SCALE;
    full_size_radius -= fmod(full_size_radius, GALAXY_SECTION_SIZE); // zero out any digits below 10,000
    double full_size_diameter = full_size_radius * 2;

    // Check whether MAX_GSTARS_ROW * GALAXY_SECTION_SIZE fit in full_size_diameter
    // If they don't fit, we must group sections together
    int grouped_sections = 1;
    int total_sections = full_size_diameter / (grouped_sections * GALAXY_SECTION_SIZE);

    // Allow <array_factor> times more than array size as galaxy_density is low
    // Increase array_factor to show more stars
    int array_factor = 12;

    while (total_sections > MAX_GSTARS_ROW * array_factor)
    {
        grouped_sections++;
        total_sections = full_size_diameter / (grouped_sections * GALAXY_SECTION_SIZE);
    }

    // Double the grouped_sections for low def
    if (!high_definition)
        grouped_sections *= 2;

    int section_size = grouped_sections * GALAXY_SECTION_SIZE;
    double ix, iy;
    int i = 0;

    // Use a local rng
    pcg32_random_t rng;

    // Set galaxy hash as initseq
    uint64_t initseq = pair_hash_order_sensitive_2(galaxy->position);

    // Density scaling parameter
    double a = galaxy->radius * GALAXY_SCALE / 2.0f;

    for (ix = -full_size_radius; ix <= full_size_radius; ix += section_size)
    {
        for (iy = -full_size_radius; iy <= full_size_radius; iy += section_size)
        {
            // Calculate the distance from the center of the galaxy
            double distance_from_center = sqrt(ix * ix + iy * iy);

            // Check that point is within galaxy radius
            if (distance_from_center > full_size_radius)
                continue;

            // Create rng seed by combining x,y values
            struct point_t position = {.x = ix, .y = iy};
            uint64_t seed = pair_hash_order_sensitive(position);

            // Seed with a fixed constant
            pcg32_srandom_r(&rng, seed, initseq);

            // Calculate density based on distance from center
            double density = (GALAXY_CLOUD_DENSITY / pow((distance_from_center / a + 1), 6));

            int has_star = abs(pcg32_random_r(&rng)) % 1000 < density;

            if (has_star)
            {
                struct gstar_t star;
                star.position.x = ix;
                star.position.y = iy;

                double distance = nearest_star_distance(position, galaxy, initseq, GALAXY_CLOUD_DENSITY);
                int class = get_star_class(distance);
                float class_opacity_max = class * (255 / 6);
                class_opacity_max = class_opacity_max > 255 ? 255 : class_opacity_max;
                float class_opacity_min = class_opacity_max - (255 / 6);
                int opacity = (abs(pcg32_random_r(&rng)) % (int)class_opacity_max + (int)class_opacity_min);
                star.opacity = opacity;
                star.opacity = star.opacity < 0 ? 0 : star.opacity;

                star.final_star = 1;

                if (high_definition)
                    galaxy->gstars_hd[i++] = star;
                else
                    galaxy->gstars[i++] = star;
            }
        }
    }

    // Set galaxy as initialized
    if (high_definition)
        galaxy->initialized_hd = i;
    else
        galaxy->initialized = i;
}

void draw_galaxy_cloud(struct galaxy_t *galaxy, const struct camera_t *camera, int gstars_count, unsigned short high_definition, long double scale)
{
    const double epsilon = ZOOM_EPSILON / GALAXY_SCALE;

    for (int i = 0; i < gstars_count; i++)
    {
        int x, y;
        float opacity, star_opacity;

        if (high_definition)
            star_opacity = galaxy->gstars_hd[i].opacity;
        else
            star_opacity = galaxy->gstars[i].opacity;

        switch (galaxy->class)
        {
        case 1:
            if (scale <= 0.000001 + epsilon)
                opacity = 0.35 * star_opacity;
            else if (scale <= 0.000002 + epsilon)
                opacity = 0.5 * star_opacity;
            else
                opacity = star_opacity;
            break;
        case 2:
            if (scale <= 0.000001 + epsilon)
                opacity = 0.5 * star_opacity;
            else
                opacity = star_opacity;
            break;
        case 3:
            if (scale <= 0.000001 + epsilon)
                opacity = 0.8 * star_opacity;
            else
                opacity = star_opacity;
            break;
        default:
            opacity = star_opacity;
        }

        if (high_definition)
        {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, (int)opacity);

            x = (galaxy->position.x - camera->x + galaxy->gstars_hd[i].position.x / GALAXY_SCALE) * scale * GALAXY_SCALE;
            y = (galaxy->position.y - camera->y + galaxy->gstars_hd[i].position.y / GALAXY_SCALE) * scale * GALAXY_SCALE;
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, (int)opacity);

            x = (galaxy->position.x - camera->x + galaxy->gstars[i].position.x / GALAXY_SCALE) * scale * GALAXY_SCALE;
            y = (galaxy->position.y - camera->y + galaxy->gstars[i].position.y / GALAXY_SCALE) * scale * GALAXY_SCALE;
        }

        SDL_RenderDrawPoint(renderer, x, y);
    }
}

/*
 * Draw a speed arc for the ship.
 */
void draw_speed_arc(struct ship_t *ship, const struct camera_t *camera, long double scale)
{
    SDL_Color color = colors[COLOR_ORANGE_32];
    int radius_1 = 50;
    int radius_2 = radius_1 + 1;
    int radius_3 = radius_1 + 2;
    int vertical_offset = -20;

    // Calculate the velocity vector
    float velocity_x = ship->vx;
    float velocity_y = ship->vy;

    // Normalize the velocity vector
    float velocity_length = sqrt(velocity_x * velocity_x + velocity_y * velocity_y);
    velocity_x /= velocity_length;
    velocity_y /= velocity_length;

    // Calculate the perpendicular vector to the velocity vector
    float perpendicular_x = -velocity_y;
    float perpendicular_y = velocity_x;

    // Normalize the perpendicular vector
    float perpendicular_length = sqrt(perpendicular_x * perpendicular_x + perpendicular_y * perpendicular_y);
    perpendicular_x /= perpendicular_length;
    perpendicular_y /= perpendicular_length;

    // Calculate the center point of the circle
    float center_x = (ship->position.x - camera->x - vertical_offset * velocity_x) * scale;
    float center_y = (ship->position.y - camera->y - vertical_offset * velocity_y) * scale;

    // Calculate opacity
    float opacity = 5 + (velocity_length - GALAXY_SPEED_LIMIT) / 80;
    opacity = opacity > 255 ? 255 : opacity;

    // Set the renderer draw color to the circle color
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, (int)opacity);

    // Draw the circle
    for (float angle = 0; angle < 2 * M_PI; angle += 0.01)
    {
        // Outer circle
        float start_x_3 = center_x + radius_3 * cos(angle);
        float start_y_3 = center_y + radius_3 * sin(angle);
        float end_x_3 = center_x + radius_3 * cos(angle + 0.01);
        float end_y_3 = center_y + radius_3 * sin(angle + 0.01);

        // Calculate the dot product between the velocity vector and the vector from the center of the circle to the current point
        float dot_product_3 = (start_x_3 - center_x) * velocity_x + (start_y_3 - center_y) * velocity_y;

        // Middle circle
        float start_x_2 = center_x + radius_2 * cos(angle);
        float start_y_2 = center_y + radius_2 * sin(angle);
        float end_x_2 = center_x + radius_2 * cos(angle + 0.01);
        float end_y_2 = center_y + radius_2 * sin(angle + 0.01);

        // Calculate the dot product between the velocity vector and the vector from the center of the circle to the current point
        float dot_product_2 = (start_x_2 - center_x) * velocity_x + (start_y_2 - center_y) * velocity_y;

        // Inner circle
        float start_x_1 = center_x + radius_1 * cos(angle);
        float start_y_1 = center_y + radius_1 * sin(angle);
        float end_x_1 = center_x + radius_1 * cos(angle + 0.01);
        float end_y_1 = center_y + radius_1 * sin(angle + 0.01);

        // Calculate the dot product between the velocity vector and the vector from the center of the circle to the current point
        float dot_product_1 = (start_x_1 - center_x) * velocity_x + (start_y_1 - center_y) * velocity_y;

        // Check if the point is above or below the diameter in the opposite direction to the velocity direction
        // Range: -50 < x < 50 (0 is half circle, positive is above diameter)
        // For v = 1800, velocity_factor = 50
        // For v = UNIVERSE_SPEED_LIMIT, velocity_factor = 10 (increase above 0 to make arc smaller)
        float velocity_factor = 20 + (UNIVERSE_SPEED_LIMIT - velocity_length) / 40;

        if (dot_product_3 >= velocity_factor)
        {
            SDL_RenderDrawLine(renderer, (int)start_x_3, (int)start_y_3, (int)end_x_3, (int)end_y_3);
        }

        if (dot_product_2 >= velocity_factor)
        {
            SDL_RenderDrawLine(renderer, (int)start_x_2, (int)start_y_2, (int)end_x_2, (int)end_y_2);
        }

        if (dot_product_1 >= velocity_factor)
        {
            SDL_RenderDrawLine(renderer, (int)start_x_1, (int)start_y_1, (int)end_x_1, (int)end_y_1);
        }
    }
}

/*
 * Delete stars outside region.
 */
void delete_stars_outside_region(struct star_entry *stars[], double bx, double by, int region_size)
{
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
            double region_radius = sqrt((double)2 * ((region_size + 1) / 2) * GALAXY_SECTION_SIZE * ((region_size + 1) / 2) * GALAXY_SECTION_SIZE);

            // If star outside region, delete it
            if (distance >= region_radius)
                delete_star(stars, position);

            entry = entry->next;
        }
    }
}

/*
 * Move and draw speed lines.
 */
void draw_speed_lines(float velocity, const struct camera_t *camera, struct speed_t speed)
{
    // Check if velocity magnitude is zero
    if (velocity == 0)
        return;

    // Define constants
    SDL_Color color = colors[COLOR_WHITE_255];
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

    if (velocity < 2.0 * base_speed)
    {
        opacity = base_opacity * (1.0 - exp(-3.0 * (velocity / base_speed)));
    }
    else if (velocity >= 2.0 * base_speed && velocity < 3.0 * base_speed)
    {
        float opacity_ratio = (velocity - 2.0 * base_speed) / (base_speed);
        opacity = 60.0 * exp(-1.0 * opacity_ratio);
        opacity = fmax(opacity, 30.0);
    }
    else
    {
        opacity = 30.0;
    }

    int final_opacity = (int)round(opacity); // Round the opacity to the nearest integer

    // Calculate the normalized velocity vector
    float velocity_x = speed.vx / velocity;
    float velocity_y = speed.vy / velocity;

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
                    line_start_x[row][col] = camera->w / 2 + line_distance / 2 + start_x + col * line_distance;
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

            if (velocity >= speed_limit)
                speed_ray_length = (float)4 * max_length / 6;
            else
            {
                if (velocity < 2 * base_speed)
                    speed_ray_length = 1;
                else
                    speed_ray_length = max_length * (velocity - 2 * base_speed) / speed_limit;
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

            if (velocity > speed_limit)
            {
                delta_x = max_speed * velocity_x / FPS;
                delta_y = max_speed * velocity_y / FPS;
            }
            else
            {
                delta_x = max_speed * velocity_x * (velocity / speed_limit) / FPS;
                delta_y = max_speed * velocity_y * (velocity / speed_limit) / FPS;
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
 * Creates colors for SDL.
 */
void create_colors(void)
{
    colors[COLOR_WHITE_255].r = 255;
    colors[COLOR_WHITE_255].g = 255;
    colors[COLOR_WHITE_255].b = 255;
    colors[COLOR_WHITE_255].a = 255;

    colors[COLOR_WHITE_100].r = 255;
    colors[COLOR_WHITE_100].g = 255;
    colors[COLOR_WHITE_100].b = 255;
    colors[COLOR_WHITE_100].a = 100;

    colors[COLOR_ORANGE_32].r = 255;
    colors[COLOR_ORANGE_32].g = 165;
    colors[COLOR_ORANGE_32].b = 0;
    colors[COLOR_ORANGE_32].a = 32;

    colors[COLOR_CYAN_70].r = 0;
    colors[COLOR_CYAN_70].g = 255;
    colors[COLOR_CYAN_70].b = 255;
    colors[COLOR_CYAN_70].a = 70;

    colors[COLOR_MAGENTA_40].r = 255;
    colors[COLOR_MAGENTA_40].g = 0;
    colors[COLOR_MAGENTA_40].b = 255;
    colors[COLOR_MAGENTA_40].a = 40;

    colors[COLOR_MAGENTA_70].r = 255;
    colors[COLOR_MAGENTA_70].g = 0;
    colors[COLOR_MAGENTA_70].b = 255;
    colors[COLOR_MAGENTA_70].a = 70;

    colors[COLOR_YELLOW_255].r = 255;
    colors[COLOR_YELLOW_255].g = 255;
    colors[COLOR_YELLOW_255].b = 0;
    colors[COLOR_YELLOW_255].a = 255;

    colors[COLOR_SKY_BLUE_255].r = 135;
    colors[COLOR_SKY_BLUE_255].g = 206;
    colors[COLOR_SKY_BLUE_255].b = 235;
    colors[COLOR_SKY_BLUE_255].a = 255;

    colors[COLOR_GAINSBORO_255].r = 220;
    colors[COLOR_GAINSBORO_255].g = 220;
    colors[COLOR_GAINSBORO_255].b = 220;
    colors[COLOR_GAINSBORO_255].a = 255;
}
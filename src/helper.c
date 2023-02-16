/*
 * helper.c - Definitions for helper functions.
 */

#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

#include "../include/common.h"
#include "../include/structs.h"
#include "../lib/pcg-c-basic-0.9/pcg_basic.h"

extern SDL_Renderer *renderer;
extern struct star_entry *stars[];
extern struct galaxy_entry *galaxies[];
extern int thrust;
extern int reverse;
extern float game_scale;

static void cleanup_planets(struct planet_t *);
void cleanup_stars(void);
static void cleanup_galaxies(void);
uint64_t pair_hash_order_sensitive(struct position_t);
uint64_t pair_hash_order_sensitive_2(struct position_t);
uint64_t double_hash(double x);
bool point_eq(struct position_t, struct position_t);
bool point_in_array(struct position_t, struct position_t arr[], int len);
int calculate_projection_opacity(float distance, int region_size, int section_size);
double find_nearest_section_axis(double n, int size);
uint64_t unique_index(struct position_t, int modulo, int entity_type);
void delete_star(struct position_t position);
void delete_galaxy(struct position_t position);
void update_projection_coordinates(void *, int entity_type, const struct camera_t *, int state);

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
static void cleanup_galaxies(void)
{
    // Loop through hash table
    for (int s = 0; s < MAX_GALAXIES; s++)
    {
        struct galaxy_entry *entry = galaxies[s];

        while (entry != NULL)
        {
            struct position_t position = {.x = entry->x, .y = entry->y};
            delete_galaxy(position);
            entry = entry->next;
        }
    }
}

/*
 * Delete all stars from hash table.
 */
void cleanup_stars(void)
{
    // Loop through hash table
    for (int s = 0; s < MAX_STARS; s++)
    {
        struct star_entry *entry = stars[s];

        while (entry != NULL)
        {
            struct position_t position = {.x = entry->x, .y = entry->y};
            delete_star(position);
            entry = entry->next;
        }
    }
}

/*
 * Clean up resources.
 */
void cleanup_resources(struct ship_t *ship, struct galaxy_t *current_galaxy, struct galaxy_t *previous_galaxy)
{
    // Clean up galaxies
    cleanup_galaxies();

    // Clean up stars
    cleanup_stars();

    // Clean up ship
    SDL_DestroyTexture(ship->projection->texture);
    ship->projection->texture = NULL;
    SDL_DestroyTexture(ship->texture);
    ship->texture = NULL;

    // Clean up current galaxy
    free(current_galaxy);

    // Clean up previous galaxy
    free(previous_galaxy);
}

/*
 * Insert a new galaxy entry in galaxies hash table.
 */
void put_galaxy(struct position_t position, struct galaxy_t *galaxy)
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
int galaxy_exists(struct position_t position)
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
struct galaxy_t *get_galaxy(struct position_t position)
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
void delete_galaxy(struct position_t position)
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
void put_star(struct position_t position, struct planet_t *star)
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
int star_exists(struct position_t position)
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
 * Get a star entry from the stars hash table.
 */
struct planet_t *get_star(struct position_t position)
{
    // Generate unique index for hash table
    uint64_t index = unique_index(position, MAX_STARS, ENTITY_STAR);

    struct star_entry *entry = stars[index];

    while (entry != NULL)
    {
        if (entry->x == position.x && entry->y == position.y)
            return entry->star;

        entry = entry->next;
    }

    return NULL;
}

/*
 * Delete a star entry from the stars hash table.
 */
void delete_star(struct position_t position)
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
            if (entry->star != NULL)
                cleanup_planets(entry->star);

            // Clean up star
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
void update_projection_coordinates(void *ptr, int entity_type, const struct camera_t *camera, int state)
{
    struct galaxy_t *galaxy = NULL;
    struct planet_t *planet = NULL;
    struct ship_t *ship = NULL;
    float delta_x, delta_y, point;
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

    int camera_w = camera->w / game_scale;
    int camera_h = camera->h / game_scale;

    // Find screen quadrant for object exit from screen; 0, 0 is screen center, negative y is up
    if (galaxy)
    {
        delta_x = galaxy->position.x - camera->x - (camera_w / 2);
        delta_y = galaxy->position.y - camera->y - (camera_h / 2);
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
            delta_x = ship->position.x - camera->x - (camera_w / 2);
            delta_y = ship->position.y - camera->y - (camera_h / 2);
        }
        else if (state == UNIVERSE)
        {
            delta_x = (UNIVERSE_START_X + ship->position.x / GALAXY_SCALE - camera->x) - (camera_w / 2);
            delta_y = (UNIVERSE_START_Y + ship->position.y / GALAXY_SCALE - camera->y) - (camera_h / 2);
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
                galaxy->projection.x = ((camera_w / 2) + point) * game_scale - offset * (point / (camera_w / 2) + 1);
                galaxy->projection.y = 0;
            }
            else if (planet)
            {
                planet->projection.x = ((camera_w / 2) + point) * game_scale - offset * (point / (camera_w / 2) + 1);
                planet->projection.y = 0;
            }
            else if (ship)
            {
                ship->projection->rect.x = ((camera_w / 2) + point) * game_scale - offset * (point / (camera_w / 2) + 1) + 3 * PROJECTION_RADIUS;
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
                galaxy->projection.x = camera_w * game_scale - (2 * offset);
                galaxy->projection.y = point * game_scale - offset * (point / (camera_h / 2));
            }
            else if (planet)
            {
                planet->projection.x = camera_w * game_scale - (2 * offset);
                planet->projection.y = point * game_scale - offset * (point / (camera_h / 2));
            }
            else if (ship)
            {
                ship->projection->rect.x = camera_w * game_scale - (2 * offset) + 3 * PROJECTION_RADIUS;
                ship->projection->rect.y = point * game_scale - offset * (point / (camera_h / 2)) + 3 * PROJECTION_RADIUS;
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
                galaxy->projection.x = camera_w * game_scale - (2 * offset);
                galaxy->projection.y = ((camera_h / 2) + point) * game_scale - offset * (point / (camera_h / 2) + 1);
            }
            else if (planet)
            {
                planet->projection.x = camera_w * game_scale - (2 * offset);
                planet->projection.y = ((camera_h / 2) + point) * game_scale - offset * (point / (camera_h / 2) + 1);
            }
            else if (ship)
            {
                ship->projection->rect.x = camera_w * game_scale - (2 * offset) + 3 * PROJECTION_RADIUS;
                ship->projection->rect.y = ((camera_h / 2) + point) * game_scale - offset * (point / (camera_h / 2) + 1) + 3 * PROJECTION_RADIUS;
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
                galaxy->projection.x = ((camera_w / 2) + point) * game_scale - offset * (point / (camera_w / 2) + 1);
                galaxy->projection.y = camera_h * game_scale - (2 * offset);
            }
            else if (planet)
            {
                planet->projection.x = ((camera_w / 2) + point) * game_scale - offset * (point / (camera_w / 2) + 1);
                planet->projection.y = camera_h * game_scale - (2 * offset);
            }
            else if (ship)
            {
                ship->projection->rect.x = ((camera_w / 2) + point) * game_scale - offset * (point / (camera_w / 2) + 1) + 3 * PROJECTION_RADIUS;
                ship->projection->rect.y = camera_h * game_scale - (2 * offset) + 3 * PROJECTION_RADIUS;
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
                galaxy->projection.x = ((camera_w / 2) - point) * game_scale - offset * (((camera_w / 2) - point) / (camera_w / 2));
                galaxy->projection.y = camera_h * game_scale - (2 * offset);
            }
            else if (planet)
            {
                planet->projection.x = ((camera_w / 2) - point) * game_scale - offset * (((camera_w / 2) - point) / (camera_w / 2));
                planet->projection.y = camera_h * game_scale - (2 * offset);
            }
            else if (ship)
            {
                ship->projection->rect.x = ((camera_w / 2) - point) * game_scale - offset * (((camera_w / 2) - point) / (camera_w / 2)) + 3 * PROJECTION_RADIUS;
                ship->projection->rect.y = camera_h * game_scale - (2 * offset) + 3 * PROJECTION_RADIUS;
            }
        }
        // Left
        else
        {
            point = (camera_h / 2) - (camera_w / 2) * delta_y / -delta_x;

            if (galaxy)
            {
                galaxy->projection.x = 0;
                galaxy->projection.y = (camera_h - point) * game_scale - offset * (((camera_h / 2) - point) / (camera_h / 2) + 1);
            }
            else if (planet)
            {
                planet->projection.x = 0;
                planet->projection.y = (camera_h - point) * game_scale - offset * (((camera_h / 2) - point) / (camera_h / 2) + 1);
            }
            else if (ship)
            {
                ship->projection->rect.x = 3 * PROJECTION_RADIUS;
                ship->projection->rect.y = (camera_h - point) * game_scale - offset * (((camera_h / 2) - point) / (camera_h / 2) + 1) + 3 * PROJECTION_RADIUS;
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
                galaxy->projection.y = ((camera_h / 2) - point) * game_scale - offset * (((camera_h / 2) - point) / (camera_h / 2));
            }
            else if (planet)
            {
                planet->projection.x = 0;
                planet->projection.y = ((camera_h / 2) - point) * game_scale - offset * (((camera_h / 2) - point) / (camera_h / 2));
            }
            else if (ship)
            {
                ship->projection->rect.x = 3 * PROJECTION_RADIUS;
                ship->projection->rect.y = ((camera_h / 2) - point) * game_scale - offset * (((camera_h / 2) - point) / (camera_h / 2)) + 3 * PROJECTION_RADIUS;
            }
        }
        // Top
        else
        {
            point = (camera_w / 2) - (camera_h / 2) * -delta_x / -delta_y;

            if (galaxy)
            {
                galaxy->projection.x = point * game_scale - offset * (point / (camera_w / 2));
                galaxy->projection.y = 0;
            }
            else if (planet)
            {
                planet->projection.x = point * game_scale - offset * (point / (camera_w / 2));
                planet->projection.y = 0;
            }
            else if (ship)
            {
                ship->projection->rect.x = point * game_scale - offset * (point / (camera_w / 2)) + 3 * PROJECTION_RADIUS;
                ship->projection->rect.y = 3 * PROJECTION_RADIUS;
            }
        }
    }
}

/*
 * Draw ship projection on axis.
 */
void project_ship(struct ship_t *ship, const struct camera_t *camera, int state)
{
    update_projection_coordinates(ship, ENTITY_SHIP, camera, state);

    // Mirror ship angle
    ship->projection->angle = ship->angle;

    // Draw ship projection
    SDL_RenderCopyEx(renderer, ship->projection->texture, &ship->projection->main_img_rect, &ship->projection->rect, ship->projection->angle, &ship->projection->rotation_pt, SDL_FLIP_NONE);

    // Draw projection thrust
    if (state == NAVIGATE && thrust)
        SDL_RenderCopyEx(renderer, ship->projection->texture, &ship->projection->thrust_img_rect, &ship->projection->rect, ship->projection->angle, &ship->projection->rotation_pt, SDL_FLIP_NONE);

    // Draw projection reverse
    if (state == NAVIGATE && reverse)
        SDL_RenderCopyEx(renderer, ship->projection->texture, &ship->projection->reverse_img_rect, &ship->projection->rect, ship->projection->angle, &ship->projection->rotation_pt, SDL_FLIP_NONE);
}

/*
 * Draw planet projection on axis.
 */
void project_planet(struct planet_t *planet, const struct camera_t *camera, int state)
{
    update_projection_coordinates(planet, ENTITY_PLANET, camera, state);

    planet->projection.w = 2 * PROJECTION_RADIUS;
    planet->projection.h = 2 * PROJECTION_RADIUS;

    double x = camera->x + (camera->w / 2) / game_scale;
    double y = camera->y + (camera->h / 2) / game_scale;
    float distance = sqrt(pow(fabs(x - planet->position.x), 2) + pow(fabs(y - planet->position.y), 2));
    int opacity = 255;

    if (planet->level == LEVEL_STAR)
        opacity = calculate_projection_opacity(distance, GALAXY_REGION_SIZE, GALAXY_SECTION_SIZE);

    SDL_SetRenderDrawColor(renderer, planet->color.r, planet->color.g, planet->color.b, opacity);
    SDL_RenderFillRect(renderer, &planet->projection);
}

/*
 * Draw galaxy projection on axis.
 */
void project_galaxy(struct galaxy_t *galaxy, const struct camera_t *camera, int state)
{
    update_projection_coordinates(galaxy, ENTITY_GALAXY, camera, state);

    galaxy->projection.w = 2 * PROJECTION_RADIUS;
    galaxy->projection.h = 2 * PROJECTION_RADIUS;

    double x = camera->x + (camera->w / 2) / game_scale;
    double y = camera->y + (camera->h / 2) / game_scale;
    float distance = sqrt(pow(fabs(x - galaxy->position.x), 2) + pow(fabs(y - galaxy->position.y), 2));
    int opacity = calculate_projection_opacity(distance, UNIVERSE_REGION_SIZE, UNIVERSE_SECTION_SIZE);

    SDL_SetRenderDrawColor(renderer, galaxy->color.r, galaxy->color.g, galaxy->color.b, opacity);
    SDL_RenderFillRect(renderer, &galaxy->projection);
}

/*
 * Calculate projection opacity according to object distance.
 * Opacity fades to 128 for <near_sections> and then fades linearly to 0.
 */
int calculate_projection_opacity(float distance, int region_size, int section_size)
{
    int a = (int)(distance / section_size);
    int near_sections = 4;
    int opacity;

    if (a <= 1)
        opacity = 255;
    else if (a <= near_sections)
        opacity = 128 + (255 - 128) * (near_sections - a) / (near_sections - 1);
    else if (a <= 10)
        opacity = 64 + (128 - 64) * (10 - a) / (10 - 4);
    else if (a <= region_size)
        opacity = 0 + (64 - 0) * (30 - a) / (30 - 10);
    else
        opacity = 0;

    return opacity;
}

/*
 * Find distance to nearest galaxy.
 */
float nearest_galaxy_center_distance(struct position_t position)
{
    // We use 5 * UNIVERSE_SECTION_SIZE as max, since a CLASS_6 galaxy needs 5 + 1 empty sections
    // We search inner circumferences of points first and work towards outward circumferences
    // If we find a galaxy, the function returns.

    // Keep track of checked points
    struct position_t checked_points[169];
    int num_checked_points = 0;

    // Use a local rng
    pcg32_random_t rng;

    for (int i = 1; i <= 5; i++)
    {
        for (float ix = position.x - i * UNIVERSE_SECTION_SIZE; ix <= position.x + i * UNIVERSE_SECTION_SIZE; ix += UNIVERSE_SECTION_SIZE)
        {
            for (float iy = position.y - i * UNIVERSE_SECTION_SIZE; iy <= position.y + i * UNIVERSE_SECTION_SIZE; iy += UNIVERSE_SECTION_SIZE)
            {
                if (ix == position.x && iy == position.y)
                    continue;

                struct position_t p = {ix, iy};

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
                    float distance = sqrt(pow(ix - position.x, 2) + pow(iy - position.y, 2));

                    return distance;
                }
            }
        }
    }

    return 6 * UNIVERSE_SECTION_SIZE;
}

/*
 * Find distance between two points.
 */
double find_distance(double x1, double y1, double x2, double y2)
{
    return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

/*
 * Find nearest galaxy to position.
 * The funtions finds the galaxy in the galaxies hash table
 * whose circumference is closest to the position.
 */
struct galaxy_t *nearest_galaxy(struct position_t position)
{
    struct galaxy_t *closest = NULL;
    double closest_distance = INFINITY;
    int sections = 10;

    for (int i = 0; i < MAX_GALAXIES; i++)
    {
        struct galaxy_entry *entry = galaxies[i];

        while (entry != NULL)
        {
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
float nearest_star_distance(struct position_t position, struct galaxy_t *current_galaxy, uint64_t initseq)
{
    // We use 5 * GALAXY_SECTION_SIZE as max, since a CLASS_6 star needs 5 + 1 empty sections
    // We search inner circumferences of points first and work towards outward circumferences
    // If we find a star, the function returns.

    // Keep track of checked points
    struct position_t checked_points[169];
    int num_checked_points = 0;

    // Use a local rng
    pcg32_random_t rng;

    // Density scaling parameter
    float a = current_galaxy->radius * GALAXY_SCALE / 2.0f;

    for (int i = 1; i <= 5; i++)
    {
        for (float ix = position.x - i * GALAXY_SECTION_SIZE; ix <= position.x + i * GALAXY_SECTION_SIZE; ix += GALAXY_SECTION_SIZE)
        {
            for (float iy = position.y - i * GALAXY_SECTION_SIZE; iy <= position.y + i * GALAXY_SECTION_SIZE; iy += GALAXY_SECTION_SIZE)
            {
                if (ix == position.x && iy == position.y)
                    continue;

                struct position_t p = {ix, iy};

                if (point_in_array(p, checked_points, num_checked_points))
                    continue;

                checked_points[num_checked_points++] = p;

                // Create rng seed by combining x,y values
                uint64_t seed = pair_hash_order_sensitive(p);

                // Seed with a fixed constant
                pcg32_srandom_r(&rng, seed, initseq);

                // Calculate density based on distance from center
                double distance_from_center = sqrt((ix - position.x) * (ix - position.x) + (iy - position.y) * (iy - position.y));
                float density = (GALAXY_DENSITY / pow((distance_from_center / a + 1), 2));

                int has_star = abs(pcg32_random_r(&rng)) % 1000 < density;

                if (has_star)
                {
                    float distance = sqrt(pow(ix - position.x, 2) + pow(iy - position.y, 2));

                    return distance;
                }
            }
        }
    }

    return 6 * GALAXY_SECTION_SIZE;
}

/*
 * Find galaxy class.
 * n is number of empty sections.
 */
int get_galaxy_class(float n)
{
    if (n < 2 * UNIVERSE_SECTION_SIZE)
        return GALAXY_CLASS_1;
    else if (n < 3 * UNIVERSE_SECTION_SIZE)
        return GALAXY_CLASS_2;
    else if (n < 4 * UNIVERSE_SECTION_SIZE)
        return GALAXY_CLASS_3;
    else if (n < 5 * UNIVERSE_SECTION_SIZE)
        return GALAXY_CLASS_4;
    else if (n < 6 * UNIVERSE_SECTION_SIZE)
        return GALAXY_CLASS_5;
    else if (n >= 6 * UNIVERSE_SECTION_SIZE)
        return GALAXY_CLASS_6;
    else
        return GALAXY_CLASS_1;
}

/*
 * Find star class.
 * n is number of empty sections.
 */
int get_star_class(float n)
{
    if (n < 2 * GALAXY_SECTION_SIZE)
        return STAR_CLASS_1;
    else if (n < 3 * GALAXY_SECTION_SIZE)
        return STAR_CLASS_2;
    else if (n < 4 * GALAXY_SECTION_SIZE)
        return STAR_CLASS_3;
    else if (n < 5 * GALAXY_SECTION_SIZE)
        return STAR_CLASS_4;
    else if (n < 6 * GALAXY_SECTION_SIZE)
        return STAR_CLASS_5;
    else if (n >= 6 * GALAXY_SECTION_SIZE)
        return STAR_CLASS_6;
    else
        return STAR_CLASS_1;
}

/*
 * Find planet class.
 * n is orbit width.
 */
int get_planet_class(float n)
{
    if (n < GALAXY_SECTION_SIZE / 20) // < 500
        return PLANET_CLASS_1;
    else if (n < GALAXY_SECTION_SIZE / 10) // < 1000
        return PLANET_CLASS_2;
    else if (n < GALAXY_SECTION_SIZE / 6.67) // < 1500
        return PLANET_CLASS_3;
    else if (n < GALAXY_SECTION_SIZE / 5) // < 2000
        return PLANET_CLASS_4;
    else if (n < 6 * GALAXY_SECTION_SIZE / 4) // < 2500
        return PLANET_CLASS_5;
    else if (n >= 6 * GALAXY_SECTION_SIZE) // >= 2500
        return PLANET_CLASS_6;
    else
        return PLANET_CLASS_1;
}

/*
 * Compares two points.
 */
bool point_eq(struct position_t a, struct position_t b)
{
    return a.x == b.x && a.y == b.y;
}

/*
 * Checks whether a point exists in an array.
 */
bool point_in_array(struct position_t p, struct position_t arr[], int len)
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
void zoom_star(struct planet_t *planet)
{
    planet->rect.x = (planet->position.x - planet->radius) * game_scale;
    planet->rect.y = (planet->position.y - planet->radius) * game_scale;
    planet->rect.w = 2 * planet->radius * game_scale;
    planet->rect.h = 2 * planet->radius * game_scale;

    // Zoom children
    if (planet->level <= LEVEL_PLANET && planet->planets != NULL && planet->planets[0] != NULL)
    {
        int max_planets = (planet->level == LEVEL_STAR) ? MAX_PLANETS : MAX_MOONS;

        for (int i = 0; i < max_planets && planet->planets[i] != NULL; i++)
        {
            zoom_star(planet->planets[i]);
        }
    }
}

/*
 * Check whether point is in camera in game scale.
 * x, y are relative to camera->x, camera->y.
 */
int in_camera_game_scale(const struct camera_t *camera, int x, int y)
{
    return x >= 0 && x < camera->w && y >= 0 && y < camera->h;
}

/*
 * Check whether object with center (x,y) and radius r is in camera.
 * x, y are absolute full scale coordinates. Radius is full scale.
 */
int in_camera(const struct camera_t *camera, double x, double y, float radius)
{
    return x + radius >= camera->x && x - radius - camera->x < camera->w / game_scale &&
           y + radius >= camera->y && y - radius - camera->y < camera->h / game_scale;
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
    if (in_camera_game_scale(camera, x1, y1) || in_camera_game_scale(camera, x2, y2))
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

void draw_section_lines(struct camera_t *camera, int section_size, SDL_Color color)
{
    double bx = find_nearest_section_axis(camera->x, section_size);
    double by = find_nearest_section_axis(camera->y, section_size);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    for (int ix = bx; ix <= bx + camera->w / game_scale; ix = ix + section_size)
    {
        SDL_RenderDrawLine(renderer, (ix - camera->x) * game_scale, 0, (ix - camera->x) * game_scale, camera->h);
    }

    for (int iy = by; iy <= by + camera->h / game_scale; iy = iy + section_size)
    {
        SDL_RenderDrawLine(renderer, 0, (iy - camera->y) * game_scale, camera->w, (iy - camera->y) * game_scale);
    }
}

void create_galaxy_cloud(struct galaxy_t *galaxy)
{
    float radius = galaxy->radius;
    int section_size = 100;
    float ix, iy;
    int i = 0;
    int max_density = 40;

    // Use a local rng
    pcg32_random_t rng;

    // Density scaling parameter
    float a = radius / 2.0f;

    for (ix = -radius; ix <= radius; ix += section_size)
    {
        for (iy = -radius; iy <= radius; iy += section_size)
        {
            // Calculate the distance from the center of the galaxy
            float distance_from_center = sqrt(ix * ix + iy * iy);

            // Check that point is within galaxy radius
            if (distance_from_center > radius)
                continue;

            // Create rng seed by combining x,y values
            struct position_t position = {.x = ix, .y = iy};
            uint64_t seed = pair_hash_order_sensitive(position);

            // Set galaxy hash as initseq
            uint64_t initseq = pair_hash_order_sensitive_2(galaxy->position);

            // Seed with a fixed constant
            pcg32_srandom_r(&rng, seed, initseq);

            // Calculate density based on distance from center
            float density = (max_density / pow((distance_from_center / a + 1), 4));

            int has_star = abs(pcg32_random_r(&rng)) % 1000 < density;

            if (has_star)
            {
                struct gstar_t star;
                star.position.x = ix;
                star.position.y = iy;
                star.opacity = ((rand() % 196) + 25); // Get a random color between 25 - 195
                galaxy->gstars[i++] = star;
            }
        }
    }

    // Set galaxy as initialized
    galaxy->initialized = i;
}

void draw_galaxy_cloud(struct galaxy_t *galaxy, const struct camera_t *camera, int gstars_count)
{
    for (int i = 0; i < gstars_count; i++)
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, galaxy->gstars[i].opacity);

        int x = ((galaxy->position.x - camera->x) * game_scale + galaxy->gstars[i].position.x * game_scale);
        int y = ((galaxy->position.y - camera->y) * game_scale + galaxy->gstars[i].position.y * game_scale);

        SDL_RenderDrawPoint(renderer, x, y);
    }
}
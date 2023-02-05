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
extern int thrust;
extern int reverse;

static void cleanup_planets(struct planet_t *planet);
static void cleanup_stars(void);
uint64_t float_pair_hash_order_sensitive(float x, float y);
uint64_t float_hash(float x);
bool point_eq(struct position_t a, struct position_t b);
bool point_in_array(struct position_t p, struct position_t arr[], int len);

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
 * Clean up stars.
 */
static void cleanup_stars(void)
{
    int i;

    // Loop through hash table
    for (i = 0; i < MAX_STARS; i++)
    {
        struct star_entry *entry = stars[i];

        while (entry != NULL)
        {
            // Clean up planets
            if (entry->star != NULL)
                cleanup_planets(entry->star);

            // Clean up star
            SDL_DestroyTexture(entry->star->texture);
            entry->star->texture = NULL;
            free(entry->star);

            // Delete entry
            struct star_entry *previous = entry;
            entry = entry->next;
            free(previous);
        }
    }
}

/*
 * Clean up resources.
 */
void cleanup_resources(struct ship_t *ship)
{
    // Clean up stars
    cleanup_stars();

    // Clean up ship
    SDL_DestroyTexture(ship->projection->texture);
    ship->projection->texture = NULL;
    SDL_DestroyTexture(ship->texture);
    ship->texture = NULL;
}

/*
 * Insert a new star entry in stars hash table.
 */
void put_star(float x, float y, struct planet_t *star)
{
    uint64_t index = float_pair_hash_order_sensitive(x, y) % MAX_STARS;
    struct star_entry *entry = (struct star_entry *)malloc(sizeof(struct star_entry));
    entry->x = x;
    entry->y = y;
    entry->star = star;
    entry->next = stars[index];
    stars[index] = entry;
}

/*
 * Check whether a star entry exists in the stars hash table.
 */
int star_exists(float x, float y)
{
    uint64_t index = float_pair_hash_order_sensitive(x, y) % MAX_STARS;
    struct star_entry *entry = stars[index];

    while (entry != NULL)
    {
        if (entry->x == x && entry->y == y)
            return 1;

        entry = entry->next;
    }

    return 0;
}

/*
 * Get a star entry from the stars hash table.
 */
struct planet_t *get_star(float x, float y)
{
    uint64_t index = float_pair_hash_order_sensitive(x, y) % MAX_STARS;
    struct star_entry *entry = stars[index];

    while (entry != NULL)
    {
        if (entry->x == x && entry->y == y)
            return entry->star;

        entry = entry->next;
    }

    return NULL;
}

/*
 * Delete a star entry from the stars hash table.
 */
void delete_star(float x, float y)
{
    uint64_t index = float_pair_hash_order_sensitive(x, y) % MAX_STARS;
    struct star_entry *previous = NULL;
    struct star_entry *entry = stars[index];

    while (entry != NULL)
    {
        if (entry->x == x && entry->y == y)
        {
            // Clean up planets
            if (entry->star != NULL)
                cleanup_planets(entry->star);

            // Clean up star
            SDL_DestroyTexture(entry->star->texture);
            entry->star->texture = NULL;
            free(entry->star);

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
 */
void update_projection_coordinates(void *ptr, int entity_type, const struct camera_t *camera)
{
    struct planet_t *planet = NULL;
    struct ship_t *ship = NULL;

    switch (entity_type)
    {
    case ENTITY_PLANET:
        planet = (struct planet_t *)ptr;
        break;
    case ENTITY_SHIP:
        ship = (struct ship_t *)ptr;
        break;
    }

    float delta_x, delta_y, point;

    // Find screen quadrant for planet exit from screen; 0, 0 is screen center, negative y is up
    if (planet)
    {
        delta_x = planet->position.x - (camera->x + (camera->w / 2));
        delta_y = planet->position.y - (camera->y + (camera->h / 2));
    }
    else if (ship)
    {
        delta_x = ship->position.x - (camera->x + (camera->w / 2));
        delta_y = ship->position.y - (camera->y + (camera->h / 2));
    }

    // 1st quadrant (clockwise)
    if (delta_x >= 0 && delta_y < 0)
    {
        point = (int)(((camera->h / 2) - PROJECTION_RADIUS) * delta_x / (-delta_y));

        // Top
        if (point <= (camera->w / 2) - PROJECTION_RADIUS)
        {
            if (planet)
            {
                planet->projection.x = (camera->w / 2) + point - PROJECTION_RADIUS;
                planet->projection.y = 0;
            }
            else if (ship)
            {
                ship->projection->rect.x = (camera->w / 2) + point - PROJECTION_RADIUS;
                ship->projection->rect.y = 0;
            }
        }
        // Right
        else
        {
            if (delta_x > 0)
                point = (int)(((camera->h / 2) - PROJECTION_RADIUS) - ((camera->w / 2) - PROJECTION_RADIUS) * (-delta_y) / delta_x);
            else
                point = (int)(((camera->h / 2) - PROJECTION_RADIUS) - ((camera->w / 2) - PROJECTION_RADIUS));

            if (planet)
            {
                planet->projection.x = camera->w - (2 * PROJECTION_RADIUS);
                planet->projection.y = point;
            }
            else if (ship)
            {
                ship->projection->rect.x = camera->w - (2 * PROJECTION_RADIUS);
                ship->projection->rect.y = point;
            }
        }
    }
    // 2nd quadrant
    else if (delta_x >= 0 && delta_y >= 0)
    {
        if (delta_x > 0)
            point = (int)(((camera->w / 2) - PROJECTION_RADIUS) * delta_y / delta_x);
        else
            point = (int)((camera->w / 2) - PROJECTION_RADIUS);

        // Right
        if (point <= (camera->h / 2) - PROJECTION_RADIUS)
        {
            if (planet)
            {
                planet->projection.x = camera->w - (2 * PROJECTION_RADIUS);
                planet->projection.y = (camera->h / 2) + point - PROJECTION_RADIUS;
            }
            else if (ship)
            {
                ship->projection->rect.x = camera->w - (2 * PROJECTION_RADIUS);
                ship->projection->rect.y = (camera->h / 2) + point - PROJECTION_RADIUS;
            }
        }
        // Bottom
        else
        {
            if (delta_y > 0)
                point = (int)(((camera->h / 2) - PROJECTION_RADIUS) * delta_x / delta_y);
            else
                point = (int)(((camera->h / 2) - PROJECTION_RADIUS));

            if (planet)
            {
                planet->projection.x = (camera->w / 2) + point - PROJECTION_RADIUS;
                planet->projection.y = camera->h - (2 * PROJECTION_RADIUS);
            }
            else if (ship)
            {
                ship->projection->rect.x = (camera->w / 2) + point - PROJECTION_RADIUS;
                ship->projection->rect.y = camera->h - (2 * PROJECTION_RADIUS);
            }
        }
    }
    // 3rd quadrant
    else if (delta_x < 0 && delta_y >= 0)
    {
        if (delta_y > 0)
            point = (int)(((camera->h / 2) - PROJECTION_RADIUS) * (-delta_x) / delta_y);
        else
            point = (int)(((camera->h / 2) - PROJECTION_RADIUS));

        // Bottom
        if (point <= (camera->w / 2) - PROJECTION_RADIUS)
        {
            if (planet)
            {
                planet->projection.x = (camera->w / 2) - point - PROJECTION_RADIUS;
                planet->projection.y = camera->h - (2 * PROJECTION_RADIUS);
            }
            else if (ship)
            {
                ship->projection->rect.x = (camera->w / 2) - point - PROJECTION_RADIUS;
                ship->projection->rect.y = camera->h - (2 * PROJECTION_RADIUS);
            }
        }
        // Left
        else
        {
            point = (int)(((camera->h / 2) - PROJECTION_RADIUS) - ((camera->w / 2) - PROJECTION_RADIUS) * delta_y / (-delta_x));

            if (planet)
            {
                planet->projection.x = 0;
                planet->projection.y = camera->h - point - (2 * PROJECTION_RADIUS);
            }
            else if (ship)
            {
                ship->projection->rect.x = 0;
                ship->projection->rect.y = camera->h - point - (2 * PROJECTION_RADIUS);
            }
        }
    }
    // 4th quadrant
    else if (delta_x < 0 && delta_y < 0)
    {
        point = (int)(((camera->w / 2) - PROJECTION_RADIUS) * (-delta_y) / (-delta_x));

        // Left
        if (point <= (camera->h / 2) - PROJECTION_RADIUS)
        {
            if (planet)
            {
                planet->projection.x = 0;
                planet->projection.y = (camera->h / 2) - point - PROJECTION_RADIUS;
            }
            else if (ship)
            {
                ship->projection->rect.x = 0;
                ship->projection->rect.y = (camera->h / 2) - point - PROJECTION_RADIUS;
            }
        }
        // Top
        else
        {
            point = (int)(((camera->w / 2) - PROJECTION_RADIUS) - ((camera->h / 2) - PROJECTION_RADIUS) * (-delta_x) / (-delta_y));

            if (planet)
            {
                planet->projection.x = point;
                planet->projection.y = 0;
            }
            else if (ship)
            {
                ship->projection->rect.x = point;
                ship->projection->rect.y = 0;
            }
        }
    }
}

/*
 * Draw ship projection on axis.
 */
void project_ship(struct ship_t *ship, const struct camera_t *camera)
{
    update_projection_coordinates(ship, ENTITY_SHIP, camera);

    // Mirror ship angle
    ship->projection->angle = ship->angle;

    // Draw ship projection
    SDL_RenderCopyEx(renderer, ship->projection->texture, &ship->projection->main_img_rect, &ship->projection->rect, ship->projection->angle, &ship->projection->rotation_pt, SDL_FLIP_NONE);

    // Draw projection thrust
    if (thrust)
        SDL_RenderCopyEx(renderer, ship->projection->texture, &ship->projection->thrust_img_rect, &ship->projection->rect, ship->projection->angle, &ship->projection->rotation_pt, SDL_FLIP_NONE);

    // Draw projection reverse
    if (reverse)
        SDL_RenderCopyEx(renderer, ship->projection->texture, &ship->projection->reverse_img_rect, &ship->projection->rect, ship->projection->angle, &ship->projection->rotation_pt, SDL_FLIP_NONE);
}

/*
 * Draw planet projection on axis.
 */
void project_planet(struct planet_t *planet, const struct camera_t *camera)
{
    update_projection_coordinates(planet, ENTITY_PLANET, camera);

    planet->projection.w = 2 * PROJECTION_RADIUS;
    planet->projection.h = 2 * PROJECTION_RADIUS;

    SDL_SetRenderDrawColor(renderer, planet->color.r, planet->color.g, planet->color.b, 255);
    SDL_RenderFillRect(renderer, &planet->projection);
}

/*
 * Find distance to nearest star.
 * x, y are range scale.
 */
float nearest_star_distance(int x, int y)
{
    // We use 5 * SECTION_SIZE as max, since a CLASS_6 star needs 5 + 1 empty sections
    // We search inner circumferences of points first and work towards outward circumferences
    // If we find a star, the function returns.

    // Keep track of checked points
    struct position_t checked_points[169];
    int num_checked_points = 0;

    // Use a local rng
    pcg32_random_t rng;

    for (int i = 1; i <= 5; i++)
    {
        for (float ix = x - i * SECTION_SIZE; ix <= x + i * SECTION_SIZE; ix += SECTION_SIZE)
        {
            for (float iy = y - i * SECTION_SIZE; iy <= y + i * SECTION_SIZE; iy += SECTION_SIZE)
            {
                if (ix == x && iy == y)
                    continue;

                struct position_t p = {ix, iy};

                if (point_in_array(p, checked_points, num_checked_points))
                    continue;

                checked_points[num_checked_points++] = p;

                // Seed with a fixed constant
                pcg32_srandom_r(&rng, float_hash(ix), float_hash(iy));

                int has_star = abs(pcg32_random_r(&rng)) % 1000 < REGION_DENSITY;

                if (has_star)
                {
                    float distance = sqrt(pow(ix - x, 2) + pow(iy - y, 2));

                    return distance;
                }
            }
        }
    }

    return 6 * SECTION_SIZE;
}

/*
 * Find star class.
 * n is region scale.
 */
int get_star_class(float n)
{
    if (n < 2 * SECTION_SIZE)
        return STAR_CLASS_1;
    else if (n < 3 * SECTION_SIZE)
        return STAR_CLASS_2;
    else if (n < 4 * SECTION_SIZE)
        return STAR_CLASS_3;
    else if (n < 5 * SECTION_SIZE)
        return STAR_CLASS_4;
    else if (n < 6 * SECTION_SIZE)
        return STAR_CLASS_5;
    else if (n >= 6 * SECTION_SIZE)
        return STAR_CLASS_6;
    else
        return STAR_CLASS_1;
}

/*
 * Find planet class.
 * n is region scale.
 */
int get_planet_class(float n)
{
    if (n < SECTION_SIZE / 20) // < 500
        return PLANET_CLASS_1;
    else if (n < SECTION_SIZE / 10) // < 1000
        return PLANET_CLASS_2;
    else if (n < SECTION_SIZE / 6.67) // < 1500
        return PLANET_CLASS_3;
    else if (n < SECTION_SIZE / 5) // < 2000
        return PLANET_CLASS_4;
    else if (n < 6 * SECTION_SIZE / 4) // < 2500
        return PLANET_CLASS_5;
    else if (n >= 6 * SECTION_SIZE) // >= 2500
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
 * Prints coordinates of all points around a specific section point.
 */
void print_nearest_star_coordinates(float x, float y)
{
    struct position_t checked_points[169];
    int num_checked_points = 0;

    // Use a local rng
    pcg32_random_t rng;

    for (int i = 1; i <= 6; i++)
    {
        for (float ix = x - i * SECTION_SIZE; ix <= x + i * SECTION_SIZE; ix += SECTION_SIZE)
        {
            for (float iy = y - i * SECTION_SIZE; iy <= y + i * SECTION_SIZE; iy += SECTION_SIZE)
            {
                if (ix == x && iy == y)
                    continue;

                struct position_t p = {ix, iy};

                if (point_in_array(p, checked_points, num_checked_points))
                    continue;

                checked_points[num_checked_points++] = p;

                // Seed with a fixed constant
                pcg32_srandom_r(&rng, float_hash(ix), float_hash(iy));

                int has_star = abs(pcg32_random_r(&rng)) % 1000 < REGION_DENSITY;

                printf("\n%f,%f ::: %d", ix, iy, has_star);
            }
        }
    }
}

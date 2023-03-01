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
double nearest_galaxy_center_distance(struct point_t position);
struct galaxy_t *find_nearest_galaxy(NavigationState nav_state, struct point_t position, int exclude);
double nearest_star_distance(struct point_t position, struct galaxy_t *current_galaxy, uint64_t initseq, int galaxy_density);
int get_galaxy_class(float distance);
int get_star_class(float distance);
int get_planet_class(float width);
void delete_stars_outside_region(struct star_entry *stars[], double bx, double by, int region_size);

// External function prototypes
uint64_t pair_hash_order_sensitive(struct point_t);
uint64_t pair_hash_order_sensitive_2(struct point_t);
uint64_t unique_index(struct point_t, int modulo, int entity_type);
double find_distance(double x1, double y1, double x2, double y2);
bool point_in_array(struct point_t p, struct point_t arr[], int len);

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

/*
 * galaxies.c - Definitions for galaxies functions.
 */

#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include "../lib/pcg-c-basic-0.9/pcg_basic.h"

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"

// External variable definitions
extern SDL_Renderer *renderer;
extern SDL_Color colors[];

// Function prototypes
void cleanup_galaxies(struct galaxy_entry *galaxies[]);
void put_galaxy(struct galaxy_entry *galaxies[], struct point_t position, struct galaxy_t *galaxy);
int galaxy_exists(struct galaxy_entry *galaxies[], struct point_t position);
struct galaxy_t *get_galaxy(struct galaxy_entry *galaxies[], struct point_t position);
void delete_galaxy(struct galaxy_entry *galaxies[], struct point_t position);
double nearest_galaxy_center_distance(struct point_t position);
struct galaxy_t *find_nearest_galaxy(NavigationState nav_state, struct point_t position, int exclude);
int get_galaxy_class(float distance);
struct galaxy_t *create_galaxy(struct point_t position);
void update_galaxy(NavigationState *nav_state, struct galaxy_t *galaxy, const struct camera_t *camera, int state, long double scale);
void generate_galaxies(GameEvents *game_events, NavigationState *nav_state, struct point_t offset);

// External function prototypes
uint64_t pair_hash_order_sensitive(struct point_t);
uint64_t pair_hash_order_sensitive_2(struct point_t);
uint64_t unique_index(struct point_t, int modulo, int entity_type);
double find_distance(double x1, double y1, double x2, double y2);
bool point_in_array(struct point_t p, struct point_t arr[], int len);
void create_galaxy_cloud(struct galaxy_t *galaxy, unsigned short high_definition);
void draw_galaxy_cloud(struct galaxy_t *galaxy, const struct camera_t *camera, int gstars_count, unsigned short high_definition, long double scale);
void cleanup_stars(struct star_entry *stars[]);
void project_galaxy(int state, NavigationState nav_state, struct galaxy_t *galaxy, const struct camera_t *camera, long double scale);
void SDL_DrawCircle(SDL_Renderer *renderer, const struct camera_t *camera, int xc, int yc, int radius, SDL_Color color);
int in_camera(const struct camera_t *camera, double x, double y, float radius, long double scale);
double find_nearest_section_axis(double offset, int size);

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
            create_galaxy_cloud(galaxy, true);

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
            draw_galaxy_cloud(galaxy, camera, galaxy->initialized_hd, true, scale);
    }
    else
    {
        // Draw galaxy cloud
        if (in_camera(camera, galaxy->position.x, galaxy->position.y, galaxy->radius, scale * GALAXY_SCALE))
        {
            if (!galaxy->initialized)
                create_galaxy_cloud(galaxy, false);

            draw_galaxy_cloud(galaxy, camera, galaxy->initialized, false, scale);
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
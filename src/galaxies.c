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
#include "../include/galaxies.h"

// External variable definitions
extern SDL_Renderer *renderer;
extern SDL_Color colors[];

// Static function prototypes
static void galaxies_add_entry(GalaxyEntry *galaxies[], Point, Galaxy *);
static bool galaxies_entry_exists(GalaxyEntry *galaxies[], Point);
static void galaxies_delete_entry(GalaxyEntry *galaxies[], Point);
static double galaxies_nearest_center_distance(Point);
static int galaxies_size_class(float distance);
static Galaxy *galaxies_create_galaxy(Point);

/*
 * Clean up galaxies.
 */
void galaxies_clear_table(GalaxyEntry *galaxies[])
{
    // Loop through hash table
    for (int s = 0; s < MAX_GALAXIES; s++)
    {
        GalaxyEntry *entry = galaxies[s];

        while (entry != NULL)
        {
            Point position = {.x = entry->x, .y = entry->y};
            galaxies_delete_entry(galaxies, position);
            entry = entry->next;
        }
    }
}

/*
 * Insert a new galaxy entry in galaxies hash table.
 */
static void galaxies_add_entry(GalaxyEntry *galaxies[], Point position, Galaxy *galaxy)
{
    // Generate index for hash table
    uint64_t index = maths_hash_position_to_index(position, MAX_GALAXIES, ENTITY_GALAXY);

    GalaxyEntry *entry = (GalaxyEntry *)malloc(sizeof(GalaxyEntry));

    if (entry == NULL)
    {
        fprintf(stderr, "Error: Could not create GalaxyEntry.\n");
        return;
    }

    entry->x = position.x;
    entry->y = position.y;
    entry->galaxy = galaxy;
    entry->next = galaxies[index];
    galaxies[index] = entry;
}

/*
 * Check whether a galaxy entry exists in the galaxies hash table.
 */
static bool galaxies_entry_exists(GalaxyEntry *galaxies[], Point position)
{
    // Generate index for hash table
    uint64_t index = maths_hash_position_to_index(position, MAX_GALAXIES, ENTITY_GALAXY);

    GalaxyEntry *entry = galaxies[index];

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
Galaxy *galaxies_get_entry(GalaxyEntry *galaxies[], Point position)
{
    // Generate index for hash table
    uint64_t index = maths_hash_position_to_index(position, MAX_GALAXIES, ENTITY_GALAXY);

    GalaxyEntry *entry = galaxies[index];

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
static void galaxies_delete_entry(GalaxyEntry *galaxies[], Point position)
{
    // Generate index for hash table
    uint64_t index = maths_hash_position_to_index(position, MAX_GALAXIES, ENTITY_GALAXY);

    GalaxyEntry *previous = NULL;
    GalaxyEntry *entry = galaxies[index];

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
static double galaxies_nearest_center_distance(Point position)
{
    // We use 6 * UNIVERSE_SECTION_SIZE as max, since a CLASS_6 galaxy needs 6 + 1 empty sections
    // We search inner circumferences of points first and work towards outward circumferences
    // If we find a galaxy, the function returns.

    // Keep track of checked points
    Point checked_points[196];
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

                Point p = {ix, iy};

                if (maths_check_point_in_array(p, checked_points, num_checked_points))
                    continue;

                checked_points[num_checked_points++] = p;

                // Create rng seed by combining x,y values
                uint64_t seed = maths_hash_position_to_uint64_2(p);

                // Seed with a fixed constant
                pcg32_srandom_r(&rng, seed, 1);

                int has_galaxy = abs(pcg32_random_r(&rng)) % 1000 < UNIVERSE_DENSITY;

                if (has_galaxy)
                {
                    double distance = maths_distance_between_points(ix, iy, position.x, position.y);

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
Galaxy *galaxies_nearest_circumference(const NavigationState *nav_state, Point position, int exclude)
{
    Galaxy *closest = NULL;
    double closest_distance = INFINITY;
    int sections = 10;

    for (int i = 0; i < MAX_GALAXIES; i++)
    {
        GalaxyEntry *entry = nav_state->galaxies[i];

        while (entry != NULL)
        {
            // Exlude current galaxy
            if (exclude)
            {
                if (entry->galaxy->position.x == nav_state->current_galaxy->position.x &&
                    entry->galaxy->position.y == nav_state->current_galaxy->position.y)
                {
                    entry = entry->next;
                    continue;
                }
            }

            double cx = entry->galaxy->position.x;
            double cy = entry->galaxy->position.y;
            double r = entry->galaxy->radius;
            double d = maths_distance_between_points(position.x, position.y, cx, cy);

            if (d <= r * GALAXY_SCALE + sections * UNIVERSE_SECTION_SIZE)
            {
                double angle = atan2(position.y - cy, position.x - cx);
                double px = cx + r * cos(angle);
                double py = cy + r * sin(angle);
                double pd = maths_distance_between_points(position.x, position.y, px, py);

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
static int galaxies_size_class(float distance)
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
static Galaxy *galaxies_create_galaxy(Point position)
{
    // Find distance to nearest galaxy
    double distance = galaxies_nearest_center_distance(position);

    // Get galaxy class
    int class = galaxies_size_class(distance);

    // Use a local rng
    pcg32_random_t rng;

    // Create rng seed by combining x,y values
    uint64_t seed = maths_hash_position_to_uint64_2(position);

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
    Galaxy *galaxy = (Galaxy *)malloc(sizeof(Galaxy));

    if (galaxy == NULL)
    {
        fprintf(stderr, "Error: Could not create Galaxy.\n");
        return NULL;
    }

    // Get unique galaxy index
    uint64_t index = maths_hash_position_to_uint64_2(position);

    galaxy->initialized = 0;
    galaxy->initialized_hd = 0;
    sprintf(galaxy->name, "%s-%lu", "G", index);
    galaxy->class = galaxies_size_class(distance);
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
 * Draw galaxy.
 */
void galaxies_draw_galaxy(NavigationState *nav_state, Galaxy *galaxy, const Camera *camera, int state, long double scale)
{
    // Get galaxy distance from position
    double distance = maths_distance_between_points(galaxy->position.x, galaxy->position.y, nav_state->universe_offset.x, nav_state->universe_offset.y);

    // Draw cutoff circle
    if (distance < galaxy->cutoff)
    {
        // Reset stars and update current_galaxy
        if (strcmp(nav_state->current_galaxy->name, galaxy->name) != 0)
        {
            stars_clear_table(nav_state->stars);
            memcpy(nav_state->current_galaxy, galaxy, sizeof(Galaxy));
        }

        int cutoff = galaxy->cutoff * scale * GALAXY_SCALE;
        int rx = (galaxy->position.x - camera->x) * scale * GALAXY_SCALE;
        int ry = (galaxy->position.y - camera->y) * scale * GALAXY_SCALE;

        gfx_draw_circle(renderer, camera, rx, ry, cutoff, colors[COLOR_CYAN_70]);

        // Create gstars_hd
        if (!galaxy->initialized_hd)
            gfx_generate_gstars(galaxy, true);

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
            gfx_draw_galaxy_cloud(galaxy, camera, galaxy->initialized_hd, true, scale);
    }
    else
    {
        // Draw galaxy cloud
        if (gfx_object_in_camera(camera, galaxy->position.x, galaxy->position.y, galaxy->radius, scale * GALAXY_SCALE))
        {
            if (!galaxy->initialized)
                gfx_generate_gstars(galaxy, false);

            gfx_draw_galaxy_cloud(galaxy, camera, galaxy->initialized, false, scale);
        }
        // Draw galaxy projection
        else if (PROJECTIONS_ON)
        {
            // Show projections only if game scale < 50 * ZOOM_UNIVERSE_MIN
            if (scale / (ZOOM_UNIVERSE_MIN / GALAXY_SCALE) < 50)
                gfx_project_galaxy_on_edge(state, nav_state, galaxy, camera, scale * GALAXY_SCALE);
        }
    }
}

/*
 * Probe region for galaxies and create them procedurally.
 * The region has intervals of size UNIVERSE_SECTION_SIZE.
 */
void galaxies_generate(GameEvents *game_events, NavigationState *nav_state, Point offset)
{
    // Keep track of current nearest section axis coordinates
    double bx = maths_get_nearest_section_axis(offset.x, UNIVERSE_SECTION_SIZE);
    double by = maths_get_nearest_section_axis(offset.y, UNIVERSE_SECTION_SIZE);

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
            Point position = {.x = ix, .y = iy};
            uint64_t seed = maths_hash_position_to_uint64_2(position);

            // Seed with a fixed constant
            pcg32_srandom_r(&rng, seed, 1);

            int has_galaxy = abs(pcg32_random_r(&rng)) % 1000 < UNIVERSE_DENSITY;

            if (has_galaxy)
            {
                // Check whether galaxy exists in hash table
                if (galaxies_entry_exists(nav_state->galaxies, position))
                    continue;
                else
                {
                    // Create galaxy
                    Galaxy *galaxy = galaxies_create_galaxy(position);

                    // Add galaxy to hash table
                    galaxies_add_entry(nav_state->galaxies, position, galaxy);
                }
            }
        }
    }

    // Delete galaxies that end up outside the region
    for (int s = 0; s < MAX_GALAXIES; s++)
    {
        if (nav_state->galaxies[s] != NULL)
        {
            Point position = {.x = nav_state->galaxies[s]->x, .y = nav_state->galaxies[s]->y};

            // Skip current galaxy, otherwise we lose track of where we are
            if (!game_events->galaxies_start)
            {
                if (position.x == nav_state->current_galaxy->position.x && position.y == nav_state->current_galaxy->position.y)
                    continue;
            }

            // Get distance from center of region
            double distance = maths_distance_between_points(position.x, position.y, bx, by);
            double region_radius = sqrt((double)2 * ((UNIVERSE_REGION_SIZE + 1) / 2) * UNIVERSE_SECTION_SIZE * ((UNIVERSE_REGION_SIZE + 1) / 2) * UNIVERSE_SECTION_SIZE);

            // If galaxy outside region, delete it
            if (distance >= region_radius)
                galaxies_delete_entry(nav_state->galaxies, position);
        }
    }

    // First galaxy generation complete
    game_events->galaxies_start = OFF;
}
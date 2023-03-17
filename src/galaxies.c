/*
 * galaxies.c
 */

#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "../lib/pcg-c-basic-0.9/pcg_basic.h"

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"
#include "../include/galaxies.h"

// External variable definitions
extern TTF_Font *fonts[];
extern SDL_Renderer *renderer;
extern SDL_Color colors[];

// Static function prototypes
static void galaxies_add_entry(GalaxyEntry *galaxies[], Point, Galaxy *);
static Galaxy *galaxies_create_galaxy(Point);
static void galaxies_delete_entry(GalaxyEntry *galaxies[], Point);
static bool galaxies_entry_exists(GalaxyEntry *galaxies[], Point);
static double galaxies_nearest_center_distance(Point);
static unsigned short galaxies_size_class(float distance);

/**
 * Adds a new entry to the galaxy hash table at the given position.
 *
 * @param galaxies[] The array of galaxy entries.
 * @param position The position of the new galaxy entry.
 * @param galaxy The galaxy entry to add.
 *
 * @return void
 */
static void galaxies_add_entry(GalaxyEntry *galaxies[], Point position, Galaxy *galaxy)
{
    // Generate index for hash table
    uint64_t index = maths_hash_position_to_index(position, MAX_GALAXIES, ENTITY_GALAXY);

    GalaxyEntry *entry = (GalaxyEntry *)malloc(sizeof(GalaxyEntry));

    if (entry == NULL)
    {
        fprintf(stderr, "Error: Could not allocate memory for GalaxyEntry.\n");
        return;
    }

    entry->x = position.x;
    entry->y = position.y;
    entry->galaxy = galaxy;
    entry->next = galaxies[index];
    galaxies[index] = entry;
}

/**
 * Clear the entire hash table of galaxies.
 *
 * @param galaxies[] The hash table of galaxies to be cleared.
 *
 * @return void
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
            GalaxyEntry *next_entry = entry->next;
            galaxies_delete_entry(galaxies, position);
            entry = next_entry;
        }
    }
}

/**
 * Generates a new Galaxy struct with random characteristics based on the provided position.
 *
 * @param position The position of the new Galaxy.
 *
 * @return A pointer to the new Galaxy struct.
 */
static Galaxy *galaxies_create_galaxy(Point position)
{
    // Find distance to nearest galaxy center
    double distance = galaxies_nearest_center_distance(position);

    unsigned short class = galaxies_size_class(distance);

    // Use a local rng
    pcg32_random_t rng;

    // Create rng seed by combining x,y values
    uint64_t seed = maths_hash_position_to_uint64_2(position);

    // Seed with a fixed constant
    pcg32_srandom_r(&rng, seed, 1);

    float radius;

    switch (class)
    {
    case GALAXY_1:
        radius = abs(pcg32_random_r(&rng)) % GALAXY_1_RADIUS_MAX + GALAXY_1_RADIUS_MIN;
        break;
    case GALAXY_2:
        radius = abs(pcg32_random_r(&rng)) % GALAXY_2_RADIUS_MAX + GALAXY_2_RADIUS_MIN;
        break;
    case GALAXY_3:
        radius = abs(pcg32_random_r(&rng)) % GALAXY_3_RADIUS_MAX + GALAXY_3_RADIUS_MIN;
        break;
    case GALAXY_4:
        radius = abs(pcg32_random_r(&rng)) % GALAXY_4_RADIUS_MAX + GALAXY_4_RADIUS_MIN;
        break;
    case GALAXY_5:
        radius = abs(pcg32_random_r(&rng)) % GALAXY_5_RADIUS_MAX + GALAXY_5_RADIUS_MIN;
        break;
    case GALAXY_6:
        radius = abs(pcg32_random_r(&rng)) % GALAXY_6_RADIUS_MAX + GALAXY_6_RADIUS_MIN;
        break;
    default:
        radius = abs(pcg32_random_r(&rng)) % GALAXY_1_RADIUS_MAX + GALAXY_1_RADIUS_MIN;
        break;
    }

    // Allocate memory for Galaxy
    Galaxy *galaxy = (Galaxy *)malloc(sizeof(Galaxy));

    if (galaxy == NULL)
    {
        fprintf(stderr, "Error: Could not allocate memory for Galaxy.\n");
        return NULL;
    }

    // Generate unique galaxy position hash
    uint64_t position_hash = maths_hash_position_to_uint64_2(position);

    galaxy->initialized = 0;
    galaxy->initialized_hd = 0;
    galaxy->last_star_index = 0;
    galaxy->last_star_index_hd = 0;
    galaxy->sections_in_group = 0;
    galaxy->sections_in_group_hd = 0;
    galaxy->total_groups = 0;
    galaxy->total_groups_hd = 0;
    memset(galaxy->name, 0, sizeof(galaxy->name));
    sprintf(galaxy->name, "%s-%lu", "G", position_hash);
    galaxy->class = class;
    galaxy->radius = radius;
    galaxy->cutoff = UNIVERSE_SECTION_SIZE * class / 2;
    galaxy->is_selected = false;
    galaxy->position.x = position.x;
    galaxy->position.y = position.y;
    galaxy->projection = (SDL_Point){0, 0};
    galaxy->color = colors[COLOR_WHITE_255];

    for (int i = 0; i < MAX_GSTARS; i++)
    {
        galaxy->gstars[i].position = (Point){0, 0};
        galaxy->gstars[i].opacity = 0;
        galaxy->gstars[i].final_star = false;
        galaxy->gstars[i].color = (SDL_Color){0, 0, 0, 0};

        galaxy->gstars_hd[i].position = (Point){0, 0};
        galaxy->gstars_hd[i].opacity = 0;
        galaxy->gstars_hd[i].final_star = false;
        galaxy->gstars_hd[i].color = (SDL_Color){0, 0, 0, 0};
    }

    return galaxy;
}

/**
 * Deletes a GalaxyEntry from the hash table by its position and frees associated memory.
 *
 * @param galaxies[] An array of GalaxyEntry pointers representing the hash table.
 * @param position A Point struct representing the position of the GalaxyEntry to be deleted.
 *
 * @return void
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

/**
 * Draws the given galaxy if it is within range, along with any associated stars.
 * If the galaxy is out of range, it draws a projection or nothing depending on the scale.
 *
 * @param nav_state A pointer to the current navigation state.
 * @param galaxy A pointer to the galaxy to draw.
 * @param camera A pointer to the current camera state.
 * @param state An integer representing the current game state.
 * @param scale A long double representing the current scale of the universe.
 *
 * @return void
 */
void galaxies_draw_galaxy(const InputState *input_state, NavigationState *nav_state, Galaxy *galaxy, const Camera *camera, int state, long double scale)
{
    // Get relative position of galaxy in game_scale
    int cutoff = galaxy->cutoff * scale * GALAXY_SCALE;
    int x = (galaxy->position.x - camera->x) * scale * GALAXY_SCALE;
    int y = (galaxy->position.y - camera->y) * scale * GALAXY_SCALE;
    Point galaxy_position = {.x = x, .y = y};

    bool galaxy_is_selected = strcmp(nav_state->current_galaxy->name, galaxy->name) == 0 && nav_state->current_galaxy->is_selected;
    bool galaxy_is_hovered = strcmp(nav_state->current_galaxy->name, galaxy->name) == 0 && input_state->is_hovering_galaxy;

    if (galaxy_is_selected || galaxy_is_hovered ||
        (maths_is_point_in_circle(input_state->mouse_position, galaxy_position, cutoff) &&
         gfx_is_object_in_camera(camera, galaxy->position.x, galaxy->position.y, galaxy->radius, scale * GALAXY_SCALE)))
    {
        // Reset stars and update current_galaxy
        if (strcmp(nav_state->current_galaxy->name, galaxy->name) != 0)
        {
            stars_clear_table(nav_state->stars, nav_state->buffer_star);
            memcpy(nav_state->current_galaxy, galaxy, sizeof(Galaxy));
        }

        // Draw cutoff area circles
        unsigned short color_code;

        if (galaxy_is_selected)
            color_code = COLOR_CYAN_70;
        else
            color_code = COLOR_MAGENTA_70;

        gfx_draw_circle_approximation(renderer, camera, x, y, cutoff, colors[color_code]);

        // Generate gstars
        if (!galaxy->initialized || galaxy->initialized < galaxy->total_groups)
            gfx_generate_gstars(galaxy, false);

        if (!galaxy->initialized_hd || galaxy->initialized_hd < galaxy->total_groups_hd)
            gfx_generate_gstars(galaxy, true);

        double zoom_generate_preview_stars = ZOOM_GENERATE_PREVIEW_STARS;

        switch (nav_state->current_galaxy->class)
        {
        case 1:
            zoom_generate_preview_stars = 0.00005;
            break;

        default:
            zoom_generate_preview_stars = ZOOM_GENERATE_PREVIEW_STARS;
            break;
        }

        const double epsilon = ZOOM_EPSILON / GALAXY_SCALE;

        if (scale < zoom_generate_preview_stars + epsilon)
        {
            // Show hi_def gstars when ready
            if (galaxy->initialized_hd == galaxy->total_groups_hd)
                gfx_draw_galaxy_cloud(galaxy, camera, galaxy->last_star_index_hd, true, scale);
            else
                gfx_draw_galaxy_cloud(galaxy, camera, galaxy->last_star_index, false, scale);
        }
    }
    else
    {
        // Draw galaxy cloud
        if (gfx_is_object_in_camera(camera, galaxy->position.x, galaxy->position.y, galaxy->radius, scale * GALAXY_SCALE) &&
            !maths_is_point_in_circle(input_state->mouse_position, galaxy_position, cutoff))
        {
            if (!galaxy->initialized || galaxy->initialized < galaxy->total_groups)
                gfx_generate_gstars(galaxy, false);

            gfx_draw_galaxy_cloud(galaxy, camera, galaxy->last_star_index, false, scale);
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

/**
 * Draws a box on the screen that displays information about a galaxy.
 *
 * @param galaxy A pointer to the Galaxy for which to display info.
 * @param camera A pointer to the current Camera object.
 *
 * @return void
 */
void galaxies_draw_info_box(const Galaxy *galaxy, const Camera *camera)
{
    // Draw background box
    SDL_SetRenderDrawColor(renderer, 12, 12, 12, 200);
    int width = 370;
    int height = 310;
    int padding = 20;

    SDL_Rect info_box_rect;
    info_box_rect.x = camera->w - (width + padding);
    info_box_rect.y = camera->h - (height + padding);
    info_box_rect.w = width;
    info_box_rect.h = height;

    SDL_RenderFillRect(renderer, &info_box_rect);

    // Create info array
    InfoBoxEntry entries[GALAXY_INFO_COUNT];

    char galaxy_name[128];
    strcpy(galaxy_name, galaxy->name);
    sprintf(entries[GALAXY_INFO_NAME].text, "%s", galaxy_name);
    entries[GALAXY_INFO_NAME].font_size = FONT_SIZE_22;

    char entity_type[32];
    strcpy(entity_type, "GALAXY");
    sprintf(entries[GALAXY_INFO_TYPE].text, "%s", entity_type);
    entries[GALAXY_INFO_TYPE].font_size = FONT_SIZE_15;

    char position_x_text[32];
    utils_add_thousand_separators((int)galaxy->position.x, position_x_text, sizeof(position_x_text));
    sprintf(entries[GALAXY_INFO_X].text, "Position X: %*s%s", 2, "", position_x_text);
    entries[GALAXY_INFO_X].font_size = FONT_SIZE_15;

    char position_y_text[32];
    utils_add_thousand_separators((int)galaxy->position.y, position_y_text, sizeof(position_y_text));
    sprintf(entries[GALAXY_INFO_Y].text, "Position Y: %*s%s", 2, "", position_y_text);
    entries[GALAXY_INFO_Y].font_size = FONT_SIZE_15;

    char class_text[32];
    sprintf(class_text, "%d", galaxy->class);
    sprintf(entries[GALAXY_INFO_CLASS].text, "Class: %*s%s", 7, "", class_text);
    entries[GALAXY_INFO_CLASS].font_size = FONT_SIZE_15;

    char radius_text[32];
    utils_add_thousand_separators((int)galaxy->radius, radius_text, sizeof(radius_text));
    sprintf(entries[GALAXY_INFO_RADIUS].text, "Radius: %*s%s", 6, "", radius_text);
    entries[GALAXY_INFO_RADIUS].font_size = FONT_SIZE_15;

    // Get stars in gstars and approximate to full scale
    int num_stars = galaxy->last_star_index * (galaxy->sections_in_group * galaxy->sections_in_group);
    char stars_text[32];
    utils_add_thousand_separators(num_stars, stars_text, sizeof(stars_text));
    sprintf(entries[GALAXY_INFO_STARS].text, "Stars: %*s%s", 7, "", stars_text);
    entries[GALAXY_INFO_STARS].font_size = FONT_SIZE_15;

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);

    for (int i = 0; i < GALAXY_INFO_COUNT; i++)
    {
        // Create a texture from the entry text
        SDL_Surface *text_surface = TTF_RenderText_Blended(fonts[entries[i].font_size], entries[i].text, colors[COLOR_WHITE_180]);
        SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
        entries[i].text_texture = text_texture;
        entries[i].texture_rect.w = text_surface->w;
        entries[i].texture_rect.h = text_surface->h;

        SDL_FreeSurface(text_surface);
    }

    // Name
    int name_height = 100;
    int entry_height = 30;
    int inner_padding = 40;
    entries[GALAXY_INFO_NAME].rect.w = width;
    entries[GALAXY_INFO_NAME].rect.h = name_height;
    entries[GALAXY_INFO_NAME].rect.x = camera->w - (width + padding);
    entries[GALAXY_INFO_NAME].rect.y = camera->h - (height + padding);

    SDL_RenderFillRect(renderer, &entries[GALAXY_INFO_NAME].rect);

    entries[GALAXY_INFO_NAME].texture_rect.x = entries[GALAXY_INFO_NAME].rect.x + inner_padding;
    entries[GALAXY_INFO_NAME].texture_rect.y = entries[GALAXY_INFO_NAME].rect.y + (entries[GALAXY_INFO_NAME].rect.h - entries[GALAXY_INFO_NAME].texture_rect.h) / 2;

    SDL_RenderCopy(renderer, entries[GALAXY_INFO_NAME].text_texture, NULL, &entries[GALAXY_INFO_NAME].texture_rect);

    // Type
    entries[GALAXY_INFO_TYPE].rect.w = width;
    entries[GALAXY_INFO_TYPE].rect.h = entry_height;
    entries[GALAXY_INFO_TYPE].rect.x = camera->w - (width + padding);
    entries[GALAXY_INFO_TYPE].rect.y = camera->h - (height + padding) + name_height;

    SDL_RenderFillRect(renderer, &entries[GALAXY_INFO_TYPE].rect);

    entries[GALAXY_INFO_TYPE].texture_rect.x = entries[GALAXY_INFO_TYPE].rect.x + inner_padding;
    entries[GALAXY_INFO_TYPE].texture_rect.y = entries[GALAXY_INFO_TYPE].rect.y + (entries[GALAXY_INFO_TYPE].rect.h - entries[GALAXY_INFO_TYPE].texture_rect.h) / 2;

    SDL_RenderCopy(renderer, entries[GALAXY_INFO_TYPE].text_texture, NULL, &entries[GALAXY_INFO_TYPE].texture_rect);

    // Set the position for the rest of the entries
    for (int i = 2; i < GALAXY_INFO_COUNT; i++)
    {
        entries[i].rect.w = width;
        entries[i].rect.h = entry_height;
        entries[i].rect.x = camera->w - (width + padding);
        entries[i].rect.y = camera->h - (height + padding) + name_height + (i - 1) * entry_height;

        SDL_RenderFillRect(renderer, &entries[i].rect);

        // Set the position of the text within the entry
        entries[i].texture_rect.x = entries[i].rect.x + inner_padding;
        entries[i].texture_rect.y = entries[i].rect.y + (entries[i].rect.h - entries[i].texture_rect.h) / 2;

        // Render the text texture onto the entry
        SDL_RenderCopy(renderer, entries[i].text_texture, NULL, &entries[i].texture_rect);
    }

    // Destroy the textures
    for (int i = 0; i < GALAXY_INFO_COUNT; i++)
    {
        SDL_DestroyTexture(entries[i].text_texture);
        entries[i].text_texture = NULL;
    }
}

/**
 * Check if a galaxy entry exists at the given position in the galaxies hash table.
 *
 * @param galaxies[] The hash table of galaxies to search in.
 * @param position The position to search for in the hash table.
 *
 * @return Whether or not a galaxy entry exists at the given position.
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

/**
 * Generates galaxies within a region of the universe, determined by offset.
 * If this is the first time calling the function, updates the navigation state
 * with the nearest section lines. Deletes galaxies that end up outside of the
 * region. Uses a local rng for random number generation.
 *
 * @param game_events A pointer to the game events struct.
 * @param nav_state A pointer to the navigation state struct.
 * @param offset A point in space around which to generate galaxies.
 *
 * @return void
 */
void galaxies_generate(GameEvents *game_events, NavigationState *nav_state, Point offset)
{
    // Keep track of current nearest section line position
    double bx = maths_get_nearest_section_line(offset.x, UNIVERSE_SECTION_SIZE);
    double by = maths_get_nearest_section_line(offset.y, UNIVERSE_SECTION_SIZE);

    // Check if this is the first time calling this function
    if (!game_events->start_galaxies_generation)
    {
        // Check whether nearest section lines have changed
        if (bx == nav_state->universe_cross_line.x && by == nav_state->universe_cross_line.y)
            return;

        // Keep track of new lines
        if (bx != nav_state->universe_cross_line.x)
            nav_state->universe_cross_line.x = bx;

        if (by != nav_state->universe_cross_line.y)
            nav_state->universe_cross_line.y = by;
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

            // Skip buffer_galaxy, otherwise we lose track of where we are
            if (!game_events->start_galaxies_generation)
            {
                if (maths_points_equal(position, nav_state->buffer_galaxy->position))
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
    game_events->start_galaxies_generation = false;
}

/**
 * Retrieve a Galaxy from a hash table of GalaxyEntry structures.
 *
 * @param galaxies[] An array of pointers to GalaxyEntry structures.
 * @param position The Point structure representing the position of the Galaxy.
 *
 * @return If the Galaxy exists in the hash table, return a pointer to the Galaxy. Otherwise, return NULL.
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

/**
 * Calculates the distance to the nearest galaxy center from a given position.
 * Uses a search algorithm that checks points in inner circumferences first and works
 * towards outward circumferences until a galaxy is found.
 *
 * @param position The point to calculate the distance from.
 *
 * @return The distance to the nearest galaxy center, or 7 * UNIVERSE_SECTION_SIZE if no galaxies are found.
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

/**
 * Finds the galaxy closest to the circumference of the circle surrounding the given point.
 * Excludes the current galaxy if exclude flag is set.
 *
 * @param nav_state A pointer to the current NavigationState.
 * @param position The point to find the closest galaxy circumference to.
 * @param exclude Flag to exclude the current galaxy.
 *
 * @return Pointer to the closest galaxy found or NULL if none found.
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
                if (maths_points_equal(entry->galaxy->position, nav_state->current_galaxy->position))
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

/**
 * Returns the size class of a galaxy based on its distance from the player position.
 *
 * @param distance The distance between the galaxy and the player position.
 *
 * @return The size class of the galaxy (GALAXY_1 to GALAXY_6).
 */
static unsigned short galaxies_size_class(float distance)
{
    if (distance < 3 * UNIVERSE_SECTION_SIZE)
        return GALAXY_1;
    else if (distance < 4 * UNIVERSE_SECTION_SIZE)
        return GALAXY_2;
    else if (distance < 5 * UNIVERSE_SECTION_SIZE)
        return GALAXY_3;
    else if (distance < 6 * UNIVERSE_SECTION_SIZE)
        return GALAXY_4;
    else if (distance < 7 * UNIVERSE_SECTION_SIZE)
        return GALAXY_5;
    else if (distance >= 7 * UNIVERSE_SECTION_SIZE)
        return GALAXY_6;
    else
        return GALAXY_1;
}

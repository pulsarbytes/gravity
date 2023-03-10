/*
 * stars.c
 */

#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include "../lib/pcg-c-basic-0.9/pcg_basic.h"

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"
#include "../include/stars.h"

// External variable definitions
extern TTF_Font *fonts[];
extern SDL_Renderer *renderer;
extern SDL_Color colors[];

// Static function prototypes
static void stars_add_entry(StarEntry *stars[], Point, Star *);
static void stars_cleanup_planets(CelestialBody *);
static Star *stars_create_star(const NavigationState *, Point, int preview, long double scale);
static void stars_delete_entry(StarEntry *stars[], Point);
static bool stars_entry_exists(StarEntry *stars[], Point);
static int stars_planet_size_class(float width);
static void stars_populate_body(CelestialBody *, Point, pcg32_random_t rng, long double scale);

/**
 * Adds a new star entry to the hash table of stars at the given position.
 *
 * @param stars An array of pointers to StarEntry structures, representing the hash table of stars.
 * @param position A Point structure representing the position of the star.
 * @param star A pointer to the Star structure to be added to the hash table.
 *
 * @return void
 */
static void stars_add_entry(StarEntry *stars[], Point position, Star *star)
{
    // Generate unique index for hash table
    uint64_t index = maths_hash_position_to_index(position, MAX_STARS, ENTITY_STAR);

    StarEntry *entry = (StarEntry *)malloc(sizeof(StarEntry));

    if (entry == NULL)
    {
        fprintf(stderr, "Error: Could not allocate memory for StarEntry.\n");
        return;
    }

    entry->x = position.x;
    entry->y = position.y;
    entry->star = star;
    entry->next = stars[index];
    stars[index] = entry;
}

/**
 * Cleans up the planets of a celestial body and destroys its texture.
 *
 * @param body A pointer to the celestial body to clean up.
 *
 * @return void
 */
static void stars_cleanup_planets(CelestialBody *body)
{
    int planets_size = sizeof(body->planets) / sizeof(body->planets[0]);
    int i;

    for (i = 0; i < planets_size && body->planets[i] != NULL; i++)
    {
        stars_cleanup_planets(body->planets[i]);
    }

    SDL_DestroyTexture(body->texture);
    body->texture = NULL;
}

/**
 * Clears the hash table of stars.
 *
 * @param stars An array of StarEntry pointers.
 *
 * @return void
 */
void stars_clear_table(StarEntry *stars[])
{
    // Loop through hash table
    for (int s = 0; s < MAX_STARS; s++)
    {
        StarEntry *entry = stars[s];

        while (entry != NULL)
        {
            Point position = {.x = entry->x, .y = entry->y};
            stars_delete_entry(stars, position);
            entry = entry->next;
        }
    }
}

/**
 * Creates a new Star object with given parameters.
 *
 * @param nav_state A pointer to NavigationState struct.
 * @param position A point struct representing the position of the star.
 * @param preview An integer representing whether or not the star is a preview.
 * @param scale A long double representing the scale factor of the star's position.
 *
 * @return Returns a pointer to a new Star object.
 */
static Star *stars_create_star(const NavigationState *nav_state, Point position, int preview, long double scale)
{
    // Find distance to nearest star
    double distance = stars_nearest_center_distance(position, nav_state->current_galaxy, nav_state->initseq, GALAXY_DENSITY);

    // Get star class
    unsigned short class = stars_size_class(distance);

    // Use a local rng
    pcg32_random_t rng;

    // Create rng seed by combining x,y values
    uint64_t seed = maths_hash_position_to_uint64(position);

    // Seed with a fixed constant
    pcg32_srandom_r(&rng, seed, nav_state->initseq);

    float radius;
    unsigned short color_code;

    switch (class)
    {
    case STAR_CLASS_1:
        radius = abs(pcg32_random_r(&rng)) % STAR_CLASS_1_RADIUS_MAX + STAR_CLASS_1_RADIUS_MIN;
        color_code = COLOR_LIGHT_RED_255;
        break;
    case STAR_CLASS_2:
        radius = abs(pcg32_random_r(&rng)) % STAR_CLASS_2_RADIUS_MAX + STAR_CLASS_2_RADIUS_MIN;
        color_code = COLOR_LIGHT_ORANGE_255;
        break;
    case STAR_CLASS_3:
        radius = abs(pcg32_random_r(&rng)) % STAR_CLASS_3_RADIUS_MAX + STAR_CLASS_3_RADIUS_MIN;
        color_code = COLOR_PALE_YELLOW_255;
        break;
    case STAR_CLASS_4:
        radius = abs(pcg32_random_r(&rng)) % STAR_CLASS_4_RADIUS_MAX + STAR_CLASS_4_RADIUS_MIN;
        color_code = COLOR_LIGHT_GREEN_255;
        break;
    case STAR_CLASS_5:
        radius = abs(pcg32_random_r(&rng)) % STAR_CLASS_5_RADIUS_MAX + STAR_CLASS_5_RADIUS_MIN;
        color_code = COLOR_LIGHT_BLUE_255;
        break;
    case STAR_CLASS_6:
        radius = abs(pcg32_random_r(&rng)) % STAR_CLASS_6_RADIUS_MAX + STAR_CLASS_6_RADIUS_MIN;
        color_code = COLOR_LAVENDER_255;
        break;
    default:
        radius = abs(pcg32_random_r(&rng)) % STAR_CLASS_1_RADIUS_MAX + STAR_CLASS_1_RADIUS_MIN;
        color_code = COLOR_LIGHT_RED_255;
        break;
    }

    // Allocate memory for Star
    Star *star = (Star *)malloc(sizeof(Star));

    if (star == NULL)
    {
        fprintf(stderr, "Error: Could not allocate memory for Star.\n");
        return NULL;
    }

    // Generate unique star position hash
    uint64_t position_hash = maths_hash_position_to_uint64(position);

    star->initialized = 0;
    sprintf(star->name, "%s-%lu", "S", position_hash);
    star->image = "../assets/images/sol.png";
    star->class = stars_size_class(distance);
    star->radius = radius;
    star->cutoff = GALAXY_SECTION_SIZE * class / 2;
    star->position.x = position.x;
    star->position.y = position.y;
    star->vx = 0.0;
    star->vy = 0.0;
    star->dx = 0.0;
    star->dy = 0.0;

    if (preview)
    {
        star->texture = NULL;
    }
    else
    {
        SDL_Surface *surface = IMG_Load(star->image);
        star->texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }

    star->rect.x = (star->position.x - star->radius) * scale;
    star->rect.y = (star->position.y - star->radius) * scale;
    star->rect.w = 2 * star->radius * scale;
    star->rect.h = 2 * star->radius * scale;
    star->color = colors[color_code];
    star->num_planets = 0;
    star->planets[0] = NULL;
    star->parent = NULL;
    star->level = LEVEL_STAR;
    star->is_selected = false;

    return star;
}

/**
 * Deletes the entry for a star at the given position from the hash table of stars.
 *
 * @param stars An array of StarEntry pointers representing the hash table of stars.
 * @param position The position of the star to delete.
 *
 * @return void
 */
static void stars_delete_entry(StarEntry *stars[], Point position)
{
    // Generate unique index for hash table
    uint64_t index = maths_hash_position_to_index(position, MAX_STARS, ENTITY_STAR);

    StarEntry *previous = NULL;
    StarEntry *entry = stars[index];

    while (entry != NULL)
    {
        if (entry->x == position.x && entry->y == position.y)
        {
            // Clean up planets
            if (entry->star != NULL && entry->star->planets[0] != NULL)
                stars_cleanup_planets(entry->star);

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

/**
 * Delete all stars outside a given region.
 *
 * @param stars An array of pointers to StarEntry structures.
 * @param bx The x coordinate of the center of the region.
 * @param by The y coordinate of the center of the region.
 * @param region_size The size of the region (in number of sections).
 *
 * @return void
 */
void stars_delete_outside_region(StarEntry *stars[], double bx, double by, int region_size)
{
    for (int s = 0; s < MAX_STARS; s++)
    {
        StarEntry *entry = stars[s];

        while (entry != NULL)
        {
            Point position = {.x = entry->x, .y = entry->y};

            // Get distance from center of region
            double distance = maths_distance_between_points(position.x, position.y, bx, by);
            double region_radius = sqrt((double)2 * ((region_size + 1) / 2) * GALAXY_SECTION_SIZE * ((region_size + 1) / 2) * GALAXY_SECTION_SIZE);

            // If star outside region, delete it
            if (distance >= region_radius)
                stars_delete_entry(stars, position);

            entry = entry->next;
        }
    }
}

/**
 * Draws a box on the screen that displays information about a star.
 *
 * @param galaxy A pointer to the Star for which to display info.
 * @param camera A pointer to the current Camera object.
 *
 * @return void
 */
void stars_draw_info_box(const Star *star, const Camera *camera)
{
    // Draw background box
    SDL_SetRenderDrawColor(renderer, 12, 12, 12, 255);
    int width = 360;
    int padding = 20;
    int height = 290;

    SDL_Rect info_box_rect;
    info_box_rect.x = camera->w - (width + padding);
    info_box_rect.y = padding;
    info_box_rect.w = width;
    info_box_rect.h = height;

    SDL_RenderFillRect(renderer, &info_box_rect);

    // Create info array
    InfoBoxEntry entries[STAR_INFO_COUNT];

    char star_name[128];
    strcpy(star_name, star->name);
    sprintf(entries[STAR_INFO_NAME].text, "%s", star_name);
    entries[STAR_INFO_NAME].font_size = FONT_SIZE_22;

    char entity_type[32];
    strcpy(entity_type, "STAR");
    sprintf(entries[STAR_INFO_TYPE].text, "%s", entity_type);
    entries[STAR_INFO_TYPE].font_size = FONT_SIZE_15;

    char position_x_text[32];
    utils_add_thousand_separators((int)star->position.x, position_x_text, sizeof(position_x_text));
    sprintf(entries[STAR_INFO_X].text, "Position X: %*s%s", 2, "", position_x_text);
    entries[STAR_INFO_X].font_size = FONT_SIZE_15;

    char position_y_text[32];
    utils_add_thousand_separators((int)star->position.y, position_y_text, sizeof(position_y_text));
    sprintf(entries[STAR_INFO_Y].text, "Position Y: %*s%s", 2, "", position_y_text);
    entries[STAR_INFO_Y].font_size = FONT_SIZE_15;

    char class_text[32];
    sprintf(class_text, "%d", star->class);
    sprintf(entries[STAR_INFO_CLASS].text, "Class: %*s%s", 7, "", class_text);
    entries[STAR_INFO_CLASS].font_size = FONT_SIZE_15;

    char radius_text[32];
    utils_add_thousand_separators((int)star->radius, radius_text, sizeof(radius_text));
    sprintf(entries[STAR_INFO_RADIUS].text, "Radius: %*s%s", 6, "", radius_text);
    entries[STAR_INFO_RADIUS].font_size = FONT_SIZE_15;

    char planets_text[32];
    sprintf(planets_text, "%d", star->num_planets);
    sprintf(entries[STAR_INFO_PLANETS].text, "Planets: %*s%s", 5, "", planets_text);
    entries[STAR_INFO_PLANETS].font_size = FONT_SIZE_15;

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);

    for (int i = 0; i < STAR_INFO_COUNT; i++)
    {
        // Create a texture from the entry text
        SDL_Surface *text_surface = TTF_RenderText_Solid(fonts[entries[i].font_size], entries[i].text, colors[COLOR_WHITE_180]);
        SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
        entries[i].text_texture = text_texture;
        entries[i].texture_rect.w = text_surface->w;
        entries[i].texture_rect.h = text_surface->h;

        SDL_FreeSurface(text_surface);
    }

    // Name
    int name_height = 80;
    int entry_height = 30;
    int inner_padding = 40;
    entries[STAR_INFO_NAME].rect.w = width;
    entries[STAR_INFO_NAME].rect.h = name_height;
    entries[STAR_INFO_NAME].rect.x = camera->w - (width + padding);
    entries[STAR_INFO_NAME].rect.y = padding;

    SDL_RenderFillRect(renderer, &entries[STAR_INFO_NAME].rect);

    entries[STAR_INFO_NAME].texture_rect.x = entries[STAR_INFO_NAME].rect.x + inner_padding;
    entries[STAR_INFO_NAME].texture_rect.y = entries[STAR_INFO_NAME].rect.y + (entries[STAR_INFO_NAME].rect.h - entries[STAR_INFO_NAME].texture_rect.h) / 2;

    SDL_RenderCopy(renderer, entries[STAR_INFO_NAME].text_texture, NULL, &entries[STAR_INFO_NAME].texture_rect);

    // Type
    entries[STAR_INFO_TYPE].rect.w = width;
    entries[STAR_INFO_TYPE].rect.h = entry_height;
    entries[STAR_INFO_TYPE].rect.x = camera->w - (width + padding);
    entries[STAR_INFO_TYPE].rect.y = padding + name_height;

    SDL_RenderFillRect(renderer, &entries[STAR_INFO_TYPE].rect);

    entries[STAR_INFO_TYPE].texture_rect.x = entries[STAR_INFO_TYPE].rect.x + inner_padding;
    entries[STAR_INFO_TYPE].texture_rect.y = entries[STAR_INFO_TYPE].rect.y + (entries[STAR_INFO_TYPE].rect.h - entries[STAR_INFO_TYPE].texture_rect.h) / 2;

    SDL_RenderCopy(renderer, entries[STAR_INFO_TYPE].text_texture, NULL, &entries[STAR_INFO_TYPE].texture_rect);

    // Set the position for the rest of the entries
    for (int i = 2; i < STAR_INFO_COUNT; i++)
    {
        entries[i].rect.w = width;
        entries[i].rect.h = entry_height;
        entries[i].rect.x = camera->w - (width + padding);
        entries[i].rect.y = padding + name_height + (i - 1) * entry_height;

        SDL_RenderFillRect(renderer, &entries[i].rect);

        // Set the position of the text within the entry
        entries[i].texture_rect.x = entries[i].rect.x + inner_padding;
        entries[i].texture_rect.y = entries[i].rect.y + (entries[i].rect.h - entries[i].texture_rect.h) / 2;

        // Render the text texture onto the entry
        SDL_RenderCopy(renderer, entries[i].text_texture, NULL, &entries[i].texture_rect);
    }

    // Destroy the textures
    for (int i = 0; i < STAR_INFO_COUNT; i++)
    {
        SDL_DestroyTexture(entries[i].text_texture);
        entries[i].text_texture = NULL;
    }
}

/**
 * Draws a box on the screen that displays information about the planets of a star.
 *
 * @param galaxy A pointer to the Star for which to display info.
 * @param camera A pointer to the current Camera object.
 *
 * @return void
 */
void stars_draw_planets_info_box(const Star *star, const Camera *camera)
{
    // Draw background box
    SDL_SetRenderDrawColor(renderer, 15, 15, 15, 255);
    int width = 360;
    int padding = 20;
    int inner_padding = 40;
    int info_box_height = 290;
    int height = camera->h - 2 * padding - info_box_height;

    SDL_Rect info_box_rect;
    info_box_rect.x = camera->w - (width + padding);
    info_box_rect.y = padding + info_box_height;
    info_box_rect.w = width;
    info_box_rect.h = height;

    SDL_RenderFillRect(renderer, &info_box_rect);

    // Calculate scaling factors from star class
    int planets_scaling_factor;
    int moon_scaling_factor;

    switch (star->class)
    {
    case STAR_CLASS_1:
        planets_scaling_factor = 4;
        moon_scaling_factor = 4;
        break;
    case STAR_CLASS_2:
        planets_scaling_factor = 8;
        moon_scaling_factor = 4;
        break;
    case STAR_CLASS_3:
        planets_scaling_factor = 12;
        moon_scaling_factor = 4;
        break;
    case STAR_CLASS_4:
        planets_scaling_factor = 16;
        moon_scaling_factor = 8;
        break;
    case STAR_CLASS_5:
        planets_scaling_factor = 16;
        moon_scaling_factor = 8;
        break;
    case STAR_CLASS_6:
        planets_scaling_factor = 20;
        moon_scaling_factor = 8;
        break;
    default:
        planets_scaling_factor = 4;
        moon_scaling_factor = 4;
        break;
    }

    // Draw star
    int x = camera->w - (width - 1.4 * inner_padding);
    int y_star = padding + info_box_height + inner_padding;
    gfx_draw_fill_circle(renderer, x, y_star, inner_padding / 2, star->color);

    // Draw line
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 30);

    int y1_line = y_star + inner_padding / 2;
    int y2_line = camera->h - padding;
    int line_height = y2_line - y1_line;
    SDL_RenderDrawLine(renderer, x, y1_line, x, y2_line);

    // Draw planets
    // Line represents star cutoff distance
    float y_so_far = y1_line;

    for (int i = 0; i < MAX_PLANETS && star->planets[i] != NULL; i++)
    {
        float planet_orbit = line_height * star->planets[i]->orbit_radius / star->cutoff;
        float planet_diameter = star->planets[i]->radius / planets_scaling_factor;
        y_so_far += planet_orbit + planet_diameter;

        gfx_draw_fill_circle(renderer, x, y_so_far - planet_diameter / 2, star->planets[i]->radius / planets_scaling_factor, star->planets[i]->color);

        // Draw moons
        if (star->planets[i]->num_planets > 0)
        {
            // Line represents planet cutoff distance
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 20);
            int x1_line = x + star->planets[i]->radius / planets_scaling_factor;
            int x2_line = camera->w - padding - 2 * inner_padding;
            int line_width = x2_line - x1_line;
            SDL_RenderDrawLine(renderer, x1_line, y_so_far - planet_diameter / 2, x2_line, y_so_far - planet_diameter / 2);
            float x_so_far = x1_line;

            for (int j = 0; j < MAX_MOONS && star->planets[i]->planets[j] != NULL; j++)
            {
                float moon_orbit = line_width * star->planets[i]->planets[j]->orbit_radius / star->planets[i]->cutoff;
                float moon_diameter = star->planets[i]->planets[j]->radius / moon_scaling_factor;
                x_so_far += moon_orbit + moon_diameter;
                gfx_draw_fill_circle(renderer, x_so_far, y_so_far - planet_diameter / 2, star->planets[i]->planets[j]->radius / moon_scaling_factor, star->planets[i]->planets[j]->color);
            }
        }
    }
}

/**
 * Draws a celestial body system, including its planets and orbits, onto the game's renderer.
 *
 * @param game_state The current game state.
 * @param input_state The current input state.
 * @param nav_state The current navigation state.
 * @param body The celestial body to draw.
 * @param camera The current camera state.
 *
 * @return void
 */
void stars_draw_star_system(GameState *game_state, const InputState *input_state, NavigationState *nav_state, CelestialBody *body, const Camera *camera)
{
    double distance;
    Point position;

    if (game_state->state == NAVIGATE)
    {
        position.x = nav_state->navigate_offset.x;
        position.y = nav_state->navigate_offset.y;
    }
    else if (game_state->state == MAP)
    {
        position.x = nav_state->map_offset.x;
        position.y = nav_state->map_offset.y;
    }

    // Draw planets
    if (body->level != LEVEL_STAR)
    {
        body->is_selected = body->parent->is_selected ? true : false;

        float orbit_opacity;

        if (game_state->state == NAVIGATE)
        {
            // Find distance from parent
            double delta_x = body->parent->position.x - body->position.x;
            double delta_y = body->parent->position.y - body->position.y;
            distance = sqrt(delta_x * delta_x + delta_y * delta_y);

            orbit_opacity = 45;
        }
        else if (game_state->state == MAP)
        {
            // Find distance from parent
            distance = maths_distance_between_points(body->parent->position.x, body->parent->position.y, body->position.x, body->position.y);

            orbit_opacity = 32;
        }

        // Draw orbit
        if (input_state->orbits_on)
        {
            int radius = distance * game_state->game_scale;
            int _x = (body->parent->position.x - camera->x) * game_state->game_scale;
            int _y = (body->parent->position.y - camera->y) * game_state->game_scale;
            SDL_Color orbit_color = {
                colors[COLOR_WHITE_255].r,
                colors[COLOR_WHITE_255].g,
                colors[COLOR_WHITE_255].b,
                orbit_opacity};

            gfx_draw_circle(renderer, camera, _x, _y, radius, orbit_color);
        }

        // Draw moons
        int max_planets = MAX_MOONS;

        for (int i = 0; i < max_planets && body->planets[i] != NULL; i++)
        {
            stars_draw_star_system(game_state, input_state, nav_state, body->planets[i], camera);
        }
    }
    else if (body->level == LEVEL_STAR)
    {
        if (game_state->state == MAP)
        {
            // Get relative position of star in game_scale
            int radius = (body->class * GALAXY_SECTION_SIZE / 2) * game_state->game_scale;
            int x = (body->position.x - camera->x) * game_state->game_scale;
            int y = (body->position.y - camera->y) * game_state->game_scale;
            Point star_position = {.x = x, .y = y};

            bool star_is_selected = strcmp(nav_state->current_star->name, body->name) == 0 && nav_state->current_star->is_selected;
            bool star_is_hovered = strcmp(nav_state->current_star->name, body->name) == 0 && input_state->is_hovering_star;

            if (star_is_selected || star_is_hovered ||
                (maths_is_point_in_circle(input_state->mouse_position, star_position, radius) &&
                 gfx_is_object_in_camera(camera, body->position.x, body->position.y, body->radius, game_state->game_scale)))
            {
                // Create system
                if (!body->initialized)
                {
                    Point star_position = {.x = body->position.x, .y = body->position.y};

                    // Use a local rng
                    pcg32_random_t rng;

                    // Create rng seed by combining x,y values
                    uint64_t seed = maths_hash_position_to_uint64(star_position);

                    // Seed with a fixed constant
                    pcg32_srandom_r(&rng, seed, nav_state->initseq);

                    stars_populate_body(body, star_position, rng, game_state->game_scale);
                }

                // Draw planets
                int max_planets = MAX_PLANETS;

                for (int i = 0; i < max_planets && body->planets[i] != NULL; i++)
                {
                    stars_draw_star_system(game_state, input_state, nav_state, body->planets[i], camera);
                }

                // Update current_star
                if (strcmp(nav_state->current_star->name, body->name) != 0)
                    memcpy(nav_state->current_star, body, sizeof(Star));

                if (input_state->orbits_on)
                {
                    unsigned short color_code;

                    if (star_is_selected)
                        color_code = COLOR_CYAN_40;
                    else
                        color_code = COLOR_MAGENTA_40;

                    // Draw cutoff area circles
                    gfx_draw_circle(renderer, camera, x, y, radius - 1, colors[color_code]);
                    gfx_draw_circle(renderer, camera, x, y, radius - 2, colors[color_code]);
                    gfx_draw_circle(renderer, camera, x, y, radius - 3, colors[color_code]);
                }
            }
        }
        else if (game_state->state == NAVIGATE)
        {
            // Get star distance from position
            distance = maths_distance_between_points(body->position.x, body->position.y, position.x, position.y);

            if (distance < body->cutoff)
            {
                // Draw planets
                int max_planets = MAX_PLANETS;

                for (int i = 0; i < max_planets && body->planets[i] != NULL; i++)
                {
                    stars_draw_star_system(game_state, input_state, nav_state, body->planets[i], camera);
                }

                // Update buffer_star
                if (strcmp(nav_state->buffer_star->name, body->name) != 0)
                    memcpy(nav_state->buffer_star, body, sizeof(Star));

                // Update current_star
                if (strcmp(nav_state->current_star->name, body->name) != 0)
                    memcpy(nav_state->current_star, body, sizeof(Star));
            }

            // Draw cutoff area circle
            if (input_state->orbits_on && distance < 2 * body->cutoff)
            {
                int cutoff = body->cutoff * game_state->game_scale;
                int x = (body->position.x - camera->x) * game_state->game_scale;
                int y = (body->position.y - camera->y) * game_state->game_scale;

                bool star_is_selected = strcmp(nav_state->current_star->name, body->name) == 0 && nav_state->current_star->is_selected;
                unsigned short color_code;

                if (star_is_selected)
                    color_code = COLOR_CYAN_70;
                else
                    color_code = COLOR_MAGENTA_70;

                gfx_draw_circle(renderer, camera, x, y, cutoff, colors[color_code]);
            }
        }
    }

    // Draw body
    if (gfx_is_object_in_camera(camera, body->position.x, body->position.y, body->radius, game_state->game_scale))
    {
        body->rect.x = (int)(body->position.x - body->radius - camera->x) * game_state->game_scale;
        body->rect.y = (int)(body->position.y - body->radius - camera->y) * game_state->game_scale;

        SDL_RenderCopy(renderer, body->texture, NULL, &body->rect);
    }
    // Draw body projection
    else if (PROJECT_BODIES_ON)
    {
        if (body->level == LEVEL_MOON)
        {
            distance = maths_distance_between_points(body->parent->position.x, body->parent->position.y, position.x, position.y);

            if (distance < 2 * body->parent->cutoff)
                gfx_project_body_on_edge(game_state, nav_state, body, camera);
        }
        else
            gfx_project_body_on_edge(game_state, nav_state, body, camera);
    }
}

/**
 * Checks whether a given position exists in the hash table of StarEntry objects.
 *
 * @param stars An array of StarEntry pointers representing the hash table.
 * @param position A Point struct representing the position to check for.
 *
 * @return True if the given position exists in the hash table, false otherwise.
 */
static bool stars_entry_exists(StarEntry *stars[], Point position)
{
    // Generate unique index for hash table
    uint64_t index = maths_hash_position_to_index(position, MAX_STARS, ENTITY_STAR);

    StarEntry *entry = stars[index];

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

/**
 * Creates a list of randomly positioned stars within a defined rectangular area.
 *
 * @param game_state A pointer to the current GameState struct.
 * @param game_events A pointer to the current GameEvents struct.
 * @param nav_state A pointer to the current NavigationState struct.
 * @param bstars A pointer to an array of Bstar structs for storing the generated background stars.
 * @param ship A pointer to the current Ship struct.
 */
void stars_generate(GameState *game_state, GameEvents *game_events, NavigationState *nav_state, Bstar *bstars, Ship *ship)
{
    Point offset;

    if (game_state->state == NAVIGATE)
    {
        offset.x = nav_state->navigate_offset.x;
        offset.y = nav_state->navigate_offset.y;
    }
    else if (game_state->state == MAP)
    {
        offset.x = nav_state->map_offset.x;
        offset.y = nav_state->map_offset.y;
    }

    // Keep track of current nearest section lines position
    double bx = maths_get_nearest_section_line(offset.x, GALAXY_SECTION_SIZE);
    double by = maths_get_nearest_section_line(offset.y, GALAXY_SECTION_SIZE);

    // Check if this is the first time calling this function
    if (!game_events->start_stars_generation)
    {
        // Check whether nearest section lines have changed
        if (bx == nav_state->cross_line.x && by == nav_state->cross_line.y)
            return;

        // Keep track of new lines
        if (bx != nav_state->cross_line.x)
            nav_state->cross_line.x = bx;

        if (by != nav_state->cross_line.y)
            nav_state->cross_line.y = by;
    }

    // If exited galaxy, check for closest galaxy, including current galaxy
    if (sqrt(offset.x * offset.x + offset.y * offset.y) > nav_state->current_galaxy->cutoff * GALAXY_SCALE)
    {
        game_events->has_exited_galaxy = true;

        // Convert offset to universe position
        Point universe_position;
        universe_position.x = nav_state->current_galaxy->position.x + offset.x / GALAXY_SCALE;
        universe_position.y = nav_state->current_galaxy->position.y + offset.y / GALAXY_SCALE;

        // Wrap around universe position (rectangle defines boundaries)
        if (universe_position.x > UNIVERSE_X_LIMIT)
            universe_position.x -= UNIVERSE_X_LIMIT * 2;
        else if (universe_position.x < -UNIVERSE_X_LIMIT)
            universe_position.x += UNIVERSE_X_LIMIT * 2;

        if (universe_position.y > UNIVERSE_Y_LIMIT)
            universe_position.y -= UNIVERSE_Y_LIMIT * 2;
        else if (universe_position.y < -UNIVERSE_Y_LIMIT)
            universe_position.y += UNIVERSE_Y_LIMIT * 2;

        // Convert to cross section offset to query for new galaxies
        Point cross_section_offset;
        cross_section_offset.x = maths_get_nearest_section_line(universe_position.x, UNIVERSE_SECTION_SIZE);
        cross_section_offset.y = maths_get_nearest_section_line(universe_position.y, UNIVERSE_SECTION_SIZE);
        galaxies_generate(game_events, nav_state, cross_section_offset);

        // Search for nearest galaxy to universe_position, including current galaxy
        Galaxy *next_galaxy = galaxies_nearest_circumference(nav_state, universe_position, false);

        // Found a new galaxy
        if (next_galaxy != NULL && !maths_points_equal(next_galaxy->position, nav_state->current_galaxy->position))
        {
            game_events->found_galaxy = true;

            // Update previous_galaxy
            memcpy(nav_state->previous_galaxy, nav_state->current_galaxy, sizeof(Galaxy));

            // Update current_galaxy
            memcpy(nav_state->current_galaxy, next_galaxy, sizeof(Galaxy));

            // Get current position relative to new galaxy
            double angle = atan2(universe_position.y - next_galaxy->position.y, universe_position.x - next_galaxy->position.x);
            double d = maths_distance_between_points(universe_position.x, universe_position.y, next_galaxy->position.x, next_galaxy->position.y);
            double px = d * cos(angle) * GALAXY_SCALE;
            double py = d * sin(angle) * GALAXY_SCALE;

            // Update galaxy_offset
            nav_state->galaxy_offset.current_x = next_galaxy->position.x;
            nav_state->galaxy_offset.current_y = next_galaxy->position.y;

            if (game_state->state == NAVIGATE)
            {
                // Update ship position
                ship->position.x = px;
                ship->position.y = py;

                // Update offset
                nav_state->navigate_offset.x = px;
                nav_state->navigate_offset.y = py;

                // Permanently in a new galaxy, update buffer in galaxy_offset
                nav_state->galaxy_offset.buffer_x = nav_state->galaxy_offset.current_x;
                nav_state->galaxy_offset.buffer_y = nav_state->galaxy_offset.current_y;

                // Update buffer_galaxy
                memcpy(nav_state->buffer_galaxy, nav_state->current_galaxy, sizeof(Galaxy));

                // Create new background stars
                game_events->generate_bstars = true;
            }
            else if (game_state->state == MAP)
            {
                // Update offset
                nav_state->map_offset.x = px;
                nav_state->map_offset.y = py;

                // Update ship position so that it always points to original location
                // First find absolute position for original ship position in universe scale
                double src_ship_position_x = nav_state->galaxy_offset.buffer_x + ship->previous_position.x / GALAXY_SCALE;
                double src_ship_position_y = nav_state->galaxy_offset.buffer_y + ship->previous_position.y / GALAXY_SCALE;
                // Then set new galaxy as center
                double src_ship_distance_x = src_ship_position_x - next_galaxy->position.x;
                double src_ship_distance_y = src_ship_position_y - next_galaxy->position.y;
                // Finally convert position to galaxy scale and update ship position
                double dest_ship_position_x = src_ship_distance_x * GALAXY_SCALE;
                double dest_ship_position_y = src_ship_distance_y * GALAXY_SCALE;
                ship->position.x = dest_ship_position_x;
                ship->position.y = dest_ship_position_y;
            }

            // Delete stars from previous galaxy
            stars_clear_table(nav_state->stars);

            return;
        }
    }
    else
        game_events->has_exited_galaxy = false;

    // Define a region of galaxy_region_size * galaxy_region_size
    // bx,by are at the center of this area
    double ix, iy;
    double left_boundary = bx - ((game_state->galaxy_region_size / 2) * GALAXY_SECTION_SIZE);
    double right_boundary = bx + ((game_state->galaxy_region_size / 2) * GALAXY_SECTION_SIZE);
    double top_boundary = by - ((game_state->galaxy_region_size / 2) * GALAXY_SECTION_SIZE);
    double bottom_boundary = by + ((game_state->galaxy_region_size / 2) * GALAXY_SECTION_SIZE);

    // Add a buffer zone of <galaxy_region_size> sections beyond galaxy radius
    int radius_plus_buffer = (nav_state->current_galaxy->radius * GALAXY_SCALE) + game_state->galaxy_region_size * GALAXY_SECTION_SIZE;
    int in_horizontal_bounds = left_boundary > -radius_plus_buffer && right_boundary < radius_plus_buffer;
    int in_vertical_bounds = top_boundary > -radius_plus_buffer && bottom_boundary < radius_plus_buffer;

    // Use a local rng
    pcg32_random_t rng;

    // Density scaling parameter
    double a = nav_state->current_galaxy->radius * GALAXY_SCALE / 2.0f;

    // Set galaxy hash as initseq
    nav_state->initseq = maths_hash_position_to_uint64_2(nav_state->current_galaxy->position);

    for (ix = left_boundary; ix < right_boundary && in_horizontal_bounds; ix += GALAXY_SECTION_SIZE)
    {
        for (iy = top_boundary; iy < bottom_boundary && in_vertical_bounds; iy += GALAXY_SECTION_SIZE)
        {
            // Check that point is within galaxy radius
            double distance_from_center = sqrt(ix * ix + iy * iy);

            if (distance_from_center > (nav_state->current_galaxy->radius * GALAXY_SCALE))
                continue;

            // Create rng seed by combining x,y values
            Point position = {.x = ix, .y = iy};
            uint64_t seed = maths_hash_position_to_uint64(position);

            // Seed with a fixed constant
            pcg32_srandom_r(&rng, seed, nav_state->initseq);

            // Calculate density based on distance from center
            double density = (GALAXY_DENSITY / pow((distance_from_center / a + 1), 6));

            int has_star = abs(pcg32_random_r(&rng)) % 1000 < density;

            if (has_star)
            {
                // Check whether star exists in hash table
                if (stars_entry_exists(nav_state->stars, position))
                    continue;
                else
                {
                    // Create star
                    Star *star = stars_create_star(nav_state, position, false, game_state->game_scale);

                    // Add star to hash table
                    stars_add_entry(nav_state->stars, position, star);
                }
            }
        }
    }

    // Delete stars that end up outside the region
    stars_delete_outside_region(nav_state->stars, bx, by, game_state->galaxy_region_size);

    // First star generation complete
    game_events->start_stars_generation = false;
}

/**
 * Generates a preview of the stars within the current section of the galaxy.
 *
 * @param nav_state A pointer to the current NavigationState struct.
 * @param camera A pointer to the current Camera struct.
 * @param cross_point A pointer to a Point struct representing the current position of the cross point.
 * @param zoom_preview An int representing the level of zoom for the preview.
 * @param scale A long double representing the current scale of the galaxy.
 *
 * @return void
 */
void stars_generate_preview(NavigationState *nav_state, const Camera *camera, Point *cross_point, int zoom_preview, long double scale)
{
    // Check how many sections fit in camera
    double section_size_scaled = GALAXY_SECTION_SIZE * scale;
    int sections_in_camera_x = (int)(camera->w / section_size_scaled);
    int sections_in_camera_y = (int)(camera->h / section_size_scaled);

    // Scale section_size with game_scale
    int section_size = GALAXY_SECTION_SIZE;
    const double epsilon = ZOOM_EPSILON / (10 * GALAXY_SCALE);
    int num_sections = 16;

    // Scale num_sections with galaxy class
    switch (nav_state->current_galaxy->class)
    {
    case 1:
        if (scale <= 0.0001 + epsilon)
            num_sections = 4;
        else if (scale <= 0.0004 + epsilon)
            num_sections = 2;
        else
            num_sections = 1;
        break;
    case 2:
        if (scale <= 0.00001 + epsilon)
            num_sections = 32;
        else if (scale <= 0.00004 + epsilon)
            num_sections = 12;
        else if (scale <= 0.00007 + epsilon)
            num_sections = 8;
        else if (scale <= 0.0001 + epsilon)
            num_sections = 4;
        else if (scale <= 0.0004 + epsilon)
            num_sections = 2;
        else
            num_sections = 1;
        break;
    case 3:
    case 4:
        if (scale <= 0.00001 + epsilon)
            num_sections = 24;
        else if (scale <= 0.00004 + epsilon)
            num_sections = 16;
        else if (scale <= 0.00007 + epsilon)
            num_sections = 8;
        else if (scale <= 0.0001 + epsilon)
            num_sections = 4;
        else if (scale <= 0.0004 + epsilon)
            num_sections = 2;
        else
            num_sections = 1;
        break;
    case 5:
    case 6:
        if (scale <= 0.00001 + epsilon)
            num_sections = 32;
        else if (scale <= 0.00004 + epsilon)
            num_sections = 16;
        else if (scale <= 0.00007 + epsilon)
            num_sections = 12;
        else if (scale <= 0.0001 + epsilon)
            num_sections = 6;
        else if (scale <= 0.0004 + epsilon)
            num_sections = 2;
        else
            num_sections = 1;
        break;
    }

    // Keep track of current nearest section line position
    section_size = num_sections * GALAXY_SECTION_SIZE;
    double bx = maths_get_nearest_section_line(nav_state->map_offset.x, section_size);
    double by = maths_get_nearest_section_line(nav_state->map_offset.y, section_size);

    // Check whether nearest section lines have changed
    if ((int)bx == (int)cross_point->x && (int)by == (int)cross_point->y)
        return;

    // Keep track of new lines
    if ((int)bx != (int)cross_point->x)
        cross_point->x = (int)bx;

    if ((int)by != (int)cross_point->y)
        cross_point->y = (int)by;

    // half_sections may lose precision due to int conversion.
    int half_sections_x = (int)(sections_in_camera_x / 2);
    int half_sections_y = (int)(sections_in_camera_y / 2);

    // Make sure that half_sections can be divided by <num_sections>
    while (half_sections_x % num_sections != 0)
        half_sections_x += 1;

    while (half_sections_y % num_sections != 0)
        half_sections_y += 1;

    double ix, iy;
    double left_boundary = bx - (half_sections_x * GALAXY_SECTION_SIZE);
    double right_boundary = bx + (half_sections_x * GALAXY_SECTION_SIZE);
    double top_boundary = by - (half_sections_y * GALAXY_SECTION_SIZE);
    double bottom_boundary = by + (half_sections_y * GALAXY_SECTION_SIZE);

    // Store previous boundaries
    static Point boundaries_minus;
    static Point boundaries_plus;
    static int initialized = false;

    // Define rect of previous boundaries
    Point rect[4];

    if (initialized)
    {
        rect[0].x = boundaries_minus.x;
        rect[0].y = boundaries_plus.y;

        rect[1].x = boundaries_plus.x;
        rect[1].y = boundaries_plus.y;

        rect[2].x = boundaries_plus.x;
        rect[2].y = boundaries_minus.y;

        rect[3].x = boundaries_minus.x;
        rect[3].y = boundaries_minus.y;
    }

    // Use a local rng
    pcg32_random_t rng;

    // Density scaling parameter
    double a = nav_state->current_galaxy->radius * GALAXY_SCALE / 2.0f;

    // Set galaxy hash as initseq
    nav_state->initseq = maths_hash_position_to_uint64_2(nav_state->current_galaxy->position);

    for (ix = left_boundary; ix < right_boundary; ix += section_size)
    {
        for (iy = top_boundary; iy < bottom_boundary; iy += section_size)
        {
            Point position = {.x = ix, .y = iy};

            // If this point has been checked in previous function call,
            // check that point is not within previous boundaries
            if (initialized && !zoom_preview && scale <= 0.001 + epsilon)
            {
                if (maths_is_point_in_rectangle(position, rect))
                    continue;
            }

            // Check that point is within galaxy radius
            double distance_from_center = sqrt(ix * ix + iy * iy);

            if (distance_from_center > (nav_state->current_galaxy->radius * GALAXY_SCALE))
                continue;

            // Create rng seed by combining x,y values
            uint64_t seed = maths_hash_position_to_uint64(position);

            // Seed with a fixed constant
            pcg32_srandom_r(&rng, seed, nav_state->initseq);

            // Calculate density based on distance from center
            double density = (GALAXY_DENSITY / pow((distance_from_center / a + 1), 6));

            int has_star = abs(pcg32_random_r(&rng)) % 1000 < density;

            if (has_star)
            {
                // Check whether star exists in hash table
                if (stars_entry_exists(nav_state->stars, position))
                    continue;
                else
                {
                    // Create star
                    Star *star = stars_create_star(nav_state, position, true, scale);

                    // Add star to hash table
                    stars_add_entry(nav_state->stars, position, star);
                }
            }
        }
    }

    // Store previous boundaries
    boundaries_minus.x = left_boundary;
    boundaries_minus.y = top_boundary;
    boundaries_plus.x = right_boundary;
    boundaries_plus.y = bottom_boundary;
    initialized = true;

    // Delete stars that end up outside the region
    int region_size = sections_in_camera_x;
    stars_delete_outside_region(nav_state->stars, bx, by, region_size);
}

/**
 * Calculates the distance from a given position to the nearest star in the current galaxy.
 * Searches inner circumferences of points first and works towards outward circumferences.
 *
 * @param position The position from which to calculate the distance.
 * @param current_galaxy A pointer to the current Galaxy object.
 * @param initseq The initialization sequence used for the RNG.
 * @param galaxy_density The density of stars in the current galaxy.
 *
 * @return The distance from the given position to the nearest star in the current galaxy.
 *         If no star is found within the galaxy, the function returns 7 * GALAXY_SECTION_SIZE.
 */
double stars_nearest_center_distance(Point position, Galaxy *current_galaxy, uint64_t initseq, int galaxy_density)
{
    // We use 6 * GALAXY_SECTION_SIZE as max, since a CLASS_6 star needs 6 + 1 empty sections
    // We search inner circumferences of points first and work towards outward circumferences
    // If we find a star, the function returns.

    // Keep track of checked points
    Point checked_points[196];
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

                Point p = {ix, iy};

                if (maths_check_point_in_array(p, checked_points, num_checked_points))
                    continue;

                checked_points[num_checked_points++] = p;

                // Create rng seed by combining x,y values
                uint64_t seed = maths_hash_position_to_uint64(p);

                // Seed with a fixed constant
                pcg32_srandom_r(&rng, seed, initseq);

                // Calculate density based on distance from center
                /*
                 * If we do this like in stars_generate, this will result in large stars at the edges,
                 * and small stars at the center.
                 * Instead, we use the trick of calculating density only in this small region.
                 * This may find fake near stars that do not really exist and force star sizes to be smaller.
                 */
                // double distance_from_center = maths_distance_between_points(ix, iy, 0, 0);
                double distance_from_center = maths_distance_between_points(ix, iy, position.x, position.y);

                double density = (galaxy_density / pow((distance_from_center / a + 1), 6));

                int has_star = abs(pcg32_random_r(&rng)) % 1000 < density;

                if (has_star)
                {
                    double distance = maths_distance_between_points(ix, iy, position.x, position.y);

                    return distance;
                }
            }
        }
    }

    return 7 * GALAXY_SECTION_SIZE;
}

/**
 * Determines the planet size class based on its width.
 *
 * @param width The width of the planet.
 *
 * @return An integer representing the planet size class (from 1 to 6).
 */
static int stars_planet_size_class(float width)
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

/**
 * Populates a CelestialBody with randomly generated planets or moons, based on its level and class.
 *
 * @param body A pointer to the CelestialBody to be populated.
 * @param position A position of the CelestialBody.
 * @param rng pcg32_random_t instance used to generate random numbers.
 * @param scale Scaling factor to convert world coordinates to screen coordinates.
 *
 * @return void
 */
static void stars_populate_body(CelestialBody *body, Point position, pcg32_random_t rng, long double scale)
{
    if (body->level == LEVEL_STAR && body->initialized == 1)
        return;

    if (body->level >= LEVEL_MOON)
        return;

    int max_planets = (body->level == LEVEL_STAR) ? MAX_PLANETS : MAX_MOONS;

    if (max_planets == 0)
        return;

    if (body->level == LEVEL_STAR)
    {
        int orbit_range_min = STAR_CLASS_1_ORBIT_MIN;
        int orbit_range_max = STAR_CLASS_1_ORBIT_MAX;
        float radius_max = STAR_CLASS_1_PLANET_RADIUS_MAX;

        switch (body->class)
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

        while (i < max_planets && width < body->cutoff - 2 * body->radius)
        {
            // Orbit is calculated between surfaces, not centers
            // Round some values to get rid of floating-point inaccuracies
            float _orbit_width = fmod(abs(pcg32_random_r(&rng)), orbit_range_max * body->radius) + orbit_range_min * body->radius;
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
            if (width + orbit_width + 2 * radius < body->cutoff - 2 * body->radius)
            {
                width += orbit_width + 2 * radius;

                Planet *planet = (Planet *)malloc(sizeof(Planet));

                if (planet == NULL)
                {
                    fprintf(stderr, "Error: Could not allocate memory for Planet.\n");
                    return;
                }

                strcpy(planet->name, body->name);                               // Copy star name to planet name
                sprintf(planet->name + strlen(planet->name), "-%s-%d", "P", i); // Append to planet name
                planet->image = "../assets/images/earth.png";
                planet->class = stars_planet_size_class(orbit_width);
                planet->color = colors[COLOR_SKY_BLUE_255];
                planet->level = LEVEL_PLANET;
                planet->is_selected = false;
                planet->radius = radius;
                planet->cutoff = orbit_width / 2;
                planet->orbit_radius = orbit_width;

                // Calculate orbital velocity
                float angle = fmod(abs(pcg32_random_r(&rng)), 360);
                float vx, vy;
                float total_width = width + body->radius - planet->radius; // center to center
                phys_calculate_orbital_velocity(total_width, angle, body->radius, &vx, &vy);

                planet->position.x = body->position.x + total_width * cos(angle * M_PI / 180);
                planet->position.y = body->position.y + total_width * sin(angle * M_PI / 180);
                planet->vx = vx;
                planet->vy = vy;
                planet->dx = 0.0;
                planet->dy = 0.0;
                SDL_Surface *surface = IMG_Load(planet->image);
                planet->texture = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_FreeSurface(surface);
                planet->rect.x = (planet->position.x - planet->radius) * scale;
                planet->rect.y = (planet->position.y - planet->radius) * scale;
                planet->rect.w = 2 * planet->radius * scale;
                planet->rect.h = 2 * planet->radius * scale;
                planet->num_planets = 0;
                planet->planets[0] = NULL;
                planet->parent = body;
                planet->parent->num_planets++;

                body->planets[i] = planet;
                body->planets[i + 1] = NULL;
                i++;

                stars_populate_body(planet, position, rng, scale);
            }
            else
                break;
        }

        // Set star as initialized
        body->initialized = 1;
    }

    // Moons
    else if (body->level == LEVEL_PLANET)
    {
        int orbit_range_min = PLANET_CLASS_1_ORBIT_MIN;
        int orbit_range_max = PLANET_CLASS_1_ORBIT_MAX;
        float radius_max = PLANET_CLASS_1_MOON_RADIUS_MAX;
        float planet_cutoff_limit;

        switch (body->class)
        {
        case PLANET_CLASS_1:
            orbit_range_min = PLANET_CLASS_1_ORBIT_MIN;
            orbit_range_max = PLANET_CLASS_1_ORBIT_MAX;
            radius_max = PLANET_CLASS_1_MOON_RADIUS_MAX;
            planet_cutoff_limit = body->cutoff / 2;
            max_planets = (int)body->cutoff % (max_planets - 4); // 0 - <max - 4>
            break;
        case PLANET_CLASS_2:
            orbit_range_min = PLANET_CLASS_2_ORBIT_MIN;
            orbit_range_max = PLANET_CLASS_2_ORBIT_MAX;
            radius_max = PLANET_CLASS_2_MOON_RADIUS_MAX;
            planet_cutoff_limit = body->cutoff / 2;
            max_planets = (int)body->cutoff % (max_planets - 3); // 0 - <max - 3>
            break;
        case PLANET_CLASS_3:
            orbit_range_min = PLANET_CLASS_3_ORBIT_MIN;
            orbit_range_max = PLANET_CLASS_3_ORBIT_MAX;
            radius_max = PLANET_CLASS_3_MOON_RADIUS_MAX;
            planet_cutoff_limit = body->cutoff / 2;
            max_planets = (int)body->cutoff % (max_planets - 2); // 0 - <max - 2>
            break;
        case PLANET_CLASS_4:
            orbit_range_min = PLANET_CLASS_4_ORBIT_MIN;
            orbit_range_max = PLANET_CLASS_4_ORBIT_MAX;
            radius_max = PLANET_CLASS_4_MOON_RADIUS_MAX;
            planet_cutoff_limit = body->cutoff / 2;
            max_planets = (int)body->cutoff % (max_planets - 2); // 0 - <max - 2>
            break;
        case PLANET_CLASS_5:
            orbit_range_min = PLANET_CLASS_5_ORBIT_MIN;
            orbit_range_max = PLANET_CLASS_5_ORBIT_MAX;
            radius_max = PLANET_CLASS_5_MOON_RADIUS_MAX;
            planet_cutoff_limit = body->cutoff / 3;
            max_planets = (int)body->cutoff % (max_planets - 1); // 0 - <max - 1>
            break;
        case PLANET_CLASS_6:
            orbit_range_min = PLANET_CLASS_6_ORBIT_MIN;
            orbit_range_max = PLANET_CLASS_6_ORBIT_MAX;
            radius_max = PLANET_CLASS_6_MOON_RADIUS_MAX;
            planet_cutoff_limit = body->cutoff / 3;
            max_planets = (int)body->cutoff % max_planets; // 0 - <max>
            break;
        default:
            orbit_range_min = PLANET_CLASS_1_ORBIT_MIN;
            orbit_range_max = PLANET_CLASS_1_ORBIT_MAX;
            radius_max = PLANET_CLASS_1_MOON_RADIUS_MAX;
            planet_cutoff_limit = body->cutoff / 2;
            max_planets = (int)body->cutoff % (max_planets - 4); // 0 - <max - 4>
            break;
        }

        float width = 0;
        int i = 0;

        while (i < max_planets && width < body->cutoff - 2 * body->radius)
        {
            // Orbit is calculated between surfaces, not centers
            float _orbit_width = fmod(abs(pcg32_random_r(&rng)), orbit_range_max * body->radius) + orbit_range_min * body->radius;
            float orbit_width = 0;

            // The first orbit should not be closer than <planet_cutoff_limit>
            while (1)
            {
                orbit_width += _orbit_width;

                if (orbit_width >= planet_cutoff_limit)
                    break;
            }

            // A moon can not be larger than class radius_max or 1 / 3 of planet radius
            float radius = fmod(orbit_width, fmin(radius_max, body->radius / 3)) + MOON_RADIUS_MIN;

            // Add moon
            if (width + orbit_width + 2 * radius < body->cutoff - 2 * body->radius)
            {
                width += orbit_width + 2 * radius;

                Planet *moon = (Planet *)malloc(sizeof(Planet));

                if (moon == NULL)
                {
                    fprintf(stderr, "Error: Could not allocate memory for Planet.\n");
                    return;
                }

                strcpy(moon->name, body->name);                             // Copy planet name to moon name
                sprintf(moon->name + strlen(moon->name), "-%s-%d", "M", i); // Append to moon name
                moon->image = "../assets/images/moon.png";
                moon->class = 0;
                moon->color = colors[COLOR_GAINSBORO_255];
                moon->level = LEVEL_MOON;
                moon->is_selected = false;
                moon->radius = radius;
                moon->orbit_radius = orbit_width;

                // Calculate orbital velocity
                float angle = fmod(abs(pcg32_random_r(&rng)), 360);
                float vx, vy;
                float total_width = width + body->radius - moon->radius;
                phys_calculate_orbital_velocity(total_width, angle, body->radius, &vx, &vy);

                moon->position.x = body->position.x + total_width * cos(angle * M_PI / 180);
                moon->position.y = body->position.y + total_width * sin(angle * M_PI / 180);
                moon->vx = vx;
                moon->vy = vy;
                moon->dx = 0.0;
                moon->dy = 0.0;
                SDL_Surface *surface = IMG_Load(moon->image);
                moon->texture = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_FreeSurface(surface);
                moon->rect.x = (moon->position.x - moon->radius) * scale;
                moon->rect.y = (moon->position.y - moon->radius) * scale;
                moon->rect.w = 2 * moon->radius * scale;
                moon->rect.h = 2 * moon->radius * scale;
                moon->num_planets = 0;
                moon->planets[0] = NULL;
                moon->parent = body;
                moon->parent->num_planets++;

                body->planets[i] = moon;
                body->planets[i + 1] = NULL;
                i++;
            }
            else
                break;
        }
    }
}

/**
 * Determine the size class of a star based on its distance.
 *
 * This function takes in a float value `distance` which represents the distance
 * of a star from the center of the galaxy and returns an integer representing
 * the star's size class.
 *
 * @param distance A float value representing the distance of the star from the center.
 *
 * @return An integer representing the star's size class.
 */
unsigned short stars_size_class(float distance)
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

/**
 * Updates the orbital positions of celestial bodies, including planets and stars,
 * based on the current game state, input state, navigation state, and ship information.
 *
 * @param game_state A pointer to the current game state.
 * @param input_state A pointer to the current input state.
 * @param nav_state A pointer to the current navigation state.
 * @param body A pointer to the celestial body being updated.
 * @param ship A pointer to the player's ship.
 * @param star_class An integer indicating the class of the current star (only relevant for stars).
 *
 * @return void
 */
void stars_update_orbital_positions(GameState *game_state, const InputState *input_state, NavigationState *nav_state, CelestialBody *body, Ship *ship, const Camera *camera, unsigned short star_class)
{
    double distance;
    Point position;

    if (game_state->state == NAVIGATE)
    {
        position.x = nav_state->navigate_offset.x;
        position.y = nav_state->navigate_offset.y;
    }
    else if (game_state->state == MAP)
    {
        position.x = nav_state->map_offset.x;
        position.y = nav_state->map_offset.y;
    }

    // Update planets
    if (body->level != LEVEL_STAR)
    {
        if (game_state->state == NAVIGATE)
        {
            // Update body position
            body->position.x += body->parent->dx;
            body->position.y += body->parent->dy;

            // Find distance from parent
            double delta_x = body->parent->position.x - body->position.x;
            double delta_y = body->parent->position.y - body->position.y;
            distance = sqrt(delta_x * delta_x + delta_y * delta_y);

            // Determine speed and position shift
            if (distance > (body->parent->radius + body->radius))
            {
                float g_body = G_CONSTANT * body->parent->radius * body->parent->radius / (distance * distance);

                body->vx += g_body * delta_x / distance;
                body->vy += g_body * delta_y / distance;
                body->dx = body->vx / FPS;
                body->dy = body->vy / FPS;
            }

            // Update body position
            body->position.x += body->vx / FPS;
            body->position.y += body->vy / FPS;
        }
        else if (game_state->state == MAP)
        {
            // Find distance from parent
            distance = maths_distance_between_points(body->parent->position.x, body->parent->position.y, body->position.x, body->position.y);
        }

        // Update moons
        int max_planets = MAX_MOONS;

        for (int i = 0; i < max_planets && body->planets[i] != NULL; i++)
        {
            stars_update_orbital_positions(game_state, input_state, nav_state, body->planets[i], ship, camera, star_class);
        }
    }
    else if (body->level == LEVEL_STAR)
    {
        // Get star distance from position
        distance = maths_distance_between_points(body->position.x, body->position.y, position.x, position.y);

        if (game_state->state == MAP)
        {
            // Get relative position of star in game_scale
            int radius = (body->class * GALAXY_SECTION_SIZE / 2) * game_state->game_scale;
            int x = (body->position.x - camera->x) * game_state->game_scale;
            int y = (body->position.y - camera->y) * game_state->game_scale;
            Point relative_star_position = {.x = x, .y = y};

            if (distance < body->cutoff || maths_is_point_in_circle(input_state->mouse_position, relative_star_position, radius))
            {
                // Update planets
                int max_planets = MAX_PLANETS;

                for (int i = 0; i < max_planets && body->planets[i] != NULL; i++)
                {
                    stars_update_orbital_positions(game_state, input_state, nav_state, body->planets[i], ship, camera, star_class);
                }
            }
        }
        else if (game_state->state == NAVIGATE)
        {
            if (distance < body->cutoff)
            {
                // Create system
                if (!body->initialized)
                {
                    Point star_position = {.x = body->position.x, .y = body->position.y};

                    // Use a local rng
                    pcg32_random_t rng;

                    // Create rng seed by combining x,y values
                    uint64_t seed = maths_hash_position_to_uint64(star_position);

                    // Seed with a fixed constant
                    pcg32_srandom_r(&rng, seed, nav_state->initseq);

                    stars_populate_body(body, star_position, rng, game_state->game_scale);
                }

                // Update planets
                int max_planets = MAX_PLANETS;

                for (int i = 0; i < max_planets && body->planets[i] != NULL; i++)
                {
                    stars_update_orbital_positions(game_state, input_state, nav_state, body->planets[i], ship, camera, star_class);
                }
            }
        }
    }

    // Update ship speed due to gravity
    if (game_state->state == NAVIGATE && SHIP_GRAVITY_ON)
        phys_apply_gravity_to_ship(game_state, input_state->thrust_on, nav_state, body, ship, star_class);

    // Update velocity
    phys_update_velocity(&nav_state->velocity, ship);
}

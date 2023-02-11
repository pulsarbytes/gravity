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
extern float game_scale;

static void cleanup_planets(struct planet_t *planet);
static void cleanup_stars(void);
uint64_t pair_hash_order_sensitive(struct position_t);
uint64_t double_hash(double x);
bool point_eq(struct position_t a, struct position_t b);
bool point_in_array(struct position_t p, struct position_t arr[], int len);
int calculate_projection_opacity(float distance);
double find_nearest_section_axis(double n);

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
void put_star(struct position_t position, struct planet_t *star)
{
    uint64_t index = pair_hash_order_sensitive(position) % MAX_STARS;
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
    uint64_t index = pair_hash_order_sensitive(position) % MAX_STARS;
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
    uint64_t index = pair_hash_order_sensitive(position) % MAX_STARS;
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
    uint64_t index = pair_hash_order_sensitive(position) % MAX_STARS;
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
    struct planet_t *planet = NULL;
    struct ship_t *ship = NULL;
    float delta_x, delta_y, point;
    int offset;

    switch (entity_type)
    {
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
    if (planet)
    {
        delta_x = planet->position.x - camera->x - (camera_w / 2);
        delta_y = planet->position.y - camera->y - (camera_h / 2);
    }
    else if (ship)
    {
        delta_x = ship->position.x - camera->x - (camera_w / 2);
        delta_y = ship->position.y - camera->y - (camera_h / 2);
    }

    // 1st quadrant (clockwise)
    if (delta_x >= 0 && delta_y < 0)
    {
        point = (camera_h / 2) * delta_x / -delta_y;

        // Top
        if (point <= camera_w / 2)
        {
            if (planet)
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

            if (planet)
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
            if (planet)
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

            if (planet)
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
            if (planet)
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

            if (planet)
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
            if (planet)
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

            if (planet)
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

    float x = camera->x + (camera->w / 2) / game_scale;
    float y = camera->y + (camera->h / 2) / game_scale;
    float distance = sqrt(pow(fabs(x - planet->position.x), 2) + pow(fabs(y - planet->position.y), 2));
    int opacity = 255;

    if (planet->level == LEVEL_STAR)
        opacity = calculate_projection_opacity(distance);

    SDL_SetRenderDrawColor(renderer, planet->color.r, planet->color.g, planet->color.b, opacity);
    SDL_RenderFillRect(renderer, &planet->projection);
}

/*
 * Calculate projection opacity according to object distance.
 * Opacity is fades to 128 for <near_sections> and then fades linearly to 0.
 */
int calculate_projection_opacity(float distance)
{
    int a = (int)(distance / SECTION_SIZE);
    int near_sections = 4;
    int opacity;

    if (a <= 1)
        opacity = 255;
    else if (a <= near_sections)
        opacity = 128 + (255 - 128) * (near_sections - a) / (near_sections - 1);
    else if (a <= 10)
        opacity = 64 + (128 - 64) * (10 - a) / (10 - 4);
    else
        opacity = 0 + (64 - 0) * (30 - a) / (30 - 10);

    return opacity;
}

/*
 * Find distance to nearest star.
 */
float nearest_star_distance(struct position_t position)
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
        for (float ix = position.x - i * SECTION_SIZE; ix <= position.x + i * SECTION_SIZE; ix += SECTION_SIZE)
        {
            for (float iy = position.y - i * SECTION_SIZE; iy <= position.y + i * SECTION_SIZE; iy += SECTION_SIZE)
            {
                if (ix == position.x && iy == position.y)
                    continue;

                struct position_t p = {ix, iy};

                if (point_in_array(p, checked_points, num_checked_points))
                    continue;

                checked_points[num_checked_points++] = p;

                // Seed with a fixed constant
                pcg32_srandom_r(&rng, double_hash(ix), double_hash(iy));

                int has_star = abs(pcg32_random_r(&rng)) % 1000 < REGION_DENSITY;

                if (has_star)
                {
                    float distance = sqrt(pow(ix - position.x, 2) + pow(iy - position.y, 2));

                    return distance;
                }
            }
        }
    }

    return 6 * SECTION_SIZE;
}

/*
 * Find star class.
 * n is number of empty sections.
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
 * n is orbit width.
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
 * Draw map section lines.
 */
void draw_section_lines(const struct camera_t *camera, struct position_t offset)
{
    double bx = find_nearest_section_axis(camera->x);
    double by = find_nearest_section_axis(camera->y);
    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 32);

    // We are far from edges (> 20 sections far)
    if ((offset.x < -X_LIMIT - 20 * SECTION_SIZE || offset.x > -X_LIMIT + 20 * SECTION_SIZE) &&
        (offset.x < X_LIMIT - 20 * SECTION_SIZE || offset.x > X_LIMIT + 20 * SECTION_SIZE) &&
        (offset.y < -Y_LIMIT - 20 * SECTION_SIZE || offset.y > -Y_LIMIT + 20 * SECTION_SIZE) &&
        (offset.y < Y_LIMIT - 20 * SECTION_SIZE || offset.y > Y_LIMIT + 20 * SECTION_SIZE))
    {
        for (int ix = bx; ix <= bx + camera->w / game_scale; ix = ix + SECTION_SIZE)
        {
            SDL_RenderDrawLine(renderer, (ix - camera->x) * game_scale, 0, (ix - camera->x) * game_scale, camera->h);
        }

        for (int iy = by; iy <= by + camera->h / game_scale; iy = iy + SECTION_SIZE)
        {
            SDL_RenderDrawLine(renderer, 0, (iy - camera->y) * game_scale, camera->w, (iy - camera->y) * game_scale);
        }
    }
    // We are close to an edge (< 20 sections far)
    else
    {
        int top_left_corner = in_camera_game_scale(camera, (-X_LIMIT - camera->x) * game_scale, (-Y_LIMIT - camera->y) * game_scale);
        int top_right_corner = in_camera_game_scale(camera, (X_LIMIT - camera->x) * game_scale, (-Y_LIMIT - camera->y) * game_scale);
        int bottom_right_corner = in_camera_game_scale(camera, (X_LIMIT - camera->x) * game_scale, (Y_LIMIT - camera->y) * game_scale);
        int bottom_left_corner = in_camera_game_scale(camera, (-X_LIMIT - camera->x) * game_scale, (Y_LIMIT - camera->y) * game_scale);
        double _x_intersection = (-X_LIMIT - camera->x) * game_scale;
        double x_intersection = (X_LIMIT - camera->x) * game_scale;
        double _y_intersection = (-Y_LIMIT - camera->y) * game_scale;
        double y_intersection = (Y_LIMIT - camera->y) * game_scale;

        for (int ix = bx; ix <= bx + camera->w / game_scale; ix = ix + SECTION_SIZE)
        {
            if (ix == -X_LIMIT)
            {
                if (top_left_corner)
                {
                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 32);
                    SDL_RenderDrawLine(renderer, (ix - camera->x) * game_scale, 0, (ix - camera->x) * game_scale, _y_intersection);

                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 80);
                    SDL_RenderDrawLine(renderer, (ix - camera->x) * game_scale, _y_intersection, (ix - camera->x) * game_scale, camera->h);
                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 32);
                }
                else if (bottom_left_corner)
                {
                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 80);
                    SDL_RenderDrawLine(renderer, (ix - camera->x) * game_scale, 0, (ix - camera->x) * game_scale, y_intersection);

                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 32);
                    SDL_RenderDrawLine(renderer, (ix - camera->x) * game_scale, y_intersection, (ix - camera->x) * game_scale, camera->h);
                }
                else
                {
                    if ((offset.y > 0 && y_intersection > camera->h) || (offset.y < 0 && _y_intersection < 0))
                    {
                        SDL_SetRenderDrawColor(renderer, 255, 165, 0, 80);
                        SDL_RenderDrawLine(renderer, (ix - camera->x) * game_scale, 0, (ix - camera->x) * game_scale, camera->h);
                        SDL_SetRenderDrawColor(renderer, 255, 165, 0, 32);
                    }
                    else
                    {
                        SDL_SetRenderDrawColor(renderer, 255, 165, 0, 32);
                        SDL_RenderDrawLine(renderer, (ix - camera->x) * game_scale, 0, (ix - camera->x) * game_scale, camera->h);
                    }
                }
            }
            else if (ix == X_LIMIT)
            {
                if (top_right_corner)
                {
                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 32);
                    SDL_RenderDrawLine(renderer, (ix - camera->x) * game_scale, 0, (ix - camera->x) * game_scale, _y_intersection);

                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 80);
                    SDL_RenderDrawLine(renderer, (ix - camera->x) * game_scale, _y_intersection, (ix - camera->x) * game_scale, camera->h);
                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 32);
                }
                else if (bottom_right_corner)
                {
                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 80);
                    SDL_RenderDrawLine(renderer, (ix - camera->x) * game_scale, 0, (ix - camera->x) * game_scale, y_intersection);

                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 32);
                    SDL_RenderDrawLine(renderer, (ix - camera->x) * game_scale, y_intersection, (ix - camera->x) * game_scale, camera->h);
                }
                else
                {
                    if ((offset.y > 0 && y_intersection > camera->h) || (offset.y < 0 && _y_intersection < 0))
                    {
                        SDL_SetRenderDrawColor(renderer, 255, 165, 0, 80);
                        SDL_RenderDrawLine(renderer, (ix - camera->x) * game_scale, 0, (ix - camera->x) * game_scale, camera->h);
                        SDL_SetRenderDrawColor(renderer, 255, 165, 0, 32);
                    }
                    else
                    {
                        SDL_SetRenderDrawColor(renderer, 255, 165, 0, 32);
                        SDL_RenderDrawLine(renderer, (ix - camera->x) * game_scale, 0, (ix - camera->x) * game_scale, camera->h);
                    }
                }
            }
            else
            {
                SDL_RenderDrawLine(renderer, (ix - camera->x) * game_scale, 0, (ix - camera->x) * game_scale, camera->h);
            }
        }

        for (int iy = by; iy <= by + camera->h / game_scale; iy = iy + SECTION_SIZE)
        {
            if (iy == -Y_LIMIT)
            {
                if (top_left_corner)
                {
                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 32);
                    SDL_RenderDrawLine(renderer, 0, (iy - camera->y) * game_scale, _x_intersection, (iy - camera->y) * game_scale);

                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 80);
                    SDL_RenderDrawLine(renderer, _x_intersection, (iy - camera->y) * game_scale, camera->w, (iy - camera->y) * game_scale);
                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 32);
                }
                else if (top_right_corner)
                {
                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 80);
                    SDL_RenderDrawLine(renderer, 0, (iy - camera->y) * game_scale, x_intersection, (iy - camera->y) * game_scale);

                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 32);
                    SDL_RenderDrawLine(renderer, x_intersection, (iy - camera->y) * game_scale, camera->w, (iy - camera->y) * game_scale);
                }
                else
                {
                    if ((offset.x > 0 && x_intersection > camera->w) || (offset.x < 0 && _x_intersection < 0))
                    {
                        SDL_SetRenderDrawColor(renderer, 255, 165, 0, 80);
                        SDL_RenderDrawLine(renderer, 0, (iy - camera->y) * game_scale, camera->w, (iy - camera->y) * game_scale);
                        SDL_SetRenderDrawColor(renderer, 255, 165, 0, 32);
                    }
                    else
                    {
                        SDL_SetRenderDrawColor(renderer, 255, 165, 0, 32);
                        SDL_RenderDrawLine(renderer, 0, (iy - camera->y) * game_scale, camera->w, (iy - camera->y) * game_scale);
                    }
                }
            }
            else if (iy == Y_LIMIT)
            {
                if (bottom_left_corner)
                {
                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 32);
                    SDL_RenderDrawLine(renderer, 0, (iy - camera->y) * game_scale, _x_intersection, (iy - camera->y) * game_scale);

                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 80);
                    SDL_RenderDrawLine(renderer, _x_intersection, (iy - camera->y) * game_scale, camera->w, (iy - camera->y) * game_scale);
                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 32);
                }
                else if (bottom_right_corner)
                {
                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 80);
                    SDL_RenderDrawLine(renderer, 0, (iy - camera->y) * game_scale, x_intersection, (iy - camera->y) * game_scale);

                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 32);
                    SDL_RenderDrawLine(renderer, x_intersection, (iy - camera->y) * game_scale, camera->w, (iy - camera->y) * game_scale);
                }
                else
                {
                    if ((offset.x > 0 && x_intersection > camera->w) || (offset.x < 0 && _x_intersection < 0))
                    {
                        SDL_SetRenderDrawColor(renderer, 255, 165, 0, 80);
                        SDL_RenderDrawLine(renderer, 0, (iy - camera->y) * game_scale, camera->w, (iy - camera->y) * game_scale);
                        SDL_SetRenderDrawColor(renderer, 255, 165, 0, 32);
                    }
                    else
                    {
                        SDL_SetRenderDrawColor(renderer, 255, 165, 0, 32);
                        SDL_RenderDrawLine(renderer, 0, (iy - camera->y) * game_scale, camera->w, (iy - camera->y) * game_scale);
                    }
                }
            }
            else
            {
                SDL_RenderDrawLine(renderer, 0, (iy - camera->y) * game_scale, camera->w, (iy - camera->y) * game_scale);
            }
        }
    }
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
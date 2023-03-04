/*
 * graphics.c - Definitions for graphics functions.
 */

#include <math.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include "../lib/pcg-c-basic-0.9/pcg_basic.h"

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"
#include "../include/graphics.h"

// External variable definitions
extern SDL_Renderer *renderer;
extern SDL_Color colors[];

// Static function prototypes
static void gfx_update_projection_position(const NavigationState *, void *ptr, int entity_type, const Camera *, int state, long double scale);
static int gfx_update_projection_opacity(double distance, int region_size, int section_size);

/*
 * Creates colors for SDL.
 */
void gfx_create_default_colors(void)
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

/*
 * Midpoint Circle Algorithm for drawing a circle in SDL.
 * xc, xy, radius are in game_scale.
 * This function is efficient only for small circles.
 */
void gfx_draw_circle(SDL_Renderer *renderer, const Camera *camera, int xc, int yc, int radius, SDL_Color color)
{
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    // Draw the circle
    while (y >= x)
    {
        // Draw the 8 points symmetrically
        if (gfx_relative_position_in_camera(camera, xc + x, yc + y))
        {
            SDL_RenderDrawPoint(renderer, xc + x, yc + y);
        }
        if (gfx_relative_position_in_camera(camera, xc + x, yc - y))
        {
            SDL_RenderDrawPoint(renderer, xc + x, yc - y);
        }
        if (gfx_relative_position_in_camera(camera, xc - x, yc + y))
        {
            SDL_RenderDrawPoint(renderer, xc - x, yc + y);
        }
        if (gfx_relative_position_in_camera(camera, xc - x, yc - y))
        {
            SDL_RenderDrawPoint(renderer, xc - x, yc - y);
        }
        if (gfx_relative_position_in_camera(camera, xc + y, yc + x))
        {
            SDL_RenderDrawPoint(renderer, xc + y, yc + x);
        }
        if (gfx_relative_position_in_camera(camera, xc + y, yc - x))
        {
            SDL_RenderDrawPoint(renderer, xc + y, yc - x);
        }
        if (gfx_relative_position_in_camera(camera, xc - y, yc + x))
        {
            SDL_RenderDrawPoint(renderer, xc - y, yc + x);
        }
        if (gfx_relative_position_in_camera(camera, xc - y, yc - x))
        {
            SDL_RenderDrawPoint(renderer, xc - y, yc - x);
        }

        if (d < 0)
            d = d + 4 * x + 6;
        else
        {
            d = d + 4 * (x - y) + 10;
            y--;
        }

        x++;
    }
}

/**
 * Draws a circle approximation using a series of bezier curves.
 * This function will only draw segments of the circle that intersect with the viewport defined by `camera`.
 * This function is efficient for very large circles.
 *
 * @param renderer The renderer to use to draw the circle
 * @param camera The camera used to view the scene
 * @param x The x coordinate of the center of the circle
 * @param y The y coordinate of the center of the circle
 * @param r The radius of the circle
 * @param color The color to use when drawing the circle
 */
void gfx_draw_circle_approximation(SDL_Renderer *renderer, const Camera *camera, int x, int y, int r, SDL_Color color)
{
    const int CIRCLE_APPROXIMATION = 500;
    int i;
    double angle;
    double x1, y1, x2, y2, x3, y3, x4, y4;

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    for (i = 0; i < CIRCLE_APPROXIMATION; i++)
    {
        angle = 2 * M_PI * i / CIRCLE_APPROXIMATION;
        x1 = x + r * cos(angle);
        y1 = y + r * sin(angle);
        angle = 2 * M_PI * (i + 1) / CIRCLE_APPROXIMATION;
        x2 = x + r * cos(angle);
        y2 = y + r * sin(angle);

        x3 = (2 * x1 + x2) / 3;
        y3 = (2 * y1 + y2) / 3;
        x4 = (x1 + 2 * x2) / 3;
        y4 = (y1 + 2 * y2) / 3;

        if (maths_line_intersects_camera(camera, x1, y1, x3, y3) ||
            maths_line_intersects_camera(camera, x3, y3, x4, y4) ||
            maths_line_intersects_camera(camera, x4, y4, x2, y2))
        {

            SDL_RenderDrawLine(renderer, x1, y1, x3, y3);
            SDL_RenderDrawLine(renderer, x3, y3, x4, y4);
            SDL_RenderDrawLine(renderer, x4, y4, x2, y2);
        }
    }
}

void gfx_draw_galaxy_cloud(Galaxy *galaxy, const Camera *camera, int gstars_count, unsigned short high_definition, long double scale)
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

void gfx_draw_menu_galaxy_cloud(const Camera *camera, Gstar *menustars)
{
    int i = 0;
    float scaling_factor = 0.15;

    while (i < MAX_GSTARS && menustars[i].final_star == 1)
    {
        int x, y;

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, menustars[i].opacity);

        x = camera->w - camera->w / 4 + (menustars[i].position.x / GALAXY_SCALE) * scaling_factor;
        y = camera->h / 3 + (menustars[i].position.y / GALAXY_SCALE) * scaling_factor;

        SDL_RenderDrawPoint(renderer, x, y);

        i++;
    }
}

/*
 * Draw screen frame.
 */
void gfx_draw_screen_frame(Camera *camera)
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

void gfx_draw_section_lines(Camera *camera, int state, SDL_Color color, long double scale)
{
    // Add small tolerance to account for floating-point precision errors
    const double epsilon = ZOOM_EPSILON;

    int section_size;

    if (state == MAP)
    {
        if (scale < 0.01 + epsilon)
            section_size = GALAXY_SECTION_SIZE * 10;
        else
            section_size = GALAXY_SECTION_SIZE;
    }
    else if (state == UNIVERSE)
    {
        section_size = UNIVERSE_SECTION_SIZE;

        if (GALAXY_SCALE == 10000)
        {
            if (scale >= 10 - epsilon)
                section_size = UNIVERSE_SECTION_SIZE / 1000;
            else if (scale >= 1 - epsilon)
                section_size = UNIVERSE_SECTION_SIZE / 100;
            else if (scale >= 0.1 - epsilon)
                section_size = UNIVERSE_SECTION_SIZE / 10;
            else if (scale >= 0.01 - epsilon)
                section_size = UNIVERSE_SECTION_SIZE;
        }
        else if (GALAXY_SCALE == 1000)
        {
            if (scale / GALAXY_SCALE >= 10 - epsilon)
                section_size = UNIVERSE_SECTION_SIZE / 100;
            else if (scale / GALAXY_SCALE >= 1 - epsilon)
                section_size = UNIVERSE_SECTION_SIZE / 10;
            else if (scale / GALAXY_SCALE >= 0.1 - epsilon)
                section_size = UNIVERSE_SECTION_SIZE;
        }
    }

    double bx = maths_get_nearest_section_line(camera->x, section_size);
    double by = maths_get_nearest_section_line(camera->y, section_size);
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

/*
 * Draw a speed arc for the ship.
 */
void gfx_draw_speed_arc(const Ship *ship, const Camera *camera, long double scale)
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
 * Move and draw speed lines.
 */
void gfx_draw_speed_lines(float velocity, const Camera *camera, Speed speed)
{
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
    static int initialized = false;

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
        initialized = true;
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

void gfx_generate_bstars(NavigationState *nav_state, Bstar *bstars, const Camera *camera)
{
    int i = 0, row, column, is_star;
    int end = false;
    int max_bstars = (int)(camera->w * camera->h * BSTARS_PER_SQUARE / BSTARS_SQUARE);

    // Use a local rng
    pcg32_random_t rng;

    // Seed with a fixed constant
    srand(1200);

    for (int j = 0; j < max_bstars; j++)
    {
        bstars[j].final_star = 0;
    }

    for (row = 0; row < camera->h && !end; row++)
    {
        for (column = 0; column < camera->w && !end; column++)
        {
            // Create rng seed by combining x,y values
            Point position = {.x = row, .y = column};
            uint64_t seed = maths_hash_position_to_uint64(position);

            // Set galaxy hash as initseq
            nav_state->initseq = maths_hash_position_to_uint64_2(nav_state->current_galaxy->position);

            // Seed with a fixed constant
            pcg32_srandom_r(&rng, seed, nav_state->initseq);

            is_star = abs(pcg32_random_r(&rng)) % BSTARS_SQUARE < BSTARS_PER_SQUARE;

            if (is_star)
            {
                Bstar star;
                star.position.x = column;
                star.position.y = row;
                star.rect.x = star.position.x;
                star.rect.y = star.position.y;

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

                // Get a color between BSTARS_MIN_OPACITY - BSTARS_MAX_OPACITY
                star.opacity = ((rand() % (BSTARS_MAX_OPACITY + 1 - BSTARS_MIN_OPACITY)) + BSTARS_MIN_OPACITY);

                star.final_star = 1;
                bstars[i++] = star;
            }

            if (i >= max_bstars)
                end = true;
        }
    }
}

void gfx_generate_gstars(Galaxy *galaxy, unsigned short high_definition)
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
    uint64_t initseq = maths_hash_position_to_uint64_2(galaxy->position);

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
            Point position = {.x = ix, .y = iy};
            uint64_t seed = maths_hash_position_to_uint64(position);

            // Seed with a fixed constant
            pcg32_srandom_r(&rng, seed, initseq);

            // Calculate density based on distance from center
            double density = (GALAXY_CLOUD_DENSITY / pow((distance_from_center / a + 1), 6));

            int has_star = abs(pcg32_random_r(&rng)) % 1000 < density;

            if (has_star)
            {
                Gstar star;
                star.position.x = ix;
                star.position.y = iy;

                double distance = stars_nearest_center_distance(position, galaxy, initseq, GALAXY_CLOUD_DENSITY);
                int class = stars_size_class(distance);
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

void gfx_generate_menu_gstars(Galaxy *galaxy, Gstar *menustars)
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
    uint64_t initseq = maths_hash_position_to_uint64_2(galaxy->position);

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
            Point position = {.x = ix, .y = iy};
            uint64_t seed = maths_hash_position_to_uint64(position);

            // Seed with a fixed constant
            pcg32_srandom_r(&rng, seed, initseq);

            // Calculate density based on distance from center
            double density = (MENU_GALAXY_CLOUD_DENSITY / pow((distance_from_center / a + 1), 6));

            int has_star = abs(pcg32_random_r(&rng)) % 1000 < density;

            if (has_star)
            {
                Gstar star;
                star.position.x = ix;
                star.position.y = iy;

                double distance = stars_nearest_center_distance(position, galaxy, initseq, MENU_GALAXY_CLOUD_DENSITY);
                int class = stars_size_class(distance);
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

/*
 * Check whether object with center (x,y) and radius r is in camera.
 * x, y is absolute galaxy scale position. Radius is galaxy scale.
 */
bool gfx_object_in_camera(const Camera *camera, double x, double y, float radius, long double scale)
{
    return x + radius >= camera->x && x - radius - camera->x < camera->w / scale &&
           y + radius >= camera->y && y - radius - camera->y < camera->h / scale;
}

/*
 * Draw body projection on screen edges.
 */
void gfx_project_body_on_edge(const GameState *game_state, const NavigationState *nav_state, CelestialBody *body, const Camera *camera)
{
    gfx_update_projection_position(nav_state, body, ENTITY_CELESTIALBODY, camera, game_state->state, game_state->game_scale);

    body->projection.w = 2 * PROJECTION_RADIUS;
    body->projection.h = 2 * PROJECTION_RADIUS;

    double x = camera->x + (camera->w / 2) / game_state->game_scale;
    double y = camera->y + (camera->h / 2) / game_state->game_scale;
    double distance = maths_distance_between_points(x, y, body->position.x, body->position.y);
    int opacity = colors[COLOR_YELLOW_255].a;
    SDL_Color color;

    if (body->level == LEVEL_STAR)
    {
        if (game_state->state == NAVIGATE)
            opacity = gfx_update_projection_opacity(distance, GALAXY_REGION_SIZE, GALAXY_SECTION_SIZE);
        else if (game_state->state == MAP)
            opacity = gfx_update_projection_opacity(distance, game_state->galaxy_region_size, GALAXY_SECTION_SIZE);

        color.r = colors[COLOR_YELLOW_255].r;
        color.g = colors[COLOR_YELLOW_255].g;
        color.b = colors[COLOR_YELLOW_255].b;
    }
    else
        color = body->color;

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, opacity);
    SDL_RenderFillRect(renderer, &body->projection);
}

/*
 * Draw galaxy projection on screen edges.
 */
void gfx_project_galaxy_on_edge(int state, const NavigationState *nav_state, Galaxy *galaxy, const Camera *camera, long double scale)
{
    int scaling_factor = 1;

    if (state == NAVIGATE || state == MAP)
        scaling_factor = GALAXY_SCALE;

    gfx_update_projection_position(nav_state, galaxy, ENTITY_GALAXY, camera, state, scale);

    galaxy->projection.w = 2 * PROJECTION_RADIUS;
    galaxy->projection.h = 2 * PROJECTION_RADIUS;

    double x = camera->x + (camera->w / 2) / scale;
    double y = camera->y + (camera->h / 2) / scale;
    double delta_x = fabs(x - galaxy->position.x * scaling_factor) / scaling_factor;
    double delta_y = fabs(y - galaxy->position.y * scaling_factor) / scaling_factor;
    double distance = sqrt(pow(delta_x, 2) + pow(delta_y, 2));
    int opacity = gfx_update_projection_opacity(distance, UNIVERSE_REGION_SIZE, UNIVERSE_SECTION_SIZE);

    SDL_SetRenderDrawColor(renderer, galaxy->color.r, galaxy->color.g, galaxy->color.b, opacity);
    SDL_RenderFillRect(renderer, &galaxy->projection);
}

/*
 * Draw ship projection on screen edges.
 */
void gfx_project_ship_on_edge(int state, const InputState *input_state, const NavigationState *nav_state, Ship *ship, const Camera *camera, long double scale)
{
    gfx_update_projection_position(nav_state, ship, ENTITY_SHIP, camera, state, scale);

    // Mirror ship angle
    ship->projection->angle = ship->angle;

    // Draw ship projection
    SDL_RenderCopyEx(renderer, ship->projection->texture, &ship->projection->main_img_rect, &ship->projection->rect, ship->projection->angle, &ship->projection->rotation_pt, SDL_FLIP_NONE);

    // Draw projection thrust
    if (state == NAVIGATE && input_state->thrust)
        SDL_RenderCopyEx(renderer, ship->projection->texture, &ship->projection->thrust_img_rect, &ship->projection->rect, ship->projection->angle, &ship->projection->rotation_pt, SDL_FLIP_NONE);

    // Draw projection reverse
    if (state == NAVIGATE && input_state->reverse)
        SDL_RenderCopyEx(renderer, ship->projection->texture, &ship->projection->reverse_img_rect, &ship->projection->rect, ship->projection->angle, &ship->projection->rotation_pt, SDL_FLIP_NONE);
}

/*
 * Check whether relative point is in camera.
 * x, y are relative to camera->x, camera->y.
 */
bool gfx_relative_position_in_camera(const Camera *camera, int x, int y)
{
    return x >= 0 && x < camera->w && y >= 0 && y < camera->h;
}

/*
 * Move and draw background stars.
 */
void gfx_update_bstars_position(int state, int camera_on, const NavigationState *nav_state, Bstar *bstars, const Camera *camera, Speed speed, double distance)
{
    int i = 0;
    int max_bstars = (int)(camera->w * camera->h * BSTARS_PER_SQUARE / BSTARS_SQUARE);
    float max_distance = 2 * nav_state->current_galaxy->radius * GALAXY_SCALE;

    while (i < max_bstars && bstars[i].final_star == 1)
    {
        if (camera_on || state == MENU)
        {
            float dx, dy;

            if (state == MENU)
            {
                dx = MENU_BSTARS_SPEED_FACTOR * speed.vx / FPS;
                dy = MENU_BSTARS_SPEED_FACTOR * speed.vy / FPS;
            }
            else
            {
                // Limit background stars speed
                if (nav_state->velocity.magnitude > GALAXY_SPEED_LIMIT)
                {
                    dx = BSTARS_SPEED_FACTOR * speed.vx / FPS;
                    dy = BSTARS_SPEED_FACTOR * speed.vy / FPS;
                }
                else
                {
                    dx = BSTARS_SPEED_FACTOR * (nav_state->velocity.magnitude / GALAXY_SPEED_LIMIT) * speed.vx / FPS;
                    dy = BSTARS_SPEED_FACTOR * (nav_state->velocity.magnitude / GALAXY_SPEED_LIMIT) * speed.vy / FPS;
                }
            }

            bstars[i].position.x -= dx;
            bstars[i].position.y -= dy;

            // Normalize within camera boundaries
            if (bstars[i].position.x > camera->w)
            {
                bstars[i].position.x = fmod(bstars[i].position.x, camera->w);
            }
            if (bstars[i].position.x < 0)
            {
                bstars[i].position.x += camera->w;
            }
            if (bstars[i].position.y > camera->h)
            {
                bstars[i].position.y = fmod(bstars[i].position.y, camera->h);
            }
            if (bstars[i].position.y < 0)
            {
                bstars[i].position.y += camera->h;
            }

            bstars[i].rect.x = (int)bstars[i].position.x;
            bstars[i].rect.y = (int)bstars[i].position.y;
        }

        float opacity;

        if (state == MENU)
            opacity = (double)(bstars[i].opacity * 1 / 2);
        else
        {
            // Fade out opacity as we move away from galaxy center
            opacity = (double)bstars[i].opacity * (1 - (distance / max_distance));

            // Opacity is 1/3 at center, 3/3 at max_distance
            opacity = (double)(opacity * ((3 - (2 - 2 * (distance / max_distance))) / 3));
        }

        if (opacity > 255)
            opacity = 255;
        else if (opacity < 0)
            opacity = 0;

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, (unsigned short)opacity);
        SDL_RenderFillRect(renderer, &bstars[i].rect);

        i++;
    }
}

/*
 * Update camera position.
 */
void gfx_update_camera(Camera *camera, Point position, long double scale)
{
    camera->x = position.x - (camera->w / 2) / scale;
    camera->y = position.y - (camera->h / 2) / scale;
}

/*
 * Move and draw galaxy cloud.
 */
void gfx_update_gstars_position(Galaxy *galaxy, Point ship_position, const Camera *camera, double distance, double limit)
{
    int i = 0;
    float min_opacity_factor = 0.35;
    float max_opacity_factor = 0.45;
    float galaxy_radius = galaxy->radius * GALAXY_SCALE;

    // Calculate position in galaxy
    double delta_x = ship_position.x / (galaxy->cutoff * GALAXY_SCALE);
    double delta_y = ship_position.y / (galaxy->cutoff * GALAXY_SCALE);

    // Galaxy has double size when we are at center
    float scaling_factor = (float)galaxy->class / (2 + 2 * (1 - distance / galaxy_radius));

    while (i < MAX_GSTARS && galaxy->gstars_hd[i].final_star == 1)
    {
        int x = (galaxy->gstars_hd[i].position.x / (GALAXY_SCALE * GSTARS_SCALE)) / scaling_factor + camera->w / 2 - delta_x * (camera->w / 2);
        int y = (galaxy->gstars_hd[i].position.y / (GALAXY_SCALE * GSTARS_SCALE)) / scaling_factor + camera->h / 2 - delta_y * (camera->h / 2);

        float opacity;

        if (distance > limit)
        {
            opacity = 0;
        }
        else if (distance <= limit && distance > galaxy_radius)
        {
            // Fade in opacity as we move in towards galaxy radius
            opacity = (float)galaxy->gstars_hd[i].opacity * max_opacity_factor * (limit - distance) / (limit - galaxy_radius);
            opacity = opacity < 0 ? 0 : opacity;
        }
        else if (distance <= galaxy_radius)
        {
            // Fade out opacity as we move towards galaxy center
            float factor = 1.0 - distance / galaxy_radius;
            factor = factor < 0 ? 0 : factor;
            opacity = galaxy->gstars_hd[i].opacity * (max_opacity_factor - (max_opacity_factor - min_opacity_factor) * factor);
        }

        if (opacity > 255)
            opacity = 255;
        else if (opacity < 0)
            opacity = 0;

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, (unsigned short)opacity);
        SDL_RenderDrawPoint(renderer, x, y);

        i++;
    }
}

/*
 * Calculate projection opacity according to object distance.
 * Opacity fades to 128 for <near_sections> and then fades linearly to 0.
 */
static int gfx_update_projection_opacity(double distance, int region_size, int section_size)
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
 * Update projection position.
 * Projection rect top-left point is where the object projection crosses a quadrant line.
 * Projection rect can be centered by moving left or up by <offset>.
 */
static void gfx_update_projection_position(const NavigationState *nav_state, void *ptr, int entity_type, const Camera *camera, int state, long double scale)
{
    Galaxy *galaxy = NULL;
    CelestialBody *body = NULL;
    Ship *ship = NULL;
    double delta_x, delta_y, point;
    int offset;

    switch (entity_type)
    {
    case ENTITY_GALAXY:
        galaxy = (Galaxy *)ptr;
        offset = PROJECTION_RADIUS;
        break;
    case ENTITY_CELESTIALBODY:
        body = (CelestialBody *)ptr;
        offset = PROJECTION_RADIUS;
        break;
    case ENTITY_SHIP:
        ship = (Ship *)ptr;
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
    else if (body)
    {
        delta_x = body->position.x - camera->x - (camera_w / 2);
        delta_y = body->position.y - camera->y - (camera_h / 2);
    }
    else if (ship)
    {
        if (state == NAVIGATE || state == MAP)
        {
            // If in other galaxy, project position at buffer galaxy
            if (state == MAP &&
                (nav_state->current_galaxy->position.x != nav_state->buffer_galaxy->position.x || nav_state->current_galaxy->position.y != nav_state->buffer_galaxy->position.y))
            {
                // Convert camera to Universe position
                long double camera_x = nav_state->current_galaxy->position.x + (nav_state->map_offset.x / GALAXY_SCALE) - (camera->w / 2);
                long double camera_y = nav_state->current_galaxy->position.y + (nav_state->map_offset.y / GALAXY_SCALE) - (camera->h / 2);

                delta_x = nav_state->buffer_galaxy->position.x + (ship->position.x / GALAXY_SCALE) - camera_x - (camera->w / 2);
                delta_y = nav_state->buffer_galaxy->position.y + (ship->position.y / GALAXY_SCALE) - camera_y - (camera->h / 2);
            }
            else
            {
                delta_x = ship->position.x - camera->x - (camera_w / 2);
                delta_y = ship->position.y - camera->y - (camera_h / 2);
            }
        }
        else if (state == UNIVERSE)
        {
            delta_x = (nav_state->buffer_galaxy->position.x + ship->position.x / GALAXY_SCALE - camera->x) - (camera_w / 2);
            delta_y = (nav_state->buffer_galaxy->position.y + ship->position.y / GALAXY_SCALE - camera->y) - (camera_h / 2);
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
            else if (body)
            {
                body->projection.x = ((camera_w / 2) + point) * scale - offset * (point / (camera_w / 2) + 1);
                body->projection.y = 0;
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
            else if (body)
            {
                body->projection.x = camera_w * scale - (2 * offset);
                body->projection.y = point * scale - offset * (point / (camera_h / 2));
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
            else if (body)
            {
                body->projection.x = camera_w * scale - (2 * offset);
                body->projection.y = ((camera_h / 2) + point) * scale - offset * (point / (camera_h / 2) + 1);
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
            else if (body)
            {
                body->projection.x = ((camera_w / 2) + point) * scale - offset * (point / (camera_w / 2) + 1);
                body->projection.y = camera_h * scale - (2 * offset);
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
            else if (body)
            {
                body->projection.x = ((camera_w / 2) - point) * scale - offset * (((camera_w / 2) - point) / (camera_w / 2));
                body->projection.y = camera_h * scale - (2 * offset);
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
            else if (body)
            {
                body->projection.x = 0;
                body->projection.y = (camera_h - point) * scale - offset * (((camera_h / 2) - point) / (camera_h / 2) + 1);
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
            else if (body)
            {
                body->projection.x = 0;
                body->projection.y = ((camera_h / 2) - point) * scale - offset * (((camera_h / 2) - point) / (camera_h / 2));
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
            else if (body)
            {
                body->projection.x = point * scale - offset * (point / (camera_w / 2));
                body->projection.y = 0;
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
 * Zoom in/out a star system (recursive).
 */
void gfx_zoom_star_system(CelestialBody *body, long double scale)
{
    body->rect.x = (body->position.x - body->radius) * scale;
    body->rect.y = (body->position.y - body->radius) * scale;
    body->rect.w = 2 * body->radius * scale;
    body->rect.h = 2 * body->radius * scale;

    // Zoom children
    if (body->level <= LEVEL_PLANET && body->planets != NULL && body->planets[0] != NULL)
    {
        int max_planets = (body->level == LEVEL_STAR) ? MAX_PLANETS : MAX_MOONS;

        for (int i = 0; i < max_planets && body->planets[i] != NULL; i++)
        {
            gfx_zoom_star_system(body->planets[i], scale);
        }
    }
}

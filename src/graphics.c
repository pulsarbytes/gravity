/*
 * graphics.c
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

/**
 * Sets the default color values for the color array.
 *
 * @return void
 */
void gfx_create_default_colors(void)
{
    colors[COLOR_CYAN_70].r = 0;
    colors[COLOR_CYAN_70].g = 255;
    colors[COLOR_CYAN_70].b = 255;
    colors[COLOR_CYAN_70].a = 70;

    colors[COLOR_CYAN_100].r = 0;
    colors[COLOR_CYAN_100].g = 255;
    colors[COLOR_CYAN_100].b = 255;
    colors[COLOR_CYAN_100].a = 100;

    colors[COLOR_CYAN_150].r = 0;
    colors[COLOR_CYAN_150].g = 255;
    colors[COLOR_CYAN_150].b = 255;
    colors[COLOR_CYAN_150].a = 150;

    colors[COLOR_GAINSBORO_255].r = 220;
    colors[COLOR_GAINSBORO_255].g = 220;
    colors[COLOR_GAINSBORO_255].b = 220;
    colors[COLOR_GAINSBORO_255].a = 255;

    colors[COLOR_LAVENDER_255].r = 224;
    colors[COLOR_LAVENDER_255].g = 176;
    colors[COLOR_LAVENDER_255].b = 255;
    colors[COLOR_LAVENDER_255].a = 255;

    colors[COLOR_LIGHT_BLUE_255].r = 192;
    colors[COLOR_LIGHT_BLUE_255].g = 192;
    colors[COLOR_LIGHT_BLUE_255].b = 255;
    colors[COLOR_LIGHT_BLUE_255].a = 255;

    colors[COLOR_LIGHT_GREEN_255].r = 192;
    colors[COLOR_LIGHT_GREEN_255].g = 255;
    colors[COLOR_LIGHT_GREEN_255].b = 192;
    colors[COLOR_LIGHT_GREEN_255].a = 255;

    colors[COLOR_LIGHT_ORANGE_255].r = 255;
    colors[COLOR_LIGHT_ORANGE_255].g = 192;
    colors[COLOR_LIGHT_ORANGE_255].b = 128;
    colors[COLOR_LIGHT_ORANGE_255].a = 255;

    colors[COLOR_LIGHT_RED_255].r = 255;
    colors[COLOR_LIGHT_RED_255].g = 165;
    colors[COLOR_LIGHT_RED_255].b = 165;
    colors[COLOR_LIGHT_RED_255].a = 255;

    colors[COLOR_LIME_GREEN_200].r = 50;
    colors[COLOR_LIME_GREEN_200].g = 205;
    colors[COLOR_LIME_GREEN_200].b = 50;
    colors[COLOR_LIME_GREEN_200].a = 200;

    colors[COLOR_MAGENTA_70].r = 255;
    colors[COLOR_MAGENTA_70].g = 0;
    colors[COLOR_MAGENTA_70].b = 255;
    colors[COLOR_MAGENTA_70].a = 70;

    colors[COLOR_MAGENTA_100].r = 255;
    colors[COLOR_MAGENTA_100].g = 0;
    colors[COLOR_MAGENTA_100].b = 255;
    colors[COLOR_MAGENTA_100].a = 100;

    colors[COLOR_MAGENTA_120].r = 255;
    colors[COLOR_MAGENTA_120].g = 0;
    colors[COLOR_MAGENTA_120].b = 255;
    colors[COLOR_MAGENTA_120].a = 120;

    colors[COLOR_ORANGE_32].r = 255;
    colors[COLOR_ORANGE_32].g = 165;
    colors[COLOR_ORANGE_32].b = 0;
    colors[COLOR_ORANGE_32].a = 32;

    colors[COLOR_PALE_YELLOW_255].r = 255;
    colors[COLOR_PALE_YELLOW_255].g = 255;
    colors[COLOR_PALE_YELLOW_255].b = 192;
    colors[COLOR_PALE_YELLOW_255].a = 255;

    colors[COLOR_SKY_BLUE_255].r = 135;
    colors[COLOR_SKY_BLUE_255].g = 206;
    colors[COLOR_SKY_BLUE_255].b = 235;
    colors[COLOR_SKY_BLUE_255].a = 255;

    colors[COLOR_WHITE_100].r = 255;
    colors[COLOR_WHITE_100].g = 255;
    colors[COLOR_WHITE_100].b = 255;
    colors[COLOR_WHITE_100].a = 100;

    colors[COLOR_WHITE_140].r = 255;
    colors[COLOR_WHITE_140].g = 255;
    colors[COLOR_WHITE_140].b = 255;
    colors[COLOR_WHITE_140].a = 140;

    colors[COLOR_WHITE_180].r = 255;
    colors[COLOR_WHITE_180].g = 255;
    colors[COLOR_WHITE_180].b = 255;
    colors[COLOR_WHITE_180].a = 180;

    colors[COLOR_WHITE_255].r = 255;
    colors[COLOR_WHITE_255].g = 255;
    colors[COLOR_WHITE_255].b = 255;
    colors[COLOR_WHITE_255].a = 255;

    colors[COLOR_YELLOW_255].r = 255;
    colors[COLOR_YELLOW_255].g = 255;
    colors[COLOR_YELLOW_255].b = 0;
    colors[COLOR_YELLOW_255].a = 255;
}

/**
 * Uses the Midpoint Circle Algorithm to draw a circle with a specified radius and color, centered at (xc, yc),
 * on the given SDL_Renderer. Only draws points within the camera's view and is efficient for small circles.
 *
 * @param renderer The SDL_Renderer to draw on.
 * @param camera The Camera that defines the viewable area.
 * @param xc The x-coordinate of the circle's center (game_scale).
 * @param yc The y-coordinate of the circle's center (game_scale).
 * @param radius The radius of the circle.
 * @param color The SDL_Color to use for drawing the circle.
 *
 * @return void
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
        if (gfx_is_relative_position_in_camera(camera, xc + x, yc + y))
        {
            SDL_RenderDrawPoint(renderer, xc + x, yc + y);
        }
        if (gfx_is_relative_position_in_camera(camera, xc + x, yc - y))
        {
            SDL_RenderDrawPoint(renderer, xc + x, yc - y);
        }
        if (gfx_is_relative_position_in_camera(camera, xc - x, yc + y))
        {
            SDL_RenderDrawPoint(renderer, xc - x, yc + y);
        }
        if (gfx_is_relative_position_in_camera(camera, xc - x, yc - y))
        {
            SDL_RenderDrawPoint(renderer, xc - x, yc - y);
        }
        if (gfx_is_relative_position_in_camera(camera, xc + y, yc + x))
        {
            SDL_RenderDrawPoint(renderer, xc + y, yc + x);
        }
        if (gfx_is_relative_position_in_camera(camera, xc + y, yc - x))
        {
            SDL_RenderDrawPoint(renderer, xc + y, yc - x);
        }
        if (gfx_is_relative_position_in_camera(camera, xc - y, yc + x))
        {
            SDL_RenderDrawPoint(renderer, xc - y, yc + x);
        }
        if (gfx_is_relative_position_in_camera(camera, xc - y, yc - x))
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
 * Draws a circle approximation using a series of bezier curves. This function will only draw segments
 * of the circle that intersect with the viewport defined by `camera` and is efficient for large circles.
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

/**
 * Draws and fills a circle on an SDL renderer using the Midpoint Circle Algorithm.
 *
 * @param renderer The SDL renderer to draw the circle on.
 * @param xc The x-coordinate of the circle's center.
 * @param yc The y-coordinate of the circle's center.
 * @param radius The radius of the circle.
 * @param color And SDL color object.
 *
 * @return None
 */
void gfx_draw_fill_circle(SDL_Renderer *renderer, int xc, int yc, int radius, SDL_Color color)
{
    int x = 0;
    int y = radius;
    int d = 1 - radius;
    int deltaE = 3;
    int deltaSE = -2 * radius + 5;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    while (y >= x)
    {
        SDL_RenderDrawLine(renderer, xc - x, yc + y, xc + x, yc + y);
        SDL_RenderDrawLine(renderer, xc - x, yc - y, xc + x, yc - y);
        SDL_RenderDrawLine(renderer, xc - y, yc + x, xc + y, yc + x);
        SDL_RenderDrawLine(renderer, xc - y, yc - x, xc + y, yc - x);
        if (d < 0)
        {
            d += deltaE;
            deltaE += 2;
            deltaSE += 2;
        }
        else
        {
            d += deltaSE;
            deltaE += 2;
            deltaSE += 4;
            y--;
        }
        x++;
    }
}

/**
 * Draws a cloud of stars for a given Galaxy structure, with optional high definition stars,
 * using a provided Camera structure and scaling factor.
 *
 * @param galaxy A pointer to Galaxy struct.
 * @param camera a pointer to Camera struct.
 * @param gstars_count Number of stars in the galaxy cloud.
 * @param high_definition A boolean to indicate whether to use high definition stars or not.
 * @param scale Scaling factor for the galaxy cloud.
 *
 * @return void
 */
void gfx_draw_galaxy_cloud(Galaxy *galaxy, const Camera *camera, int gstars_count, bool high_definition, long double scale)
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
            if (scale <= (ZOOM_UNIVERSE_MIN / GALAXY_SCALE) + epsilon)
                opacity = 0.35 * star_opacity;
            else if (scale <= 0.000002 + epsilon)
                opacity = 0.5 * star_opacity;
            else
                opacity = star_opacity;
            break;
        case 2:
            if (scale <= (ZOOM_UNIVERSE_MIN / GALAXY_SCALE) + epsilon)
                opacity = 0.5 * star_opacity;
            else
                opacity = star_opacity;
            break;
        case 3:
            if (scale <= (ZOOM_UNIVERSE_MIN / GALAXY_SCALE) + epsilon)
                opacity = 0.8 * star_opacity;
            else
                opacity = star_opacity;
            break;
        default:
            opacity = star_opacity;
        }

        if (high_definition)
        {
            SDL_SetRenderDrawColor(renderer, galaxy->gstars_hd[i].color.r, galaxy->gstars_hd[i].color.g, galaxy->gstars_hd[i].color.b, (int)opacity);

            x = (galaxy->position.x - camera->x + galaxy->gstars_hd[i].position.x / GALAXY_SCALE) * scale * GALAXY_SCALE;
            y = (galaxy->position.y - camera->y + galaxy->gstars_hd[i].position.y / GALAXY_SCALE) * scale * GALAXY_SCALE;
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, galaxy->gstars[i].color.r, galaxy->gstars[i].color.g, galaxy->gstars[i].color.b, (int)opacity);

            x = (galaxy->position.x - camera->x + galaxy->gstars[i].position.x / GALAXY_SCALE) * scale * GALAXY_SCALE;
            y = (galaxy->position.y - camera->y + galaxy->gstars[i].position.y / GALAXY_SCALE) * scale * GALAXY_SCALE;
        }

        SDL_RenderDrawPoint(renderer, x, y);
    }
}

/**
 * Draws a cloud of stars for the menu screen, using a provided Camera structure and an array of Gstar structures.
 *
 * @param camera A pointer to the Camera structure indicating the current viewpoint.
 * @param menustars An array of Gstar structures containing information about the stars in the cloud.
 *
 * @return void
 */
void gfx_draw_menu_galaxy_cloud(const Camera *camera, Gstar *menustars)
{
    int i = 0;
    float scaling_factor = 0.15;

    while (i < MAX_GSTARS && menustars[i].final_star == true)
    {
        int x, y;

        SDL_SetRenderDrawColor(renderer, menustars[i].color.r, menustars[i].color.g, menustars[i].color.b, menustars[i].opacity);

        x = camera->w - camera->w / 4 + (menustars[i].position.x / GALAXY_SCALE) * scaling_factor;
        y = camera->h / 3 + (menustars[i].position.y / GALAXY_SCALE) * scaling_factor;

        SDL_RenderDrawPoint(renderer, x, y);

        i++;
    }
}

/**
 * Draws a screen frame consisting of four rectangles on the renderer.
 * The top and bottom rectangles have a width equal to the camera's width
 * and a height of 2 * PROJECTION_RADIUS. The left and right rectangles have
 * a height equal to the camera's height minus 4 * PROJECTION_RADIUS and a
 * width of 2 * PROJECTION_RADIUS.
 *
 * @param camera A pointer to the Camera struct representing the current camera position and dimensions.
 *
 * @return void
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

/**
 * Draws section lines on the screen for a given camera state and scale.
 *
 * @param camera A pointer to the camera used to draw the section lines.
 * @param state An integer representing the current state of the camera, either MAP or UNIVERSE.
 * @param color An SDL_Color struct representing the color of the section lines.
 * @param scale A long double representing the current zoom scale of the camera.
 *
 * @return void
 */
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

/**
 * Draws an arc around a ship to represent its speed, with a color and opacity based on the ship's velocity.
 * The arc is drawn using three concentric circles, with the center of the arc offset from the center of the ship
 * in the direction of the ship's velocity.
 *
 * @param ship A pointer to a Ship struct that contains the position and velocity of the ship.
 * @param camera A pointer to a Camera struct that contains the position of the camera.
 * @param scale Long double value that represents the scale at which to draw the arc.
 *
 * @return void
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

/**
 * Draws a set of speed lines that represent the velocity of an object.The speed of the object
 * determines the opacity of the lines, and the length of the lines is proportional to the speed
 * of the object. The starting position of the lines is determined by the current camera position.
 * The lines move in the opposite direction of the object's velocity and wrap around to the other
 * side of the screen if they go off-screen.
 *
 * @param velocity The speed of the object.
 * @param camera A pointer to the current camera position.
 * @param speed The current speed of the object.
 *
 * @return void
 */
void gfx_draw_speed_lines(float velocity, const Camera *camera, Speed speed)
{
    if (velocity < 10)
        return;

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
    float velocity_x;
    float velocity_y;

    if (velocity <= 0)
    {
        velocity_x = 0;
        velocity_y = 0;
    }
    else
    {
        velocity_x = speed.vx / velocity;
        velocity_y = speed.vy / velocity;
    }

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

/**
 * Generates a set of randomly placed and sized stars on a given camera view.
 * The function implements lazy initialization of bstars in batches.
 *
 * @param nav_state A pointer to NavigationState object.
 * @param bstars Array of Bstar objects to populate with generated stars.
 * @param camera A pointer to Camera object representing current view.
 * @param lazy_load A boolean indicating whether or not bstars will be lazy-loaded.
 *
 * @return void
 */
void gfx_generate_bstars(GameEvents *game_events, NavigationState *nav_state, Bstar *bstars, const Camera *camera, bool lazy_load)
{
    int row, column, is_star;
    int end = false;
    int max_bstars = (int)(camera->w * camera->h * BSTARS_PER_SQUARE / BSTARS_SQUARE);
    int current_batch = 0;
    static int last_star_index = 0;
    int current_cell = 0;
    static int initialized_cells = 0;
    int i = last_star_index;

    // Use a local rng
    pcg32_random_t rng;

    // Seed with a fixed constant (for size and opacity)
    srand(1200);

    // Set galaxy hash as initseq
    nav_state->initseq = maths_hash_position_to_uint64_2(nav_state->current_galaxy->position);

    // Reset final_star
    if (initialized_cells == 0)
    {
        for (int j = 0; j < max_bstars; j++)
        {
            bstars[j].final_star = false;
        }
    }

    for (row = 0; row < camera->h && !end; row++)
    {
        for (column = 0; column < camera->w && !end; column++)
        {
            if (lazy_load)
            {
                current_cell++;

                if (initialized_cells >= current_cell)
                    continue;

                initialized_cells = current_cell;
            }

            // Create rng seed by combining x,y values
            Point position = {.x = row, .y = column};
            uint64_t seed = maths_hash_position_to_uint64(position);

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

                star.final_star = true;

                if (lazy_load)
                {
                    last_star_index = i;
                    current_batch++;
                }

                bstars[i++] = star;
            }

            if (i >= max_bstars)
                end = true;

            if (lazy_load && current_batch >= BSTARS_BATCH_SIZE)
                return;
        }
    }

    game_events->generate_bstars = false;
    last_star_index = 0;
    initialized_cells = 0;

    // printf("\n Galaxy: %s, initseq: %lu", nav_state->current_galaxy->name, nav_state->initseq);
    // printf("\n Cells checked: %d ::: Stars found: %d", current_cell, i);
    // printf("\n initialized cells: %d", initialized_cells);
    // printf("\n end");
}

/**
 * Generates a collection of stars within a given galaxy object. The number and position of the stars
 * are determined by the size and density of the galaxy. The stars are stored in the galaxy's 'gstars'
 * array for low definition or 'gstars_hd' array for high definition.
 * The function implements lazy initialization of gstars in batches.
 *
 * @param galaxy A pointer to a Galaxy object.
 * @param high_definition A boolean to determine if high definition mode is enabled (1) or not (0).
 *
 * @return This function does not return anything.
 */
void gfx_generate_gstars(Galaxy *galaxy, bool high_definition)
{
    float radius = galaxy->radius;
    double full_size_radius = radius * GALAXY_SCALE;
    full_size_radius -= fmod(full_size_radius, GALAXY_SECTION_SIZE); // zero out any digits below 10,000
    double full_size_diameter = full_size_radius * 2;
    int sections_in_group;
    int total_groups;
    int corrected_radius;
    int initialized;
    int i;

    if (high_definition)
    {
        initialized = galaxy->initialized_hd;
        i = galaxy->last_star_index_hd;
    }
    else
    {
        initialized = galaxy->initialized;
        i = galaxy->last_star_index;
    }

    if ((!high_definition && !galaxy->total_groups) || (high_definition && !galaxy->total_groups_hd))
    {
        // Check whether MAX_GSTARS_ROW * GALAXY_SECTION_SIZE fit in full_size_diameter
        // If they don't fit, we must group sections together
        sections_in_group = 1;
        total_groups = full_size_diameter / (sections_in_group * GALAXY_SECTION_SIZE);

        // Allow <array_factor> times more than array size as galaxy_density is low
        // Increase array_factor to show more stars
        int array_factor = 12;

        while (total_groups > MAX_GSTARS_ROW * array_factor)
        {
            sections_in_group++;
            total_groups = full_size_diameter / (sections_in_group * GALAXY_SECTION_SIZE);
        }

        if (high_definition)
            galaxy->sections_in_group_hd = sections_in_group;
        else
        {
            sections_in_group *= 2;
            galaxy->sections_in_group = sections_in_group;
        }

        // Make sure that full_size_radius can be divided by <section_size>
        corrected_radius = full_size_radius;

        while (fmod(corrected_radius, sections_in_group * GALAXY_SECTION_SIZE) != 0)
            corrected_radius += GALAXY_SECTION_SIZE;

        // Total groups to check
        if (high_definition)
            galaxy->total_groups_hd = ((2 * corrected_radius / (sections_in_group * GALAXY_SECTION_SIZE)) + 1) *
                                      ((2 * corrected_radius / (sections_in_group * GALAXY_SECTION_SIZE)) + 1);
        else
            galaxy->total_groups = ((2 * corrected_radius / (sections_in_group * GALAXY_SECTION_SIZE)) + 1) *
                                   ((2 * corrected_radius / (sections_in_group * GALAXY_SECTION_SIZE)) + 1);
    }
    else
    {
        if (high_definition)
        {
            sections_in_group = galaxy->sections_in_group_hd;
            total_groups = galaxy->total_groups_hd;
        }
        else
        {
            sections_in_group = galaxy->sections_in_group;
            total_groups = galaxy->total_groups;
        }

        // Make sure that full_size_radius can be divided by <section_size>
        corrected_radius = full_size_radius;

        while (fmod(corrected_radius, sections_in_group * GALAXY_SECTION_SIZE) != 0)
            corrected_radius += GALAXY_SECTION_SIZE;
    }

    int section_size = sections_in_group * GALAXY_SECTION_SIZE;
    double ix, iy;
    int current_group = 0;
    int current_batch = 0;

    // Use a local rng
    pcg32_random_t rng;

    // Set galaxy hash as initseq
    uint64_t initseq = maths_hash_position_to_uint64_2(galaxy->position);

    // Density scaling parameter
    double a = galaxy->radius * GALAXY_SCALE / 2.0f;

    for (ix = -corrected_radius; ix <= corrected_radius; ix += section_size)
    {
        for (iy = -corrected_radius; iy <= corrected_radius; iy += section_size)
        {
            current_group++;

            if (initialized >= current_group)
                continue;

            initialized = current_group;

            if (high_definition)
                galaxy->initialized_hd = initialized;
            else
                galaxy->initialized = initialized;

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

                // Calculate opacity
                double distance = stars_nearest_center_distance(position, galaxy, initseq, GALAXY_CLOUD_DENSITY);
                unsigned short class = stars_size_class(distance);
                float class_opacity_max = class * (255 / 6);
                class_opacity_max = class_opacity_max > 255 ? 255 : class_opacity_max;
                float class_opacity_min = class_opacity_max - (255 / 6);
                int opacity = (abs(pcg32_random_r(&rng)) % (int)class_opacity_max + (int)class_opacity_min);
                star.opacity = opacity;
                star.opacity = star.opacity < 0 ? 0 : star.opacity;

                // Calculate color
                unsigned short color_code;

                switch (class)
                {
                case STAR_CLASS_1:
                    color_code = COLOR_LIGHT_RED_255;
                    break;
                case STAR_CLASS_2:
                    color_code = COLOR_LIGHT_ORANGE_255;
                    break;
                case STAR_CLASS_3:
                    color_code = COLOR_PALE_YELLOW_255;
                    break;
                case STAR_CLASS_4:
                    color_code = COLOR_LIGHT_GREEN_255;
                    break;
                case STAR_CLASS_5:
                    color_code = COLOR_LIGHT_BLUE_255;
                    break;
                case STAR_CLASS_6:
                    color_code = COLOR_LAVENDER_255;
                    break;
                default:
                    color_code = COLOR_LIGHT_RED_255;
                    break;
                }

                star.color = colors[color_code];

                star.final_star = true;

                if (high_definition)
                {
                    galaxy->last_star_index_hd = i;
                    galaxy->gstars_hd[i++] = star;
                }
                else
                {
                    galaxy->last_star_index = i;
                    galaxy->gstars[i++] = star;
                }

                current_batch++;
            }

            if (current_batch >= BSTARS_BATCH_SIZE)
                return;
        }
    }

    // printf("\n Name: %s ::: hi_def: %d", galaxy->name, high_definition);
    // printf("\n Groups checked: %d ::: Stars found:%d", current_group, i);
    // printf("\n Total groups to check: %d ::: Sections in group: %d", total_groups, sections_in_group);
    // printf("\n initialized: %d", initialized);
    // printf("\n last_star_index: %d ::: last_star_index_hd: %d", galaxy->last_star_index, galaxy->last_star_index_hd);
    // printf("\n end: %d", current_batch);
}

/**
 * Generates menu stars for a given galaxy.
 *
 * @param galaxy A pointer to the Galaxy object to generate menu stars for.
 * @param menustars A pointer to the array of Gstar objects to store the generated stars in.
 *
 * @return void
 */
void gfx_generate_menu_gstars(Galaxy *galaxy, Gstar *menustars)
{
    // Initialize menustars
    for (int j = 0; j < MAX_GSTARS; j++)
    {
        menustars[j].position.x = 0;
        menustars[j].position.y = 0;
        menustars[j].opacity = 0;
        menustars[j].final_star = false;
        menustars[j].color = (SDL_Color){0, 0, 0, 0};
    }

    float radius = galaxy->radius;
    double full_size_radius = radius * GALAXY_SCALE;
    full_size_radius -= fmod(full_size_radius, GALAXY_SECTION_SIZE); // zero out any digits below 10,000
    double full_size_diameter = full_size_radius * 2;

    // Check whether MAX_GSTARS_ROW * GALAXY_SECTION_SIZE fit in full_size_diameter
    // If they don't fit, we must group sections together
    int sections_in_group = 1;
    double total_sections = full_size_diameter / (sections_in_group * GALAXY_SECTION_SIZE);

    // Allow <array_factor> times more than array size as galaxy_density is low
    // Increase array_factor to show more stars
    int array_factor = 12;

    while (total_sections > MAX_GSTARS_ROW * array_factor)
    {
        sections_in_group++;
        total_sections = full_size_diameter / (sections_in_group * GALAXY_SECTION_SIZE);
    }

    int section_size = sections_in_group * GALAXY_SECTION_SIZE;
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

                // Calculate opacity
                double distance = stars_nearest_center_distance(position, galaxy, initseq, MENU_GALAXY_CLOUD_DENSITY);
                unsigned short class = stars_size_class(distance);
                float class_opacity_max = class * (255 / 6) + 20; // There are only a few Class 6 galaxies, increase max value by 20
                class_opacity_max = class_opacity_max > 255 ? 255 : class_opacity_max;
                float class_opacity_min = class_opacity_max - (255 / 6);
                int opacity = (abs(pcg32_random_r(&rng)) % (int)class_opacity_max + (int)class_opacity_min);
                star.opacity = opacity * (1 - pow(distance_from_center / (galaxy->radius * GALAXY_SCALE), 3));
                star.opacity = star.opacity < 0 ? 0 : star.opacity;

                // Calculate color
                unsigned short color_code;

                switch (class)
                {
                case STAR_CLASS_1:
                    color_code = COLOR_LIGHT_RED_255;
                    break;
                case STAR_CLASS_2:
                    color_code = COLOR_LIGHT_ORANGE_255;
                    break;
                case STAR_CLASS_3:
                    color_code = COLOR_PALE_YELLOW_255;
                    break;
                case STAR_CLASS_4:
                    color_code = COLOR_LIGHT_GREEN_255;
                    break;
                case STAR_CLASS_5:
                    color_code = COLOR_LIGHT_BLUE_255;
                    break;
                case STAR_CLASS_6:
                    color_code = COLOR_LAVENDER_255;
                    break;
                default:
                    color_code = COLOR_LIGHT_RED_255;
                    break;
                }

                star.color = colors[color_code];

                star.final_star = true;
                menustars[i++] = star;
            }
        }
    }
}

/**
 * Checks if an object with a given position and radius is within the bounds of the camera.
 *
 * @param camera The camera to check against.
 * @param x The x-coordinate of the object's center.
 * @param y The y-coordinate of the object's center.
 * @param radius The radius of the object.
 * @param scale The scaling factor applied to the camera view.
 *
 * @return True if the object is within the camera bounds, false otherwise.
 */
bool gfx_is_object_in_camera(const Camera *camera, double x, double y, float radius, long double scale)
{
    return x + radius >= camera->x && x - radius - camera->x < camera->w / scale &&
           y + radius >= camera->y && y - radius - camera->y < camera->h / scale;
}

/**
 * Determines if a relative position (x,y) is within the bounds of the camera.
 *
 * @param camera A pointer to the Camera object.
 * @param x The x-coordinate of the position relative to the camera.
 * @param y The y-coordinate of the position relative to the camera.
 *
 * @return True if the position is within the camera bounds, false otherwise.
 */
bool gfx_is_relative_position_in_camera(const Camera *camera, int x, int y)
{
    return x >= 0 && x < camera->w && y >= 0 && y < camera->h;
}

/**
 * Projects the given CelestialBody onto the edge of the screen.
 *
 * @param game_state A pointer to the current GameState.
 * @param nav_state A pointer to the current NavigationState.
 * @param body A pointer to the CelestialBody to be projected.
 * @param camera A pointer to the current Camera.
 *
 * @return void
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

/**
 * Projects the given Galaxy object onto the edge of the given Camera object, with the given state and scale.
 *
 * @param state An integer representing the current state of the game.
 * @param nav_state A pointer to a NavigationState object representing the current state of the navigation system.
 * @param galaxy A pointer to a Galaxy object to be projected.
 * @param camera A pointer to a Camera object representing the current camera position and size.
 * @param scale A long double representing the current game scale.
 *
 * @return void
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

/**
 * Draw the ship projection on the edge of the screen, taking into account the current camera position and zoom level.
 *
 * @param state An integer representing the current game state (NAVIGATE or MAP).
 * @param input_state A pointer to the current InputState struct.
 * @param nav_state A pointer to the current NavigationState struct.
 * @param ship A pointer to the Ship struct to be drawn.
 * @param camera A pointer to the current Camera struct.
 * @param scale A long double representing the current zoom level.
 *
 * @return void
 */
void gfx_project_ship_on_edge(int state, const InputState *input_state, const NavigationState *nav_state, Ship *ship, const Camera *camera, long double scale)
{
    gfx_update_projection_position(nav_state, ship, ENTITY_SHIP, camera, state, scale);

    // Mirror ship angle
    ship->projection->angle = ship->angle;

    // Draw ship projection
    SDL_RenderCopyEx(renderer, ship->projection->texture, &ship->projection->main_img_rect, &ship->projection->rect, ship->projection->angle, &ship->projection->rotation_pt, SDL_FLIP_NONE);

    // Draw projection thrust
    if (state == NAVIGATE && input_state->thrust_on)
        SDL_RenderCopyEx(renderer, ship->projection->texture, &ship->projection->thrust_img_rect, &ship->projection->rect, ship->projection->angle, &ship->projection->rotation_pt, SDL_FLIP_NONE);

    // Draw projection reverse
    if (state == NAVIGATE && input_state->reverse_on)
        SDL_RenderCopyEx(renderer, ship->projection->texture, &ship->projection->reverse_img_rect, &ship->projection->rect, ship->projection->angle, &ship->projection->rotation_pt, SDL_FLIP_NONE);
}

/**
 * Checks if the mouse is over the current galaxy and toggles the variable input_state.is_hovering_galaxy.
 *
 * @param input_state A pointer to the current InputState.
 * @param nav_state A pointer to the current NavigationState.
 * @param camera A pointer to the current Camera object.
 * @param scale The scaling factor applied to the camera view.
 *
 * @return void
 */
void gfx_toggle_galaxy_hover(InputState *input_state, const NavigationState *nav_state, const Camera *camera, long double scale)
{
    // Get relative position of current galaxy in game_scale
    int current_cutoff = nav_state->current_galaxy->cutoff * scale * GALAXY_SCALE;
    int current_x = (nav_state->current_galaxy->position.x - camera->x) * scale * GALAXY_SCALE;
    int current_y = (nav_state->current_galaxy->position.y - camera->y) * scale * GALAXY_SCALE;

    // Get current galaxy distance from mouse position
    double distance = maths_distance_between_points(current_x, current_y, input_state->mouse_position.x, input_state->mouse_position.y);

    if (distance > current_cutoff)
    {
        input_state->is_hovering_galaxy = false;
    }
    else
    {
        input_state->is_hovering_galaxy = true;
    }
}

/**
 * Checks if the mouse is over the current star and toggles the variable input_state.is_hovering_star.
 *
 * @param input_state A pointer to the current InputState.
 * @param nav_state A pointer to the current NavigationState.
 * @param camera A pointer to the current Camera object.
 * @param scale The scaling factor applied to the camera view.
 *
 * @return void
 */
void gfx_toggle_star_hover(InputState *input_state, const NavigationState *nav_state, const Camera *camera, long double scale, int state)
{
    // Get relative position of current star in game_scale
    int current_cutoff = nav_state->current_star->cutoff * scale;
    int current_x = 0;
    int current_y = 0;

    if (state == MAP)
    {
        current_x = (nav_state->current_star->position.x - camera->x) * scale;
        current_y = (nav_state->current_star->position.y - camera->y) * scale;
    }
    else if (state == UNIVERSE)
    {
        current_x = (nav_state->current_galaxy->position.x - camera->x + nav_state->current_star->position.x / GALAXY_SCALE) * scale * GALAXY_SCALE;
        current_y = (nav_state->current_galaxy->position.y - camera->y + nav_state->current_star->position.y / GALAXY_SCALE) * scale * GALAXY_SCALE;
    }

    // Get current star distance from mouse position
    double distance = maths_distance_between_points(current_x, current_y, input_state->mouse_position.x, input_state->mouse_position.y);

    if (distance > current_cutoff)
        input_state->is_hovering_star = false;
    else
    {
        input_state->is_hovering_star = true;

        if (state == UNIVERSE)
            input_state->is_hovering_galaxy = false;
    }
}

/**
 * Updates the position and opacity of background stars.
 *
 * @param state The current game state.
 * @param camera_on Whether or not the camera is turned on.
 * @param nav_state The current navigation state.
 * @param bstars An array of background stars.
 * @param camera The current camera object.
 * @param speed The current speed object.
 * @param distance The distance from the current galaxy center.
 *
 * @return void
 */
void gfx_update_bstars_position(int state, bool camera_on, const NavigationState *nav_state, Bstar *bstars, const Camera *camera, Speed speed, double distance)
{
    int i = 0;
    int max_bstars = (int)(camera->w * camera->h * BSTARS_PER_SQUARE / BSTARS_SQUARE);
    float max_distance = 2 * nav_state->current_galaxy->radius * GALAXY_SCALE;

    while (i < max_bstars && bstars[i].final_star == true)
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

/**
 * Updates the camera position and scale.
 *
 * @param camera A pointer to the Camera struct to be updated.
 * @param position The position of the camera center.
 * @param scale The scaling factor applied to the camera view.
 *
 * @return void
 */
void gfx_update_camera(Camera *camera, Point position, long double scale)
{
    camera->x = position.x - (camera->w / 2) / scale;
    camera->y = position.y - (camera->h / 2) / scale;
}

/**
 * Updates the position of all galaxy stars on the screen based on the current camera view.
 *
 * @param galaxy A pointer to a Galaxy struct representing the galaxy.
 * @param ship_position The position of the player's ship in the galaxy.
 * @param camera A pointer to a Camera struct representing the current camera view.
 * @param distance The distance between the player's ship and the center of the galaxy.
 * @param limit The maximum distance at which stars will be drawn.
 *
 * @return void
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

    while (i < MAX_GSTARS && galaxy->gstars_hd[i].final_star == true)
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

        SDL_SetRenderDrawColor(renderer, galaxy->gstars_hd[i].color.r, galaxy->gstars_hd[i].color.g, galaxy->gstars_hd[i].color.b, (unsigned short)opacity);
        SDL_RenderDrawPoint(renderer, x, y);

        i++;
    }
}

/**
 * Calculates the opacity of a given projection based on its distance from the camera.
 * The opacity is determined by dividing the distance by the section size and mapping
 * the result to one of several opacity ranges based on the corresponding section.
 * The opacity gradually fades from 255 to 0 as the distance increases.
 *
 * @param distance The distance from the projection to the camera.
 * @param region_size The size of the projection region.
 * @param section_size The size of each section in the projection.
 *
 * @return The opacity value for the projection.
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

/**
 * Updates the projection position of a given entity based on the current navigation state, camera position,
 * scale, and screen quadrant. Finds the screen quadrant for the entity's exit from the screen.
 *
 * @param nav_state The current navigation state.
 * @param ptr A pointer to the entity to update the projection position of.
 * @param entity_type The type of entity to update the projection position of.
 * @param camera The current camera.
 * @param state The current state of the program.
 * @param scale The scale to apply to the projection.
 *
 * @return void
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
            if (state == MAP && !maths_points_equal(nav_state->current_galaxy->position, nav_state->buffer_galaxy->position))
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

/**
 * Zooms in or out a CelestialBody and its children in a star system. The function updates the position
 * and size of a CelestialBody rectangle, given a zoom scale. It also recursively zooms in or out its
 * children CelestialBodies.
 *
 * @param body A pointer to the CelestialBody to zoom.
 * @param scale The zoom scale to apply to the body and its children.
 *
 * @return: void
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

/*
 * graphics.c
 */

#include <math.h>
#include <stdbool.h>
#include <limits.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "../lib/pcg-c-basic-0.9/pcg_basic.h"

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"
#include "../include/graphics.h"

// External variable definitions
extern TTF_Font *fonts[];
extern SDL_Renderer *renderer;
extern SDL_Color colors[];

// Static function prototypes
static void gfx_calculate_path_segment(double cx, double cy, double px, double py, double radius, int direction, double *x, double *y);
static bool gfx_has_line_of_sight(NavigationState *, double x1, double y1, double x2, double y2, Star *stars[], int max_stars);
static void gfx_move_point_on_circumference(NavigationState *, Point *, Star *stars[], int max_stars);
static void gfx_normalize_waypoint_path(NavigationState *, PathPoint path[], int total_points);
static void gfx_shift_path_segment(NavigationState *, Point, Star *stars[], int max_stars, double *x, double *y);
static int gfx_update_projection_opacity(double distance, int region_size, int section_size);
static void gfx_update_projection_position(const NavigationState *, void *ptr, int entity_type, const Camera *, int state, long double scale);

/**
 * Takes a point outside a circle and calculates a new point at <step> distance that is closer to the circle
 * in a clockwise or counter-clockwise direction.
 *
 * @param cx The x-coordinate of the star's center.
 * @param cy The y-coordinate of the star's center.
 * @param px The x-coordinate of the point outside the star.
 * @param py The y-coordinate of the point outside the star.
 * @param radius The cutoff of the star.
 * @param direction Whether the direction of the new point is clockwise to the original point or counter-clockwise.
 * @param x The x-coordinate of the new point.
 * @param y The y-coordinate of the new point.
 *
 * @return void
 */
static void gfx_calculate_path_segment(double cx, double cy, double px, double py, double radius, int direction, double *x, double *y)
{
    double step = radius * 0.1;
    double dist = sqrt((px - cx) * (px - cx) + (py - cy) * (py - cy));
    double mx = (px + cx) / 2.0;
    double my = (py + cy) / 2.0;
    double d = sqrt(radius * radius - dist * dist / 4.0);
    double dx = cx - px;
    double dy = cy - py;
    double perpendicular_x = dy / dist * d;
    double perpendicular_y = -dx / dist * d;

    if (direction == 1)
    {
        *x = mx + perpendicular_x;
        *y = my + perpendicular_y;
    }
    else
    {
        *x = mx - perpendicular_x;
        *y = my - perpendicular_y;
    }

    double scale = step / sqrt((*x - px) * (*x - px) + (*y - py) * (*y - py));
    *x = px + (*x - px) * scale;
    *y = py + (*y - py) * scale;
}

/**
 * Calculates a path between the current position and the waypoint star.
 *
 * @param nav_state A pointer to the current NavigationState object.
 *
 * @return void
 */
void gfx_calculate_waypoint_path(NavigationState *nav_state)
{
    if (nav_state->waypoint_star->waypoint_points > 0 && nav_state->waypoint_planet_index >= 0)
    {
        // Update last point in path (planet)
        if (nav_state->waypoint_star->waypoint_path != NULL)
            nav_state->waypoint_star->waypoint_path[nav_state->waypoint_star->waypoint_points - 1].position =
                nav_state->waypoint_star->planets[nav_state->waypoint_planet_index]->position;

        return;
    }
    else if (nav_state->waypoint_star->waypoint_points > 0)
        return;

    double buffer_x = nav_state->buffer_star->position.x;
    double buffer_y = nav_state->buffer_star->position.y;
    double start_x = nav_state->navigate_offset.x;
    double start_y = nav_state->navigate_offset.y;
    double dest_x;
    double dest_y;

    if (nav_state->waypoint_planet_index >= 0)
    {
        dest_x = nav_state->waypoint_star->planets[nav_state->waypoint_planet_index]->position.x;
        dest_y = nav_state->waypoint_star->planets[nav_state->waypoint_planet_index]->position.y;
    }
    else
    {
        dest_x = nav_state->waypoint_star->position.x;
        dest_y = nav_state->waypoint_star->position.y;
    }

    double dest_cutoff = nav_state->waypoint_star->cutoff;
    double x = start_x;
    double y = start_y;
    int direction;
    double degrees;
    int total_points = 0;

    // 1. Add starting position to path
    PathPoint *raw_path;

    raw_path = (PathPoint *)malloc(sizeof(PathPoint));
    raw_path[total_points].type = PATH_POINT_STRAIGHT;
    raw_path[total_points].position.x = start_x;
    raw_path[total_points].position.y = start_y;
    total_points++;

    // 2. Move starting position outside star cutoff if needed
    if (maths_is_point_in_circle((Point){start_x, start_y},
                                 (Point){buffer_x, buffer_y},
                                 nav_state->buffer_star->cutoff))
    {
        degrees = 0;

        direction = maths_get_rotation_direction(buffer_x, buffer_y,
                                                 start_x, start_y,
                                                 dest_x, dest_y);

        // Move Starting position just outside the cutoff
        double radius_ratio = 1.01;
        maths_closest_point_outside_circle(buffer_x, buffer_y,
                                           nav_state->buffer_star->cutoff,
                                           radius_ratio,
                                           start_x, start_y,
                                           &x, &y,
                                           degrees);

        // Check for nearest star in nav_state->stars, so that the starting position does not end up in another star
        Star *nearest_star = stars_nearest_star_in_nav_state(nav_state, (Point){x, y}, true);

        if (nearest_star != NULL && strcmp(nearest_star->name, nav_state->waypoint_star->name) != 0)
        {
            // Check if new starting position is inside nearest star
            while (maths_is_point_in_circle((Point){x, y}, (Point){nearest_star->position.x, nearest_star->position.y}, nearest_star->cutoff))
            {
                degrees = degrees + (direction * 30);

                // Move Starting position by <degrees>
                maths_closest_point_outside_circle(buffer_x, buffer_y, nav_state->buffer_star->cutoff, radius_ratio,
                                                   start_x, start_y,
                                                   &x, &y,
                                                   degrees);
            }
        }

        if (x != start_x && y != start_y)
        {
            // Add new starting position outside circle to path
            raw_path = (PathPoint *)realloc(raw_path, (total_points + 1) * sizeof(PathPoint));
            raw_path[total_points].type = PATH_POINT_STRAIGHT;
            raw_path[total_points].position.x = x;
            raw_path[total_points].position.y = y;
            total_points++;
        }
    }

    // 3. Calculate path
    double _x = x;
    double _y = y;
    double bx, by;

    bool path_reached_waypoint = maths_is_point_in_circle((Point){_x, _y}, (Point){dest_x, dest_y}, dest_cutoff) ||
                                 maths_line_intersects_circle(x, y, _x, _y, dest_x, dest_y, dest_cutoff);

    while (!path_reached_waypoint)
    {
        unsigned short segment_type = PATH_POINT_STRAIGHT;

        // Move point (_x,_y) closer to waypoint
        double step = GALAXY_SECTION_SIZE * 0.5;
        maths_move_point_along_line(x, y, dest_x, dest_y, step, &_x, &_y);

        // Find nearest stars to (_x,_y)
        Star *nearest_stars[MAX_NEAREST_STARS];

        for (int i = 0; i < MAX_NEAREST_STARS; i++)
            nearest_stars[i] = NULL;

        bx = maths_get_nearest_section_line(_x, GALAXY_SECTION_SIZE);
        by = maths_get_nearest_section_line(_y, GALAXY_SECTION_SIZE);
        int nearest_stars_count = stars_nearest_stars_to_point(nav_state, (Point){bx, by}, nearest_stars);

        if (nearest_stars_count > 0)
        {
            for (int s = 0; s < nearest_stars_count; s++)
            {
                if (nearest_stars[s] != NULL && strcmp(nearest_stars[s]->name, nav_state->waypoint_star->name) != 0)
                {
                    direction = maths_get_rotation_direction(nearest_stars[s]->position.x, nearest_stars[s]->position.y,
                                                             x, y,
                                                             dest_x, dest_y);

                    bool star_obstructs_path = maths_is_point_in_circle((Point){_x, _y},
                                                                        (Point){nearest_stars[s]->position.x, nearest_stars[s]->position.y},
                                                                        nearest_stars[s]->cutoff) ||
                                               maths_line_intersects_circle(x, y,
                                                                            _x, _y,
                                                                            nearest_stars[s]->position.x, nearest_stars[s]->position.y,
                                                                            nearest_stars[s]->cutoff);

                    if (star_obstructs_path)
                    {
                        segment_type = PATH_POINT_TURN;

                        gfx_calculate_path_segment(nearest_stars[s]->position.x, nearest_stars[s]->position.y,
                                                   x, y,
                                                   nearest_stars[s]->cutoff,
                                                   direction,
                                                   &_x, &_y);

                        gfx_shift_path_segment(nav_state, (Point){x, y}, nearest_stars, nearest_stars_count, &_x, &_y);

                        x = _x;
                        y = _y;
                    }
                }
            }
        }

        x = _x;
        y = _y;

        // Add point to path
        raw_path = (PathPoint *)realloc(raw_path, (total_points + 1) * sizeof(PathPoint));
        raw_path[total_points].type = segment_type;
        raw_path[total_points].position.x = x;
        raw_path[total_points].position.y = y;
        total_points++;

        path_reached_waypoint = maths_is_point_in_circle((Point){_x, _y}, (Point){dest_x, dest_y}, dest_cutoff) ||
                                maths_line_intersects_circle(x, y, _x, _y, dest_x, dest_y, dest_cutoff);
    }

    // 4. Add waypoint star position to path
    raw_path = (PathPoint *)realloc(raw_path, (total_points + 1) * sizeof(PathPoint));
    raw_path[total_points].type = PATH_POINT_STRAIGHT;
    raw_path[total_points].position.x = dest_x;
    raw_path[total_points].position.y = dest_y;
    total_points++;

    // 5. Normalize path
    gfx_normalize_waypoint_path(nav_state, raw_path, total_points);

    // 6. Clean up raw path
    free(raw_path);
}

/*
 * Sets the default color values for the color array.
 *
 * @return void
 */
void gfx_create_default_colors(void)
{
    colors[COLOR_CYAN_70] = (SDL_Color){0, 255, 255, 70};
    colors[COLOR_CYAN_100] = (SDL_Color){0, 255, 255, 100};
    colors[COLOR_CYAN_150] = (SDL_Color){0, 255, 255, 150};

    colors[COLOR_MAGENTA_70] = (SDL_Color){255, 0, 255, 70};
    colors[COLOR_MAGENTA_100] = (SDL_Color){255, 0, 255, 100};
    colors[COLOR_MAGENTA_120] = (SDL_Color){255, 0, 255, 120};

    colors[COLOR_ORANGE_32] = (SDL_Color){255, 165, 0, 32};

    colors[COLOR_WHITE_100] = (SDL_Color){255, 255, 255, 100};
    colors[COLOR_WHITE_140] = (SDL_Color){255, 255, 255, 140};
    colors[COLOR_WHITE_180] = (SDL_Color){255, 255, 255, 180};
    colors[COLOR_WHITE_255] = (SDL_Color){255, 255, 255, 255};

    // Stars
    colors[COLOR_STAR_1] = (SDL_Color){255, 165, 165, 255};
    colors[COLOR_STAR_2] = (SDL_Color){255, 192, 128, 255};
    colors[COLOR_STAR_3] = (SDL_Color){255, 255, 192, 255};
    colors[COLOR_STAR_4] = (SDL_Color){192, 255, 192, 255};
    colors[COLOR_STAR_5] = (SDL_Color){192, 192, 255, 255};
    colors[COLOR_STAR_6] = (SDL_Color){224, 176, 255, 255};

    // Terrestrial planet
    colors[COLOR_PLANET_1] = (SDL_Color){150, 150, 150, 255};

    // Earth, Super-Earth
    colors[COLOR_PLANET_2] = (SDL_Color){93, 148, 217, 255};

    // Sub-Neptune
    colors[COLOR_PLANET_3] = (SDL_Color){221, 188, 157, 255};

    // Neptune-like
    colors[COLOR_PLANET_4] = (SDL_Color){127, 193, 189, 255};

    // Ice giant
    colors[COLOR_PLANET_5] = (SDL_Color){217, 195, 236, 255};

    // Gas giant
    colors[COLOR_PLANET_6] = (SDL_Color){237, 179, 136, 255};

    // Rocky moon
    colors[COLOR_MOON_1] = (SDL_Color){150, 150, 150, 255};

    // Icy moon
    colors[COLOR_MOON_2] = (SDL_Color){179, 229, 252, 255};

    // Volcanic moon
    colors[COLOR_MOON_3] = (SDL_Color){255, 176, 140, 255};

    // Green
    colors[COLOR_GREEN] = (SDL_Color){0, 255, 0, 150};

    // Red
    colors[COLOR_RED] = (SDL_Color){255, 99, 71, 150};
}

/**
 * Draws a button with the specified text and color at the given position and size.
 *
 * @param text The text to display on the button (max size 64).
 * @param font_size The font size to use for the text.
 * @param rect The rectangle representing the position and size of the button.
 * @param button_color The color to use for the button background.
 * @param text_color The color to use for the text.
 *
 * @return void
 */
void gfx_draw_button(char *text, unsigned short font_size, SDL_Rect rect, SDL_Color button_color, SDL_Color text_color)
{
    // Text
    SDL_SetRenderDrawColor(renderer, button_color.r, button_color.g, button_color.b, button_color.a);
    char button_text[64];
    memset(button_text, 0, sizeof(button_text));
    sprintf(button_text, "%s", text);

    // Draw button rect
    SDL_RenderFillRect(renderer, &rect);

    // Text texture
    SDL_Surface *surface = TTF_RenderText_Blended(fonts[font_size], button_text, text_color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect texture_rect;
    texture_rect.w = surface->w;
    texture_rect.h = surface->h;
    texture_rect.x = rect.x + (rect.w - texture_rect.w) / 2;
    texture_rect.y = rect.y + (rect.h - texture_rect.h) / 2;

    // Draw text
    SDL_RenderCopy(renderer, texture, NULL, &texture_rect);

    // Clean-up
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

/**
 * Uses the Midpoint Circle Algorithm to draw a circle with a specified radius and color, centered at (xc, yc),
 * on the given SDL_Renderer. Only draws points within the camera's view and is efficient for small circles.
 *
 * @param renderer The SDL_Renderer to draw on.
 * @param camera A pointer to the current Camera object.
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
 * @param renderer The renderer to use to draw the circle.
 * @param camera A pointer to the current Camera object.
 * @param x The x coordinate of the center of the circle.
 * @param y The y coordinate of the center of the circle.
 * @param r The radius of the circle.
 * @param color The color to use when drawing the circle.
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
 * Draws a diamond shape.
 *
 * @param renderer The renderer to draw the diamond on.
 * @param x The x-coordinate of the center point of the diamond.
 * @param y The y-coordinate of the center point of the diamond.
 * @param size The size of the diamond (the distance from the center point to each corner).
 * @param color The color of the diamond.
 *
 * @return void
 */

void gfx_draw_diamond(SDL_Renderer *renderer, int x, int y, int size, SDL_Color color)
{
    // Calculate the corner points of the diamond
    SDL_Point points[5];
    points[0].x = x;
    points[0].y = y - size;
    points[1].x = x + size;
    points[1].y = y;
    points[2].x = x;
    points[2].y = y + size;
    points[3].x = x - size;
    points[3].y = y;
    points[4].x = x;
    points[4].y = y - size;

    // Draw the outline of the diamond
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLines(renderer, points, 5);
}

/**
 * Draws a cloud of stars for a given Galaxy structure, with optional high definition stars,
 * using a provided Camera structure and scaling factor.
 *
 * @param galaxy A pointer to Galaxy struct.
 * @param camera A pointer to the current Camera object.
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
                opacity = 0.7 * star_opacity;
            else if (scale <= 0.000002 + epsilon)
                opacity = 0.8 * star_opacity;
            else
                opacity = star_opacity;
            break;
        case 2:
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
 * Draws a diamond shape and fills it with color.
 *
 * @param renderer The renderer to draw the diamond on.
 * @param x The x-coordinate of the center point of the diamond.
 * @param y The y-coordinate of the center point of the diamond.
 * @param size The size of the diamond (the distance from the center point to each corner).
 * @param color The color of the diamond.
 *
 * @return void
 */
void gfx_draw_fill_diamond(SDL_Renderer *renderer, int x, int y, int size, SDL_Color color)
{
    // Calculate the corner points of the diamond
    SDL_Point points[5];
    points[0].x = x;
    points[0].y = y - size;
    points[1].x = x + size;
    points[1].y = y;
    points[2].x = x;
    points[2].y = y + size;
    points[3].x = x - size;
    points[3].y = y;
    points[4].x = x;
    points[4].y = y - size;

    // Draw the outline of the diamond
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLines(renderer, points, 5);

    // Fill the diamond with color using scanlines
    int min_y = y - size;
    int max_y = y + size;

    for (int y = min_y; y <= max_y; y++)
    {
        int x_left = x;
        int x_right = x;

        // Find the intersection points of the scanline with the edges of the diamond
        for (int i = 0; i < 4; i++)
        {
            SDL_Point p1 = points[i];
            SDL_Point p2 = points[i + 1];

            if ((p1.y <= y && p2.y > y) || (p2.y <= y && p1.y > y))
            {
                double slope = (double)(p2.x - p1.x) / (p2.y - p1.y);
                int x_int = (int)(p1.x + slope * (y - p1.y));

                if (x_int < x_left)
                {
                    x_left = x_int;
                }

                if (x_int > x_right)
                {
                    x_right = x_int;
                }
            }
        }

        // Draw the scanline segment
        if (x_left <= x_right)
        {
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            SDL_RenderDrawLine(renderer, x_left, y, x_right, y);
        }
    }
}

/**
 * Draws a cloud of stars for the menu screen, using a provided Camera structure and an array of Gstar structures.
 *
 * @param camera A pointer to the current Camera object.
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
 * @param camera A pointer to the current Camera object.
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
 * @param camera A pointer to the current Camera object.
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
        if (GALAXY_SECTION_SIZE == 10000 && scale < 0.01 + epsilon)
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
 * @param camera A pointer to the current Camera object.
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
 * @param camera A pointer to the current Camera object.
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
    const int speed_limit = 5 * BASE_SPEED_LIMIT;
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
 * Draws the waypoint path for a star.
 *
 * @param game_state A pointer to the current GameState object.
 * @param nav_state A pointer to the current NavigationState object.
 * @param camera A pointer to the current Camera object.
 *
 * @return void
 */
void gfx_draw_waypoint_path(const GameState *game_state, const NavigationState *nav_state, const Camera *camera)
{
    if (game_state->state == MAP || game_state->state == UNIVERSE)
        SDL_SetRenderDrawColor(renderer, nav_state->waypoint_star->color.r, nav_state->waypoint_star->color.g, nav_state->waypoint_star->color.b, 80);
    else if (game_state->state == NAVIGATE)
        SDL_SetRenderDrawColor(renderer, nav_state->waypoint_star->color.r, nav_state->waypoint_star->color.g, nav_state->waypoint_star->color.b, 50);

    if (nav_state->waypoint_star->waypoint_points < 1)
        return;

    for (int i = 1; i < nav_state->waypoint_star->waypoint_points; i++)
    {
        double x1, y1;
        double x2, y2;

        if (game_state->state == MAP || game_state->state == NAVIGATE)
        {
            x1 = (nav_state->waypoint_star->waypoint_path[i - 1].position.x - camera->x) * game_state->game_scale;
            y1 = (nav_state->waypoint_star->waypoint_path[i - 1].position.y - camera->y) * game_state->game_scale;
            x2 = (nav_state->waypoint_star->waypoint_path[i].position.x - camera->x) * game_state->game_scale;
            y2 = (nav_state->waypoint_star->waypoint_path[i].position.y - camera->y) * game_state->game_scale;
        }
        else if (game_state->state == UNIVERSE)
        {
            x1 = (nav_state->current_galaxy->position.x - camera->x + nav_state->waypoint_star->waypoint_path[i - 1].position.x / GALAXY_SCALE) * game_state->game_scale * GALAXY_SCALE;
            y1 = (nav_state->current_galaxy->position.y - camera->y + nav_state->waypoint_star->waypoint_path[i - 1].position.y / GALAXY_SCALE) * game_state->game_scale * GALAXY_SCALE;
            x2 = (nav_state->current_galaxy->position.x - camera->x + nav_state->waypoint_star->waypoint_path[i].position.x / GALAXY_SCALE) * game_state->game_scale * GALAXY_SCALE;
            y2 = (nav_state->current_galaxy->position.y - camera->y + nav_state->waypoint_star->waypoint_path[i].position.y / GALAXY_SCALE) * game_state->game_scale * GALAXY_SCALE;
        }

        if (maths_line_intersects_camera(camera, x1, y1, x2, y2))
            SDL_RenderDrawLine(renderer, (int)x1, (int)y1, (int)x2, (int)y2);
    }

    // Highlight next path point
    if (nav_state->waypoint_star->waypoint_points)
        gfx_draw_circle(renderer, camera,
                        (nav_state->waypoint_star->waypoint_path[nav_state->next_path_point].position.x - camera->x) * game_state->game_scale,
                        (nav_state->waypoint_star->waypoint_path[nav_state->next_path_point].position.y - camera->y) * game_state->game_scale,
                        WAYPOINT_CIRCLE_RADIUS * game_state->game_scale, colors[COLOR_CYAN_70]);

    // Draw perpendicular line at path point
    double x1 = (nav_state->waypoint_star->waypoint_path[nav_state->next_path_point - 1].position.x - camera->x) * game_state->game_scale;
    double y1 = (nav_state->waypoint_star->waypoint_path[nav_state->next_path_point - 1].position.y - camera->y) * game_state->game_scale;
    double x2 = (nav_state->waypoint_star->waypoint_path[nav_state->next_path_point].position.x - camera->x) * game_state->game_scale;
    double y2 = (nav_state->waypoint_star->waypoint_path[nav_state->next_path_point].position.y - camera->y) * game_state->game_scale;

    double slope = (y2 - y1) / (x2 - x1);
    float angle = atan(slope);
    float perp_angle = angle + M_PI / 2;

    float dx = WAYPOINT_LINE_WIDTH * game_state->game_scale * cos(perp_angle) / 2;
    float dy = WAYPOINT_LINE_WIDTH * game_state->game_scale * sin(perp_angle) / 2;

    int px1 = round(x2 + dx);
    int py1 = round(y2 + dy);
    int px2 = round(x2 - dx);
    int py2 = round(y2 - dy);

    SDL_SetRenderDrawColor(renderer, colors[COLOR_CYAN_70].r, colors[COLOR_CYAN_70].g, colors[COLOR_CYAN_70].b, colors[COLOR_CYAN_70].a);
    SDL_RenderDrawLine(renderer, px1, py1, px2, py2);
}

/**
 * Generates a set of randomly placed and sized stars on a given camera view.
 * The function implements lazy initialization of bstars in batches.
 *
 * @param game_state A pointer to the current GameState object.
 * @param nav_state A pointer to the current NavigationState object.
 * @param bstars Array of Bstar objects to populate with generated stars.
 * @param camera A pointer to the current Camera object.
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
        int array_factor = 16;

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
                double distance = stars_nearest_star_distance(position, galaxy, initseq, GALAXY_CLOUD_DENSITY);
                unsigned short class = stars_size_class(distance);
                float class_opacity_max = class * (255 / 6);
                class_opacity_max = class_opacity_max > 255 ? 255 : class_opacity_max;
                // float class_opacity_min = class_opacity_max - (255 / 6);
                // int opacity = (abs(pcg32_random_r(&rng)) % (int)class_opacity_max + (int)class_opacity_min);
                // star.opacity = opacity;
                // star.opacity = star.opacity < 0 ? 0 : star.opacity;

                star.opacity = class_opacity_max;

                if (star.opacity < 120)
                    star.opacity = 120;

                // Calculate color
                unsigned short color_code;

                switch (class)
                {
                case STAR_1:
                    color_code = COLOR_STAR_1;
                    break;
                case STAR_2:
                    color_code = COLOR_STAR_2;
                    break;
                case STAR_3:
                    color_code = COLOR_STAR_3;
                    break;
                case STAR_4:
                    color_code = COLOR_STAR_4;
                    break;
                case STAR_5:
                    color_code = COLOR_STAR_5;
                    break;
                case STAR_6:
                    color_code = COLOR_STAR_6;
                    break;
                default:
                    color_code = COLOR_STAR_1;
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
                double distance = stars_nearest_star_distance(position, galaxy, initseq, MENU_GALAXY_CLOUD_DENSITY);
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
                case STAR_1:
                    color_code = COLOR_STAR_1;
                    break;
                case STAR_2:
                    color_code = COLOR_STAR_2;
                    break;
                case STAR_3:
                    color_code = COLOR_STAR_3;
                    break;
                case STAR_4:
                    color_code = COLOR_STAR_4;
                    break;
                case STAR_5:
                    color_code = COLOR_STAR_5;
                    break;
                case STAR_6:
                    color_code = COLOR_STAR_6;
                    break;
                default:
                    color_code = COLOR_STAR_1;
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
 * Checks if there is a direct line-of-sight between two points (x1,y1) and (x2,y2)
 * without intersecting any stars cutoff areas.
 *
 * @param nav_state A pointer to the current NavigationState object.
 * @param x1 the x-coordinate of the first point.
 * @param y1 the y-coordinate of the first point.
 * @param x2 the x-coordinate of the second point.
 * @param y2 the y-coordinate of the second point.
 * @param stars An array of pointers to stars.
 * @param max_stars The number of stars in the stars array.
 *
 * @return True if there is line of sight between the two points, false otherwise.
 */
static bool gfx_has_line_of_sight(NavigationState *nav_state, double x1, double y1, double x2, double y2, Star *stars[], int max_stars)
{
    for (int i = 0; i < max_stars; i++)
    {
        if (stars[i] != NULL)
        {
            if (strcmp(stars[i]->name, nav_state->waypoint_star->name) == 0)
                continue;

            bool star_obstructs_path = maths_is_point_in_circle((Point){x2, y2},
                                                                (Point){stars[i]->position.x, stars[i]->position.y},
                                                                stars[i]->cutoff) ||
                                       maths_line_intersects_circle(x1, y1,
                                                                    x2, y2,
                                                                    stars[i]->position.x, stars[i]->position.y,
                                                                    stars[i]->cutoff);

            if (!star_obstructs_path)
                continue;
            else
                return false;
        }
    }

    return true;
}

/**
 * Checks if an object with a given position and radius is within the bounds of the camera.
 *
 * @param camera A pointer to the current Camera object.
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
 * @param camera A pointer to the current Camera object.
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
 * Takes a point inside a circle and calculates the closest point on the circumference.
 *
 * @param nav_state A pointer to the current NavigationState object.
 * @param position The point inside the circle.
 * @param stars An array of pointers to nearest stars.
 * @param max_stars The number of nearest stars.
 *
 * @return void
 */

static void gfx_move_point_on_circumference(NavigationState *nav_state, Point *position, Star *stars[], int max_stars)
{
    double radius_ratio = 1.0;

    for (int i = 0; i < max_stars; i++)
    {
        if (stars[i] != NULL && strcmp(stars[i]->name, nav_state->waypoint_star->name) != 0)
        {
            if (maths_is_point_in_circle(*position, (Point){stars[i]->position.x, stars[i]->position.y}, stars[i]->cutoff))
            {
                maths_closest_point_outside_circle(stars[i]->position.x, stars[i]->position.y, stars[i]->cutoff, radius_ratio,
                                                   position->x, position->y,
                                                   &position->x, &position->y,
                                                   0);
            }
        }
    }
}

/**
 * Normalizes the waypoint path by moving points that are inside circles and by omitting
 * in-between points when there is a direct line of sight between distant points.
 *
 * @param nav_state A pointer to the current NavigationState object.
 * @param path The array of path points.
 * @param total_points The number of points.
 *
 * @return void
 */

static void gfx_normalize_waypoint_path(NavigationState *nav_state, PathPoint path[], int total_points)
{
    if (total_points < 3)
        return;

    // Add first point to path
    nav_state->waypoint_star->waypoint_path = (PathPoint *)malloc(sizeof(PathPoint));
    nav_state->waypoint_star->waypoint_path[0].type = PATH_POINT_TURN;
    nav_state->waypoint_star->waypoint_path[0].position = path[0].position;
    nav_state->waypoint_star->waypoint_points = 1;

    int point_offset = 3;

    for (int i = 1; i < total_points - 1 && i + point_offset < total_points; i++)
    {
        Point point = path[i].position;

        // Find nearest stars to (_x,_y)
        Star *nearest_stars[MAX_NEAREST_STARS];

        for (int s = 0; s < MAX_NEAREST_STARS; s++)
            nearest_stars[s] = NULL;

        double bx = maths_get_nearest_section_line(point.x, GALAXY_SECTION_SIZE);
        double by = maths_get_nearest_section_line(point.y, GALAXY_SECTION_SIZE);
        int nearest_stars_count = stars_nearest_stars_to_point(nav_state, (Point){bx, by}, nearest_stars);

        if (nearest_stars_count > 1)
            gfx_move_point_on_circumference(nav_state, &point, nearest_stars, nearest_stars_count);

        int j = 1;

        // If point is a turn point, check for line of sight
        if (nearest_stars_count > 0 &&
            path[i + point_offset].type == PATH_POINT_TURN && path[i + point_offset - 1].type == PATH_POINT_STRAIGHT)
        {
            PathPoint start = path[i];

            if (start.type == PATH_POINT_STRAIGHT)
            {
                while (path[i + point_offset + j].type == PATH_POINT_TURN)
                {
                    point = path[i + point_offset + j].position;

                    if (nearest_stars_count > 1)
                        gfx_move_point_on_circumference(nav_state, &point, nearest_stars, nearest_stars_count);

                    if (gfx_has_line_of_sight(nav_state, start.position.x, start.position.y,
                                              point.x, point.y, nearest_stars, nearest_stars_count))
                    {
                        j++;
                        continue;
                    }
                    else
                        break;
                }
            }
        }

        if (j > 1)
        {
            // Add starting point to path
            Point start_point = path[i].position;
            nav_state->waypoint_star->waypoint_path = (PathPoint *)realloc(nav_state->waypoint_star->waypoint_path, (nav_state->waypoint_star->waypoint_points + 1) * sizeof(PathPoint));
            nav_state->waypoint_star->waypoint_path[nav_state->waypoint_star->waypoint_points].type = PATH_POINT_TURN;
            nav_state->waypoint_star->waypoint_path[nav_state->waypoint_star->waypoint_points].position = start_point;
            nav_state->waypoint_star->waypoint_points++;

            // Bypass omitted points
            i = i + point_offset + j - 2;
        }
        else
        {
            // Add point to the new path
            point = path[i].position;

            if (nearest_stars_count > 1)
                gfx_move_point_on_circumference(nav_state, &point, nearest_stars, nearest_stars_count);

            nav_state->waypoint_star->waypoint_path = (PathPoint *)realloc(nav_state->waypoint_star->waypoint_path, (nav_state->waypoint_star->waypoint_points + 1) * sizeof(PathPoint));
            nav_state->waypoint_star->waypoint_path[nav_state->waypoint_star->waypoint_points].type = PATH_POINT_TURN;
            nav_state->waypoint_star->waypoint_path[nav_state->waypoint_star->waypoint_points].position = point;
            nav_state->waypoint_star->waypoint_points++;
        }
    }

    // Add last point to path
    nav_state->waypoint_star->waypoint_path = (PathPoint *)realloc(nav_state->waypoint_star->waypoint_path, (nav_state->waypoint_star->waypoint_points + 1) * sizeof(PathPoint));
    nav_state->waypoint_star->waypoint_path[nav_state->waypoint_star->waypoint_points].type = PATH_POINT_TURN;
    nav_state->waypoint_star->waypoint_path[nav_state->waypoint_star->waypoint_points].position = path[total_points - 1].position;
    nav_state->waypoint_star->waypoint_points++;
}

/**
 * Projects the given CelestialBody onto the edge of the screen.
 *
 * @param game_state A pointer to the current GameState object.
 * @param nav_state A pointer to the current NavigationState object.
 * @param body A pointer to the CelestialBody to be projected.
 * @param camera A pointer to the current Camera object.
 *
 * @return void
 */
void gfx_project_body_on_edge(const GameState *game_state, const NavigationState *nav_state, CelestialBody *body, const Camera *camera)
{
    gfx_update_projection_position(nav_state, body, ENTITY_CELESTIALBODY, camera, game_state->state, game_state->game_scale);

    double x = camera->x + (camera->w / 2) / game_state->game_scale;
    double y = camera->y + (camera->h / 2) / game_state->game_scale;
    double distance = maths_distance_between_points(x, y, body->position.x, body->position.y);

    if (body->level == LEVEL_STAR)
    {
        SDL_Color color;
        int center_x = body->projection.x + PROJECTION_RADIUS;
        int center_y = body->projection.y + PROJECTION_RADIUS;
        color = body->color;

        if (strcmp(nav_state->waypoint_star->name, body->name) == 0)
        {
            color.a = 255;
            gfx_draw_diamond(renderer, center_x, center_y, PROJECTION_RADIUS + 5, color);
        }
        else
            color.a = gfx_update_projection_opacity(distance, GALAXY_REGION_SIZE, GALAXY_SECTION_SIZE);

        gfx_draw_fill_diamond(renderer, center_x, center_y, PROJECTION_RADIUS - 1, color);
    }
    else
    {
        int center_x = body->projection.x + PROJECTION_RADIUS;
        int center_y = body->projection.y + PROJECTION_RADIUS;

        gfx_draw_fill_circle(renderer, center_x, center_y, PROJECTION_RADIUS - 2, body->color);
    }
}

/**
 * Projects the given Galaxy object onto the edge of the given Camera object, with the given state and scale.
 *
 * @param state An integer representing the current state of the game.
 * @param nav_state A pointer to the current NavigationState object.
 * @param galaxy A pointer to a Galaxy object to be projected.
 * @param camera A pointer to the current Camera object.
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

    double x = camera->x + (camera->w / 2) / scale;
    double y = camera->y + (camera->h / 2) / scale;
    double delta_x = fabs(x - galaxy->position.x * scaling_factor) / scaling_factor;
    double delta_y = fabs(y - galaxy->position.y * scaling_factor) / scaling_factor;
    double distance = sqrt(pow(delta_x, 2) + pow(delta_y, 2));
    int opacity = gfx_update_projection_opacity(distance, UNIVERSE_REGION_SIZE, UNIVERSE_SECTION_SIZE);

    SDL_Color color = galaxy->color;
    color.a = opacity;

    int center_x = galaxy->projection.x + PROJECTION_RADIUS;
    int center_y = galaxy->projection.y + PROJECTION_RADIUS;

    gfx_draw_fill_diamond(renderer, center_x, center_y, PROJECTION_RADIUS - 1, color);
}

/**
 * Draw the ship projection on the edge of the screen, taking into account the current camera position and zoom level.
 *
 * @param state An integer representing the current game state (NAVIGATE or MAP).
 * @param input_state A pointer to the current InputState object.
 * @param nav_state A pointer to the current NavigationState object.
 * @param ship A pointer to the Ship struct to be drawn.
 * @param camera A pointer to the current Camera object.
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
 * Takes a point and an array of nearest stars and checks whether the point is inside any of the stars.
 * It then moves the point just outside the star.
 *
 * @param nav_state A pointer to the current NavigationState object.
 * @param position The starting point outside the star.
 * @param stars An array of pointers to nearest stars.
 * @param max_stars The number of stars in the stars array.
 * @param x The x-coordinate of the new point.
 * @param y The y-coordinate of the new point.
 *
 * @return void
 */

static void gfx_shift_path_segment(NavigationState *nav_state, Point position, Star *stars[], int max_stars, double *x, double *y)
{
    double radius_ratio = 1.01;

    for (int i = 0; i < max_stars; i++)
    {
        if (stars[i] != NULL && strcmp(stars[i]->name, nav_state->waypoint_star->name) != 0)
        {
            while (maths_is_point_in_circle((Point){*x, *y}, (Point){stars[i]->position.x, stars[i]->position.y}, stars[i]->cutoff))
            {
                maths_closest_point_outside_circle(stars[i]->position.x, stars[i]->position.y, stars[i]->cutoff, radius_ratio,
                                                   *x, *y,
                                                   x, y,
                                                   0);
            }
        }
    }
}

/**
 * Checks if the mouse is over the current galaxy and toggles the variable input_state.is_hovering_galaxy.
 *
 * @param input_state A pointer to the current InputState object.
 * @param nav_state A pointer to the current NavigationState object.
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
 * Checks if the mouse is over the current_star and toggles the variable input_state.is_hovering_star.
 *
 * @param input_state A pointer to the current InputState object.
 * @param nav_state A pointer to the current NavigationState object.
 * @param camera A pointer to the current Camera object.
 * @param scale The scaling factor applied to the camera view.
 * @param state The current state.
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

    // Get star distance from mouse position
    double current_distance = maths_distance_between_points(current_x, current_y, input_state->mouse_position.x, input_state->mouse_position.y);

    if (current_distance <= current_cutoff)
    {
        input_state->is_hovering_star = true;

        if (state == UNIVERSE)
            input_state->is_hovering_galaxy = false;
    }
    else
        input_state->is_hovering_star = false;
}

/**
 * Checks if the mouse is over the current star info box.
 *
 * @param input_state A pointer to the current InputState object.
 * @param nav_state A pointer to the current NavigationState object.
 * @param camera A pointer to the current Camera object.
 *
 * @return True if the mouse is over the current star info box, false otherwise.
 */
bool gfx_toggle_star_info_hover(InputState *input_state, const NavigationState *nav_state, const Camera *camera)
{
    if (!nav_state->selected_star->initialized || !nav_state->selected_star->is_selected)
        return false;

    int padding = INFO_BOX_PADDING;
    int width = INFO_BOX_WIDTH;

    // Define rect of info box
    Point rect[4];

    rect[0].x = camera->w - (width + padding);
    rect[0].y = padding;

    rect[1].x = camera->w - padding;
    rect[1].y = padding;

    rect[2].x = camera->w - padding;
    rect[2].y = camera->h - padding;

    rect[3].x = camera->w - (width + padding);
    rect[3].y = camera->h - padding;

    // Get mouse position
    Point mouse_position = {.x = input_state->mouse_position.x, .y = input_state->mouse_position.y};

    if (maths_is_point_in_rectangle(mouse_position, rect))
    {
        input_state->is_hovering_star = false;
        return true;
    }
    else
        return false;
}

/**
 * Checks if the mouse is over a planet rect in the current star info box.
 *
 * @param input_state A pointer to the current InputState object.
 * @param camera A pointer to the current Camera object.
 * @param button_rect The planet waypoint button rect.
 * @param index The index of the planet.
 *
 * @return void.
 */
void gfx_toggle_star_info_planet_hover(InputState *input_state, const Camera *camera, SDL_Rect button_rect, int index)
{
    int width = INFO_BOX_WIDTH;
    int padding = INFO_BOX_PADDING;
    int inner_padding = 5;

    // Get mouse position
    Point mouse_position = {.x = input_state->mouse_position.x, .y = input_state->mouse_position.y};

    // Define planet rect
    Point planet_rect[4];

    planet_rect[0].x = camera->w - (width + padding);
    planet_rect[0].y = button_rect.y - inner_padding;

    planet_rect[1].x = camera->w - padding;
    planet_rect[1].y = button_rect.y - inner_padding;

    planet_rect[2].x = camera->w - padding;
    planet_rect[2].y = button_rect.y + WAYPOINT_BUTTON_HEIGHT + 2 * inner_padding;

    planet_rect[3].x = camera->w - (width + padding);
    planet_rect[3].y = button_rect.y + WAYPOINT_BUTTON_HEIGHT + 2 * inner_padding;

    input_state->is_hovering_star_info_planet = false;

    if (maths_is_point_in_rectangle(mouse_position, planet_rect))
    {
        input_state->is_hovering_star_info_planet = true;
        input_state->selected_star_info_planet_index = index;

        // Define waypoint button rect
        Point waypoint_button_rect[4];

        waypoint_button_rect[0].x = button_rect.x;
        waypoint_button_rect[0].y = button_rect.y;

        waypoint_button_rect[1].x = button_rect.x + button_rect.w;
        waypoint_button_rect[1].y = button_rect.y;

        waypoint_button_rect[2].x = button_rect.x + button_rect.w;
        waypoint_button_rect[2].y = button_rect.y + button_rect.h;

        waypoint_button_rect[3].x = button_rect.x;
        waypoint_button_rect[3].y = button_rect.y + button_rect.h;

        if (maths_is_point_in_rectangle(mouse_position, waypoint_button_rect))
        {
            input_state->is_hovering_star_waypoint_button = false;
            input_state->is_hovering_planet_waypoint_button = true;
        }

        return;
    }
}

/**
 * Checks if the mouse is over the star waypoint button in the current star info box.
 *
 * @param input_state A pointer to the current InputState object.
 * @param button_rect The waypoint button rect.
 *
 * @return void.
 */
void gfx_toggle_star_waypoint_button_hover(InputState *input_state, SDL_Rect button_rect)
{
    // Get mouse position
    Point mouse_position = {.x = input_state->mouse_position.x, .y = input_state->mouse_position.y};

    // Define waypoint button rect
    Point waypoint_button_rect[4];

    waypoint_button_rect[0].x = button_rect.x;
    waypoint_button_rect[0].y = button_rect.y;

    waypoint_button_rect[1].x = button_rect.x + button_rect.w;
    waypoint_button_rect[1].y = button_rect.y;

    waypoint_button_rect[2].x = button_rect.x + button_rect.w;
    waypoint_button_rect[2].y = button_rect.y + button_rect.h;

    waypoint_button_rect[3].x = button_rect.x;
    waypoint_button_rect[3].y = button_rect.y + button_rect.h;

    if (maths_is_point_in_rectangle(mouse_position, waypoint_button_rect))
    {
        input_state->is_hovering_star_waypoint_button = true;
        input_state->is_hovering_planet_waypoint_button = false;
    }
    else
        input_state->is_hovering_star_waypoint_button = false;
}

/**
 * Updates the position and opacity of background stars.
 *
 * @param state The current game state.
 * @param camera_on Whether or not the camera is turned on.
 * @param nav_state A pointer to the current NavigationState object.
 * @param bstars An array of background stars.
 * @param camera A pointer to the current Camera object.
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
        if (camera_on || state == MENU || state == CONTROLS)
        {
            float dx, dy;

            if (state == MENU || state == CONTROLS)
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
        else if (state == CONTROLS)
            opacity = (double)(bstars[i].opacity * 1 / 3);
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
 * @param camera A pointer to the current Camera object.
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
 * @param camera A pointer to the current Camera object.
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
 * @param nav_state A pointer to the current NavigationState object.
 * @param ptr A pointer to the entity to update the projection position of.
 * @param entity_type The type of entity to update the projection position of.
 * @param camera A pointer to the current Camera object.
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

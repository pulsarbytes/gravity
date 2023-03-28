/*
 * console.c
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"
#include "../include/console.h"

// External variable definitions
extern TTF_Font *fonts[];
extern SDL_Renderer *renderer;
extern SDL_Color colors[];

// Static function prototypes
static void console_draw_velocity_vector(const Ship *, Point, const Camera *);

/**
 * Draws the current frames per second (FPS) on the screen.
 *
 * @param fps An integer representing the current FPS value.
 * @param camera A pointer to the current Camera object.
 *
 * @return void
 */

void console_draw_fps(unsigned int fps, const Camera *camera)
{
    char fps_text[16];
    memset(fps_text, 0, sizeof(fps_text));
    sprintf(fps_text, "%d", fps);

    // Create text texture
    SDL_Surface *fps_surface = TTF_RenderText_Blended(fonts[FONT_SIZE_22], fps_text, colors[COLOR_CYAN_100]);
    SDL_Texture *fps_texture = SDL_CreateTextureFromSurface(renderer, fps_surface);
    SDL_Rect fps_texture_rect;
    fps_texture_rect.x = 30;
    fps_texture_rect.y = camera->h - 40;
    fps_texture_rect.w = fps_surface->w;
    fps_texture_rect.h = fps_surface->h;
    SDL_RenderCopy(renderer, fps_texture, NULL, &fps_texture_rect);
    SDL_FreeSurface(fps_surface);

    // Destroy texture
    SDL_DestroyTexture(fps_texture);
    fps_texture = NULL;
}

/**
 * Draws a console displaying the ship's velocity vector and position.
 *
 * @param game_state A pointer to the current GameState object.
 * @param nav_state A pointer to the current NavigationState object.
 * @param camera A pointer to the current Camera object.
 *
 * @return void
 */
void console_draw_position_console(const GameState *game_state, const NavigationState *nav_state, const Camera *camera)
{
    // Draw background box
    SDL_SetRenderDrawColor(renderer, 12, 12, 12, 230);
    int box_width = 300;
    int box_height = 70;
    int padding = INFO_BOX_PADDING;
    int inner_padding = 10;

    SDL_Rect box_rect;
    box_rect.x = (camera->w / 2) - (box_width / 2);
    box_rect.y = camera->h - (box_height + padding);
    box_rect.w = box_width;
    box_rect.h = box_height;

    SDL_RenderFillRect(renderer, &box_rect);

    // Draw separator line
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 20);
    int section_width = 100;
    int separator_1_x = (camera->w / 2) - (section_width / 2);
    int separator_y1 = camera->h - (box_height + padding);
    int separator_y2 = separator_y1 + box_height;

    SDL_RenderDrawLine(renderer, separator_1_x, separator_y1, separator_1_x, separator_y2);

    // Zoom
    char *zoom_title = "ZOOM";
    SDL_Surface *zoom_title_surface = TTF_RenderText_Blended(fonts[FONT_SIZE_12], zoom_title, colors[COLOR_WHITE_100]);
    SDL_Texture *zoom_title_texture = SDL_CreateTextureFromSurface(renderer, zoom_title_surface);
    SDL_Rect zoom_title_texture_rect;
    zoom_title_texture_rect.w = zoom_title_surface->w;
    zoom_title_texture_rect.h = zoom_title_surface->h;
    zoom_title_texture_rect.x = (camera->w / 2) - section_width - (zoom_title_texture_rect.w / 2);
    zoom_title_texture_rect.y = box_rect.y + inner_padding;
    SDL_RenderCopy(renderer, zoom_title_texture, NULL, &zoom_title_texture_rect);
    SDL_FreeSurface(zoom_title_surface);

    char zoom_value[16];
    memset(zoom_value, 0, sizeof(zoom_value));

    if (game_state->state == UNIVERSE)
    {
        const double epsilon = ZOOM_EPSILON / GALAXY_SCALE;

        if (game_state->game_scale >= 0.0001 - epsilon)
            sprintf(zoom_value, "%.2Lf", 100 * game_state->game_scale);
        else if (game_state->game_scale >= 0.00001 - epsilon)
            sprintf(zoom_value, "%.3Lf", 100 * game_state->game_scale);
        else if (game_state->game_scale >= 0.000001 - epsilon)
            sprintf(zoom_value, "%.4Lf", 100 * game_state->game_scale);
    }
    else
        sprintf(zoom_value, "%.2Lf", 100 * game_state->game_scale);

    SDL_Surface *zoom_value_surface = TTF_RenderText_Blended(fonts[FONT_SIZE_15], zoom_value, colors[COLOR_WHITE_180]);
    SDL_Texture *zoom_value_texture = SDL_CreateTextureFromSurface(renderer, zoom_value_surface);
    SDL_Rect zoom_value_texture_rect;
    zoom_value_texture_rect.w = zoom_value_surface->w;
    zoom_value_texture_rect.h = zoom_value_surface->h;
    zoom_value_texture_rect.x = (camera->w / 2) - section_width - (zoom_value_texture_rect.w / 2);
    zoom_value_texture_rect.y = box_rect.y + 3.4 * inner_padding;
    SDL_RenderCopy(renderer, zoom_value_texture, NULL, &zoom_value_texture_rect);
    SDL_FreeSurface(zoom_value_surface);

    // Position
    Point offset = {.x = 0.0, .y = 0.0};
    const double epsilon = ZOOM_EPSILON / GALAXY_SCALE;
    double zoom_generate_preview_stars;

    switch (nav_state->current_galaxy->class)
    {
    case 1:
        zoom_generate_preview_stars = ZOOM_STAR_1_PREVIEW_STARS;
        break;
    case 2:
        zoom_generate_preview_stars = ZOOM_STAR_2_PREVIEW_STARS;
        break;
    case 3:
        zoom_generate_preview_stars = ZOOM_STAR_3_PREVIEW_STARS;
        break;
    case 4:
        zoom_generate_preview_stars = ZOOM_STAR_4_PREVIEW_STARS;
        break;
    case 5:
        zoom_generate_preview_stars = ZOOM_STAR_5_PREVIEW_STARS;
        break;
    case 6:
        zoom_generate_preview_stars = ZOOM_STAR_6_PREVIEW_STARS;
        break;
    default:
        zoom_generate_preview_stars = ZOOM_STAR_1_PREVIEW_STARS;
        break;
    }

    char position_title[32];

    if (game_state->state == UNIVERSE)
    {
        if (game_state->game_scale >= zoom_generate_preview_stars - epsilon)
        {
            sprintf(position_title, "POSITION IN GALAXY");
            offset.x = nav_state->map_offset.x;
            offset.y = nav_state->map_offset.y;
        }
        else
        {
            sprintf(position_title, "POSITION IN UNIVERSE");
            offset.x = nav_state->universe_offset.x;
            offset.y = nav_state->universe_offset.y;
        }
    }
    else if (game_state->state == MAP)
    {
        sprintf(position_title, "POSITION IN GALAXY");
        offset.x = nav_state->map_offset.x;
        offset.y = nav_state->map_offset.y;
    }

    SDL_Surface *position_title_surface = TTF_RenderText_Blended(fonts[FONT_SIZE_12], position_title, colors[COLOR_WHITE_100]);
    SDL_Texture *position_title_texture = SDL_CreateTextureFromSurface(renderer, position_title_surface);
    SDL_Rect position_title_texture_rect;
    position_title_texture_rect.w = position_title_surface->w;
    position_title_texture_rect.h = position_title_surface->h;
    position_title_texture_rect.x = (camera->w / 2) + (section_width / 2) - (position_title_texture_rect.w / 2);
    position_title_texture_rect.y = box_rect.y + inner_padding;
    SDL_RenderCopy(renderer, position_title_texture, NULL, &position_title_texture_rect);
    SDL_FreeSurface(position_title_surface);

    char position_x_value[32];
    memset(position_x_value, 0, sizeof(position_x_value));
    char position_x_text[48];
    memset(position_x_text, 0, sizeof(position_x_value));
    utils_add_thousand_separators((int)offset.x, position_x_value, sizeof(position_x_value));
    sprintf(position_x_text, "X: %*s%s", 1, "", position_x_value);
    SDL_Surface *position_x_surface = TTF_RenderText_Blended(fonts[FONT_SIZE_12], position_x_text, colors[COLOR_WHITE_140]);
    SDL_Texture *position_x_texture = SDL_CreateTextureFromSurface(renderer, position_x_surface);
    SDL_Rect position_x_texture_rect;
    position_x_texture_rect.w = position_x_surface->w;
    position_x_texture_rect.h = position_x_surface->h;
    position_x_texture_rect.x = (camera->w / 2);
    position_x_texture_rect.y = box_rect.y + 3 * inner_padding;
    SDL_RenderCopy(renderer, position_x_texture, NULL, &position_x_texture_rect);
    SDL_FreeSurface(position_x_surface);

    char position_y_value[32];
    memset(position_y_value, 0, sizeof(position_y_value));
    char position_y_text[48];
    memset(position_y_text, 0, sizeof(position_y_value));
    utils_add_thousand_separators((int)offset.y, position_y_value, sizeof(position_y_value));
    sprintf(position_y_text, "Y: %*s%s", 1, "", position_y_value);
    SDL_Surface *position_y_surface = TTF_RenderText_Blended(fonts[FONT_SIZE_12], position_y_text, colors[COLOR_WHITE_140]);
    SDL_Texture *position_y_texture = SDL_CreateTextureFromSurface(renderer, position_y_surface);
    SDL_Rect position_y_texture_rect;
    position_y_texture_rect.w = position_y_surface->w;
    position_y_texture_rect.h = position_y_surface->h;
    position_y_texture_rect.x = (camera->w / 2);
    position_y_texture_rect.y = box_rect.y + 5 * inner_padding;
    SDL_RenderCopy(renderer, position_y_texture, NULL, &position_y_texture_rect);
    SDL_FreeSurface(position_y_surface);

    // Destroy the textures
    SDL_DestroyTexture(zoom_title_texture);
    zoom_title_texture = NULL;
    SDL_DestroyTexture(zoom_value_texture);
    zoom_value_texture = NULL;
    SDL_DestroyTexture(position_title_texture);
    position_title_texture = NULL;
    SDL_DestroyTexture(position_x_texture);
    position_x_texture = NULL;
    SDL_DestroyTexture(position_y_texture);
    position_y_texture = NULL;
}

/**
 * Draws a console displaying the ship's velocity vector and position.
 *
 * @param game_state A pointer to the current GameState object.
 * @param nav_state A pointer to the current NavigationState object.
 * @param ship A pointer to the Ship struct containing the ship's current state.
 * @param camera A pointer to the current Camera object.
 *
 * @return void
 */
void console_draw_ship_console(const GameState *game_state, const NavigationState *nav_state, const Ship *ship, const Camera *camera)
{
    // Draw background box
    SDL_SetRenderDrawColor(renderer, 12, 12, 12, 230);
    int box_width = 400;
    int box_height = 70;
    int padding = INFO_BOX_PADDING;
    int inner_padding = 10;

    SDL_Rect box_rect;
    box_rect.x = (camera->w / 2) - (box_width / 2);
    box_rect.y = camera->h - padding - box_height;
    box_rect.w = box_width;
    box_rect.h = box_height;

    SDL_RenderFillRect(renderer, &box_rect);

    // Draw separator lines
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 20);

    int section_width = 100;
    int separator_1_x = (camera->w / 2) - section_width;
    int separator_2_x = (camera->w / 2);
    int separator_3_x = (camera->w / 2) + section_width;
    int separator_y1 = camera->h - (box_height + padding);
    int separator_y2 = separator_y1 + box_height;

    SDL_RenderDrawLine(renderer, separator_1_x, separator_y1, separator_1_x, separator_y2);
    SDL_RenderDrawLine(renderer, separator_2_x, separator_y1, separator_2_x, separator_y2);
    SDL_RenderDrawLine(renderer, separator_3_x, separator_y1, separator_3_x, separator_y2);

    // Zoom
    char *zoom_title = "ZOOM";
    SDL_Surface *zoom_title_surface = TTF_RenderText_Blended(fonts[FONT_SIZE_12], zoom_title, colors[COLOR_WHITE_100]);
    SDL_Texture *zoom_title_texture = SDL_CreateTextureFromSurface(renderer, zoom_title_surface);
    SDL_Rect zoom_title_texture_rect;
    zoom_title_texture_rect.w = zoom_title_surface->w;
    zoom_title_texture_rect.h = zoom_title_surface->h;
    zoom_title_texture_rect.x = (camera->w / 2) - 1.5 * section_width - (zoom_title_texture_rect.w / 2);
    zoom_title_texture_rect.y = box_rect.y + inner_padding;
    SDL_RenderCopy(renderer, zoom_title_texture, NULL, &zoom_title_texture_rect);
    SDL_FreeSurface(zoom_title_surface);

    char zoom_value[16];
    memset(zoom_value, 0, sizeof(zoom_value));
    sprintf(zoom_value, "%.2Lf", 100 * game_state->game_scale);

    SDL_Surface *zoom_value_surface = TTF_RenderText_Blended(fonts[FONT_SIZE_15], zoom_value, colors[COLOR_WHITE_180]);
    SDL_Texture *zoom_value_texture = SDL_CreateTextureFromSurface(renderer, zoom_value_surface);
    SDL_Rect zoom_value_texture_rect;
    zoom_value_texture_rect.w = zoom_value_surface->w;
    zoom_value_texture_rect.h = zoom_value_surface->h;
    zoom_value_texture_rect.x = (camera->w / 2) - 1.5 * section_width - (zoom_value_texture_rect.w / 2);
    zoom_value_texture_rect.y = box_rect.y + 3.4 * inner_padding;
    SDL_RenderCopy(renderer, zoom_value_texture, NULL, &zoom_value_texture_rect);
    SDL_FreeSurface(zoom_value_surface);

    // Velocity vector
    Point center = {.x = (camera->w / 2) - 0.5 * section_width,
                    .y = camera->h - (box_height / 2) - padding};
    console_draw_velocity_vector(ship, center, camera);

    // Speed
    char *speed_title = "SPEED";
    SDL_Surface *speed_title_surface = TTF_RenderText_Blended(fonts[FONT_SIZE_12], speed_title, colors[COLOR_WHITE_100]);
    SDL_Texture *speed_title_texture = SDL_CreateTextureFromSurface(renderer, speed_title_surface);
    SDL_Rect speed_title_texture_rect;
    speed_title_texture_rect.w = speed_title_surface->w;
    speed_title_texture_rect.h = speed_title_surface->h;
    speed_title_texture_rect.x = (camera->w / 2) + 0.5 * section_width - (speed_title_texture_rect.w / 2);
    speed_title_texture_rect.y = box_rect.y + inner_padding;
    SDL_RenderCopy(renderer, speed_title_texture, NULL, &speed_title_texture_rect);
    SDL_FreeSurface(speed_title_surface);

    char speed_value[16];
    memset(speed_value, 0, sizeof(speed_value));
    sprintf(speed_value, "%d", (int)nav_state->velocity.magnitude);
    SDL_Surface *speed_value_surface = TTF_RenderText_Blended(fonts[FONT_SIZE_22], speed_value, colors[COLOR_WHITE_180]);
    SDL_Texture *speed_value_texture = SDL_CreateTextureFromSurface(renderer, speed_value_surface);
    SDL_Rect speed_value_texture_rect;
    speed_value_texture_rect.w = speed_value_surface->w;
    speed_value_texture_rect.h = speed_value_surface->h;
    speed_value_texture_rect.x = (camera->w / 2) + 0.5 * section_width - (speed_value_texture_rect.w / 2);
    speed_value_texture_rect.y = box_rect.y + 3.4 * inner_padding;
    SDL_RenderCopy(renderer, speed_value_texture, NULL, &speed_value_texture_rect);
    SDL_FreeSurface(speed_value_surface);

    // Position
    char *position_title = "POSITION";
    SDL_Surface *position_title_surface = TTF_RenderText_Blended(fonts[FONT_SIZE_12], position_title, colors[COLOR_WHITE_100]);
    SDL_Texture *position_title_texture = SDL_CreateTextureFromSurface(renderer, position_title_surface);
    SDL_Rect position_title_texture_rect;
    position_title_texture_rect.w = position_title_surface->w;
    position_title_texture_rect.h = position_title_surface->h;
    position_title_texture_rect.x = (camera->w / 2) + 1.5 * section_width - (position_title_texture_rect.w / 2);
    position_title_texture_rect.y = box_rect.y + inner_padding;
    SDL_RenderCopy(renderer, position_title_texture, NULL, &position_title_texture_rect);
    SDL_FreeSurface(position_title_surface);

    char position_x_value[32];
    memset(position_x_value, 0, sizeof(position_x_value));
    utils_add_thousand_separators((int)nav_state->navigate_offset.x, position_x_value, sizeof(position_x_value));
    SDL_Surface *position_x_surface = TTF_RenderText_Blended(fonts[FONT_SIZE_12], position_x_value, colors[COLOR_WHITE_140]);
    SDL_Texture *position_x_texture = SDL_CreateTextureFromSurface(renderer, position_x_surface);
    SDL_Rect position_x_texture_rect;
    position_x_texture_rect.w = position_x_surface->w;
    position_x_texture_rect.h = position_x_surface->h;
    position_x_texture_rect.x = (camera->w / 2) + 1.5 * section_width - (position_x_texture_rect.w / 2);
    position_x_texture_rect.y = box_rect.y + 3 * inner_padding;
    SDL_RenderCopy(renderer, position_x_texture, NULL, &position_x_texture_rect);
    SDL_FreeSurface(position_x_surface);

    char position_y_value[32];
    memset(position_y_value, 0, sizeof(position_y_value));
    utils_add_thousand_separators((int)nav_state->navigate_offset.y, position_y_value, sizeof(position_y_value));
    SDL_Surface *position_y_surface = TTF_RenderText_Blended(fonts[FONT_SIZE_12], position_y_value, colors[COLOR_WHITE_140]);
    SDL_Texture *position_y_texture = SDL_CreateTextureFromSurface(renderer, position_y_surface);
    SDL_Rect position_y_texture_rect;
    position_y_texture_rect.w = position_y_surface->w;
    position_y_texture_rect.h = position_y_surface->h;
    position_y_texture_rect.x = (camera->w / 2) + 1.5 * section_width - (position_x_texture_rect.w / 2);
    position_y_texture_rect.y = box_rect.y + 5 * inner_padding;
    SDL_RenderCopy(renderer, position_y_texture, NULL, &position_y_texture_rect);
    SDL_FreeSurface(position_y_surface);

    // Destroy the textures
    SDL_DestroyTexture(zoom_value_texture);
    zoom_value_texture = NULL;
    SDL_DestroyTexture(speed_title_texture);
    speed_title_texture = NULL;
    SDL_DestroyTexture(speed_value_texture);
    speed_value_texture = NULL;
    SDL_DestroyTexture(position_title_texture);
    position_title_texture = NULL;
    SDL_DestroyTexture(position_x_texture);
    position_x_texture = NULL;
    SDL_DestroyTexture(position_y_texture);
    position_y_texture = NULL;
}

/**
 * Draws a console displaying information about current star.
 *
 * @param star A pointer to the current star.
 * @param camera A pointer to the current Camera object.
 *
 * @return void
 */
void console_draw_star_console(const Star *star, const Camera *camera)
{
    // Draw background box
    SDL_SetRenderDrawColor(renderer, 12, 12, 12, 230);
    int box_width = INFO_BOX_WIDTH;
    int box_height = 70;
    int padding = INFO_BOX_PADDING;
    int inner_padding = 40;

    SDL_Rect box_rect;
    box_rect.x = camera->w - (box_width + padding);
    box_rect.y = camera->h - (box_height + padding);
    box_rect.w = box_width;
    box_rect.h = box_height;

    SDL_RenderFillRect(renderer, &box_rect);

    // Star name
    char star_name[128];
    memset(star_name, 0, sizeof(star_name));
    sprintf(star_name, "%s", star->name);
    SDL_Surface *star_name_surface = TTF_RenderText_Blended(fonts[FONT_SIZE_18], star_name, colors[COLOR_WHITE_140]);
    SDL_Texture *star_name_texture = SDL_CreateTextureFromSurface(renderer, star_name_surface);
    SDL_Rect star_name_texture_rect;
    star_name_texture_rect.w = star_name_surface->w;
    star_name_texture_rect.h = star_name_surface->h;
    star_name_texture_rect.x = camera->w - (box_width + padding) + 1.5 * padding + inner_padding;
    star_name_texture_rect.y = camera->h - padding - (box_height / 2) - (star_name_texture_rect.h / 2) + 1;
    SDL_RenderCopy(renderer, star_name_texture, NULL, &star_name_texture_rect);
    SDL_FreeSurface(star_name_surface);
    SDL_DestroyTexture(star_name_texture);
    star_name_texture = NULL;

    // Star circle
    int x_star = camera->w - (box_width + padding) + inner_padding + 5;
    int y_star = star_name_texture_rect.y - 2 + padding / 2;
    gfx_draw_fill_circle(renderer, x_star, y_star, 8, star->color);
}

/**
 * Draws a velocity vector on the ship console.
 *
 * @param ship A pointer to a Ship struct representing the ship whose velocity vector will be drawn.
 * @param center A Point struct representing the center point from which the velocity vector will be drawn.
 * @param camera A pointer to the current Camera object.
 *
 * @return void
 */
static void console_draw_velocity_vector(const Ship *ship, Point center, const Camera *camera)
{
    const int VELOCITY_VECTOR_LENGTH = 15;
    const int ARROW_SIZE = 8;

    // Calculate the velocity vector
    float velocity_x = ship->vx;
    float velocity_y = ship->vy;

    // Normalize the velocity vector
    float velocity_length = sqrt(velocity_x * velocity_x + velocity_y * velocity_y);
    velocity_x /= velocity_length;
    velocity_y /= velocity_length;

    // Calculate the starting position
    float start_x = center.x - velocity_x * VELOCITY_VECTOR_LENGTH;
    float start_y = center.y - velocity_y * VELOCITY_VECTOR_LENGTH;

    // Calculate the ending position
    float end_x = center.x + velocity_x * VELOCITY_VECTOR_LENGTH;
    float end_y = center.y + velocity_y * VELOCITY_VECTOR_LENGTH;

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
    SDL_RenderDrawLine(renderer, (int)start_x, (int)start_y, (int)end_x, (int)end_y);

    // Calculate the position of the arrow points
    float arrow_x1 = end_x - velocity_x * ARROW_SIZE + velocity_y * ARROW_SIZE / 2;
    float arrow_y1 = end_y - velocity_y * ARROW_SIZE - velocity_x * ARROW_SIZE / 2;
    float arrow_x2 = end_x - velocity_x * ARROW_SIZE - velocity_y * ARROW_SIZE / 2;
    float arrow_y2 = end_y - velocity_y * ARROW_SIZE + velocity_x * ARROW_SIZE / 2;

    // Draw the arrow
    SDL_RenderDrawLine(renderer, (int)end_x, (int)end_y, (int)arrow_x1, (int)arrow_y1);
    SDL_RenderDrawLine(renderer, (int)end_x, (int)end_y, (int)arrow_x2, (int)arrow_y2);
    SDL_RenderDrawLine(renderer, (int)arrow_x1, (int)arrow_y1, (int)arrow_x2, (int)arrow_y2);

    // Draw circle around vector
    gfx_draw_circle(renderer, camera, center.x, center.y, 20, colors[COLOR_CYAN_70]);
}

/**
 * Draws a console displaying information about waypoint star.
 *
 * @param nav_state A pointer to the current NavigationState object.
 * @param camera A pointer to the current Camera object.
 *
 * @return void
 */
void console_draw_waypoint_console(const NavigationState *nav_state, const Camera *camera)
{
    // Draw background box
    SDL_SetRenderDrawColor(renderer, 12, 12, 12, 230);
    int box_width = INFO_BOX_WIDTH;
    int box_height = 130;
    int padding = INFO_BOX_PADDING;
    int inner_padding = 40;
    int star_name_height = 70;
    int entry_height = 25;

    SDL_Rect box_rect;
    box_rect.x = camera->w - (box_width + padding);
    box_rect.y = camera->h - (box_height + padding);
    box_rect.w = box_width;
    box_rect.h = box_height;

    SDL_RenderFillRect(renderer, &box_rect);

    // Star name
    char star_name[128];
    memset(star_name, 0, sizeof(star_name));
    sprintf(star_name, "%s", nav_state->waypoint_star->name);
    SDL_Surface *star_name_surface = TTF_RenderText_Blended(fonts[FONT_SIZE_18], star_name, colors[COLOR_WHITE_140]);
    SDL_Texture *star_name_texture = SDL_CreateTextureFromSurface(renderer, star_name_surface);
    SDL_Rect star_name_texture_rect;
    star_name_texture_rect.w = star_name_surface->w;
    star_name_texture_rect.h = star_name_surface->h;
    star_name_texture_rect.x = camera->w - (box_width + padding) + 1.5 * padding + inner_padding;
    star_name_texture_rect.y = camera->h - (box_height + padding) + (star_name_height - star_name_texture_rect.h) / 2 + 1;
    SDL_RenderCopy(renderer, star_name_texture, NULL, &star_name_texture_rect);
    SDL_FreeSurface(star_name_surface);
    SDL_DestroyTexture(star_name_texture);
    star_name_texture = NULL;

    // Star diamond
    int x_star = camera->w - (box_width + padding) + inner_padding + 5;
    int y_star = star_name_texture_rect.y - 2 + padding / 2;
    gfx_draw_diamond(renderer, x_star, y_star, PROJECTION_RADIUS + 6, nav_state->waypoint_star->color);
    gfx_draw_fill_diamond(renderer, x_star, y_star, PROJECTION_RADIUS, nav_state->waypoint_star->color);

    // Distance
    double distance_star = maths_distance_between_points(nav_state->waypoint_star->position.x, nav_state->waypoint_star->position.y, nav_state->navigate_offset.x, nav_state->navigate_offset.y);
    char distance_text[16];
    memset(distance_text, 0, sizeof(distance_text));
    utils_add_thousand_separators((int)distance_star, distance_text, sizeof(distance_text));

    char distance_row_text[64];
    memset(distance_row_text, 0, sizeof(distance_row_text));
    sprintf(distance_row_text, "Distance: %*s%s", 4, "", distance_text);

    SDL_Surface *distance_surface = TTF_RenderText_Blended(fonts[FONT_SIZE_15], distance_row_text, colors[COLOR_WHITE_140]);
    SDL_Texture *distance_texture = SDL_CreateTextureFromSurface(renderer, distance_surface);
    SDL_Rect distance_texture_rect = {.x = camera->w - (box_width - 2.5 * padding),
                                      .y = camera->h - (box_height + padding) + star_name_height,
                                      .w = distance_surface->w,
                                      .h = distance_surface->h};

    SDL_FreeSurface(distance_surface);
    SDL_RenderCopy(renderer, distance_texture, NULL, &distance_texture_rect);
    SDL_DestroyTexture(distance_texture);

    // Time
    if (nav_state->velocity.magnitude > 5)
    {
        int seconds = distance_star / nav_state->velocity.magnitude;
        char time_row_text[64];
        memset(time_row_text, 0, sizeof(time_row_text));
        char time_text[32];
        memset(time_text, 0, sizeof(time_text));
        utils_convert_seconds_to_time_string(seconds, time_text);
        sprintf(time_row_text, "%*s%s", 14, "", time_text);

        SDL_Surface *time_surface = TTF_RenderText_Blended(fonts[FONT_SIZE_14], time_row_text, colors[COLOR_WHITE_140]);
        SDL_Texture *time_texture = SDL_CreateTextureFromSurface(renderer, time_surface);
        SDL_Rect time_texture_rect = {.x = camera->w - (box_width - 2.5 * padding),
                                      .y = camera->h - (box_height + padding) + star_name_height + entry_height,
                                      .w = time_surface->w,
                                      .h = time_surface->h};

        SDL_FreeSurface(time_surface);
        SDL_RenderCopy(renderer, time_texture, NULL, &time_texture_rect);
        SDL_DestroyTexture(time_texture);
    }
}

/**
 * Measures the current frames per second (FPS).
 *
 * @param game_state A pointer to the current GameState object.
 * @param last_time A pointer to the last time the FPS was updated.
 * @param frame_count A pointer to the current frame count.
 *
 * @return void
 */
void console_measure_fps(GameState *game_state, unsigned int *last_time, unsigned int *frame_count)
{
    unsigned int current_time = SDL_GetTicks();
    unsigned int time_diff = current_time - *last_time;

    // Only update FPS once per second
    if (time_diff >= 1000)
    {
        game_state->fps = *frame_count;
        *frame_count = 0;
        *last_time = current_time;
    }
    else
        *frame_count += 1;
}
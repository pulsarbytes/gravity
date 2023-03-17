/*
 * controls.c
 */

#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"
#include "../include/controls.h"

// External variable definitions
extern TTF_Font *fonts[];
extern SDL_Renderer *renderer;
extern SDL_Color colors[];

// Static function prototypes
static void controls_draw_table(const GameState *, const Camera *);

/**
 * Creates the controls table.
 *
 * @param game_state A pointer to the current game state.
 *
 * @return void
 */
void controls_create_table(GameState *game_state, const Camera *camera)
{
    // Navigate mode
    sprintf(game_state->controls_groups[0].title, "%s", "Navigate mode");
    game_state->controls_groups[0].num_controls = 12;

    sprintf(game_state->controls_groups[0].controls[0].key, "%s", "Up");
    sprintf(game_state->controls_groups[0].controls[1].key, "%s", "Down");
    sprintf(game_state->controls_groups[0].controls[2].key, "%s", "Right");
    sprintf(game_state->controls_groups[0].controls[3].key, "%s", "Left");
    sprintf(game_state->controls_groups[0].controls[4].key, "%s", "C");
    sprintf(game_state->controls_groups[0].controls[5].key, "%s", "M");
    sprintf(game_state->controls_groups[0].controls[6].key, "%s", "O");
    sprintf(game_state->controls_groups[0].controls[7].key, "%s", "S");
    sprintf(game_state->controls_groups[0].controls[8].key, "%s", "U");
    sprintf(game_state->controls_groups[0].controls[9].key, "%s", "[ or Mouse Wheel Backward");
    sprintf(game_state->controls_groups[0].controls[10].key, "%s", "] or Mouse Wheel Forward");
    sprintf(game_state->controls_groups[0].controls[11].key, "%s", "Space");

    sprintf(game_state->controls_groups[0].controls[0].description, "%s", "Forward thrust");
    sprintf(game_state->controls_groups[0].controls[1].description, "%s", "Reverse thrust");
    sprintf(game_state->controls_groups[0].controls[2].description, "%s", "Rotate right");
    sprintf(game_state->controls_groups[0].controls[3].description, "%s", "Rotate left");
    sprintf(game_state->controls_groups[0].controls[4].description, "%s", "Toggle camera");
    sprintf(game_state->controls_groups[0].controls[5].description, "%s", "Enter Map mode");
    sprintf(game_state->controls_groups[0].controls[6].description, "%s", "Show orbits");
    sprintf(game_state->controls_groups[0].controls[7].description, "%s", "Stop ship");
    sprintf(game_state->controls_groups[0].controls[8].description, "%s", "Enter Universe mode");
    sprintf(game_state->controls_groups[0].controls[9].description, "%s", "Zoom out");
    sprintf(game_state->controls_groups[0].controls[10].description, "%s", "Zoom in");
    sprintf(game_state->controls_groups[0].controls[11].description, "%s", "Reset zoom scale");

    // Map mode
    sprintf(game_state->controls_groups[1].title, "%s", "Map mode");
    game_state->controls_groups[1].num_controls = 12;

    sprintf(game_state->controls_groups[1].controls[0].key, "%s", "Up");
    sprintf(game_state->controls_groups[1].controls[1].key, "%s", "Down");
    sprintf(game_state->controls_groups[1].controls[2].key, "%s", "Right");
    sprintf(game_state->controls_groups[1].controls[3].key, "%s", "Left");
    sprintf(game_state->controls_groups[1].controls[4].key, "%s", "N");
    sprintf(game_state->controls_groups[1].controls[5].key, "%s", "O");
    sprintf(game_state->controls_groups[1].controls[6].key, "%s", "U");
    sprintf(game_state->controls_groups[1].controls[7].key, "%s", "[ or Mouse Wheel Backward");
    sprintf(game_state->controls_groups[1].controls[8].key, "%s", "] or Mouse Wheel Forward");
    sprintf(game_state->controls_groups[1].controls[9].key, "%s", "Space");
    sprintf(game_state->controls_groups[1].controls[10].key, "%s", "Left Mouse Button Click");
    sprintf(game_state->controls_groups[1].controls[11].key, "%s", "Left Mouse Button Double Click");

    sprintf(game_state->controls_groups[1].controls[0].description, "%s", "Scroll up");
    sprintf(game_state->controls_groups[1].controls[1].description, "%s", "Scroll down");
    sprintf(game_state->controls_groups[1].controls[2].description, "%s", "Scroll right");
    sprintf(game_state->controls_groups[1].controls[3].description, "%s", "Scroll left");
    sprintf(game_state->controls_groups[1].controls[4].description, "%s", "Enter Navigate mode");
    sprintf(game_state->controls_groups[1].controls[5].description, "%s", "Show orbits");
    sprintf(game_state->controls_groups[1].controls[6].description, "%s", "Enter Universe mode");
    sprintf(game_state->controls_groups[1].controls[7].description, "%s", "Zoom out");
    sprintf(game_state->controls_groups[1].controls[8].description, "%s", "Zoom in");
    sprintf(game_state->controls_groups[1].controls[9].description, "%s", "Reset zoom scale");
    sprintf(game_state->controls_groups[1].controls[10].description, "%s", "Select star");
    sprintf(game_state->controls_groups[1].controls[11].description, "%s", "Center star");

    // Universe mode
    sprintf(game_state->controls_groups[2].title, "%s", "Universe mode");
    game_state->controls_groups[2].num_controls = 10;

    sprintf(game_state->controls_groups[2].controls[0].key, "%s", "Up");
    sprintf(game_state->controls_groups[2].controls[1].key, "%s", "Down");
    sprintf(game_state->controls_groups[2].controls[2].key, "%s", "Right");
    sprintf(game_state->controls_groups[2].controls[3].key, "%s", "Left");
    sprintf(game_state->controls_groups[2].controls[4].key, "%s", "M");
    sprintf(game_state->controls_groups[2].controls[5].key, "%s", "N");
    sprintf(game_state->controls_groups[2].controls[6].key, "%s", "[ or Mouse Wheel Backward");
    sprintf(game_state->controls_groups[2].controls[7].key, "%s", "] or Mouse Wheel Forward");
    sprintf(game_state->controls_groups[2].controls[8].key, "%s", "Space");
    sprintf(game_state->controls_groups[2].controls[9].key, "%s", "Left Mouse Button Double Click");

    sprintf(game_state->controls_groups[2].controls[0].description, "%s", "Scroll up");
    sprintf(game_state->controls_groups[2].controls[1].description, "%s", "Scroll down");
    sprintf(game_state->controls_groups[2].controls[2].description, "%s", "Scroll right");
    sprintf(game_state->controls_groups[2].controls[3].description, "%s", "Scroll left");
    sprintf(game_state->controls_groups[2].controls[4].description, "%s", "Enter Map mode");
    sprintf(game_state->controls_groups[2].controls[5].description, "%s", "Enter Navigate mode");
    sprintf(game_state->controls_groups[2].controls[6].description, "%s", "Zoom out");
    sprintf(game_state->controls_groups[2].controls[7].description, "%s", "Zoom in");
    sprintf(game_state->controls_groups[2].controls[8].description, "%s", "Reset zoom scale");
    sprintf(game_state->controls_groups[2].controls[9].description, "%s", "Center star");

    // General Controls
    sprintf(game_state->controls_groups[3].title, "%s", "General controls");
    game_state->controls_groups[3].num_controls = 2;

    sprintf(game_state->controls_groups[3].controls[0].key, "%s", "F");
    sprintf(game_state->controls_groups[3].controls[1].key, "%s", "Esc");

    sprintf(game_state->controls_groups[3].controls[0].description, "%s", "Toggle FPS");
    sprintf(game_state->controls_groups[3].controls[1].description, "%s", "Show menu / Pause");

    // Initialize game_state variables
    int line_height = 50;
    int margin = 50;
    int padding = 10;

    game_state->table_top_row = 0;
    game_state->table_num_rows_displayed = (camera->h - 2 * margin) / (line_height + 2 * padding + 1);
    game_state->table_num_rows = MAX_CONTROLS_GROUPS * 2 - 1; // Add number of groups titles + empty rows - 1 empty row for last group

    for (int i = 0; i < MAX_CONTROLS_GROUPS; i++)
    {
        game_state->table_num_rows += game_state->controls_groups[i].num_controls;
    }
}

/**
 * Draws a table displaying the game controls and their corresponding keys and descriptions.
 *
 * @param game_state A pointer to the current game state.
 * @param camera A pointer to the camera used to display the game.
 *
 * @return void
 */
static void controls_draw_table(const GameState *game_state, const Camera *camera)
{
    int margin = 50;
    int line_height = 50;
    int padding = 10;

    // Define the table rectangle
    SDL_Rect table_rect = {
        (camera->w / 2) - (camera->w / 4),
        margin,
        (camera->w / 2) + (camera->w / 4) - margin,
        game_state->table_num_rows * (line_height + 2 * padding + 1) // Add 1 for border bottom
    };

    SDL_SetRenderDrawColor(renderer, 12, 12, 12, 230);

    // Define the cell width and height
    int cell_width = table_rect.w / 2;
    int cell_height = line_height + 2 * padding + 1;

    // Define the starting coordinates for the first cell
    int x = table_rect.x;
    int y = table_rect.y - (game_state->table_top_row * cell_height);

    // Loop through groups
    for (int i = 0; i < MAX_CONTROLS_GROUPS && y < table_rect.y + table_rect.h; i++)
    {
        if (y >= margin && y < camera->h - margin)
        {
            // Render the group title
            SDL_Surface *title_surface = TTF_RenderText_Solid(fonts[FONT_SIZE_26], game_state->controls_groups[i].title, colors[COLOR_WHITE_140]);
            SDL_Texture *title_texture = SDL_CreateTextureFromSurface(renderer, title_surface);
            SDL_Rect title_rect = {
                x + padding,
                y + padding + (line_height / 2) - (title_surface->h / 2),
                title_surface->w,
                title_surface->h};
            SDL_RenderCopy(renderer, title_texture, NULL, &title_rect);
            SDL_FreeSurface(title_surface);
            SDL_DestroyTexture(title_texture);
        }

        y += cell_height;

        // Loop through the controls and render each cell
        for (int j = 0; j < game_state->controls_groups[i].num_controls && y < table_rect.y + table_rect.h; j++)
        {
            if (y >= margin && y < camera->h - margin)
            {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 25);
                SDL_RenderDrawLine(renderer, table_rect.x, y, table_rect.x + table_rect.w, y);

                // Render the control key and description
                SDL_Surface *key_surface = TTF_RenderText_Solid(fonts[FONT_SIZE_18], game_state->controls_groups[i].controls[j].key, colors[COLOR_WHITE_140]);
                SDL_Texture *key_texture = SDL_CreateTextureFromSurface(renderer, key_surface);
                SDL_Surface *description_surface = TTF_RenderText_Solid(fonts[FONT_SIZE_18], game_state->controls_groups[i].controls[j].description, colors[COLOR_WHITE_140]);
                SDL_Texture *description_texture = SDL_CreateTextureFromSurface(renderer, description_surface);

                SDL_Rect key_rect = {
                    x + padding,
                    y + padding + (line_height / 2) - (key_surface->h / 2),
                    key_surface->w,
                    key_surface->h};
                SDL_Rect description_rect = {
                    x + padding + cell_width,
                    y + padding + (line_height / 2) - (description_surface->h / 2),
                    description_surface->w,
                    description_surface->h};

                SDL_RenderCopy(renderer, key_texture, NULL, &key_rect);
                SDL_RenderCopy(renderer, description_texture, NULL, &description_rect);

                SDL_FreeSurface(key_surface);
                SDL_FreeSurface(description_surface);
                SDL_DestroyTexture(key_texture);
                SDL_DestroyTexture(description_texture);
            }

            // Update the y coordinate for the next cell
            y += cell_height;
        }

        if (i < MAX_CONTROLS_GROUPS - 1)
        {
            SDL_RenderDrawLine(renderer, table_rect.x, y, table_rect.x + table_rect.w, y);

            // Add an empty row
            y += cell_height;
        }
    }

    int total_height = camera->h - 2 * margin;
    int scrollbar_height = total_height * game_state->table_num_rows_displayed / game_state->table_num_rows;
    int scrollbar_y = margin + total_height * game_state->table_top_row / game_state->table_num_rows;

    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_Rect scrollbar_rect = {
        table_rect.x + table_rect.w,
        scrollbar_y,
        10,
        scrollbar_height};
    SDL_RenderFillRect(renderer, &scrollbar_rect);
}

/**
 * Runs the controls state by updating and rendering the background stars, logo, menu, galaxy, speed lines and controls table.
 *
 * @param game_state A pointer to the current game state.
 * @param input_state A pointer to the current input state.
 * @param is_game_started A boolean indicating whether the game has already started.
 * @param nav_state A pointer to the current navigation state.
 * @param bstars A pointer to an array of background stars.
 * @param menustars A pointer to an array of menu galaxy stars.
 * @param camera A pointer to the camera object.
 *
 * @return void
 */
void controls_run_state(GameState *game_state, InputState *input_state, bool is_game_started, const NavigationState *nav_state, Bstar *bstars, Gstar *menustars, const Camera *camera)
{
    // Draw background stars
    Speed speed = {.vx = 1000, .vy = 0};
    gfx_update_bstars_position(game_state->state, input_state->camera_on, nav_state, bstars, camera, speed, 0);

    // Draw logo
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderFillRect(renderer, &game_state->logo.rect);
    SDL_RenderCopy(renderer, game_state->logo.text_texture, NULL, &game_state->logo.texture_rect);

    // Draw menu
    menu_draw_menu(game_state, input_state, is_game_started);

    // Draw galaxy
    gfx_draw_menu_galaxy_cloud(camera, menustars);

    // Draw speed lines
    Speed lines_speed = {.vx = 100, .vy = 0};
    gfx_draw_speed_lines(1500, camera, lines_speed);

    // Draw controls table
    controls_draw_table(game_state, camera);

    // Check if mouse is over menu buttons
    if (menu_is_hovering_menu(game_state, input_state))
        SDL_SetCursor(input_state->pointing_cursor);
    else
        SDL_SetCursor(input_state->default_cursor);

    // Check if mouse is over scrollbar
    // to-do
}
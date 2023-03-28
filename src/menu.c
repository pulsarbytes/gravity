/*
 * menu.c
 */

#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"
#include "../include/menu.h"

// External variable definitions
extern TTF_Font *fonts[];
extern SDL_Renderer *renderer;
extern SDL_Color colors[];

// Static function prototypes
static void menu_create_logo(MenuButton *logo);
static void menu_draw_footer(const Camera *camera);
static void menu_populate_menu_array(MenuButton menu[]);

/**
 * Creates the main menu by populating the menu array, creating the logo, generating and placing menu stars.
 *
 * @param game_state A pointer to the current GameState object.
 * @param nav_state The current NavigationState object.
 * @param menustars The array of menu stars to be generated and placed.
 *
 * @return void
 */
void menu_create(GameState *game_state, NavigationState nav_state, Gstar *menustars)
{
    // Populate menu
    menu_populate_menu_array(game_state->menu);

    // Create logo
    menu_create_logo(&game_state->logo);

    Point menu_galaxy_position = {.x = -140000, .y = -70000};
    Galaxy *menu_galaxy = galaxies_get_entry(nav_state.galaxies, menu_galaxy_position);
    gfx_generate_menu_gstars(menu_galaxy, menustars);
}

/**
 * Create the game logo.
 *
 * @param logo Pointer to the MenuButton struct to store the logo information in.
 *
 * @return void
 */
static void menu_create_logo(MenuButton *logo)
{
    strcpy(logo->text, "Gravity");

    // Set the position of the logo
    logo->rect.w = 300;
    logo->rect.h = 200;
    logo->rect.x = 50;
    logo->rect.y = 0;

    // Create a texture from the text
    SDL_Surface *logo_surface = TTF_RenderText_Blended(fonts[LOGO_FONT_SIZE_32], logo->text, colors[COLOR_PLANET_1]);
    SDL_Texture *logo_texture = SDL_CreateTextureFromSurface(renderer, logo_surface);
    logo->text_texture = logo_texture;

    // Set the position of the text within the button
    logo->texture_rect.w = logo_surface->w;
    logo->texture_rect.h = logo_surface->h;
    logo->texture_rect.x = logo->rect.x + (logo->rect.w - logo->texture_rect.w) / 2;
    logo->texture_rect.y = logo->rect.y + (logo->rect.h - logo->texture_rect.h) / 2;

    SDL_FreeSurface(logo_surface);
}

/**
 * Renders the footer onto the screen.
 *
 * @param camera A pointer to the current Camera object.
 *
 * @return void
 */
static void menu_draw_footer(const Camera *camera)
{
    int margin = 40;

    char footer_text[] = "Gravity v1.4.3 - Copyright \xA9 2020 Yannis Maragos";
    SDL_Surface *footer_text_surface = TTF_RenderText_Blended(fonts[FONT_SIZE_15], footer_text, colors[COLOR_WHITE_100]);
    SDL_Texture *footer_text_texture = SDL_CreateTextureFromSurface(renderer, footer_text_surface);
    SDL_Rect footer_text_rect = {camera->w - margin - footer_text_surface->w, camera->h - margin - footer_text_surface->h, footer_text_surface->w, footer_text_surface->h};
    SDL_RenderCopy(renderer, footer_text_texture, NULL, &footer_text_rect);
    SDL_FreeSurface(footer_text_surface);
    SDL_DestroyTexture(footer_text_texture);
}

/**
 * Renders the menu buttons onto the screen.
 *
 * @param game_state A pointer to the current GameState object.
 * @param input_state A pointer to the current InputState object.
 * @param is_game_started Flag indicating if the game has started.
 *
 * @return void
 */
void menu_draw_menu(GameState *game_state, InputState *input_state, bool is_game_started)
{
    int num_buttons = 0;

    for (int i = 0; i < MENU_BUTTON_COUNT; i++)
    {
        if (!game_state->menu[i].disabled &&
            input_state->mouse_position.x >= game_state->menu[i].rect.x &&
            input_state->mouse_position.x <= game_state->menu[i].rect.x + game_state->menu[i].rect.w &&
            input_state->mouse_position.y >= game_state->menu[i].rect.y &&
            input_state->mouse_position.y <= game_state->menu[i].rect.y + game_state->menu[i].rect.h)
        {
            input_state->selected_menu_button_index = i;
        }

        if (i == input_state->selected_menu_button_index && game_state->menu[i].disabled)
        {
            do
            {
                input_state->selected_menu_button_index = (input_state->selected_menu_button_index + 1) % MENU_BUTTON_COUNT;
            } while (game_state->menu[input_state->selected_menu_button_index].disabled);
        }

        if (game_state->menu[i].disabled)
            continue;

        if (i == input_state->selected_menu_button_index)
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 40);
        else
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);

        // Set the position of the button
        game_state->menu[i].rect.x = 50;
        game_state->menu[i].rect.y = 200 + 50 * num_buttons;

        SDL_RenderFillRect(renderer, &game_state->menu[i].rect);

        // Set the position of the text within the button
        game_state->menu[i].texture_rect.x = game_state->menu[i].rect.x + (game_state->menu[i].rect.w - game_state->menu[i].texture_rect.w) / 2;
        game_state->menu[i].texture_rect.y = game_state->menu[i].rect.y + (game_state->menu[i].rect.h - game_state->menu[i].texture_rect.h) / 2;

        // Render the text texture onto the button
        SDL_RenderCopy(renderer, game_state->menu[i].text_texture, NULL, &game_state->menu[i].texture_rect);

        num_buttons++;
    }
}

/**
 * Checks if the mouse is over the current menu button and changes mouse cursor.
 *
 * @param game_state A pointer to the current GameState object.
 * @param input_state A pointer to the current InputState object.
 *
 * @return void
 */
bool menu_is_hovering_menu(GameState *game_state, InputState *input_state)
{
    bool is_hovering_menu = false;

    for (int i = 0; i < MENU_BUTTON_COUNT; i++)
    {
        if (game_state->menu[i].disabled)
            continue;

        Point rect[4];

        int x1 = game_state->menu[i].rect.x;
        int y1 = game_state->menu[i].rect.y;
        int x2 = x1 + game_state->menu[i].rect.w;
        int y2 = y1 + game_state->menu[i].rect.h;

        rect[0].x = x1;
        rect[0].y = y2;

        rect[1].x = x2;
        rect[1].y = y2;

        rect[2].x = x2;
        rect[2].y = y1;

        rect[3].x = x1;
        rect[3].y = y1;

        if (maths_is_point_in_rectangle(input_state->mouse_position, rect))
        {
            is_hovering_menu = true;
            break;
        }
    }

    return is_hovering_menu;
}

/**
 * Populates a given menu button array with button information, including the text to be displayed on each button,
 * the state to transition to when each button is pressed, and whether or not each button should initially be disabled.
 *
 * @param menu An array of MenuButton struct to be populated with button information.
 *
 * @return void
 */
static void menu_populate_menu_array(MenuButton menu[])
{
    // Start
    memset(menu[MENU_BUTTON_START].text, 0, sizeof(menu[MENU_BUTTON_START].text));
    strcpy(menu[MENU_BUTTON_START].text, "Start");
    menu[MENU_BUTTON_START].state = NAVIGATE;
    menu[MENU_BUTTON_START].disabled = false;

    // Resume
    memset(menu[MENU_BUTTON_RESUME].text, 0, sizeof(menu[MENU_BUTTON_RESUME].text));
    strcpy(menu[MENU_BUTTON_RESUME].text, "Resume");
    menu[MENU_BUTTON_RESUME].state = RESUME;
    menu[MENU_BUTTON_RESUME].disabled = true;

    // New
    memset(menu[MENU_BUTTON_NEW].text, 0, sizeof(menu[MENU_BUTTON_NEW].text));
    strcpy(menu[MENU_BUTTON_NEW].text, "New Game");
    menu[MENU_BUTTON_NEW].state = NEW;
    menu[MENU_BUTTON_NEW].disabled = true;

    // Controls
    memset(menu[MENU_BUTTON_CONTROLS].text, 0, sizeof(menu[MENU_BUTTON_CONTROLS].text));
    strcpy(menu[MENU_BUTTON_CONTROLS].text, "Controls");
    menu[MENU_BUTTON_CONTROLS].state = CONTROLS;
    menu[MENU_BUTTON_CONTROLS].disabled = false;

    // Exit
    memset(menu[MENU_BUTTON_EXIT].text, 0, sizeof(menu[MENU_BUTTON_EXIT].text));
    strcpy(menu[MENU_BUTTON_EXIT].text, "Exit");
    menu[MENU_BUTTON_EXIT].state = QUIT;
    menu[MENU_BUTTON_EXIT].disabled = false;

    // Back
    memset(menu[MENU_BUTTON_BACK].text, 0, sizeof(menu[MENU_BUTTON_BACK].text));
    strcpy(menu[MENU_BUTTON_BACK].text, "Back");
    menu[MENU_BUTTON_BACK].state = MENU;
    menu[MENU_BUTTON_BACK].disabled = true;

    for (int i = 0; i < MENU_BUTTON_COUNT; i++)
    {
        menu[i].rect.x = 0;
        menu[i].rect.y = 0;
        menu[i].rect.w = 300;
        menu[i].rect.h = 50;

        // Create a texture from the button text
        SDL_Surface *text_surface = TTF_RenderText_Blended(fonts[FONT_SIZE_15], menu[i].text, colors[COLOR_WHITE_180]);
        SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
        menu[i].text_texture = text_texture;
        menu[i].texture_rect.x = 0;
        menu[i].texture_rect.y = 0;
        menu[i].texture_rect.w = text_surface->w;
        menu[i].texture_rect.h = text_surface->h;

        SDL_FreeSurface(text_surface);
    }
}

/**
 * Runs the menu state by updating and rendering the background stars, logo, menu, galaxy, and speed lines.
 *
 * @param game_state A pointer to the current GameState object.
 * @param input_state A pointer to the current InputState object.
 * @param is_game_started A boolean indicating whether the game has already started.
 * @param nav_state A pointer to the current NavigationState object.
 * @param bstars A pointer to an array of background stars.
 * @param menustars A pointer to an array of menu galaxy stars.
 * @param camera A pointer to the current Camera object.
 *
 * @return void
 */
void menu_run_state(GameState *game_state, InputState *input_state, bool is_game_started, const NavigationState *nav_state, Bstar *bstars, Gstar *menustars, Camera *camera)
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

    // Draw footer
    menu_draw_footer(camera);
}

/**
 * Update the menu entries by updating the text color and disabling buttons as appropriate.
 *
 * @param game_state A pointer to the current GameState object.
 * @param game_events A pointer to the current GameEvents object.
 *
 * @return void
 */
void menu_update_menu_entries(GameState *game_state, GameEvents *game_events)
{
    if (game_state->state == MENU)
    {
        if (game_events->is_game_started)
        {
            game_state->menu[MENU_BUTTON_START].disabled = true;
            game_state->menu[MENU_BUTTON_RESUME].disabled = false;
            game_state->menu[MENU_BUTTON_NEW].disabled = false;
            game_state->menu[MENU_BUTTON_CONTROLS].disabled = false;
            game_state->menu[MENU_BUTTON_EXIT].disabled = false;
            game_state->menu[MENU_BUTTON_BACK].disabled = true;
        }
        else
        {
            game_state->menu[MENU_BUTTON_START].disabled = false;
            game_state->menu[MENU_BUTTON_RESUME].disabled = true;
            game_state->menu[MENU_BUTTON_NEW].disabled = true;
            game_state->menu[MENU_BUTTON_CONTROLS].disabled = false;
            game_state->menu[MENU_BUTTON_EXIT].disabled = false;
            game_state->menu[MENU_BUTTON_BACK].disabled = true;
        }
    }
    else if (game_state->state == CONTROLS)
    {
        game_state->menu[MENU_BUTTON_START].disabled = true;
        game_state->menu[MENU_BUTTON_RESUME].disabled = true;
        game_state->menu[MENU_BUTTON_NEW].disabled = true;
        game_state->menu[MENU_BUTTON_EXIT].disabled = true;
        game_state->menu[MENU_BUTTON_CONTROLS].disabled = true;
        game_state->menu[MENU_BUTTON_BACK].disabled = false;
    }
}
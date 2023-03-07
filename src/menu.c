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
static void menu_draw_menu(GameState *, InputState *, int game_started);
static void menu_populate_menu_array(MenuButton menu[]);

/**
 * Creates the main menu by populating the menu array, creating the logo, generating and placing menu stars.
 *
 * @param game_state The current state of the game.
 * @param nav_state The current navigation state of the game.
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
    SDL_Surface *logo_surface = TTF_RenderText_Solid(fonts[FONT_SIZE_36], logo->text, colors[COLOR_WHITE_255]);
    SDL_Texture *logo_texture = SDL_CreateTextureFromSurface(renderer, logo_surface);
    logo->texture = logo_texture;

    // Set the position of the text within the button
    logo->texture_rect.w = logo_surface->w;
    logo->texture_rect.h = logo_surface->h;
    logo->texture_rect.x = logo->rect.x + (logo->rect.w - logo->texture_rect.w) / 2;
    logo->texture_rect.y = logo->rect.y + (logo->rect.h - logo->texture_rect.h) / 2;

    SDL_FreeSurface(logo_surface);
}

/**
 * Renders the menu buttons onto the screen.
 *
 * @param game_state A pointer to the game state struct.
 * @param input_state A pointer to the input state struct.
 * @param game_started Flag indicating if the game has started.
 *
 * @return void
 */
static void menu_draw_menu(GameState *game_state, InputState *input_state, int game_started)
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
            input_state->selected_button = i;
        }

        if (i == input_state->selected_button && game_state->menu[i].disabled)
        {
            do
            {
                input_state->selected_button = (input_state->selected_button + 1) % MENU_BUTTON_COUNT;
            } while (game_state->menu[input_state->selected_button].disabled);
        }

        if (game_started && i == MENU_BUTTON_START)
            continue;

        if (!game_started && i == MENU_BUTTON_RESUME)
            continue;

        if (!game_started && i == MENU_BUTTON_NEW)
            continue;

        if (i == input_state->selected_button)
        {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 40);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        }

        // Set the position of the button
        game_state->menu[i].rect.x = 50;
        game_state->menu[i].rect.y = 200 + 50 * num_buttons;

        SDL_RenderFillRect(renderer, &game_state->menu[i].rect);

        // Set the position of the text within the button
        game_state->menu[i].texture_rect.x = game_state->menu[i].rect.x + (game_state->menu[i].rect.w - game_state->menu[i].texture_rect.w) / 2;
        game_state->menu[i].texture_rect.y = game_state->menu[i].rect.y + (game_state->menu[i].rect.h - game_state->menu[i].texture_rect.h) / 2;

        // Render the text texture onto the button
        SDL_RenderCopy(renderer, game_state->menu[i].texture, NULL, &game_state->menu[i].texture_rect);

        num_buttons++;
    }
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
    strcpy(menu[MENU_BUTTON_START].text, "Start");
    menu[MENU_BUTTON_START].state = NAVIGATE;
    menu[MENU_BUTTON_START].disabled = false;

    // Resume
    strcpy(menu[MENU_BUTTON_RESUME].text, "Resume");
    menu[MENU_BUTTON_RESUME].state = RESUME;
    menu[MENU_BUTTON_RESUME].disabled = true;

    // New
    strcpy(menu[MENU_BUTTON_NEW].text, "New Game");
    menu[MENU_BUTTON_NEW].state = NEW;
    menu[MENU_BUTTON_NEW].disabled = true;

    // Exit
    strcpy(menu[MENU_BUTTON_EXIT].text, "Exit");
    menu[MENU_BUTTON_EXIT].state = QUIT;
    menu[MENU_BUTTON_EXIT].disabled = false;

    for (int i = 0; i < MENU_BUTTON_COUNT; i++)
    {
        menu[i].rect.w = 300;
        menu[i].rect.h = 50;

        // Create a texture from the button text
        SDL_Surface *text_surface = TTF_RenderText_Solid(fonts[FONT_SIZE_14], menu[i].text, colors[COLOR_WHITE_255]);
        SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
        menu[i].texture = text_texture;
        menu[i].texture_rect.w = text_surface->w;
        menu[i].texture_rect.h = text_surface->h;

        SDL_FreeSurface(text_surface);
    }
}

/**
 * Runs the menu state by updating and rendering the background stars, logo, menu, galaxy, and speed lines.
 *
 * @param game_state A pointer to the current game state.
 * @param input_state A pointer to the current input state.
 * @param game_started An integer value indicating whether the game has already started.
 * @param nav_state A pointer to the current navigation state.
 * @param bstars A pointer to an array of background stars.
 * @param menustars A pointer to an array of menu galaxy stars.
 * @param camera A pointer to the camera object.
 *
 * @return void
 */
void menu_run_menu_state(GameState *game_state, InputState *input_state, int game_started, const NavigationState *nav_state, Bstar *bstars, Gstar *menustars, Camera *camera)
{
    // Draw background stars
    Speed speed = {.vx = 1000, .vy = 0};
    gfx_update_bstars_position(game_state->state, input_state->camera_on, nav_state, bstars, camera, speed, 0);

    // Draw logo
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderFillRect(renderer, &game_state->logo.rect);
    SDL_RenderCopy(renderer, game_state->logo.texture, NULL, &game_state->logo.texture_rect);

    // Draw menu
    menu_draw_menu(game_state, input_state, game_started);

    // Draw galaxy
    gfx_draw_menu_galaxy_cloud(camera, menustars);

    // Draw speed lines
    Speed lines_speed = {.vx = 100, .vy = 0};
    gfx_draw_speed_lines(1500, camera, lines_speed);
}

/**
 * Update the menu entries by updating the text color and disabling buttons as appropriate.
 *
 * @param game_state A pointer to the game state struct.
 *
 * @return void
 */
void menu_update_menu_entries(GameState *game_state)
{
    // Update Start button
    game_state->menu[MENU_BUTTON_START].disabled = true;
    SDL_Surface *start_surface = TTF_RenderText_Solid(fonts[FONT_SIZE_14], game_state->menu[MENU_BUTTON_START].text, colors[COLOR_WHITE_100]);
    SDL_DestroyTexture(game_state->menu[MENU_BUTTON_START].texture);
    SDL_Texture *start_texture = SDL_CreateTextureFromSurface(renderer, start_surface);
    game_state->menu[MENU_BUTTON_START].texture = start_texture;
    SDL_FreeSurface(start_surface);

    // Update Resume button
    game_state->menu[MENU_BUTTON_RESUME].disabled = false;
    SDL_Surface *resume_surface = TTF_RenderText_Solid(fonts[FONT_SIZE_14], game_state->menu[MENU_BUTTON_RESUME].text, colors[COLOR_WHITE_255]);
    SDL_DestroyTexture(game_state->menu[MENU_BUTTON_RESUME].texture);
    SDL_Texture *resume_texture = SDL_CreateTextureFromSurface(renderer, resume_surface);
    game_state->menu[MENU_BUTTON_RESUME].texture = resume_texture;
    SDL_FreeSurface(resume_surface);

    // Update New button
    game_state->menu[MENU_BUTTON_NEW].disabled = false;
    SDL_Surface *new_surface = TTF_RenderText_Solid(fonts[FONT_SIZE_14], game_state->menu[MENU_BUTTON_NEW].text, colors[COLOR_WHITE_255]);
    SDL_DestroyTexture(game_state->menu[MENU_BUTTON_NEW].texture);
    SDL_Texture *new_texture = SDL_CreateTextureFromSurface(renderer, new_surface);
    game_state->menu[MENU_BUTTON_NEW].texture = new_texture;
    SDL_FreeSurface(new_surface);
}
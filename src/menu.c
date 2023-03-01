/*
 * menu.c - Definitions for menu functions.
 */

#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"

// External variable definitions
extern TTF_Font *fonts[];
extern SDL_Renderer *renderer;
extern SDL_Color colors[];

// Function prototypes
void create_menu(MenuButton menu[]);
void create_logo(MenuButton *logo);
void update_menu(GameState *, int game_started);
void onMenu(GameState *, InputState *, int game_started, const NavigationState *, Bstar bstars[], Gstar menustars[], Camera *);

// External function prototypes
void update_bstars(int state, int camera_on, const NavigationState *, Bstar bstars[], const Camera *, Speed, double distance);
void draw_menu_galaxy_cloud(const Camera *, Gstar menustars[]);
void draw_speed_lines(float velocity, const Camera *, Speed);

void create_menu(MenuButton menu[])
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

void create_logo(MenuButton *logo)
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

void update_menu(GameState *game_state, int game_started)
{
    if (game_started)
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
}

void onMenu(GameState *game_state, InputState *input_state, int game_started, const NavigationState *nav_state, Bstar bstars[], Gstar menustars[], Camera *camera)
{
    int num_buttons = 0;

    // Draw background stars
    Speed speed = {.vx = 1000, .vy = 0};
    update_bstars(game_state->state, input_state->camera_on, nav_state, bstars, camera, speed, 0);

    // Draw logo
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderFillRect(renderer, &game_state->logo.rect);
    SDL_RenderCopy(renderer, game_state->logo.texture, NULL, &game_state->logo.texture_rect);

    // Draw menu
    for (int i = 0; i < MENU_BUTTON_COUNT; i++)
    {
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

    // Draw galaxy
    draw_menu_galaxy_cloud(camera, menustars);

    // Draw speed lines
    Speed lines_speed = {.vx = 100, .vy = 0};
    draw_speed_lines(1500, camera, lines_speed);
}
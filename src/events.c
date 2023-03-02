/*
 * events.c - Polls for currently pending events.
 */

#include <SDL2/SDL.h>

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"
#include "../include/events.h"

/*
 * Poll SDL events.
 */
void poll_events(GameState *game_state, InputState *input_state, GameEvents *game_events)
{
    SDL_Event event;
    static int save_state;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            change_state(game_state, game_events, QUIT);
            break;
        case SDL_MOUSEBUTTONDOWN:
            int x, y;
            SDL_GetMouseState(&x, &y);
            printf("\nButton was clicked!");
            break;
        case SDL_KEYDOWN:
            switch (event.key.keysym.scancode)
            {
            case SDL_SCANCODE_C:
                if (game_state->state == NAVIGATE)
                    input_state->camera_on = !input_state->camera_on;
                else if (game_state->state == MAP || game_state->state == UNIVERSE)
                    input_state->camera_on = ON;
                break;
            case SDL_SCANCODE_K:
                input_state->console = !input_state->console;
                break;
            case SDL_SCANCODE_M:
                if (game_state->state == UNIVERSE)
                    game_events->universe_exit = ON;
                if (game_state->state == NAVIGATE || game_state->state == UNIVERSE)
                {
                    change_state(game_state, game_events, MAP);
                    game_events->map_enter = ON;
                    input_state->camera_on = ON;
                }
                break;
            case SDL_SCANCODE_N:
                if (game_state->state == MAP)
                {
                    change_state(game_state, game_events, NAVIGATE);
                    game_events->map_exit = ON;
                }
                else if (game_state->state == UNIVERSE)
                {
                    change_state(game_state, game_events, NAVIGATE);
                    game_events->universe_exit = ON;
                }
                break;
            case SDL_SCANCODE_O:
                input_state->orbits_on = !input_state->orbits_on;
                break;
            case SDL_SCANCODE_S:
                if (game_state->state == NAVIGATE)
                    input_state->stop = ON;
                break;
            case SDL_SCANCODE_U:
                if (game_state->state == MAP)
                    game_events->map_exit = ON;
                if (game_state->state == NAVIGATE || game_state->state == MAP)
                {
                    change_state(game_state, game_events, UNIVERSE);
                    game_events->universe_enter = ON;
                    input_state->camera_on = ON;
                }
                break;
            case SDL_SCANCODE_LEFT:
                input_state->right = OFF;
                input_state->left = ON;
                break;
            case SDL_SCANCODE_RIGHT:
                input_state->left = OFF;
                input_state->right = ON;
                break;
            case SDL_SCANCODE_UP:
                if (game_state->state == MENU)
                {
                    do
                    {
                        input_state->selected_button = (input_state->selected_button + MENU_BUTTON_COUNT - 1) % MENU_BUTTON_COUNT;
                    } while (game_state->menu[input_state->selected_button].disabled);
                }
                else
                {
                    input_state->reverse = OFF;
                    input_state->down = OFF;
                    input_state->thrust = ON;
                    input_state->up = ON;
                }
                break;
            case SDL_SCANCODE_DOWN:
                if (game_state->state == MENU)
                {
                    do
                    {
                        input_state->selected_button = (input_state->selected_button + 1) % MENU_BUTTON_COUNT;
                    } while (game_state->menu[input_state->selected_button].disabled);
                }
                else
                {
                    input_state->thrust = OFF;
                    input_state->up = OFF;
                    input_state->reverse = ON;
                    input_state->down = ON;
                }
                break;
            case SDL_SCANCODE_RETURN:
                if (game_state->state == MENU)
                {
                    if (game_state->menu[input_state->selected_button].state == RESUME)
                        change_state(game_state, game_events, save_state);
                    else
                        change_state(game_state, game_events, game_state->menu[input_state->selected_button].state);
                }
                break;
            case SDL_SCANCODE_SPACE:
                if (game_state->state == MAP)
                    game_events->map_center = ON;
                else if (game_state->state == UNIVERSE)
                    game_events->universe_center = ON;
                break;
            case SDL_SCANCODE_ESCAPE:
                if (game_state->state != MENU)
                {
                    save_state = game_state->state;
                    change_state(game_state, game_events, MENU);
                }
                else if (game_state->state == MENU && game_events->game_started)
                    change_state(game_state, game_events, save_state);
                break;
            case SDL_SCANCODE_LEFTBRACKET:
                input_state->zoom_in = OFF;
                input_state->zoom_out = ON;
                break;
            case SDL_SCANCODE_RIGHTBRACKET:
                input_state->zoom_in = ON;
                input_state->zoom_out = OFF;
                break;
            default:
                break;
            }
            break;
        case SDL_KEYUP:
            switch (event.key.keysym.scancode)
            {
            case SDL_SCANCODE_S:
                input_state->stop = OFF;
                break;
            case SDL_SCANCODE_LEFT:
                input_state->left = OFF;
                break;
            case SDL_SCANCODE_RIGHT:
                input_state->right = OFF;
                break;
            case SDL_SCANCODE_UP:
                input_state->thrust = OFF;
                input_state->up = OFF;
                break;
            case SDL_SCANCODE_DOWN:
                input_state->reverse = OFF;
                input_state->down = OFF;
                break;
            case SDL_SCANCODE_LEFTBRACKET:
                input_state->zoom_out = OFF;
                break;
            case SDL_SCANCODE_RIGHTBRACKET:
                input_state->zoom_in = OFF;
                break;
            default:
                break;
            }
            break;
        }
    }
}

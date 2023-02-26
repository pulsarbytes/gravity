/*
 * events.c - Polls for currently pending events.
 */

#include <SDL2/SDL.h>

#include "../include/common.h"
#include "../include/structs.h"

extern int state;
extern int game_started;
extern int left;
extern int right;
extern int up;
extern int down;
extern int thrust;
extern int reverse;
extern int camera_on;
extern int stop;
extern int zoom_in;
extern int zoom_out;
extern int console;
extern int orbits_on;
extern int map_enter;
extern int map_exit;
extern int map_center;
extern int universe_enter;
extern int universe_exit;
extern int universe_center;
extern int selected_button;

void change_state(int new_state);

/*
 * Poll SDL events.
 */
void poll_events(struct menu_button menu[])
{
    SDL_Event event;
    static int save_state;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            change_state(QUIT);
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
                if (state == NAVIGATE)
                    camera_on = !camera_on;
                else if (state == MAP || state == UNIVERSE)
                    camera_on = ON;
                break;
            case SDL_SCANCODE_K:
                console = !console;
                break;
            case SDL_SCANCODE_M:
                if (state == UNIVERSE)
                    universe_exit = ON;
                if (state == NAVIGATE || state == UNIVERSE)
                {
                    change_state(MAP);
                    map_enter = ON;
                    camera_on = ON;
                }
                break;
            case SDL_SCANCODE_N:
                if (state == MAP)
                {
                    change_state(NAVIGATE);
                    map_exit = ON;
                }
                else if (state == UNIVERSE)
                {
                    change_state(NAVIGATE);
                    universe_exit = ON;
                }
                break;
            case SDL_SCANCODE_O:
                orbits_on = !orbits_on;
                break;
            case SDL_SCANCODE_S:
                if (state == NAVIGATE)
                    stop = ON;
                break;
            case SDL_SCANCODE_U:
                if (state == MAP)
                    map_exit = ON;
                if (state == NAVIGATE || state == MAP)
                {
                    change_state(UNIVERSE);
                    universe_enter = ON;
                    camera_on = ON;
                }
                break;
            case SDL_SCANCODE_LEFT:
                right = OFF;
                left = ON;
                break;
            case SDL_SCANCODE_RIGHT:
                left = OFF;
                right = ON;
                break;
            case SDL_SCANCODE_UP:
                if (state == MENU)
                {
                    do
                    {
                        selected_button = (selected_button + MENU_BUTTON_COUNT - 1) % MENU_BUTTON_COUNT;
                    } while (menu[selected_button].disabled);
                }
                else
                {
                    reverse = OFF;
                    down = OFF;
                    thrust = ON;
                    up = ON;
                }
                break;
            case SDL_SCANCODE_DOWN:
                if (state == MENU)
                {
                    do
                    {
                        selected_button = (selected_button + 1) % MENU_BUTTON_COUNT;
                    } while (menu[selected_button].disabled);
                }
                else
                {
                    thrust = OFF;
                    up = OFF;
                    reverse = ON;
                    down = ON;
                }
                break;
            case SDL_SCANCODE_RETURN:
                if (menu[selected_button].state == RESUME)
                    change_state(save_state);
                else
                    change_state(menu[selected_button].state);
                break;
            case SDL_SCANCODE_SPACE:
                if (state == MAP)
                    map_center = ON;
                else if (state == UNIVERSE)
                    universe_center = ON;
                break;
            case SDL_SCANCODE_ESCAPE:
                if (state != MENU)
                {
                    save_state = state;
                    change_state(MENU);
                }
                else if (state == MENU && game_started)
                    change_state(save_state);
                break;
            case SDL_SCANCODE_LEFTBRACKET:
                zoom_in = OFF;
                zoom_out = ON;
                break;
            case SDL_SCANCODE_RIGHTBRACKET:
                zoom_in = ON;
                zoom_out = OFF;
                break;
            default:
                break;
            }
            break;
        case SDL_KEYUP:
            switch (event.key.keysym.scancode)
            {
            case SDL_SCANCODE_S:
                stop = OFF;
                break;
            case SDL_SCANCODE_LEFT:
                left = OFF;
                break;
            case SDL_SCANCODE_RIGHT:
                right = OFF;
                break;
            case SDL_SCANCODE_UP:
                thrust = OFF;
                up = OFF;
                break;
            case SDL_SCANCODE_DOWN:
                reverse = OFF;
                down = OFF;
                break;
            case SDL_SCANCODE_LEFTBRACKET:
                zoom_out = OFF;
                break;
            case SDL_SCANCODE_RIGHTBRACKET:
                zoom_in = OFF;
                break;
            default:
                break;
            }
            break;
        }
    }
}

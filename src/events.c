/*
 * events.c - Polls for currently pending events.
 */

#include <SDL2/SDL.h>

#include "../include/common.h"

extern int state;
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
extern int map_enter;
extern int map_exit;

/*
 * Poll SDL events.
 */
void poll_events(int *quit)
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            *quit = 1;
            break;
        case SDL_KEYDOWN:
            switch (event.key.keysym.scancode)
            {
            case SDL_SCANCODE_LEFT:
                right = OFF;
                left = ON;
                break;
            case SDL_SCANCODE_RIGHT:
                left = OFF;
                right = ON;
                break;
            case SDL_SCANCODE_UP:
                reverse = OFF;
                down = OFF;
                thrust = ON;
                up = ON;
                break;
            case SDL_SCANCODE_DOWN:
                thrust = OFF;
                up = OFF;
                reverse = ON;
                down = ON;
                break;
            case SDL_SCANCODE_C:
                if (state == NAVIGATE)
                    camera_on = !camera_on;
                else if (state == MAP)
                    camera_on = ON;
                break;
            case SDL_SCANCODE_K:
                console = !console;
                break;
            case SDL_SCANCODE_P:
                if (state == NAVIGATE)
                    state = PAUSE;
                else if (state == PAUSE)
                    state = NAVIGATE;
                break;
            case SDL_SCANCODE_S:
                if (state == NAVIGATE)
                    stop = ON;
                break;
            case SDL_SCANCODE_M:
                if (state == NAVIGATE)
                {
                    state = MAP;
                    map_enter = ON;
                    camera_on = ON;
                }
                break;
            case SDL_SCANCODE_ESCAPE:
                if (state == MAP)
                {
                    state = NAVIGATE;
                    map_exit = ON;
                }
                else if (state == NAVIGATE)
                    *quit = 1;
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
            case SDL_SCANCODE_S:
                stop = OFF;
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

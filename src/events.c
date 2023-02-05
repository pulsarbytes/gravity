/*
 * events.c - Polls for currently pending events.
 */

#include <SDL2/SDL.h>

#include "../include/common.h"

extern int left;
extern int right;
extern int up;
extern int down;
extern int thrust;
extern int reverse;
extern int console;
extern int camera_on;
extern int pause;
extern int stop;
extern int map_on;
extern int zoom_in;
extern int zoom_out;

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
                left = ON;
                right = OFF;
                break;
            case SDL_SCANCODE_RIGHT:
                right = ON;
                left = OFF;
                break;
            case SDL_SCANCODE_UP:
                reverse = OFF;
                thrust = ON;
                up = ON;
                down = OFF;
                break;
            case SDL_SCANCODE_DOWN:
                thrust = OFF;
                reverse = ON;
                down = ON;
                up = OFF;
                break;
            case SDL_SCANCODE_C:
                camera_on = camera_on ? OFF : ON;
                break;
            case SDL_SCANCODE_K:
                console = console ? OFF : ON;
                break;
            case SDL_SCANCODE_P:
                pause = pause ? OFF : ON;
                break;
            case SDL_SCANCODE_S:
                stop = ON;
                break;
            case SDL_SCANCODE_M:
                map_on = map_on ? OFF : ON;
                break;
            case SDL_SCANCODE_ESCAPE:
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

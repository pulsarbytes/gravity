/*
 * events.c - Polls for currently pending events
 */

#include <SDL2/SDL.h>

#include "../include/common.h"

extern int left;
extern int right;
extern int thrust;
extern int console;
extern int camera_on;

/*
 * Poll SDL events
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
            case SDL_SCANCODE_A:
            case SDL_SCANCODE_LEFT:
                left = ON;
                break;
            case SDL_SCANCODE_D:
            case SDL_SCANCODE_RIGHT:
                right = ON;
                break;
            case SDL_SCANCODE_SPACE:
                thrust = ON;
                break;
            case SDL_SCANCODE_K:
                console = console ? OFF : ON;
                break;
            case SDL_SCANCODE_C:
                camera_on = camera_on ? OFF : ON;
                break;
            default:
                break;
            }
            break;
        case SDL_KEYUP:
            switch (event.key.keysym.scancode)
            {
            case SDL_SCANCODE_A:
            case SDL_SCANCODE_LEFT:
                left = OFF;
                break;
            case SDL_SCANCODE_D:
            case SDL_SCANCODE_RIGHT:
                right = OFF;
                break;
            case SDL_SCANCODE_SPACE:
                thrust = OFF;
                break;
            case SDL_SCANCODE_ESCAPE:
                *quit = 1;
                break;
            default:
                break;
            }
            break;
        }
    }
}

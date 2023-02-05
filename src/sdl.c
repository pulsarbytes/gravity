/*
 * sdl.c - Functions for initializing and closing SDL.
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "../include/common.h"

extern SDL_Window *window;
extern SDL_DisplayMode display_mode;
extern SDL_Renderer *renderer;
extern TTF_Font *font;
extern SDL_Color text_color;

/*
 * Initialize SDL.
 */
int init_sdl()
{
    // Attempt to initialize SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        SDL_Log("Could not initialize SDL: %s\n", SDL_GetError());
        return FALSE;
    }

    // Get display mode
    if (SDL_GetDesktopDisplayMode(0, &display_mode))
    {
        SDL_Log("Could not get desktop display mode: %s\n", SDL_GetError());
        return FALSE;
    }

    // Create a window
    // Use the flag SDL_WINDOW_OPENGL to load OpenGL
    window = SDL_CreateWindow("Gravity",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              display_mode.w,
                              display_mode.h,
                              0);

    if (window == NULL)
    {
        SDL_Log("Could not create window: %s\n", SDL_GetError());
        SDL_Quit();
        return FALSE;
    }

    // Make Fullscreen
    if (FULLSCREEN)
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

    // Create a 2D rendering context for the window
    Uint32 render_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
    renderer = SDL_CreateRenderer(window, -1, render_flags);

    if (renderer == NULL)
    {
        SDL_Log("Could not create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return FALSE;
    }

    // Initialize SDL_ttf library
    if (TTF_Init() == -1)
    {
        SDL_Log("Could not initialize SDL_ttf: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return FALSE;
    }

    // Load a font into memory
    font = TTF_OpenFont("../assets/fonts/consola.ttf", FONT_SIZE);

    if (font == NULL)
    {
        SDL_Log("Could not load font: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return FALSE;
    }

    // Text color for game console
    text_color.r = 255;
    text_color.g = 255;
    text_color.b = 255;

    return TRUE;
}

/*
 * Clean up SDL resources.
 */
void close_sdl(void)
{
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

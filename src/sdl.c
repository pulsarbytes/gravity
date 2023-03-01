/*
 * sdl.c - Functions for initializing and closing SDL.
 */

#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"

// External variable definitions
extern TTF_Font *fonts[];
extern SDL_DisplayMode display_mode;
extern SDL_Renderer *renderer;

// Function prototypes
int init_sdl(SDL_Window *window);
void close_sdl(SDL_Window *window);

/*
 * Initialize SDL.
 */
int init_sdl(SDL_Window *window)
{
    // Attempt to initialize SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        SDL_Log("Could not initialize SDL: %s\n", SDL_GetError());
        return false;
    }

    // Get display mode
    if (SDL_GetDesktopDisplayMode(0, &display_mode))
    {
        SDL_Log("Could not get desktop display mode: %s\n", SDL_GetError());
        return false;
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
        return false;
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
        return false;
    }

    // Initialize SDL_ttf library
    if (TTF_Init() == -1)
    {
        SDL_Log("Could not initialize SDL_ttf: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    // Load fonts into memory
    fonts[FONT_SIZE_14] = TTF_OpenFont("../assets/fonts/consola.ttf", 14);

    if (fonts[FONT_SIZE_14] == NULL)
    {
        SDL_Log("Could not load font: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    fonts[FONT_SIZE_36] = TTF_OpenFont("../assets/fonts/consola.ttf", 36);

    if (fonts[FONT_SIZE_36] == NULL)
    {
        SDL_Log("Could not load font: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    // Set blend mode
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    return true;
}

/*
 * Clean up SDL resources.
 */
void close_sdl(SDL_Window *window)
{
    for (int i = 0; i < FONT_COUNT; i++)
    {
        TTF_CloseFont(fonts[i]);
    }

    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

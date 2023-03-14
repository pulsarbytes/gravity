/*
 * sdl.c
 */

#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/sdl.h"

// External variable definitions
extern TTF_Font *fonts[];
extern SDL_DisplayMode display_mode;
extern SDL_Renderer *renderer;

/**
 * Cleans up SDL and TTF resources by closing all open fonts, quitting IMG and TTF,
 * destroying the renderer and window, and quitting SDL.
 *
 * @param window A pointer to the SDL_Window to be destroyed.
 *
 * @return void
 */
void sdl_cleanup(SDL_Window *window)
{
    for (int i = 0; i < FONT_COUNT; i++)
    {
        TTF_CloseFont(fonts[i]);
    }

    IMG_Quit();
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

/**
 * Initializes SDL with given window and renderer, and sets up the rendering context.
 *
 * @param window A pointer to the SDL_Window to be created.
 * @return True if SDL was successfully initialized, and false otherwise.
 */
bool sdl_initialize(SDL_Window *window)
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
        SDL_Quit();
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
    Uint32 render_flags;

    if (VSYNC_ON)
        render_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
    else
        render_flags = SDL_RENDERER_ACCELERATED;

    renderer = SDL_CreateRenderer(window, -1, render_flags);

    if (renderer == NULL)
    {
        SDL_Log("Could not create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    // Set blend mode
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    return true;
}

/**
 * Initializes the SDL_ttf library and loads fonts into memory.
 * Loads Consola font with sizes 14 and 36 into fonts array.
 *
 * @param window A pointer to the SDL window to be used for rendering.
 *
 * @return Boolean indicating whether initialization and font loading was successful.
 */
bool sdl_ttf_load_fonts(SDL_Window *window)
{
    // Initialize SDL_ttf library
    if (TTF_Init() == -1)
    {
        SDL_Log("Could not initialize SDL_ttf: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    // FONT_SIZE_12
    fonts[FONT_SIZE_12] = TTF_OpenFont("../assets/fonts/consola.ttf", 12);

    if (fonts[FONT_SIZE_12] == NULL)
    {
        SDL_Log("Could not load font: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    // FONT_SIZE_14
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

    // FONT_SIZE_15
    fonts[FONT_SIZE_15] = TTF_OpenFont("../assets/fonts/consola.ttf", 15);

    if (fonts[FONT_SIZE_15] == NULL)
    {
        SDL_Log("Could not load font: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    // FONT_SIZE_22
    fonts[FONT_SIZE_22] = TTF_OpenFont("../assets/fonts/consola.ttf", 22);

    if (fonts[FONT_SIZE_22] == NULL)
    {
        SDL_Log("Could not load font: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    // FONT_SIZE_32
    fonts[FONT_SIZE_32] = TTF_OpenFont("../assets/fonts/consola.ttf", 32);

    if (fonts[FONT_SIZE_32] == NULL)
    {
        SDL_Log("Could not load font: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    return true;
}

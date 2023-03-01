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
void SDL_DrawCircle(SDL_Renderer *renderer, const struct camera_t *camera, int xc, int yc, int radius, SDL_Color color);
void SDL_DrawCircleApprox(SDL_Renderer *renderer, const struct camera_t *camera, int x, int y, int r, SDL_Color color);

// External function prototypes
int in_camera_relative(const struct camera_t *camera, int x, int y);
bool line_intersects_viewport(const struct camera_t *camera, double x1, double y1, double x2, double y2);

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

/*
 * Midpoint Circle Algorithm for drawing a circle in SDL.
 * xc, xy, radius are in game_scale.
 * This function is efficient only for small circles.
 */
void SDL_DrawCircle(SDL_Renderer *renderer, const struct camera_t *camera, int xc, int yc, int radius, SDL_Color color)
{
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    // Draw the circle
    while (y >= x)
    {
        // Draw the 8 points symmetrically
        if (in_camera_relative(camera, xc + x, yc + y))
        {
            SDL_RenderDrawPoint(renderer, xc + x, yc + y);
        }
        if (in_camera_relative(camera, xc + x, yc - y))
        {
            SDL_RenderDrawPoint(renderer, xc + x, yc - y);
        }
        if (in_camera_relative(camera, xc - x, yc + y))
        {
            SDL_RenderDrawPoint(renderer, xc - x, yc + y);
        }
        if (in_camera_relative(camera, xc - x, yc - y))
        {
            SDL_RenderDrawPoint(renderer, xc - x, yc - y);
        }
        if (in_camera_relative(camera, xc + y, yc + x))
        {
            SDL_RenderDrawPoint(renderer, xc + y, yc + x);
        }
        if (in_camera_relative(camera, xc + y, yc - x))
        {
            SDL_RenderDrawPoint(renderer, xc + y, yc - x);
        }
        if (in_camera_relative(camera, xc - y, yc + x))
        {
            SDL_RenderDrawPoint(renderer, xc - y, yc + x);
        }
        if (in_camera_relative(camera, xc - y, yc - x))
        {
            SDL_RenderDrawPoint(renderer, xc - y, yc - x);
        }

        if (d < 0)
            d = d + 4 * x + 6;
        else
        {
            d = d + 4 * (x - y) + 10;
            y--;
        }

        x++;
    }
}

/**
 * Draws a circle approximation using a series of bezier curves.
 * This function will only draw segments of the circle that intersect with the viewport defined by `camera`.
 * This function is efficient for very large circles.
 *
 * @param renderer The renderer to use to draw the circle
 * @param camera The camera used to view the scene
 * @param x The x coordinate of the center of the circle
 * @param y The y coordinate of the center of the circle
 * @param r The radius of the circle
 * @param color The color to use when drawing the circle
 */
void SDL_DrawCircleApprox(SDL_Renderer *renderer, const struct camera_t *camera, int x, int y, int r, SDL_Color color)
{
    const int CIRCLE_APPROXIMATION = 500;
    int i;
    double angle;
    double x1, y1, x2, y2, x3, y3, x4, y4;

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    for (i = 0; i < CIRCLE_APPROXIMATION; i++)
    {
        angle = 2 * M_PI * i / CIRCLE_APPROXIMATION;
        x1 = x + r * cos(angle);
        y1 = y + r * sin(angle);
        angle = 2 * M_PI * (i + 1) / CIRCLE_APPROXIMATION;
        x2 = x + r * cos(angle);
        y2 = y + r * sin(angle);

        x3 = (2 * x1 + x2) / 3;
        y3 = (2 * y1 + y2) / 3;
        x4 = (x1 + 2 * x2) / 3;
        y4 = (y1 + 2 * y2) / 3;

        if (line_intersects_viewport(camera, x1, y1, x3, y3) ||
            line_intersects_viewport(camera, x3, y3, x4, y4) ||
            line_intersects_viewport(camera, x4, y4, x2, y2))
        {

            SDL_RenderDrawLine(renderer, x1, y1, x3, y3);
            SDL_RenderDrawLine(renderer, x3, y3, x4, y4);
            SDL_RenderDrawLine(renderer, x4, y4, x2, y2);
        }
    }
}

#ifndef SDL_H
#define SDL_H

// Function prototypes
int sdl_initialize(SDL_Window *);
int sdl_ttf_load_fonts(SDL_Window *);
void sdl_cleanup(SDL_Window *);

#endif
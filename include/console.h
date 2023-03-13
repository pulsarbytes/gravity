#ifndef CONSOLE_H
#define CONSOLE_H

// Function prototypes
void console_draw_fps(unsigned int fps, const Camera *);
void console_draw_galaxy_console(const Galaxy *, const Camera *);
void console_draw_position_console(const GameState *, const NavigationState *, const Camera *, Point);
void console_draw_ship_console(const NavigationState *, const Ship *, const Camera *);
void console_draw_star_console(const Star *, const Camera *);
void console_measure_fps(unsigned int *fps, unsigned int *last_time, unsigned int *frame_count);

// External function prototypes
void gfx_draw_circle(SDL_Renderer *, const Camera *, int xc, int yc, int radius, SDL_Color);
void gfx_draw_fill_circle(SDL_Renderer *, int xc, int yc, int radius, SDL_Color);
void utils_add_thousand_separators(int num, char *result, size_t result_size);

#endif
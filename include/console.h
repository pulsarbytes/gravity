#ifndef CONSOLE_H
#define CONSOLE_H

// Function prototypes
void console_draw_fps(unsigned int fps, const Camera *);
void console_draw_position_console(const GameState *, const NavigationState *, const Camera *);
void console_draw_ship_console(const GameState *, const NavigationState *, const Ship *, const Camera *);
void console_draw_star_console(const Star *, const Camera *);
void console_draw_waypoint_console(const NavigationState *, const Camera *);
void console_measure_fps(GameState *, unsigned int *last_time, unsigned int *frame_count);

// External function prototypes
void gfx_draw_circle(SDL_Renderer *, const Camera *, int xc, int yc, int radius, SDL_Color);
void gfx_draw_diamond(SDL_Renderer *, int x, int y, int size, SDL_Color);
void gfx_draw_fill_circle(SDL_Renderer *, int xc, int yc, int radius, SDL_Color);
void gfx_draw_fill_diamond(SDL_Renderer *, int x, int y, int size, SDL_Color);
double maths_distance_between_points(double x1, double y1, double x2, double y2);
void utils_add_thousand_separators(int num, char *result, size_t result_size);
void utils_convert_seconds_to_time_string(int seconds, char timeString[]);

#endif
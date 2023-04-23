#ifndef EVENTS_H
#define EVENTS_H

// Function prototypes
void events_loop(GameState *, InputState *, GameEvents *, NavigationState *, const Camera *);
void events_set_cursor(GameState *, InputState *);

// External function prototypes
void game_change_state(GameState *, GameEvents *, int new_state);
long double game_zoom_generate_preview_stars(unsigned short galaxy_class);
bool menu_is_hovering_menu(GameState *game_state, InputState *input_state);
void stars_cleanup_planets(CelestialBody *);
void stars_initialize_star(Star *);

#endif
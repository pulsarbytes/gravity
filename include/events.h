#ifndef EVENTS_H
#define EVENTS_H

// Function prototypes
void events_loop(GameState *, InputState *, GameEvents *, NavigationState *, const Camera *);

// External function prototypes
void game_change_state(GameState *, GameEvents *, int new_state);
long double game_zoom_generate_preview_stars(unsigned short galaxy_class);

#endif
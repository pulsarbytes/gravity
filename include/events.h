#ifndef EVENTS_H
#define EVENTS_H

// Function prototypes
void events_loop(GameState *, InputState *, GameEvents *);

// External function prototypes
void game_change_state(GameState *, GameEvents *, int new_state);

#endif
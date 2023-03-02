#ifndef EVENTS_H
#define EVENTS_H

// Function prototypes
void poll_events(GameState *, InputState *, GameEvents *);

// External function prototypes
void change_state(GameState *, GameEvents *, int new_state);

#endif
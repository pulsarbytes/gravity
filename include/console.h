#ifndef CONSOLE_H
#define CONSOLE_H

// Function prototypes
void log_console(ConsoleEntry entries[], int index, double value);
void log_fps(ConsoleEntry entries[], unsigned int time_diff);
void update_console(GameState *, const NavigationState *);

#endif
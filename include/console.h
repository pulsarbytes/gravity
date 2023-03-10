#ifndef CONSOLE_H
#define CONSOLE_H

// Function prototypes
void console_log_fps(ConsoleEntry entries[], unsigned int *fps, unsigned int *last_time, unsigned int *frame_count);
void console_log_position(GameState *, NavigationState);
void console_render(ConsoleEntry entries[], const Camera *);
void console_update_entry(ConsoleEntry entries[], int index, double value);

#endif
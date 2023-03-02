#ifndef CONSOLE_H
#define CONSOLE_H

// Function prototypes
void console_update_entry(ConsoleEntry entries[], int index, double value);
void console_measure_fps(unsigned int *fps, unsigned int *last_time, unsigned int *frame_count);
void console_render(ConsoleEntry entries[]);

#endif
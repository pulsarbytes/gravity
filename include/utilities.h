#ifndef UTILITIES_H
#define UTILITIES_H

// Function prototypes
void utils_add_thousand_separators(int num, char *result, size_t result_size);
void utils_cleanup_resources(GameState *, InputState *, NavigationState *, Bstar *bstars, Ship *);
void utils_convert_seconds_to_time_string(int seconds, char timeString[]);

// External function prototypes
void galaxies_clear_table(GalaxyEntry *galaxies[]);
void stars_clear_table(StarEntry *stars[], const NavigationState *, bool delete_all);

#endif
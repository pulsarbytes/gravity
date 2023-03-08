#ifndef UTILITIES_H
#define UTILITIES_H

// Function prototypes
void utils_add_thousand_separators(int num, char *result, size_t result_size);
void utils_cleanup_resources(GameState *, NavigationState *, Bstar *bstars, Ship *);

// External function prototypes
void galaxies_clear_table(GalaxyEntry *galaxies[]);
void stars_clear_table(StarEntry *stars[]);

#endif
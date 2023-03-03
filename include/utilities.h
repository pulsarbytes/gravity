#ifndef UTILITIES_H
#define UTILITIES_H

// Function prototypes
void utils_cleanup_resources(GameState *, NavigationState *, Ship *);

// External function prototypes
void galaxies_clear_table(GalaxyEntry *galaxies[]);
void stars_clear_table(StarEntry *stars[]);

#endif
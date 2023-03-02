#ifndef UTILITIES_H
#define UTILITIES_H

// Function prototypes
void cleanup_resources(GameState *, NavigationState *, Ship *);

// External function prototypes
void cleanup_galaxies(GalaxyEntry *galaxies[]);
void cleanup_stars(StarEntry *stars[]);

#endif
#ifndef COMMON_H
#define COMMON_H

#define MAX_PLANETS 10
#define MAX_MOONS 8
#define G_CONSTANT 5

enum
{
    FALSE,
    TRUE
};
enum
{
    OFF,
    ON
};

// Must be in sync with game_console_entries
enum
{
    FPS_INDEX,
    X_INDEX,
    Y_INDEX,
    V_INDEX,
    DISTANCE_INDEX,
    G_INDEX,
    LOG_COUNT // number of elements in enumeration
};

#endif /* COMMON_H */

#ifndef COMMON_H
#define COMMON_H

#define MAX_PLANETS 20
#define MAX_MOONS 8
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MAX_PLANETS_MOONS MAX(MAX_PLANETS, MAX_MOONS)
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

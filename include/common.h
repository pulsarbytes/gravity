#ifndef COMMON_H
#define COMMON_H

#define FPS 60
#define FULLSCREEN 1
#define CAMERA_ON 1

#define CONSOLE_ON 1
#define FONT_SIZE 14
#define PROJECTION_RADIUS 10

#define COSMIC_CONSTANT 7.75
#define G_CONSTANT 5

#define EMPTY_SPACE 1
#define STARS_SQUARE 10000
#define STARS_PER_SQUARE 5
#define STAR_CUTOFF 60

#define MAX_PLANETS 10
#define MAX_MOONS 8
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MAX_PLANETS_MOONS MAX(MAX_PLANETS, MAX_MOONS)
#define MAX_PLANET_NAME 100

#define PLANET_CUTOFF 10
#define PLANET_DISTANCE 1500
#define MOON_DISTANCE 250

#define SHIP_RADIUS 17
#define SHIP_STARTING_X 0    // 0 is star center, negative is left
#define SHIP_STARTING_Y -700 // 0 is star center, negative is up
#define SHIP_IN_ORBIT 0
#define SPEED_LIMIT 300

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

enum
{
    LEVEL_STAR = 1,
    LEVEL_PLANET,
    LEVEL_MOON
};

enum
{
    ENTITY_SHIP,
    ENTITY_PLANET
};

#endif /* COMMON_H */

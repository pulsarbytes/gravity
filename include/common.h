#ifndef COMMON_H
#define COMMON_H

// System settings
#define FPS 60 // Default: 60
#define FULLSCREEN 1
#define FONT_SIZE 14 // Default: 14
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MAX_OBJECT_NAME 64 // Default: 64

// Math/Physics
#define COSMIC_CONSTANT 7.75 // Default: 7.75
#define G_CONSTANT 5         // Default: 5
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Background stars
#define BSTARS_MAX_SPEED 300 // Default: 300
#define BSTARS_SQUARE 10000  // Groups X pixels. Default: 10000
#define BSTARS_PER_SQUARE 5  // X stars per square. Default: 3

// Game settings
#define BSTARS_ON 1
#define CAMERA_ON 1
#define CONSOLE_ON 1
#define COLLISIONS_ON 1
#define SHIP_GRAVITY_ON 1
#define PROJECTIONS_ON 1
#define SOLAR_SYSTEMS_ON 1
#define SHOW_ORBITS 1
#define START_IN_ORBIT 1
#define PROJECTION_RADIUS 5       // Default: 5
#define SHIP_PROJECTION_RADIUS 12 // Default: 12
#define SHIP_RADIUS 17            // Default: 17
#define BASE_SPEED_LIMIT 300      // Default: 300
#define MAP_SPEED_MAX 35          // Zoom in. Default: 35
#define MAP_SPEED_MIN 10          // Zoom out. Default: 10

// Zoom
#define ZOOM_STEP 0.01         // Default: 0.01
#define ZOOM_MAX 1             // Default: 1
#define ZOOM_UNIVERSE 0.01     // Default: 0.01
#define ZOOM_NAVIGATE_MIN 0.20 // Default: 0.20
#define ZOOM_NAVIGATE 1        // Default: 1
#define ZOOM_MAP_MIN 0.01      // Default: 0.01
#define ZOOM_MAP 0.02          // Default: 0.02

// Universe
#define UNIVERSE_REGION_SIZE 30     // Sections per axis. Even number; Default: 30
#define UNIVERSE_DENSITY 10         // Per 1000 sections. Default: 20
#define MAX_GALAXIES 907            // First prime number > (UNIVERSE_REGION_SIZE * UNIVERSE_REGION_SIZE). Default 907
                                    // We use this in the modulo operations of the hash function output
#define UNIVERSE_SECTION_SIZE 10000 // Default: 10000
#define UNIVERSE_X_LIMIT 200000000  // Default: 200000000
#define UNIVERSE_Y_LIMIT 200000000  // Default: 200000000

// Galaxy
#define GALAXY_REGION_SIZE 30     // Sections per axis. Even number; Default: 30
#define GALAXY_DENSITY 30         // Maximum at galaxy center per 1000 sections. Default: 30
#define MAX_STARS 907             // First prime number > (GALAXY_REGION_SIZE * GALAXY_REGION_SIZE). Default 907
                                  // We use this in the modulo operations of the hash function output
#define GALAXY_SECTION_SIZE 10000 // Default: 10000

// Galaxy cloud
#define MAX_GSTARS 90000 // Default 90000. (300 * 300)

// Starting position
#define UNIVERSE_START_X -140000
#define UNIVERSE_START_Y -70000
#define GALAXY_START_X -56032 // Default: -56032000
#define GALAXY_START_Y 0      // Default: 0

// Galaxies
#define GALAXY_SCALE 1000               // We multiply radius by this factor to get galaxy full size radius in points. Default: 100 (min 10, max 10000)
                                        // Use a smaller number to generate smaller galaxies
#define GALAXY_CLASS_1_RADIUS_MIN 3000  // Default: 3000
#define GALAXY_CLASS_1_RADIUS_MAX 1000  // Default: 1000 (+ 3000 = 4000) (max 5000)
#define GALAXY_CLASS_2_RADIUS_MIN 5000  // Default: 5000
#define GALAXY_CLASS_2_RADIUS_MAX 3000  // Default: 3000 (+ 5000 = 8000) (max 10000)
#define GALAXY_CLASS_3_RADIUS_MIN 10000 // Default: 10000
#define GALAXY_CLASS_3_RADIUS_MAX 3000  // Default: 3000 (+ 10000 = 13000) (max 15000)
#define GALAXY_CLASS_4_RADIUS_MIN 15000 // Default: 15000
#define GALAXY_CLASS_4_RADIUS_MAX 3000  // Default: 3000 (+ 15000 = 18000) (max 20000)
#define GALAXY_CLASS_5_RADIUS_MIN 20000 // Default: 20000
#define GALAXY_CLASS_5_RADIUS_MAX 3000  // Default: 3000 (+ 20000 = 23000) (max 25000)
#define GALAXY_CLASS_6_RADIUS_MIN 25000 // Default: 25000
#define GALAXY_CLASS_6_RADIUS_MAX 3000  // Default: 3000 (+ 25000 = 28000) (max 30000)

// Stars
#define STAR_CLASS_1_RADIUS_MIN 100 // Default: 100
#define STAR_CLASS_1_RADIUS_MAX 100 // Default: 100 (+ 100 = 200)
#define STAR_CLASS_2_RADIUS_MIN 200 // Default: 200
#define STAR_CLASS_2_RADIUS_MAX 100 // Default: 100 (+ 200 = 300)
#define STAR_CLASS_3_RADIUS_MIN 300 // Default: 300
#define STAR_CLASS_3_RADIUS_MAX 150 // Default: 150 (+ 300 = 450)
#define STAR_CLASS_4_RADIUS_MIN 450 // Default: 450
#define STAR_CLASS_4_RADIUS_MAX 150 // Default: 150 (+ 450 = 600)
#define STAR_CLASS_5_RADIUS_MIN 600 // Default: 600
#define STAR_CLASS_5_RADIUS_MAX 200 // Default: 200 (+ 600 = 800)
#define STAR_CLASS_6_RADIUS_MIN 800 // Default: 800
#define STAR_CLASS_6_RADIUS_MAX 200 // Default: 200 (+ 800 = 1000)

// Planets
#define MAX_PLANETS 10 // Default: 10

/* Number of star radiuses */
#define STAR_CLASS_1_ORBIT_MIN 5 // Default: 5
#define STAR_CLASS_1_ORBIT_MAX 8 // Default: 8 (+ 5 = 13)
#define STAR_CLASS_2_ORBIT_MIN 5 // Default: 5
#define STAR_CLASS_2_ORBIT_MAX 8 // Default: 8 (+ 5 = 13)
#define STAR_CLASS_3_ORBIT_MIN 4 // Default: 4
#define STAR_CLASS_3_ORBIT_MAX 7 // Default: 7 (+ 4 = 11)
#define STAR_CLASS_4_ORBIT_MIN 3 // Default: 3
#define STAR_CLASS_4_ORBIT_MAX 6 // Default: 6 (+ 3 = 9)
#define STAR_CLASS_5_ORBIT_MIN 2 // Default: 2
#define STAR_CLASS_5_ORBIT_MAX 5 // Default: 5 (+ 2 = 7)
#define STAR_CLASS_6_ORBIT_MIN 1 // Default: 1
#define STAR_CLASS_6_ORBIT_MAX 4 // Default: 4 (+ 1 = 5)

#define PLANET_RADIUS_MIN 20               // Default: 20
#define STAR_CLASS_1_PLANET_RADIUS_MAX 50  // Default: 50
#define STAR_CLASS_2_PLANET_RADIUS_MAX 60  // Default: 60
#define STAR_CLASS_3_PLANET_RADIUS_MAX 80  // Default: 80
#define STAR_CLASS_4_PLANET_RADIUS_MAX 100 // Default: 100
#define STAR_CLASS_5_PLANET_RADIUS_MAX 150 // Default: 150
#define STAR_CLASS_6_PLANET_RADIUS_MAX 200 // Default: 200

// Moons
#define MAX_MOONS 5 // Default: 5
#define MAX_PLANETS_MOONS MAX(MAX_PLANETS, MAX_MOONS)

/* Number of planet radiuses */
#define PLANET_CLASS_1_ORBIT_MIN 6 // Default: 6
#define PLANET_CLASS_1_ORBIT_MAX 1 // Default: 1 (+ 6 = 7)
#define PLANET_CLASS_2_ORBIT_MIN 6 // Default: 6
#define PLANET_CLASS_2_ORBIT_MAX 2 // Default: 2 (+ 6 = 8)
#define PLANET_CLASS_3_ORBIT_MIN 6 // Default: 6
#define PLANET_CLASS_3_ORBIT_MAX 3 // Default: 3 (+ 6 = 9)
#define PLANET_CLASS_4_ORBIT_MIN 5 // Default: 5
#define PLANET_CLASS_4_ORBIT_MAX 4 // Default: 4 (+ 5 = 9)
#define PLANET_CLASS_5_ORBIT_MIN 5 // Default: 5
#define PLANET_CLASS_5_ORBIT_MAX 4 // Default: 4 (+ 5 = 9)
#define PLANET_CLASS_6_ORBIT_MIN 5 // Default: 5
#define PLANET_CLASS_6_ORBIT_MAX 4 // Default: 4 (+ 5 = 9)

#define MOON_RADIUS_MIN 3                  // Default: 3
#define PLANET_CLASS_1_MOON_RADIUS_MAX 12  // Default: 15
#define PLANET_CLASS_2_MOON_RADIUS_MAX 24  // Default: 25
#define PLANET_CLASS_3_MOON_RADIUS_MAX 32  // Default: 35
#define PLANET_CLASS_4_MOON_RADIUS_MAX 48  // Default: 50
#define PLANET_CLASS_5_MOON_RADIUS_MAX 72  // Default: 75
#define PLANET_CLASS_6_MOON_RADIUS_MAX 100 // Default: 100

// States
enum
{
    MENU,
    NAVIGATE,
    MAP,
    UNIVERSE,
    PAUSE
};

enum
{
    STAGE_OFF = -1, // Not landed
    STAGE_0         // Landed
};

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
    SCALE_INDEX,
    LOG_COUNT // number of elements in enumeration
};

enum
{
    GALAXY_CLASS_1 = 1,
    GALAXY_CLASS_2,
    GALAXY_CLASS_3,
    GALAXY_CLASS_4,
    GALAXY_CLASS_5,
    GALAXY_CLASS_6
};

enum
{
    STAR_CLASS_1 = 1,
    STAR_CLASS_2,
    STAR_CLASS_3,
    STAR_CLASS_4,
    STAR_CLASS_5,
    STAR_CLASS_6,
    PLANET_CLASS_1,
    PLANET_CLASS_2,
    PLANET_CLASS_3,
    PLANET_CLASS_4,
    PLANET_CLASS_5,
    PLANET_CLASS_6
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
    ENTITY_PLANET,
    ENTITY_STAR,
    ENTITY_GALAXY
};

#endif /* COMMON_H */

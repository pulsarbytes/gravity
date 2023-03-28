#ifndef CONSTANTS_H
#define CONSTANTS_H

// System settings
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define FULLSCREEN 1
#define FPS 60                    // Default: 60
#define VSYNC_ON 1                // Default: 1
#define MAX_OBJECT_NAME 64        // Default: 64
#define DOUBLE_CLICK_INTERVAL 400 // Default: 400
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Physics
#define COSMIC_CONSTANT 7.75      // Default: 7.75
#define G_CONSTANT 5              // Default: 5
#define G_LAUNCH 0.7 * G_CONSTANT // Default: 0.7 * G_CONSTANT
#define G_THRUST 1 * G_CONSTANT   // Default: 1 * G_CONSTANT

// Background stars
#define BSTARS_SPEED_FACTOR 0.04                  // Default: 0.04
#define BSTARS_MAX_OPACITY 140                    // Default: 140
#define BSTARS_MIN_OPACITY 45                     // Default: 45
#define BSTARS_SQUARE 10000                       // Groups X pixels. Default: 10000
#define BSTARS_PER_SQUARE 4                       // X stars per square. Default: 4
#define BSTARS_BATCH_SIZE 50                      // Default: 50
#define MAX_GSTARS_ROW 100                        // Default: 100
#define MAX_GSTARS MAX_GSTARS_ROW *MAX_GSTARS_ROW // Default: MAX_GSTARS_ROW *MAX_GSTARS_ROW
#define GSTARS_SCALE 10                           // Designates gstars scaling compared to universe mode. Default: 10
#define SPEED_LINES_NUM 8                         // Number of rows and columns for the speeding lines array. Default: 8

// Game settings
#define BSTARS_ON 1      // Default: 1
#define SPEED_LINES_ON 1 // Default: 1
#define GSTARS_ON 1      // Default: 1
#define FPS_ON 1
#define COLLISIONS_ON 1
#define SHIP_GRAVITY_ON 1
#define PROJECTIONS_ON 1
#define PROJECT_BODIES_ON 1
#define SHOW_ORBITS 1
#define PROJECTION_RADIUS 5       // Default: 5
#define SHIP_PROJECTION_RADIUS 12 // Default: 12
#define SHIP_RADIUS 17            // Default: 17
#define BASE_SPEED_LIMIT 300      // Default: 300
#define GALAXY_SPEED_LIMIT 1500   // Default: 1800
#define UNIVERSE_SPEED_LIMIT 3000 // Default: 3000
#define MAP_SPEED_MAX 25          // Zoom in. Default: 25
#define MAP_SPEED_MIN 10          // Zoom out. Default: 10
#define UNIVERSE_SPEED_MAX 15     // Zoom in. Default: 15
#define UNIVERSE_SPEED_MIN 4      // Zoom out. Default: 4
#define MOUSE_SCROLL_DISTANCE 20  // Distance from the edge of the screen where scrolling starts. Default: 20
#define MAX_CONTROLS_GROUPS 4
#define MAX_CONTROLS_ENTRIES 13

// Menu
#define MENU_GALAXY_CLOUD_DENSITY 300  // Default: 300 (max 500)
#define MENU_BSTARS_SPEED_FACTOR 0.012 // Default: 0.012

// Zoom
#define ZOOM_NAVIGATE 1.0     // Default: 1.0
#define ZOOM_NAVIGATE_MIN 0.1 // Default: 0.1
#define ZOOM_MAX 1.0          // Default: 1.0
#define ZOOM_STEP 0.01        // Default: 0.01
#define ZOOM_EPSILON 0.0001   // Default: 0.0001

#define ZOOM_MAP 0.002                       // Default: 0.002
#define ZOOM_MAP_STEP_SWITCH 0.01            // Default: 0.01
#define ZOOM_MAP_SWITCH 0.002                // Default: 0.002
#define ZOOM_MAP_MIN ZOOM_MAP_SWITCH - 0.001 // Default: ZOOM_MAP_SWITCH - 0.001
#define ZOOM_UNIVERSE_STAR_SYSTEMS 0.0005    // Default: 0.0005
#define ZOOM_UNIVERSE 0.01                   // Universe scale. Default: 0.01
#define ZOOM_UNIVERSE_MIN 0.01               // Universe scale. Default: 0.01
#define ZOOM_UNIVERSE_STEP 0.001             // Default: 0.001
#define ZOOM_STAR_1_PREVIEW_STARS 0.00005    // Generate more stars over this limit.
#define ZOOM_STAR_2_PREVIEW_STARS 0.00002
#define ZOOM_STAR_3_PREVIEW_STARS 0.00004
#define ZOOM_STAR_4_PREVIEW_STARS 0.00004
#define ZOOM_STAR_5_PREVIEW_STARS 0.00005
#define ZOOM_STAR_6_PREVIEW_STARS 0.00001

// Universe
#define UNIVERSE_REGION_SIZE 40     // Sections per axis. Even number; Default: 40
#define UNIVERSE_DENSITY 10         // Per 1000 sections. Default: 10
#define MAX_GALAXIES 907            // First prime number > (UNIVERSE_REGION_SIZE * UNIVERSE_REGION_SIZE). Default 907
                                    // We use this in the modulo operations of the hash function output
#define UNIVERSE_SECTION_SIZE 10000 // Default: 10000
#define UNIVERSE_X_LIMIT 200000000  // Default: 200000000
#define UNIVERSE_Y_LIMIT 200000000  // Default: 200000000

// Galaxy
#define GALAXY_REGION_SIZE 30      // Sections per axis. Even number; Default: 30
#define GALAXY_DENSITY 40          // Maximum at galaxy center per 1000 sections. Default: 40 (max 150)
#define GALAXY_CLOUD_DENSITY 40    // Default: 40 (max 150)
#define MAX_STARS 907              // First prime number > (GALAXY_REGION_SIZE * GALAXY_REGION_SIZE). Default 907
                                   // We use this in the modulo operations of the hash function output
#define GALAXY_SECTION_SIZE 100000 // Default: 100000

// Starting position
#define UNIVERSE_START_X -140000
#define UNIVERSE_START_Y -70000 // Class 1: -140000, -70000
                                // Class 2: -280000, 520000
                                // Class 3: -240000, -210000
                                // Class 4: -100000, -40000
                                // Class 5: -110000, -120000
                                // Class 6: -250000, -60000
#define GALAXY_START_X -109000  // Default: 0
#define GALAXY_START_Y -60000   // Default: 0

// Galaxies
#define GALAXY_SCALE 10000        // We multiply by this factor to get values in galaxy scale. Default: 10000 (min 1000)
                                  // Use a smaller number to generate smaller galaxies
#define GALAXY_1_RADIUS_MIN 3000  // Default: 3000
#define GALAXY_1_RADIUS_MAX 1000  // Default: 1000 (+ 3000 = 4000) (max 5000)
#define GALAXY_2_RADIUS_MIN 5000  // Default: 5000
#define GALAXY_2_RADIUS_MAX 3000  // Default: 3000 (+ 5000 = 8000) (max 10000)
#define GALAXY_3_RADIUS_MIN 10000 // Default: 10000
#define GALAXY_3_RADIUS_MAX 3000  // Default: 3000 (+ 10000 = 13000) (max 15000)
#define GALAXY_4_RADIUS_MIN 15000 // Default: 15000
#define GALAXY_4_RADIUS_MAX 3000  // Default: 3000 (+ 15000 = 18000) (max 20000)
#define GALAXY_5_RADIUS_MIN 20000 // Default: 20000
#define GALAXY_5_RADIUS_MAX 3000  // Default: 3000 (+ 20000 = 23000) (max 25000)
#define GALAXY_6_RADIUS_MIN 25000 // Default: 25000
#define GALAXY_6_RADIUS_MAX 3000  // Default: 3000 (+ 25000 = 28000) (max 30000)

// Stars
#define STAR_1_RADIUS_MIN 120 // Default: 120
#define STAR_1_RADIUS_MAX 100 // Default: 100
#define STAR_2_RADIUS_MIN 220 // Default: 220
#define STAR_2_RADIUS_MAX 130 // Default: 130
#define STAR_3_RADIUS_MIN 350 // Default: 350
#define STAR_3_RADIUS_MAX 150 // Default: 150
#define STAR_4_RADIUS_MIN 500 // Default: 500
#define STAR_4_RADIUS_MAX 160 // Default: 160
#define STAR_5_RADIUS_MIN 660 // Default: 660
#define STAR_5_RADIUS_MAX 200 // Default: 200
#define STAR_6_RADIUS_MIN 860 // Default: 860
#define STAR_6_RADIUS_MAX 200 // Default: 200

// Planets
#define MAX_PLANETS 12 // Default: 10

/* Number of star radiuses for orbits */
#define STAR_1_PLANET_ORBIT_MIN 25 // Default: 25
#define STAR_1_PLANET_ORBIT_MAX 8  // Default: 8
#define STAR_2_PLANET_ORBIT_MIN 25 // Default: 25
#define STAR_2_PLANET_ORBIT_MAX 8  // Default: 8
#define STAR_3_PLANET_ORBIT_MIN 24 // Default: 24
#define STAR_3_PLANET_ORBIT_MAX 7  // Default: 7
#define STAR_4_PLANET_ORBIT_MIN 23 // Default: 23
#define STAR_4_PLANET_ORBIT_MAX 6  // Default: 6
#define STAR_5_PLANET_ORBIT_MIN 22 // Default: 22
#define STAR_5_PLANET_ORBIT_MAX 5  // Default: 5
#define STAR_6_PLANET_ORBIT_MIN 22 // Default: 22
#define STAR_6_PLANET_ORBIT_MAX 4  // Default: 4

#define PLANET_RADIUS_MIN 20 // Default: 20
#define TERRESTRIAL_RADIUS_MAX 3 * PLANET_RADIUS_MIN
#define EARTH_RADIUS_MAX 4 * PLANET_RADIUS_MIN
#define SUBNEPTUNE_RADIUS_MAX 5 * PLANET_RADIUS_MIN
#define NEPTUNE_RADIUS_MAX 6 * PLANET_RADIUS_MIN
#define ICE_GIANT_RADIUS_MAX 6 * PLANET_RADIUS_MIN
#define GAS_GIANT_RADIUS_MAX 7 * PLANET_RADIUS_MIN

#define STAR_1_PLANET_RADIUS_MAX SUBNEPTUNE_RADIUS_MAX - 10
#define STAR_2_PLANET_RADIUS_MAX SUBNEPTUNE_RADIUS_MAX
#define STAR_3_PLANET_RADIUS_MAX ICE_GIANT_RADIUS_MAX - 10
#define STAR_4_PLANET_RADIUS_MAX ICE_GIANT_RADIUS_MAX
#define STAR_5_PLANET_RADIUS_MAX GAS_GIANT_RADIUS_MAX - 10
#define STAR_6_PLANET_RADIUS_MAX GAS_GIANT_RADIUS_MAX

// Moons
#define MAX_MOONS 5                                   // Default: 5
#define MAX_PLANETS_MOONS MAX(MAX_PLANETS, MAX_MOONS) // Default: MAX(MAX_PLANETS, MAX_MOONS)

/* Number of planet radiuses for orbits */
#define PLANET_1_ORBIT_MIN 6 // Default: 6
#define PLANET_1_ORBIT_MAX 1 // Default: 1
#define PLANET_2_ORBIT_MIN 6 // Default: 6
#define PLANET_2_ORBIT_MAX 2 // Default: 2
#define PLANET_3_ORBIT_MIN 6 // Default: 6
#define PLANET_3_ORBIT_MAX 3 // Default: 3
#define PLANET_4_ORBIT_MIN 5 // Default: 5
#define PLANET_4_ORBIT_MAX 4 // Default: 4
#define PLANET_5_ORBIT_MIN 5 // Default: 5
#define PLANET_5_ORBIT_MAX 4 // Default: 4
#define PLANET_6_ORBIT_MIN 5 // Default: 5
#define PLANET_6_ORBIT_MAX 4 // Default: 4

#define MOON_RADIUS_MIN 10           // Default: 7
#define PLANET_1_MOON_RADIUS_MAX 12  // Default: 12
#define PLANET_2_MOON_RADIUS_MAX 24  // Default: 24
#define PLANET_3_MOON_RADIUS_MAX 32  // Default: 32
#define PLANET_4_MOON_RADIUS_MAX 48  // Default: 48
#define PLANET_5_MOON_RADIUS_MAX 72  // Default: 72
#define PLANET_6_MOON_RADIUS_MAX 100 // Default: 100

// Info box
#define INFO_BOX_WIDTH 370         // Default: 370
#define STAR_INFO_BOX_HEIGHT 290   // Default: 290
#define GALAXY_INFO_BOX_HEIGHT 300 // Default: 300
#define INFO_BOX_PADDING 20        // Default: 20

// Buttons
#define WAYPOINT_BUTTON_WIDTH 130 // Default: 130
#define WAYPOINT_BUTTON_HEIGHT 26 // Default: 26

#endif /* CONSTANTS_H */

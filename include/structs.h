#ifndef STRUCTS_H
#define STRUCTS_H

// Struct for a menu button
typedef struct
{
    char text[32];
    int state;
    SDL_Rect rect;
    SDL_Texture *texture;
    SDL_Rect texture_rect;
    unsigned short disabled;
} MenuButton;

// Struct for a console entry
typedef struct
{
    char title[32];
    char value[16];
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_Rect rect;
} ConsoleEntry;

typedef struct
{
    double x;
    double y;
} Point;

typedef struct
{
    double x;
    double y;
    float magnitude;
    float angle; // radians between the positive x-axis and the line connecting the origin to the point (vx, vy)
} Vector;

typedef struct
{
    float vx;
    float vy;
} Speed;

// Struct for 2 points
typedef struct
{
    double current_x;
    double current_y;
    double buffer_x;
    double buffer_y;
} PointState;

typedef struct CelestialBody
{
    int initialized;
    char name[MAX_OBJECT_NAME];
    char *image;
    int class;
    float radius;
    float cutoff;
    Point position;
    float vx;
    float vy;
    float dx;
    float dy;
    SDL_Texture *texture;
    SDL_Rect rect;
    SDL_Rect projection;
    SDL_Color color;
    struct CelestialBody *planets[MAX_PLANETS_MOONS];
    struct CelestialBody *parent;
    int level;
} CelestialBody;

typedef CelestialBody Planet;
typedef CelestialBody Star;

// Struct for a star entry in stars hash table
typedef struct StarEntry
{
    double x;
    double y;
    Star *star;
    struct StarEntry *next;
} StarEntry;

// Struct for a galaxy cloud star
typedef struct
{
    Point position;
    unsigned short opacity;
    unsigned short final_star;
} Gstar;

typedef struct
{
    int initialized;
    int initialized_hd;
    char name[MAX_OBJECT_NAME];
    int class;
    float radius;
    float cutoff;
    Point position;
    SDL_Rect projection;
    SDL_Color color;
    Gstar gstars[MAX_GSTARS];
    Gstar gstars_hd[MAX_GSTARS];
} Galaxy;

// Struct for a galaxy entry in galaxies hash table
typedef struct GalaxyEntry
{
    double x;
    double y;
    Galaxy *galaxy;
    struct GalaxyEntry *next;
} GalaxyEntry;

typedef struct Ship
{
    char *image;
    int radius;
    Point position;
    Point previous_position;
    float angle;
    float vx;
    float vy;
    SDL_Texture *texture;
    SDL_Rect rect;
    struct Ship *projection;
    SDL_Rect main_img_rect;
    SDL_Rect thrust_img_rect;
    SDL_Rect reverse_img_rect;
    SDL_Point rotation_pt;
} Ship;

// Struct for a background star
typedef struct
{
    Point position;
    SDL_Rect rect;
    unsigned short opacity;
    unsigned short final_star;
} Bstar;

typedef struct
{
    double x;
    double y;
    int w;
    int h;
} Camera;

typedef struct
{
    int left;
    int right;
    int up;
    int down;
    int thrust;
    int reverse;
    int camera_on;
    int stop;
    int zoom_in;
    int zoom_out;
    int console;
    int orbits_on;
    int selected_button;
} InputState;

typedef struct
{
    int stars_start;
    int galaxies_start;
    int game_started;
    int map_enter;
    int map_exit;
    int map_center;
    int map_switch;
    int universe_enter;
    int universe_exit;
    int universe_center;
    int universe_switch;
    int exited_galaxy;
    int galaxy_found;
} GameEvents;

typedef struct
{
    StarEntry *stars[MAX_STARS];         // Hash table for stars
    GalaxyEntry *galaxies[MAX_GALAXIES]; // Hash table for galaxies
    Galaxy *current_galaxy;
    Galaxy *buffer_galaxy; // Stores galaxy of current ship position
    Galaxy *previous_galaxy;
    PointState galaxy_offset;
    Point universe_cross_line; // Keep track of nearest line position
    Point navigate_offset;
    Point map_offset;
    Point universe_offset;
    Point cross_line; // Keep track of nearest line position
    Vector velocity;
    uint64_t initseq; // Output sequence for the RNG of stars; Changes for every new current_galaxy
} NavigationState;

typedef struct
{
    int state;
    int speed_limit;
    int landing_stage;
    long double game_scale;
    long double save_scale;
    int galaxy_region_size;
    MenuButton menu[MENU_BUTTON_COUNT];
    MenuButton logo;
    ConsoleEntry console_entries[CONSOLE_COUNT];
} GameState;

#endif /* STRUCTS_H */

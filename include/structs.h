#ifndef STRUCTS_H
#define STRUCTS_H

// Struct for a menu button
struct menu_button
{
    char text[32];
    int state;
    SDL_Rect rect;
    SDL_Texture *texture;
    SDL_Rect texture_rect;
    unsigned short disabled;
};

// Struct for a game console entry
struct game_console_entry
{
    char title[32];
    char value[16];
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_Rect rect;
};

// Struct for a point
struct point_t
{
    double x;
    double y;
};

// Struct for a vector
struct vector_t
{
    float x;
    float y;
    float magnitude;
    float angle; // radians between the positive x-axis and the line connecting the origin to the point (vx, vy)
};

struct speed_t
{
    float vx;
    float vy;
};

// Struct for 2 points
struct point_state
{
    double current_x;
    double current_y;
    double buffer_x;
    double buffer_y;
};

// Struct for a planet
struct planet_t
{
    int initialized;
    char name[MAX_OBJECT_NAME];
    char *image;
    int class;
    float radius;
    float cutoff;
    struct point_t position;
    float vx;
    float vy;
    float dx;
    float dy;
    SDL_Texture *texture;
    SDL_Rect rect;
    SDL_Rect projection;
    SDL_Color color;
    struct planet_t *planets[MAX_PLANETS_MOONS];
    struct planet_t *parent;
    int level;
};

// Struct for a star entry in stars hash table
struct star_entry
{
    double x;
    double y;
    struct planet_t *star;
    struct star_entry *next;
};

// Struct for a galaxy cloud star
struct gstar_t
{
    struct point_t position;
    unsigned short opacity;
    unsigned short final_star;
};

// Struct for a galaxy
struct galaxy_t
{
    int initialized;
    int initialized_hd;
    char name[MAX_OBJECT_NAME];
    int class;
    float radius;
    float cutoff;
    struct point_t position;
    SDL_Rect projection;
    SDL_Color color;
    struct gstar_t gstars[MAX_GSTARS];
    struct gstar_t gstars_hd[MAX_GSTARS];
};

// Struct for a galaxy entry in galaxies hash table
struct galaxy_entry
{
    double x;
    double y;
    struct galaxy_t *galaxy;
    struct galaxy_entry *next;
};

// Struct for a ship
struct ship_t
{
    char *image;
    int radius;
    struct point_t position;
    struct point_t previous_position;
    float angle;
    float vx;
    float vy;
    SDL_Texture *texture;
    SDL_Rect rect;
    struct ship_t *projection;
    SDL_Rect main_img_rect;
    SDL_Rect thrust_img_rect;
    SDL_Rect reverse_img_rect;
    SDL_Point rotation_pt;
};

// Struct for a background star
struct bstar_t
{
    struct point_t position;
    SDL_Rect rect;
    unsigned short opacity;
    unsigned short final_star;
};

// Struct for camera
struct camera_t
{
    double x;
    double y;
    int w;
    int h;
};

// Struct for input state
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

// Struct for game events
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

// Struct for navigation state
typedef struct
{
    struct star_entry *stars[MAX_STARS];         // Hash table for stars
    struct galaxy_entry *galaxies[MAX_GALAXIES]; // Hash table for galaxies
    struct galaxy_t *current_galaxy;
    struct galaxy_t *buffer_galaxy; // Stores galaxy of current ship position
    struct galaxy_t *previous_galaxy;
    struct point_state galaxy_offset;
    struct point_t universe_cross_axis; // Keep track of nearest axis coordinates
    struct point_t navigate_offset;
    struct point_t map_offset;
    struct point_t universe_offset;
    struct point_t cross_axis; // Keep track of nearest axis coordinates
    struct vector_t velocity;
    uint64_t initseq; // Output sequence for the RNG of stars; Changes for every new current_galaxy
} NavigationState;

typedef struct
{
    int state;
    int speed_limit;
    int landing_stage;
    long double game_scale;
    float save_scale;
    int galaxy_region_size;
    struct menu_button menu[MENU_BUTTON_COUNT];
    struct menu_button logo;
    struct game_console_entry game_console_entries[LOG_COUNT];
} GameState;

#endif /* STRUCTS_H */

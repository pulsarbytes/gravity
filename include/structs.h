#ifndef STRUCTS_H
#define STRUCTS_H

// Struct for a menu button
typedef struct
{
    char text[32];
    int state;
    SDL_Rect rect;
    SDL_Texture *text_texture;
    SDL_Rect texture_rect;
    bool disabled;
} MenuButton;

// Struct for an info box entry
typedef struct
{
    char text[128];
    unsigned short font_size;
    SDL_Rect rect;
    SDL_Texture *text_texture;
    SDL_Rect texture_rect;
} InfoBoxEntry;

// Struct for a console entry
typedef struct
{
    char title[32];
    char value[16];
    SDL_Texture *text_texture;
    SDL_Rect texture_rect;
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
    unsigned short class;
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
    unsigned short level;
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
    bool final_star;
} Gstar;

typedef struct
{
    int initialized; // Initialized groups of sections so far
    int initialized_hd;
    int last_star_index; // Index of last star added to array
    int last_star_index_hd;
    int sections_in_group; // Stored so that we don't calculate on every iteration
    int sections_in_group_hd;
    int total_groups; // Total groups of sections grouped by <sections_in_group>
    int total_groups_hd;
    char name[MAX_OBJECT_NAME];
    unsigned short class;
    float radius;
    float cutoff;
    bool is_selected; // Whether the galaxy is selected in Universe
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
    bool final_star;
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
    Point mouse_position;
    Point mouse_down_position;
    Uint32 last_click_time;
    int click_count;
    bool is_mouse_dragging;
    bool left_on;
    bool right_on;
    bool up_on;
    bool down_on;
    bool thrust_on;
    bool reverse_on;
    bool camera_on;
    bool stop_on;
    bool zoom_in;
    bool zoom_out;
    bool console_on;
    int orbits_on;
    int selected_button_index;
    int is_hovering_galaxy;
} InputState;

typedef struct
{
    bool start_stars_generation;
    bool start_stars_preview;
    bool start_galaxies_generation;
    bool is_game_started;
    bool is_entering_map;
    bool is_exiting_map;
    bool is_centering_map;
    bool switch_to_universe;
    bool is_entering_universe;
    bool is_exiting_universe;
    bool is_centering_universe;
    bool switch_to_map;
    bool has_exited_galaxy;
    bool found_galaxy;
    bool generate_bstars;
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

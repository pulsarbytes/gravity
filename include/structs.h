#ifndef STRUCTS_H
#define STRUCTS_H

// Structs for the controls table
typedef struct
{
    char key[32];
    char description[64];
} ControlsEntry;

typedef struct
{
    char title[32];
    ControlsEntry controls[MAX_CONTROLS_ENTRIES];
    int num_controls;
} ControlsGroup;

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

typedef struct
{
    double x;
    double y;
} Point;

// Struct for a waypoint button
typedef struct
{
    SDL_Rect rect;
} WaypointButton;

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
    unsigned short class;
    float radius;
    float cutoff;
    float orbit_radius;
    Point position;
    float vx;
    float vy;
    float dx;
    float dy;
    SDL_Point projection; // Top-left point of the projection
    SDL_Color color;
    unsigned short num_planets;
    struct CelestialBody *planets[MAX_PLANETS_MOONS];
    struct CelestialBody *parent;
    unsigned short level;
    bool is_selected; // Whether the body is selected in Map
    char galaxy_name[MAX_OBJECT_NAME];
    WaypointButton waypoint_button;
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
    SDL_Color color;
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
    SDL_Point projection; // Top-left point of the projection
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
    SDL_Cursor *default_cursor;
    SDL_Cursor *pointing_cursor;
    SDL_Cursor *drag_cursor;
    SDL_Cursor *previous_cursor;
    Point mouse_position;
    Point mouse_down_position;
    Uint32 last_click_time;
    int click_count;
    bool is_mouse_double_clicked;
    bool is_mouse_dragging;
    bool clicked_inside_galaxy;
    bool clicked_inside_star;
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
    bool fps_on;
    bool orbits_on;
    unsigned short selected_menu_button_index;
    bool is_hovering_galaxy;
    bool is_hovering_star;
    bool is_hovering_star_info;
    bool is_hovering_star_waypoint_button;
    bool is_hovering_planet_waypoint_button;
    bool is_hovering_star_info_planet;
    unsigned short selected_star_info_planet_index;
} InputState;

typedef struct
{
    bool is_game_started;
    bool start_stars_generation;
    bool start_stars_preview;
    bool start_galaxies_generation;
    bool has_exited_galaxy;
    bool found_galaxy;
    bool generate_bstars;
    bool is_centering_navigate;
    bool switch_to_map;   // Enter map from Universe via zoom
    bool is_entering_map; // Enter Map via `M`
    bool is_exiting_map;  // Exit Map via `N` or `U`
    bool is_centering_map;
    bool switch_to_universe;   // Enter Universe from Map via zoom
    bool is_entering_universe; // Enter Universe via `U`
    bool is_exiting_universe;  // Exit Universe via `N` or `M`
    bool is_centering_universe;
    bool zoom_preview;      // Whether the stars preview has been requested by a zoom event
    bool lazy_load_started; // Whether lazy-loading for stars preview has started
} GameEvents;

typedef struct
{
    StarEntry *stars[MAX_STARS];         // Hash table for stars
    GalaxyEntry *galaxies[MAX_GALAXIES]; // Hash table for galaxies
    Galaxy *current_galaxy;
    Galaxy *buffer_galaxy; // Stores galaxy of current ship position
    Galaxy *previous_galaxy;
    Star *current_star;
    Star *selected_star;
    Star *waypoint_star;
    int waypoint_planet_index;
    Star *buffer_star; // Stores star of current ship position
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
    unsigned int fps;
    int speed_limit;
    int landing_stage;
    long double game_scale;
    long double save_scale;
    long double game_scale_override;
    MenuButton menu[MENU_BUTTON_COUNT];
    MenuButton logo;
    ControlsGroup controls_groups[MAX_CONTROLS_GROUPS];
    int table_top_row;
    int table_num_rows_displayed;
    unsigned short table_num_rows;
} GameState;

#endif /* STRUCTS_H */

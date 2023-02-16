#ifndef STRUCTS_H
#define STRUCTS_H

// Struct for a game console entry
struct game_console_entry
{
    char title[30];
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
    double previous_x;
    double previous_y;
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
};

// Struct for a galaxy
struct galaxy_t
{
    int initialized;
    char name[MAX_OBJECT_NAME];
    int class;
    float radius;
    float cutoff;
    struct point_t position;
    SDL_Rect projection;
    SDL_Color color;
    struct gstar_t gstars[MAX_GSTARS];
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

#endif /* STRUCTS_H */

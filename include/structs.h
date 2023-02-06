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

// Struct for x,y float coordinates
struct position_t
{
    float x;
    float y;
};

// Struct for a vector
struct vector_t
{
    float x;
    float y;
    float magnitude;
    float angle; // radians between the positive x-axis and the line connecting the origin to the point (vx, vy)
};

// Struct for a planet
struct planet_t
{
    char name[MAX_PLANET_NAME];
    char *image;
    int class;
    float radius;
    float cutoff;
    struct position_t position;
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
    float x;
    float y;
    struct planet_t *star;
    struct star_entry *next;
};

// Struct for a ship
struct ship_t
{
    char *image;
    int radius;
    struct position_t position;
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
struct bgstar_t
{
    struct position_t position;
    SDL_Rect rect;
    unsigned short opacity;
};

// Struct for camera
struct camera_t
{
    float x;
    float y;
    int w;
    int h;
};

#endif /* STRUCTS_H */

#ifndef STRUCTS_H
#define STRUCTS_H

// Struct for a game console entry
struct game_console_entry {
    char title[30];
    char value[16];
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_Rect rect;
};

// Struct for x,y float coordinates
struct position_t {
    float x;
    float y;
};

// Struct for a planet
struct planet_t {
    char *name;
    char *image;
    int radius;
    struct position_t position;
    float vx;
    float vy;
    float dx;
    float dy;
    SDL_Texture *texture;
    SDL_Rect rect;
    SDL_Rect projection;
    SDL_Color color;
    struct planet_t *moons[MAX_MOONS];
};

// Struct for a ship
struct ship_t {
    char *image;
    int radius;
    struct position_t position;
    float angle;
    float vx;
    float vy;
    SDL_Texture *texture;
    SDL_Rect rect;
    SDL_Rect main_img_rect;
    SDL_Rect thrust_img_rect;
    SDL_Point rotation_pt;
};

// Struct for a background star
struct bgstar_t {
    struct position_t position;
    SDL_Rect rect;
    unsigned short opacity;
};

// Struct for camera
struct camera_t {
    float x;
    float y;
    int w;
    int h;
};

#endif /* STRUCTS_H */

#ifndef MENU_H
#define MENU_H

// Function prototypes
void create_menu(MenuButton menu[]);
void create_logo(MenuButton *logo);
void update_menu(GameState *, int game_started);
void onMenu(GameState *, InputState *, int game_started, const NavigationState *, Bstar bstars[], Gstar menustars[], Camera *);

// External function prototypes
void update_bstars(int state, int camera_on, const NavigationState *, Bstar bstars[], const Camera *, Speed, double distance);
void draw_menu_galaxy_cloud(const Camera *, Gstar menustars[]);
void draw_speed_lines(float velocity, const Camera *, Speed);

#endif
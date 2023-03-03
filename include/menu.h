#ifndef MENU_H
#define MENU_H

// Function prototypes
void menu_populate_menu_array(MenuButton menu[]);
void menu_create_logo(MenuButton *logo);
void menu_update_menu_entries(GameState *);
void menu_run_menu_state(GameState *, InputState *, int game_started, const NavigationState *, Bstar *bstars, Gstar menustars[], Camera *);

// External function prototypes
void gfx_update_bstars_position(int state, int camera_on, const NavigationState *, Bstar *bstars, const Camera *, Speed, double distance);
void gfx_draw_menu_galaxy_cloud(const Camera *, Gstar menustars[]);
void gfx_draw_speed_lines(float velocity, const Camera *, Speed);

#endif
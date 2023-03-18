#ifndef MENU_H
#define MENU_H

// Function prototypes
void menu_create(GameState *, NavigationState, Gstar *menustars);
void menu_draw_menu(GameState *, InputState *, bool is_game_started);
bool menu_is_hovering_menu(GameState *game_state, InputState *input_state);
void menu_run_state(GameState *, InputState *, bool is_game_started, const NavigationState *, Bstar *bstars, Gstar *menustars, Camera *);
void menu_update_menu_entries(GameState *, GameEvents *);

// External function prototypes
Galaxy *galaxies_get_entry(GalaxyEntry *galaxies[], Point);
void gfx_draw_menu_galaxy_cloud(const Camera *, Gstar *menustars);
void gfx_draw_speed_lines(float velocity, const Camera *, Speed);
void gfx_generate_menu_gstars(Galaxy *, Gstar *menustars);
void gfx_update_bstars_position(int state, bool camera_on, const NavigationState *, Bstar *bstars, const Camera *, Speed, double distance);
bool maths_is_point_in_rectangle(Point, Point rect[]);

#endif
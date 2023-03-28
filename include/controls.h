#ifndef CONTROLS_H
#define CONTROLS_H

// Function prototypes
void controls_create_table(GameState *, const Camera *);
void controls_run_state(GameState *, InputState *, bool is_game_started, const NavigationState *, Bstar *bstars, Gstar *menustars, const Camera *);

// External function prototypes
void gfx_draw_menu_galaxy_cloud(const Camera *, Gstar *menustars);
void gfx_draw_speed_lines(float velocity, const Camera *, Speed);
void gfx_update_bstars_position(int state, bool camera_on, const NavigationState *, Bstar *bstars, const Camera *, Speed, double distance);
void menu_draw_menu(GameState *, InputState *, bool is_game_started);
void sdl_set_cursor(InputState *, unsigned short cursor_type);

#endif
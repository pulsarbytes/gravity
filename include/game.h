#ifndef GAME_H
#define GAME_H

// Function prototypes
void game_reset(GameState *, InputState *, GameEvents *, NavigationState *, Bstar *bstars, Ship *, Camera *, bool reset);
void game_change_state(GameState *, GameEvents *, int new_state);
void game_run_navigate_state(GameState *, InputState *, GameEvents *, NavigationState *, Bstar *bstars, Ship *, Camera *);
void game_run_map_state(GameState *, InputState *, GameEvents *, NavigationState *, Bstar *bstars, Ship *, Camera *);
void game_run_universe_state(GameState *, InputState *, GameEvents *, NavigationState *, Ship *, Camera *);
Ship game_create_ship(int radius, Point, long double scale);

// External function prototypes
void menu_update_menu_entries(GameState *);
void stars_clear_table(StarEntry *stars[]);
void galaxies_clear_table(GalaxyEntry *galaxies[]);
void galaxies_generate(GameEvents *, NavigationState *, Point);
Galaxy *galaxies_get_entry(GalaxyEntry *galaxies[], Point);
double maths_get_nearest_section_axis(double offset, int size);
void stars_delete_outside_region(StarEntry *stars[], double bx, double by, int region_size);
void gfx_update_camera(Camera *, Point, long double scale);
void gfx_zoom_star_system(CelestialBody *, long double scale);
void stars_generate(GameState *, GameEvents *, NavigationState *, Bstar *bstars, Ship *, const Camera *);
double maths_distance_between_points(double x1, double y1, double x2, double y2);
void gfx_update_gstars_position(Galaxy *, Point, const Camera *, double distance, double limit);
void gfx_update_bstars_position(int state, int camera_on, const NavigationState *, Bstar *bstars, const Camera *, Speed speed, double distance);
void gfx_draw_speed_lines(float velocity, const Camera *, Speed);
void gfx_draw_speed_arc(const Ship *, const Camera *, long double scale);
void stars_update_orbital_positions(GameState *, const InputState *, NavigationState *, CelestialBody *, Ship *, int star_class);
void stars_draw_star_system(GameState *, const InputState *, NavigationState *, CelestialBody *, const Camera *);
void phys_update_velocity(Vector *velocity, const Ship *);
Galaxy *galaxies_nearest_circumference(const NavigationState *, Point, int exclude);
void gfx_project_galaxy_on_edge(int state, const NavigationState *, Galaxy *, const Camera *, long double scale);
void gfx_generate_gstars(Galaxy *, unsigned short high_definition);
void gfx_draw_screen_frame(Camera *);
void gfx_draw_section_lines(Camera *, int section_size, SDL_Color color, long double scale);
void gfx_project_ship_on_edge(int state, const InputState *, const NavigationState *, Ship *, const Camera *, long double scale);
void gfx_draw_circle_approximation(SDL_Renderer *renderer, const Camera *, int x, int y, int r, SDL_Color color);
void stars_generate_preview(NavigationState *, const Camera *, Point *, int zoom_preview, long double scale);
void galaxies_draw_galaxy(NavigationState *, Galaxy *, const Camera *, int state, long double scale);
bool gfx_object_in_camera(const Camera *, double x, double y, float radius, long double scale);
void gfx_generate_bstars(NavigationState *, Bstar *bstars, const Camera *);
void console_update_entry(ConsoleEntry entries[], int index, double value);

#endif
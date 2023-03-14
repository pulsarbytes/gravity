#ifndef GAME_H
#define GAME_H

// Function prototypes
void game_change_state(GameState *, GameEvents *, int new_state);
Ship game_create_ship(int radius, Point, long double scale);
void game_reset(GameState *, InputState *, GameEvents *, NavigationState *, Bstar *bstars, Ship *, Camera *, bool reset);
void game_run_map_state(GameState *, InputState *, GameEvents *, NavigationState *, Bstar *bstars, Ship *, Camera *);
void game_run_navigate_state(GameState *, InputState *, GameEvents *, NavigationState *, Bstar *bstars, Ship *, Camera *);
void game_run_universe_state(GameState *, InputState *, GameEvents *, NavigationState *, Ship *, Camera *);

// External function prototypes
void console_draw_galaxy_console(const Galaxy *, const Camera *);
void console_draw_position_console(const GameState *, const NavigationState *, const Camera *, Point);
void console_draw_ship_console(const NavigationState *, const Ship *, const Camera *);
void console_draw_star_console(const Star *, const Camera *);
void galaxies_clear_table(GalaxyEntry *galaxies[]);
void galaxies_draw_galaxy(const InputState *, NavigationState *, Galaxy *, const Camera *, int state, long double scale);
void galaxies_generate(GameEvents *, NavigationState *, Point);
Galaxy *galaxies_get_entry(GalaxyEntry *galaxies[], Point);
Galaxy *galaxies_nearest_circumference(const NavigationState *, Point, int exclude);
void gfx_draw_circle(SDL_Renderer *, const Camera *, int xc, int yc, int radius, SDL_Color);
void gfx_draw_circle_approximation(SDL_Renderer *renderer, const Camera *, int x, int y, int r, SDL_Color color);
void galaxies_draw_info_box(const Galaxy *, const Camera *);
void gfx_draw_screen_frame(Camera *);
void gfx_draw_section_lines(Camera *, int state, SDL_Color color, long double scale);
void gfx_draw_speed_arc(const Ship *, const Camera *, long double scale);
void gfx_draw_speed_lines(float velocity, const Camera *, Speed);
void gfx_generate_bstars(GameEvents *, NavigationState *, Bstar *bstars, const Camera *, bool lazy_load);
void gfx_generate_gstars(Galaxy *, bool high_definition);
bool gfx_is_object_in_camera(const Camera *, double x, double y, float radius, long double scale);
void gfx_project_galaxy_on_edge(int state, const NavigationState *, Galaxy *, const Camera *, long double scale);
void gfx_project_ship_on_edge(int state, const InputState *, const NavigationState *, Ship *, const Camera *, long double scale);
void gfx_toggle_galaxy_hover(InputState *, const NavigationState *, const Camera *, long double scale);
void gfx_toggle_star_hover(InputState *, const NavigationState *, const Camera *, long double scale, int state);
void gfx_update_bstars_position(int state, bool camera_on, const NavigationState *, Bstar *bstars, const Camera *, Speed speed, double distance);
void gfx_update_camera(Camera *, Point, long double scale);
void gfx_update_gstars_position(Galaxy *, Point, const Camera *, double distance, double limit);
void gfx_zoom_star_system(CelestialBody *, long double scale);
double maths_distance_between_points(double x1, double y1, double x2, double y2);
double maths_get_nearest_section_line(double offset, int size);
uint64_t maths_hash_position_to_uint64(Point);
bool maths_points_equal(Point, Point);
void menu_update_menu_entries(GameState *);
void phys_update_velocity(Vector *velocity, const Ship *);
void stars_clear_table(StarEntry *stars[], Star *);
void stars_delete_outside_region(StarEntry *stars[], const Star *, double bx, double by, int region_size);
void stars_draw_info_box(const Star *, const Camera *);
void stars_draw_planets_info_box(const Star *, const Camera *);
void stars_draw_star_system(GameState *, const InputState *, NavigationState *, CelestialBody *, const Camera *);
void stars_generate(GameState *, GameEvents *, NavigationState *, Bstar *bstars, Ship *);
void stars_generate_preview(GameEvents *, NavigationState *, const Camera *, long double scale);
void stars_initialize_star(Star *);
void stars_populate_body(CelestialBody *, Point, pcg32_random_t rng, long double scale);
void stars_update_orbital_positions(GameState *, const InputState *, NavigationState *, CelestialBody *, Ship *, const Camera *, unsigned short star_class);

#endif
#ifndef GRAPHICS_H
#define GRAPHICS_H

// Function prototypes
void gfx_create_default_colors(void);
void gfx_draw_circle(SDL_Renderer *, const Camera *, int xc, int yc, int radius, SDL_Color);
void gfx_draw_circle_approximation(SDL_Renderer *, const Camera *, int x, int y, int r, SDL_Color);
void gfx_draw_galaxy_cloud(Galaxy *, const Camera *, int gstars_count, bool high_definition, long double scale);
void gfx_draw_fill_circle(SDL_Renderer *, int xc, int yc, int radius, SDL_Color);
void gfx_draw_menu_galaxy_cloud(const Camera *, Gstar *menustars);
void gfx_draw_screen_frame(Camera *);
void gfx_draw_section_lines(Camera *, int state, SDL_Color color, long double scale);
void gfx_draw_speed_arc(const Ship *, const Camera *, long double scale);
void gfx_draw_speed_lines(float velocity, const Camera *, Speed);
void gfx_generate_bstars(GameEvents *, NavigationState *nav_state, Bstar *bstars, const Camera *camera, bool lazy_load);
void gfx_generate_gstars(Galaxy *, bool high_definition);
void gfx_generate_menu_gstars(Galaxy *, Gstar *menustars);
bool gfx_is_object_in_camera(const Camera *, double x, double y, float radius, long double scale);
bool gfx_is_relative_position_in_camera(const Camera *, int x, int y);
void gfx_project_body_on_edge(const GameState *, const NavigationState *, CelestialBody *, const Camera *);
void gfx_project_galaxy_on_edge(int state, const NavigationState *, Galaxy *, const Camera *, long double scale);
void gfx_project_ship_on_edge(int state, const InputState *, const NavigationState *, Ship *, const Camera *, long double scale);
void gfx_toggle_galaxy_hover(InputState *, const NavigationState *, const Camera *, long double scale);
void gfx_toggle_star_hover(InputState *, const NavigationState *, const Camera *, long double scale, int state);
void gfx_update_bstars_position(int state, bool camera_on, const NavigationState *, Bstar *bstars, const Camera *, Speed, double distance);
void gfx_update_camera(Camera *, Point, long double scale);
void gfx_update_gstars_position(Galaxy *, Point, const Camera *, double distance, double limit);

// External function prototypes
double maths_distance_between_points(double x1, double y1, double x2, double y2);
double maths_get_nearest_section_line(double offset, int size);
uint64_t maths_hash_position_to_uint64(Point);
uint64_t maths_hash_position_to_uint64_2(Point);
bool maths_line_intersects_camera(const Camera *, double x1, double y1, double x2, double y2);
bool maths_points_equal(Point, Point);
double stars_nearest_center_distance(Point, Galaxy *, uint64_t initseq, int galaxy_density);
unsigned short stars_size_class(float distance);

#endif
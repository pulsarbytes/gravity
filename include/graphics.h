#ifndef GRAPHICS_H
#define GRAPHICS_H

// Function prototypes
void gfx_update_camera(Camera *, Point, long double scale);
void gfx_project_ship_on_edge(int state, const InputState *, const NavigationState *, Ship *, const Camera *, long double scale);
void gfx_project_body_on_edge(const GameState *, const NavigationState *, CelestialBody *, const Camera *);
void gfx_project_galaxy_on_edge(int state, const NavigationState *, Galaxy *, const Camera *, long double scale);
void gfx_zoom_star_system(CelestialBody *, long double scale);
bool gfx_relative_position_in_camera(const Camera *, int x, int y);
bool gfx_object_in_camera(const Camera *, double x, double y, float radius, long double scale);
void gfx_draw_screen_frame(Camera *);
void gfx_draw_section_lines(Camera *, int section_size, SDL_Color color, long double scale);
void gfx_generate_menu_gstars(Galaxy *, Gstar menustars[]);
void gfx_draw_menu_galaxy_cloud(const Camera *, Gstar menustars[]);
void gfx_generate_gstars(Galaxy *, unsigned short high_definition);
void gfx_draw_galaxy_cloud(Galaxy *, const Camera *, int gstars_count, unsigned short high_definition, long double scale);
void gfx_draw_speed_arc(const Ship *, const Camera *, long double scale);
void gfx_draw_speed_lines(float velocity, const Camera *, Speed);
void gfx_create_default_colors(void);
void gfx_draw_circle(SDL_Renderer *, const Camera *, int xc, int yc, int radius, SDL_Color);
void gfx_draw_circle_approximation(SDL_Renderer *, const Camera *, int x, int y, int r, SDL_Color);
void gfx_update_gstars_position(Galaxy *, Point, const Camera *, double distance, double limit);
void gfx_generate_bstars(NavigationState *nav_state, Bstar *bstars, const Camera *camera);
void gfx_update_bstars_position(int state, int camera_on, const NavigationState *, Bstar *bstars, const Camera *, Speed, double distance);

// External function prototypes
uint64_t maths_hash_position_to_uint64(Point);
uint64_t maths_hash_position_to_uint64_2(Point);
double maths_get_nearest_section_axis(double offset, int size);
double stars_nearest_center_distance(Point, Galaxy *, uint64_t initseq, int galaxy_density);
int stars_size_class(float distance);
bool maths_line_intersects_camera(const Camera *, double x1, double y1, double x2, double y2);
double maths_distance_between_points(double x1, double y1, double x2, double y2);

#endif
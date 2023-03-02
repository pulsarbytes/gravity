#ifndef GRAPHICS_H
#define GRAPHICS_H

// Function prototypes
void update_camera(Camera *, Point, long double scale);
void update_projection_coordinates(const NavigationState *, void *ptr, int entity_type, const Camera *, int state, long double scale);
void project_ship(int state, const InputState *, const NavigationState *, Ship *, const Camera *, long double scale);
void project_body(const GameState *, const NavigationState *, CelestialBody *, const Camera *);
void project_galaxy(int state, const NavigationState *, Galaxy *, const Camera *, long double scale);
int calculate_projection_opacity(double distance, int region_size, int section_size);
void zoom_star(CelestialBody *, long double scale);
int in_camera_relative(const Camera *, int x, int y);
int in_camera(const Camera *, double x, double y, float radius, long double scale);
void draw_screen_frame(Camera *);
void draw_section_lines(Camera *, int section_size, SDL_Color color, long double scale);
void create_menu_galaxy_cloud(Galaxy *, Gstar menustars[]);
void draw_menu_galaxy_cloud(const Camera *, Gstar menustars[]);
void create_galaxy_cloud(Galaxy *, unsigned short high_definition);
void draw_galaxy_cloud(Galaxy *, const Camera *, int gstars_count, unsigned short high_definition, long double scale);
void draw_speed_arc(const Ship *, const Camera *, long double scale);
void draw_speed_lines(float velocity, const Camera *, Speed);
void create_colors(void);
void SDL_DrawCircle(SDL_Renderer *renderer, const Camera *, int xc, int yc, int radius, SDL_Color color);
void SDL_DrawCircleApprox(SDL_Renderer *renderer, const Camera *, int x, int y, int r, SDL_Color color);
void update_gstars(Galaxy *, Point, const Camera *, double distance, double limit);
void update_bstars(int state, int camera_on, const NavigationState *, Bstar bstars[], const Camera *, Speed, double distance);

// External function prototypes
uint64_t pair_hash_order_sensitive(Point);
uint64_t pair_hash_order_sensitive_2(Point);
double find_nearest_section_axis(double offset, int size);
double nearest_star_distance(Point, Galaxy *, uint64_t initseq, int galaxy_density);
int get_star_class(float distance);
bool line_intersects_viewport(const Camera *, double x1, double y1, double x2, double y2);
double find_distance(double x1, double y1, double x2, double y2);

#endif
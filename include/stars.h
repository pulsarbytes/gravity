#ifndef STARS_H
#define STARS_H

// Function prototypes
void stars_clear_table(StarEntry *stars[]);
void stars_delete_outside_region(StarEntry *stars[], double bx, double by, int region_size);
void stars_draw_info_box(const Star *, const Camera *);
void stars_draw_planets_info_box(const Star *, const Camera *);
void stars_draw_star_system(GameState *, const InputState *, NavigationState *, CelestialBody *, const Camera *);
void stars_generate(GameState *, GameEvents *, NavigationState *, Bstar *bstars, Ship *ship);
void stars_generate_preview(GameEvents *, NavigationState *, const Camera *, Point *, long double scale);
void stars_initialize_star(Star *);
double stars_nearest_center_distance(Point, Galaxy *, uint64_t initseq, int galaxy_density);
unsigned short stars_size_class(float distance);
void stars_update_orbital_positions(GameState *, const InputState *, NavigationState *, CelestialBody *, Ship *, const Camera *, unsigned short star_class);

// External function prototypes
void galaxies_generate(GameEvents *, NavigationState *, Point);
Galaxy *galaxies_nearest_circumference(const NavigationState *, Point, int exclude);
void gfx_draw_circle(SDL_Renderer *renderer, const Camera *, int xc, int yc, int radius, SDL_Color color);
void gfx_draw_fill_circle(SDL_Renderer *, int xc, int yc, int radius, SDL_Color);
bool gfx_is_object_in_camera(const Camera *, double x, double y, float radius, long double scale);
void gfx_project_body_on_edge(const GameState *, const NavigationState *, CelestialBody *, const Camera *);
bool maths_check_point_in_array(Point, Point arr[], int len);
double maths_distance_between_points(double x1, double y1, double x2, double y2);
double maths_get_nearest_section_line(double offset, int size);
uint64_t maths_hash_position_to_index(Point, int modulo, int entity_type);
uint64_t maths_hash_position_to_uint64(Point);
uint64_t maths_hash_position_to_uint64_2(Point);
bool maths_is_point_in_circle(Point, Point, int radius);
bool maths_is_point_in_rectangle(Point, Point rect[]);
bool maths_points_equal(Point, Point);
void phys_apply_gravity_to_ship(GameState *, bool thrust_on, NavigationState *, CelestialBody *, Ship *ship, unsigned short star_class);
void phys_calculate_orbital_velocity(float distance, float angle, float radius, float *vx, float *vy);
void phys_update_velocity(Vector *velocity, const Ship *ship);
void utils_add_thousand_separators(int num, char *result, size_t result_size);

#endif
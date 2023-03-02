#ifndef STARS_H
#define STARS_H

// Function prototypes
static void cleanup_planets(CelestialBody *);
void cleanup_stars(StarEntry *stars[]);
void put_star(StarEntry *stars[], Point, Star *);
int star_exists(StarEntry *stars[], Point);
void delete_star(StarEntry *stars[], Point);
double nearest_star_distance(Point, Galaxy *, uint64_t initseq, int galaxy_density);
int get_star_class(float distance);
int get_planet_class(float width);
void delete_stars_outside_region(StarEntry *stars[], double bx, double by, int region_size);
Star *create_star(const NavigationState *, Point, int preview, long double scale);
void generate_stars(GameState *, GameEvents *, NavigationState *, Bstar bstars[], Ship *ship, const Camera *);
void generate_stars_preview(NavigationState *, const Camera *, Point *, int zoom_preview, long double scale);
void populate_star_system(CelestialBody *, Point, pcg32_random_t rng, long double scale);
void update_star_system(GameState *, const InputState *, NavigationState *, CelestialBody *, Ship *ship, const Camera *);

// External function prototypes
uint64_t pair_hash_order_sensitive(Point);
uint64_t pair_hash_order_sensitive_2(Point);
uint64_t unique_index(Point, int modulo, int entity_type);
bool point_in_array(Point, Point arr[], int len);
double find_nearest_section_axis(double offset, int size);
void generate_galaxies(GameEvents *, NavigationState *, Point);
Galaxy *find_nearest_galaxy(const NavigationState *, Point, int exclude);
double find_distance(double x1, double y1, double x2, double y2);
void create_bstars(NavigationState *, Bstar bstars[], const Camera *);
int point_in_rect(Point, Point rect[]);
void calc_orbital_velocity(float distance, float angle, float radius, float *vx, float *vy);
void SDL_DrawCircle(SDL_Renderer *renderer, const Camera *, int xc, int yc, int radius, SDL_Color color);
void project_body(const GameState *, const NavigationState *, CelestialBody *, const Camera *);
void apply_gravity_to_ship(GameState *, int thrust, NavigationState *, CelestialBody *, Ship *ship, const Camera *);
int in_camera(const Camera *, double x, double y, float radius, long double scale);

#endif
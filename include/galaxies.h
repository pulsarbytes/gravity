#ifndef GALAXIES_H
#define GALAXIES_H

// Function prototypes
void cleanup_galaxies(GalaxyEntry *galaxies[]);
void put_galaxy(GalaxyEntry *galaxies[], Point, Galaxy *);
int galaxy_exists(GalaxyEntry *galaxies[], Point);
Galaxy *get_galaxy(GalaxyEntry *galaxies[], Point);
void delete_galaxy(GalaxyEntry *galaxies[], Point);
double nearest_galaxy_center_distance(Point);
Galaxy *find_nearest_galaxy(const NavigationState *, Point, int exclude);
int get_galaxy_class(float distance);
Galaxy *create_galaxy(Point);
void update_galaxy(NavigationState *, Galaxy *, const Camera *, int state, long double scale);
void generate_galaxies(GameEvents *, NavigationState *, Point);

// External function prototypes
uint64_t pair_hash_order_sensitive(Point);
uint64_t pair_hash_order_sensitive_2(Point);
uint64_t unique_index(Point, int modulo, int entity_type);
double find_distance(double x1, double y1, double x2, double y2);
bool point_in_array(Point, Point arr[], int len);
void create_galaxy_cloud(Galaxy *, unsigned short high_definition);
void draw_galaxy_cloud(Galaxy *, const Camera *, int gstars_count, unsigned short high_definition, long double scale);
void cleanup_stars(StarEntry *stars[]);
void project_galaxy(int state, const NavigationState *, Galaxy *, const Camera *, long double scale);
void SDL_DrawCircle(SDL_Renderer *renderer, const Camera *, int xc, int yc, int radius, SDL_Color color);
int in_camera(const Camera *, double x, double y, float radius, long double scale);
double find_nearest_section_axis(double offset, int size);

#endif
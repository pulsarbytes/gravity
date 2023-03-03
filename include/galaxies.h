#ifndef GALAXIES_H
#define GALAXIES_H

// Function prototypes
void galaxies_clear_table(GalaxyEntry *galaxies[]);
void galaxies_draw_galaxy(NavigationState *, Galaxy *, const Camera *, int state, long double scale);
void galaxies_generate(GameEvents *, NavigationState *, Point);
Galaxy *galaxies_get_entry(GalaxyEntry *galaxies[], Point);
Galaxy *galaxies_nearest_circumference(const NavigationState *, Point, int exclude);

// External function prototypes
void gfx_draw_circle(SDL_Renderer *renderer, const Camera *, int xc, int yc, int radius, SDL_Color color);
void gfx_draw_galaxy_cloud(Galaxy *, const Camera *, int gstars_count, unsigned short high_definition, long double scale);
void gfx_generate_gstars(Galaxy *, unsigned short high_definition);
bool gfx_object_in_camera(const Camera *, double x, double y, float radius, long double scale);
void gfx_project_galaxy_on_edge(int state, const NavigationState *, Galaxy *, const Camera *, long double scale);
bool maths_check_point_in_array(Point, Point arr[], int len);
double maths_distance_between_points(double x1, double y1, double x2, double y2);
double maths_get_nearest_section_line(double offset, int size);
uint64_t maths_hash_position_to_index(Point, int modulo, int entity_type);
uint64_t maths_hash_position_to_uint64(Point);
uint64_t maths_hash_position_to_uint64_2(Point);
void stars_clear_table(StarEntry *stars[]);

#endif
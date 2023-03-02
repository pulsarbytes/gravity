#ifndef GAME_H
#define GAME_H

// Function prototypes
void change_state(GameState *, GameEvents *, int new_state);
void reset_game(GameState *, InputState *, GameEvents *, NavigationState *, Ship *);
void onNavigate(GameState *, InputState *, GameEvents *, NavigationState *, Bstar bstars[], Ship *, Camera *);
void onMap(GameState *, InputState *, GameEvents *, NavigationState *, Bstar bstars[], Ship *, Camera *);
void onUniverse(GameState *, InputState *, GameEvents *, NavigationState *, Ship *, Camera *);
Ship create_ship(int radius, Point, long double scale);
void update_ship(GameState *, const InputState *, const NavigationState *, Ship *, const Camera *);

// External function prototypes
void update_menu(GameState *, int game_started);
void cleanup_stars(StarEntry *stars[]);
void cleanup_galaxies(GalaxyEntry *galaxies[]);
void generate_galaxies(GameEvents *, NavigationState *, Point);
Galaxy *get_galaxy(GalaxyEntry *galaxies[], Point);
double find_nearest_section_axis(double offset, int size);
void delete_stars_outside_region(StarEntry *stars[], double bx, double by, int region_size);
void update_camera(Camera *, Point, long double scale);
void zoom_star(CelestialBody *, long double scale);
void generate_stars(GameState *, GameEvents *, NavigationState *, Bstar bstars[], Ship *, const Camera *);
double find_distance(double x1, double y1, double x2, double y2);
void update_gstars(Galaxy *, Point, const Camera *, double distance, double limit);
void update_bstars(int state, int camera_on, const NavigationState *, Bstar bstars[], const Camera *, Speed speed, double distance);
void draw_speed_lines(float velocity, const Camera *, Speed);
void draw_speed_arc(const Ship *, const Camera *, long double scale);
void update_star_system(GameState *, const InputState *, NavigationState *, CelestialBody *, Ship *, const Camera *);
void update_velocity(Vector *velocity, const Ship *);
Galaxy *find_nearest_galaxy(const NavigationState *, Point, int exclude);
void project_galaxy(int state, const NavigationState *, Galaxy *, const Camera *, long double scale);
void create_galaxy_cloud(Galaxy *, unsigned short high_definition);
void draw_screen_frame(Camera *);
void draw_section_lines(Camera *, int section_size, SDL_Color color, long double scale);
void project_ship(int state, const InputState *, const NavigationState *, Ship *, const Camera *, long double scale);
void SDL_DrawCircleApprox(SDL_Renderer *renderer, const Camera *, int x, int y, int r, SDL_Color color);
void generate_stars_preview(NavigationState *, const Camera *, Point *, int zoom_preview, long double scale);
void update_galaxy(NavigationState *, Galaxy *, const Camera *, int state, long double scale);
int in_camera(const Camera *, double x, double y, float radius, long double scale);

#endif
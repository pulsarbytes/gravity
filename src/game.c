/*
 * game.c
 */

#include <stdlib.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "../lib/pcg-c-basic-0.9/pcg_basic.h"

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"
#include "../include/game.h"

// External variable definitions
extern SDL_DisplayMode display_mode;
extern SDL_Renderer *renderer;
extern SDL_Color colors[];

// Static function prototypes
static void game_draw_ship(GameState *, const InputState *, const NavigationState *, Ship *, const Camera *);
static void game_engage_autopilot(InputState *, GameEvents *, NavigationState *, Ship *, double distance);
static void game_put_ship_in_orbit(CelestialBody *, Ship *, int radii);
static void game_scroll_map(const GameState *, const InputState *, NavigationState *, const Camera *);
static void game_scroll_universe(const GameState *, const InputState *, GameEvents *, NavigationState *, const Camera *);
static void game_update_ship_position(GameState *, const InputState *, Ship *, const Camera *);
static void game_zoom_map(GameState *, InputState *, GameEvents *, NavigationState *);
static void game_zoom_universe(GameState *, InputState *, GameEvents *, NavigationState *);

/**
 * Changes the state of the game to a new state and updates relevant game events.
 *
 * @param game_state A pointer to the current GameState object.
 * @param game_events A pointer to the current GameEvents object.
 * @param new_state An integer representing the new state to transition to.
 *
 * @return void
 */
void game_change_state(GameState *game_state, GameEvents *game_events, int new_state)
{
    game_state->state = new_state;

    if (game_state->state == NAVIGATE)
        game_events->is_game_started = true;
    else if (game_state->state == CONTROLS)
        game_state->table_top_row = 0;

    menu_update_menu_entries(game_state, game_events);
}

/**
 * Creates a ship object with the given parameters.
 *
 * @param radius The radius of the ship.
 * @param position The starting position of the ship.
 * @param scale The scale to apply to the ship's size.
 *
 * @return a Ship object with the specified properties
 */
Ship game_create_ship(int radius, Point position, long double scale)
{
    Ship ship;

    ship.image = "../assets/sprites/ship.png";
    ship.radius = radius;
    ship.position.x = (int)position.x;
    ship.position.y = (int)position.y;
    ship.previous_position.x = 0;
    ship.previous_position.y = 0;
    ship.vx = 0.0;
    ship.vy = 0.0;
    ship.angle = 0.0;
    SDL_Surface *surface = IMG_Load(ship.image);
    ship.texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    ship.rect.x = ship.position.x * scale - ship.radius;
    ship.rect.y = ship.position.y * scale - ship.radius;
    ship.rect.w = 2 * ship.radius;
    ship.rect.h = 2 * ship.radius;
    ship.main_img_rect.x = 0;
    ship.main_img_rect.y = 0;
    ship.main_img_rect.w = 162;
    ship.main_img_rect.h = 162;
    ship.thrust_img_rect.x = 256;
    ship.thrust_img_rect.y = 0;
    ship.thrust_img_rect.w = 162;
    ship.thrust_img_rect.h = 162;
    ship.reverse_img_rect.x = 428;
    ship.reverse_img_rect.y = 0;
    ship.reverse_img_rect.w = 162;
    ship.reverse_img_rect.h = 162;

    // Point around which ship will be rotated (relative to destination rect)
    ship.rotation_pt.x = ship.radius;
    ship.rotation_pt.y = ship.radius;

    return ship;
}

/**
 * Draws the ship on the screen with the given state and camera
 *
 * @param game_state A pointer to the current GameState object.
 * @param input_state A pointer to the current InputState object.
 * @param nav_state A pointer to the current NavigationState object.
 * @param ship A pointer to the ship to be drawn.
 * @param camera A pointer to the current Camera object.
 *
 * @return void
 */
static void game_draw_ship(GameState *game_state, const InputState *input_state, const NavigationState *nav_state, Ship *ship, const Camera *camera)
{
    if (gfx_is_object_in_camera(camera, ship->position.x, ship->position.y, ship->radius, game_state->game_scale))
    {
        SDL_RenderCopyEx(renderer, ship->texture, &ship->main_img_rect, &ship->rect, ship->angle, &ship->rotation_pt, SDL_FLIP_NONE);
    }
    // Draw ship projection
    else if (PROJECTIONS_ON)
        gfx_project_ship_on_edge(NAVIGATE, input_state, nav_state, ship, camera, game_state->game_scale);

    // Draw thrust
    if (input_state->thrust_on)
        SDL_RenderCopyEx(renderer, ship->texture, &ship->thrust_img_rect, &ship->rect, ship->angle, &ship->rotation_pt, SDL_FLIP_NONE);

    // Draw reverse thrust
    if (input_state->reverse_on)
        SDL_RenderCopyEx(renderer, ship->texture, &ship->reverse_img_rect, &ship->rect, ship->angle, &ship->rotation_pt, SDL_FLIP_NONE);
}

/**
 * Engages the autopilot and navigates the ship to the waypoint.
 *
 * @param input_state A pointer to the current InputState object.
 * @param game_events A pointer to the current GameEvents object.
 * @param nav_state A pointer to the current NavigationState object.
 * @param ship A pointer to the current ship.
 * @param distance The distance between ship and waypoint.
 *
 * @return void
 */
static void game_engage_autopilot(InputState *input_state, GameEvents *game_events, NavigationState *nav_state, Ship *ship, double distance)
{
    game_events->deccelerate_to_waypoint = false;

    // Ship must point to next path point
    double segment_dx = nav_state->waypoint_star->waypoint_path[nav_state->next_path_point - 1].position.x - nav_state->waypoint_star->waypoint_path[nav_state->next_path_point].position.x;
    double segment_dy = nav_state->waypoint_star->waypoint_path[nav_state->next_path_point - 1].position.y - nav_state->waypoint_star->waypoint_path[nav_state->next_path_point].position.y;
    double segment_angle = atan2(-segment_dx, segment_dy) * 180.0 / M_PI;

    double ship_to_point_dx = ship->position.x - nav_state->waypoint_star->waypoint_path[nav_state->next_path_point].position.x;
    double ship_to_point_dy = ship->position.y - nav_state->waypoint_star->waypoint_path[nav_state->next_path_point].position.y;
    double ship_to_point_angle = atan2(-ship_to_point_dx, ship_to_point_dy) * 180.0 / M_PI;

    // Check whether ship is on the segment line
    bool ship_on_segment = maths_is_point_on_line(nav_state->waypoint_star->waypoint_path[nav_state->next_path_point - 1].position,
                                                  nav_state->waypoint_star->waypoint_path[nav_state->next_path_point].position,
                                                  ship->position);

    if (ship_on_segment && nav_state->velocity.magnitude > GALAXY_SPEED_LIMIT - 1)
        ship->angle = segment_angle;
    else
    {
        float delta_01 = 0.1;
        float delta_1 = 1.0;
        float delta_2 = 2.0;
        float delta_3 = 3.0;
        float delta_4 = 4.0;
        float delta_5 = 5.0;

        if (ship_to_point_angle < 0)
            ship_to_point_angle += 360.0;

        double angle_diff = fmod(ship_to_point_angle - ship->angle + 360.0, 360.0);

        if (angle_diff > 180.0)
            angle_diff -= 360.0;
        else if (angle_diff < -180.0)
            angle_diff += 360.0;

        if (!game_events->autopilot_rotated_ship)
        {
            if (angle_diff > 3)
                ship->angle += delta_3;
            else if (angle_diff < -3)
                ship->angle -= delta_3;
        }
        else
        {
            if (angle_diff > 135.0)
                ship->angle += delta_5;
            else if (angle_diff < -135.0)
                ship->angle -= delta_5;
            else if (angle_diff > 90.0)
                ship->angle += delta_4;
            else if (angle_diff < -90.0)
                ship->angle -= delta_4;
            else if (angle_diff > 45.0)
                ship->angle += delta_3;
            else if (angle_diff < -45.0)
                ship->angle -= delta_3;
            else if (angle_diff > 22.5)
                ship->angle += delta_2;
            else if (angle_diff < -22.5)
                ship->angle -= delta_2;
            else if (angle_diff > 1)
                ship->angle += delta_1;
            else if (angle_diff < -1)
                ship->angle -= delta_1;
            else if (angle_diff > 0.1)
                ship->angle += delta_01;
            else if (angle_diff < -0.1)
                ship->angle -= delta_01;
        }
    }

    // Keep ship on path
    double diff;

    if (segment_angle < 0)
        segment_angle += 360.0;

    if (segment_angle > ship->angle)
    {
        diff = ceil(segment_angle - ship->angle);

        if (segment_angle - ship->angle < 1)
            diff = 1;
    }
    else
    {
        diff = ceil(ship->angle - segment_angle);

        if (ship->angle - segment_angle < 1)
            diff = 1;
    }

    if (diff >= 360)
        diff -= 360;

    int speed_limit = GALAXY_SPEED_LIMIT;
    double ship_velocity = sqrt((ship->vx * ship->vx) + (ship->vy * ship->vy));

    if (game_events->autopilot_rotated_ship)
    {
        double radians = ship->angle * M_PI / 180;

        bool ship_in_start_cutoff = maths_is_point_in_circle(ship->position,
                                                             nav_state->buffer_star->position,
                                                             nav_state->buffer_star->cutoff);

        if ((ship_in_start_cutoff && distance < nav_state->buffer_star->cutoff) || speed_limit - ship_velocity > 1)
        {
            ship->vx += diff * G_THRUST * sin(radians);
            ship->vy -= diff * G_THRUST * cos(radians);
        }
        else
        {
            ship->vx += 10 * diff * G_THRUST * sin(radians);
            ship->vy -= 10 * diff * G_THRUST * cos(radians);
        }

        ship_velocity = sqrt((ship->vx * ship->vx) + (ship->vy * ship->vy));
    }

    if (!game_events->autopilot_rotated_ship && fabs(ship_to_point_angle - ship->angle) < 3)
        game_events->autopilot_rotated_ship = true;

    // Deccelerate when approaching last point in path
    bool ship_in_waypoint_cutoff = false;
    bool ship_in_planet_cutoff = false;

    if (nav_state->waypoint_star->initialized)
        ship_in_waypoint_cutoff = maths_is_point_in_circle(ship->position,
                                                           nav_state->waypoint_star->position,
                                                           nav_state->waypoint_star->cutoff);

    if (nav_state->waypoint_planet_index >= 0)
        ship_in_planet_cutoff = maths_is_point_in_circle(ship->position,
                                                         nav_state->waypoint_star->planets[nav_state->waypoint_planet_index]->position,
                                                         nav_state->waypoint_star->planets[nav_state->waypoint_planet_index]->cutoff);

    if (nav_state->waypoint_star->initialized &&
        ((nav_state->waypoint_planet_index < 0 && ship_in_waypoint_cutoff && distance < 10 * nav_state->waypoint_star->radius) ||
         (nav_state->waypoint_planet_index >= 0 && ship_in_planet_cutoff && distance < 10 * nav_state->waypoint_star->planets[nav_state->waypoint_planet_index]->radius)))
    {
        // Star
        if (nav_state->waypoint_planet_index < 0)
        {
            speed_limit = 20 + GALAXY_SPEED_LIMIT * (distance - 3 * nav_state->waypoint_star->radius) / (7 * nav_state->waypoint_star->radius);
        }
        // Planet
        else
        {
            speed_limit = 20 + GALAXY_SPEED_LIMIT * (distance - 3 * nav_state->waypoint_star->planets[nav_state->waypoint_planet_index]->radius) / (7 * nav_state->waypoint_star->planets[nav_state->waypoint_planet_index]->radius);
        }

        if (ship_velocity >= speed_limit)
        {
            game_events->deccelerate_to_waypoint = true;

            // Draw reverse thrust
            SDL_RenderCopyEx(renderer, ship->texture, &ship->reverse_img_rect, &ship->rect, ship->angle, &ship->rotation_pt, SDL_FLIP_NONE);
        }
    }

    // Limit speed to speed_limit
    if (ship_velocity >= speed_limit)
    {
        ship->vx = speed_limit * ship->vx / ship_velocity;
        ship->vy = speed_limit * ship->vy / ship_velocity;
    }

    // Draw ship thrust
    if (game_events->autopilot_rotated_ship && !game_events->deccelerate_to_waypoint)
    {
        double velocity_angle = 90 + atan2(ship->vy, ship->vx) * 180 / M_PI;

        if (velocity_angle < 0)
            velocity_angle += 360.0;

        if (fabs(ship_to_point_angle - velocity_angle) > 1 || speed_limit - ship_velocity > 1)
            SDL_RenderCopyEx(renderer, ship->texture, &ship->thrust_img_rect, &ship->rect, ship->angle, &ship->rotation_pt, SDL_FLIP_NONE);
    }

    // Disengage autopilot
    if (nav_state->next_path_point > nav_state->waypoint_star->waypoint_points)
        input_state->autopilot_on = false;

    double arrived_at_waypoint = false;

    // Star
    if (nav_state->waypoint_planet_index < 0)
    {
        arrived_at_waypoint = maths_distance_between_points(ship->position.x, ship->position.y,
                                                            nav_state->waypoint_star->position.x,
                                                            nav_state->waypoint_star->position.y) <= WAYPOINT_ORBIT_RADII * nav_state->waypoint_star->radius;
    }
    // Planet
    else
    {
        arrived_at_waypoint = maths_distance_between_points(ship->position.x, ship->position.y,
                                                            nav_state->waypoint_star->planets[nav_state->waypoint_planet_index]->position.x,
                                                            nav_state->waypoint_star->planets[nav_state->waypoint_planet_index]->position.y) <= WAYPOINT_ORBIT_RADII * nav_state->waypoint_star->planets[nav_state->waypoint_planet_index]->radius;
    }

    if (arrived_at_waypoint)
    {
        input_state->autopilot_on = false;
        ship->vx = 0;
        ship->vy = 0;

        game_events->arrived_at_waypoint = true;
    }
}

/**
 * Puts ship in orbit around a celestial body.
 *
 * @param body The celestial body to be orbited.
 * @param ship A pointer to the current Ship object.
 * @param radii Distance of ship from body in radii.
 *
 * @return void
 */
static void game_put_ship_in_orbit(CelestialBody *body, Ship *ship, int radii)
{
    float vx, vy;
    double radius = body->radius;
    double dx = ship->position.x - body->position.x;
    double dy = ship->position.y - body->position.y;
    double angle = atan2(dy, dx);
    double angle_degrees = angle * 180 / M_PI;

    phys_calculate_orbital_velocity(radii * radius, angle_degrees, radius, &vx, &vy);

    if (body->level == LEVEL_STAR)
    {
        ship->vx = vx;
        ship->vy = vy;
    }
    else if (body->level == LEVEL_PLANET)
    {
        ship->vx = vx + body->vx;
        ship->vy = vy + body->vy;
    }
}

/**
 * Resets the game state to the initial state.
 *
 * @param game_state A pointer to the current GameState object.
 * @param input_state A pointer to the current InputState object.
 * @param game_events A pointer to the current GameEvents object.
 * @param nav_state A pointer to the current NavigationState object.
 * @param bstars A pointer to the bstars.
 * @param ship A pointer to the ship.
 * @param camera A pointer to the current Camera object.
 * @param reset A boolean value indicating whether the game is being reset or not.
 *
 * @return void
 */
void game_reset(GameState *game_state, InputState *input_state, GameEvents *game_events, NavigationState *nav_state, Bstar *bstars, Ship *ship, Camera *camera, bool reset)
{
    if (reset)
        game_state->state = NAVIGATE;
    else
        game_state->state = MENU;

    // GameState
    game_state->fps = 0;
    game_state->speed_limit = BASE_SPEED_LIMIT;
    game_state->landing_stage = STAGE_OFF;
    game_state->game_scale = ZOOM_NAVIGATE;
    game_state->save_scale = false;
    game_state->game_scale_override = 0;

    // InputState
    input_state->default_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    input_state->pointing_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    input_state->drag_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
    input_state->previous_cursor = NULL;
    input_state->mouse_position.x = 0;
    input_state->mouse_position.y = 0;
    input_state->mouse_down_position.x = 0;
    input_state->mouse_down_position.y = 0;
    input_state->last_click_time = 0;
    input_state->click_count = 0;
    input_state->is_mouse_double_clicked = false;
    input_state->is_mouse_dragging = false;
    input_state->clicked_inside_galaxy = false;
    input_state->clicked_inside_star = false;
    input_state->left_on = false;
    input_state->right_on = false;
    input_state->up_on = false;
    input_state->down_on = false;
    input_state->thrust_on = false;
    input_state->reverse_on = false;
    input_state->camera_on = true;
    input_state->stop_on = false;
    input_state->zoom_in = false;
    input_state->zoom_out = false;
    input_state->fps_on = true;
    input_state->orbits_on = SHOW_ORBITS;
    input_state->autopilot_on = false;
    input_state->selected_menu_button_index = 0;
    input_state->is_hovering_galaxy = false;
    input_state->is_hovering_star = false;
    input_state->is_hovering_star_info = false;
    input_state->is_hovering_star_info_planet = false;
    input_state->is_hovering_star_waypoint_button = false;
    input_state->is_hovering_planet_waypoint_button = false;
    input_state->selected_star_info_planet_index = 0;

    // GameEvents
    if (reset)
        game_events->is_game_started = true;
    else
        game_events->is_game_started = false;

    game_events->start_stars_generation = true;
    game_events->start_stars_preview = true;
    game_events->start_galaxies_generation = true;
    game_events->has_exited_galaxy = false;
    game_events->found_galaxy = false;
    game_events->generate_bstars = false;
    game_events->is_centering_navigate = false;
    game_events->switch_to_map = false;
    game_events->is_entering_map = false;
    game_events->is_exiting_map = false;
    game_events->is_centering_map = false;
    game_events->switch_to_universe = false;
    game_events->is_entering_universe = false;
    game_events->is_exiting_universe = false;
    game_events->is_centering_universe = false;
    game_events->zoom_preview = false;
    game_events->lazy_load_started = false;
    game_events->arrived_at_waypoint = false;
    game_events->autopilot_rotated_ship = false;
    game_events->deccelerate_to_waypoint = false;
    game_events->is_centering_waypoint = false;

    // Galaxy position
    // Retrieved from saved game or use default values if this is a new game
    nav_state->galaxy_offset.current_x = UNIVERSE_START_X;
    nav_state->galaxy_offset.current_y = UNIVERSE_START_Y;
    nav_state->galaxy_offset.buffer_x = UNIVERSE_START_X; // stores x of buffer galaxy
    nav_state->galaxy_offset.buffer_y = UNIVERSE_START_Y; // stores y of buffer galaxy

    // Initialize universe sections cross lines
    nav_state->universe_cross_line.x = nav_state->galaxy_offset.current_x;
    nav_state->universe_cross_line.y = nav_state->galaxy_offset.current_y;

    // Navigation position
    nav_state->navigate_offset.x = GALAXY_START_X;
    nav_state->navigate_offset.y = GALAXY_START_Y;

    // Map position
    nav_state->map_offset.x = GALAXY_START_X;
    nav_state->map_offset.y = GALAXY_START_Y;

    // Universe position
    nav_state->universe_offset.x = nav_state->galaxy_offset.current_x;
    nav_state->universe_offset.y = nav_state->galaxy_offset.current_y;

    // Ship
    ship->position.x = GALAXY_START_X;
    ship->position.y = GALAXY_START_Y;
    ship->vx = 0;
    ship->vy = 0;
    ship->previous_position.x = 0;
    ship->previous_position.y = 0;
    ship->angle = 0;

    // Sync camera position with ship
    camera->x = ship->position.x - (display_mode.w / 2);
    camera->y = ship->position.y - (display_mode.h / 2);
    camera->w = display_mode.w;
    camera->h = display_mode.h;

    // Initialize galaxy sections cross lines
    // Add GALAXY_SECTION_SIZE so that stars generation is triggered on startup
    nav_state->cross_line.x = ship->position.x + GALAXY_SECTION_SIZE;
    nav_state->cross_line.y = ship->position.y + GALAXY_SECTION_SIZE;

    // Initialize velocity
    nav_state->velocity.magnitude = 0;
    nav_state->velocity.angle = 0;

    // Initialize stars hash table to NULL pointers
    for (int i = 0; i < MAX_STARS; i++)
    {
        nav_state->stars[i] = NULL;
    }

    if (reset)
        stars_clear_table(nav_state->stars, nav_state, true);

    // Initialize galaxies hash table to NULL pointers
    for (int i = 0; i < MAX_GALAXIES; i++)
    {
        nav_state->galaxies[i] = NULL;
    }

    if (reset)
        galaxies_clear_table(nav_state->galaxies);

    // Generate galaxies
    Point initial_position = {
        .x = nav_state->galaxy_offset.current_x,
        .y = nav_state->galaxy_offset.current_y};
    galaxies_generate(game_events, nav_state, initial_position);

    // Get a copy of current galaxy from the hash table
    Point galaxy_position = {
        .x = nav_state->galaxy_offset.current_x,
        .y = nav_state->galaxy_offset.current_y};
    Galaxy *current_galaxy_copy = galaxies_get_entry(nav_state->galaxies, galaxy_position);

    if (!reset)
    {
        // Allocate memory for current_galaxy
        nav_state->current_galaxy = (Galaxy *)malloc(sizeof(Galaxy));

        if (nav_state->current_galaxy == NULL)
        {
            fprintf(stderr, "Error: Could not allocate memory for current_galaxy.\n");
            return;
        }

        // Allocate memory for buffer_galaxy
        nav_state->buffer_galaxy = (Galaxy *)malloc(sizeof(Galaxy));

        if (nav_state->buffer_galaxy == NULL)
        {
            fprintf(stderr, "Error: Could not allocate memory for buffer_galaxy.\n");
            return;
        }

        // Allocate memory for previous_galaxy
        nav_state->previous_galaxy = (Galaxy *)malloc(sizeof(Galaxy));

        if (nav_state->previous_galaxy == NULL)
        {
            fprintf(stderr, "Error: Could not allocate memory for previous_galaxy.\n");
            return;
        }

        // Allocate memory for current_star
        nav_state->current_star = (Star *)malloc(sizeof(Star));

        if (nav_state->current_star == NULL)
        {
            fprintf(stderr, "Error: Could not allocate memory for current_star.\n");
            return;
        }

        stars_initialize_star(nav_state->current_star);

        // Allocate memory for selected_star
        nav_state->selected_star = (Star *)malloc(sizeof(Star));

        if (nav_state->selected_star == NULL)
        {
            fprintf(stderr, "Error: Could not allocate memory for selected_star.\n");
            return;
        }

        stars_initialize_star(nav_state->selected_star);

        // Allocate memory for waypoint_star
        nav_state->waypoint_star = (Star *)malloc(sizeof(Star));

        if (nav_state->waypoint_star == NULL)
        {
            fprintf(stderr, "Error: Could not allocate memory for waypoint_star.\n");
            return;
        }

        stars_initialize_star(nav_state->waypoint_star);

        // Allocate memory for buffer_star
        nav_state->buffer_star = (Star *)malloc(sizeof(Star));

        if (nav_state->buffer_star == NULL)
        {
            fprintf(stderr, "Error: Could not allocate memory for buffer_star.\n");
            return;
        }

        stars_initialize_star(nav_state->buffer_star);
    }
    else
    {
        if (nav_state->waypoint_star->waypoint_path != NULL)
        {
            free(nav_state->waypoint_star->waypoint_path);
            nav_state->waypoint_star->waypoint_path = NULL;
            nav_state->waypoint_star->waypoint_points = 0;
            stars_initialize_star(nav_state->waypoint_star);
        }
    }

    nav_state->waypoint_planet_index = -1;
    nav_state->next_path_point = 1;

    // Copy current_galaxy_copy to current_galaxy
    memcpy(nav_state->current_galaxy, current_galaxy_copy, sizeof(Galaxy));

    // Set current galaxy as selected
    nav_state->current_galaxy->is_selected = true;

    // Copy current_galaxy to buffer_galaxy
    memcpy(nav_state->buffer_galaxy, nav_state->current_galaxy, sizeof(Galaxy));

    // Generate background stars
    gfx_generate_bstars(game_events, nav_state, bstars, camera, false);
}

/**
 * Updates the game state and graphics for the map mode. This includes handling user input for zooming and
 * centering the view on the player's ship, as well as generating and rendering stars and galaxies.
 *
 * @param game_state A pointer to the current GameState object.
 * @param input_state A pointer to the current InputState object.
 * @param game_events A pointer to the current GameEvents object.
 * @param nav_state A pointer to the current NavigationState object.
 * @param bstars A pointer to the Bstar structure that holds information about the binary stars.
 * @param ship A pointer to the current ship.
 * @param camera A pointer to the current Camera object.
 *
 * @return void
 */
void game_run_map_state(GameState *game_state, InputState *input_state, GameEvents *game_events, NavigationState *nav_state, Bstar *bstars, Ship *ship, Camera *camera)
{
    if (game_events->is_entering_map || game_events->is_centering_map)
    {
        stars_clear_table(nav_state->stars, nav_state, false);
        game_events->start_stars_generation = true;

        if (game_events->is_entering_map)
        {
            // Save ship position
            ship->previous_position.x = ship->position.x;
            ship->previous_position.y = ship->position.y;

            if (!game_state->save_scale)
                game_state->save_scale = game_state->game_scale;
        }

        if (!game_events->switch_to_map)
        {
            game_state->game_scale = ZOOM_MAP;

            // Reset map_offset
            nav_state->map_offset.x = ship->previous_position.x;
            nav_state->map_offset.y = ship->previous_position.y;

            // Reset galaxy to current position
            if (!maths_points_equal(nav_state->current_galaxy->position, nav_state->buffer_galaxy->position))
            {
                memcpy(nav_state->current_galaxy, nav_state->buffer_galaxy, sizeof(Galaxy));

                // Reset galaxy_offset
                nav_state->galaxy_offset.current_x = nav_state->galaxy_offset.buffer_x;
                nav_state->galaxy_offset.current_y = nav_state->galaxy_offset.buffer_y;
            }

            // Reset current_star, selected_star
            if (maths_points_equal(nav_state->current_galaxy->position, nav_state->buffer_galaxy->position))
            {
                if (nav_state->current_star != NULL && nav_state->buffer_star != NULL)
                {
                    if (strcmp(nav_state->current_star->name, nav_state->buffer_star->name) != 0)
                        memcpy(nav_state->current_star, nav_state->buffer_star, sizeof(Star));

                    // Select current star
                    if (nav_state->selected_star != NULL && nav_state->buffer_star != NULL)
                    {
                        if (strcmp(nav_state->selected_star->name, nav_state->buffer_star->name) != 0)
                            memcpy(nav_state->selected_star, nav_state->buffer_star, sizeof(Star));

                        nav_state->selected_star->is_selected = true;
                    }
                }
            }
        }

        if (game_events->is_centering_waypoint)
        {
            // Reset current_star, selected_star
            if (maths_points_equal(nav_state->current_galaxy->position, nav_state->buffer_galaxy->position))
            {
                if (nav_state->current_star != NULL && nav_state->waypoint_star != NULL)
                {
                    if (strcmp(nav_state->current_star->name, nav_state->waypoint_star->name) != 0)
                        memcpy(nav_state->current_star, nav_state->waypoint_star, sizeof(Star));
                }

                if (nav_state->waypoint_star != NULL)
                {
                    if (strcmp(nav_state->selected_star->name, nav_state->waypoint_star->name) != 0)
                        memcpy(nav_state->selected_star, nav_state->waypoint_star, sizeof(Star));
                }

                nav_state->selected_star->is_selected = true;
            }

            game_events->is_centering_waypoint = false;
        }

        gfx_update_camera(camera, nav_state->map_offset, game_state->game_scale);

        if (game_events->is_centering_map)
        {
            // Reset ship position
            if (!game_events->is_exiting_universe)
            {
                ship->position.x = ship->previous_position.x;
                ship->position.y = ship->previous_position.y;
            }

            game_events->is_centering_map = false;
        }
    }

    game_zoom_map(game_state, input_state, game_events, nav_state);
    stars_generate(game_state, game_events, nav_state, bstars, ship);
    gfx_update_camera(camera, nav_state->map_offset, game_state->game_scale);

    if (!game_events->switch_to_universe)
        gfx_draw_section_lines(camera, game_state->state, colors[COLOR_ORANGE_32], game_state->game_scale);

    // Check for nearest galaxy, excluding current galaxy
    if (game_events->has_exited_galaxy && PROJECTIONS_ON)
    {
        // Convert offset to universe position
        Point universe_position;
        universe_position.x = nav_state->current_galaxy->position.x + nav_state->map_offset.x / GALAXY_SCALE;
        universe_position.y = nav_state->current_galaxy->position.y + nav_state->map_offset.y / GALAXY_SCALE;

        // Calculate camera position in universe scale
        Camera universe_camera = {
            .x = nav_state->current_galaxy->position.x * GALAXY_SCALE + camera->x,
            .y = nav_state->current_galaxy->position.y * GALAXY_SCALE + camera->y,
            .w = camera->w,
            .h = camera->h};

        Galaxy *nearest_galaxy = galaxies_nearest_circumference(nav_state, universe_position, true);

        if (nearest_galaxy != NULL && !maths_points_equal(nearest_galaxy->position, nav_state->current_galaxy->position))
        {
            // Project nearest galaxy
            gfx_project_galaxy_on_edge(MAP, nav_state, nearest_galaxy, &universe_camera, game_state->game_scale);
        }

        // Project current galaxy
        gfx_project_galaxy_on_edge(MAP, nav_state, nav_state->current_galaxy, &universe_camera, game_state->game_scale);
    }

    // Create galaxy cloud
    if (!nav_state->current_galaxy->initialized_hd || nav_state->current_galaxy->initialized_hd < nav_state->current_galaxy->total_groups_hd)
        gfx_generate_gstars(nav_state->current_galaxy, true);

    game_scroll_map(game_state, input_state, nav_state, camera);

    // Process star system
    if (!game_events->switch_to_universe && !game_events->is_entering_map && !input_state->zoom_in && !input_state->zoom_out)
    {
        for (int i = 0; i < MAX_STARS; i++)
        {
            if (nav_state->stars[i] != NULL)
            {
                // Each index can have many entries, loop through all of them
                StarEntry *entry = nav_state->stars[i];

                while (entry != NULL)
                {
                    // Don't show buffer_star if in another galaxy
                    if (strcmp(nav_state->current_galaxy->name, entry->star->galaxy_name) != 0)
                    {
                        entry = entry->next;
                        continue;
                    }

                    stars_draw_star_system(game_state, input_state, nav_state, entry->star, camera);
                    entry = entry->next;
                }
            }
        }
    }

    // Waypoint star
    if (nav_state->waypoint_star->initialized &&
        !game_events->switch_to_universe &&
        !input_state->zoom_in && !input_state->zoom_out)
    {
        // Don't show waypoint_star if in another galaxy
        if (strcmp(nav_state->current_galaxy->name, nav_state->waypoint_star->galaxy_name) == 0)
        {
            // Draw waypoint circle
            int waypoint_x = (nav_state->waypoint_star->position.x - camera->x) * game_state->game_scale;
            int waypoint_y = (nav_state->waypoint_star->position.y - camera->y) * game_state->game_scale;
            double star_waypoint_radius = nav_state->waypoint_star->cutoff * game_state->game_scale;
            SDL_Color waypoint_circle_color = nav_state->waypoint_star->color;
            waypoint_circle_color.a = 150;
            gfx_draw_circle(renderer, camera, waypoint_x, waypoint_y, star_waypoint_radius, waypoint_circle_color);
        }
    }

    // Check if mouse is over star info box
    input_state->is_hovering_star_info = gfx_toggle_star_info_hover(input_state, nav_state, camera);

    // Check if mouse is over current star (enables click on hovered star)
    if (!input_state->is_hovering_star_info)
        gfx_toggle_star_hover(input_state, nav_state, camera, game_state->game_scale, MAP);

    // Calculate and draw waypoint path
    if (nav_state->waypoint_star->initialized &&
        !input_state->zoom_in && !input_state->zoom_out &&
        strcmp(nav_state->current_galaxy->name, nav_state->waypoint_star->galaxy_name) == 0)
    {
        gfx_calculate_waypoint_path(nav_state);

        if (nav_state->waypoint_star->waypoint_path != NULL && nav_state->waypoint_star->waypoint_points > 0)
            gfx_draw_waypoint_path(game_state, nav_state, camera);
    }

    // Draw ship projection
    ship->projection->rect.x = (ship->position.x - nav_state->map_offset.x) * game_state->game_scale + (camera->w / 2 - ship->projection->radius);
    ship->projection->rect.y = (ship->position.y - nav_state->map_offset.y) * game_state->game_scale + (camera->h / 2 - ship->projection->radius);
    ship->projection->angle = ship->angle;

    bool position_in_buffer_galaxy = strcmp(nav_state->current_galaxy->name, nav_state->buffer_galaxy->name) == 0;

    if (!game_events->switch_to_universe && (!position_in_buffer_galaxy || (ship->projection->rect.x + ship->projection->radius < 0 ||
                                                                            ship->projection->rect.x + ship->projection->radius > camera->w ||
                                                                            ship->projection->rect.y + ship->projection->radius < 0 ||
                                                                            ship->projection->rect.y + ship->projection->radius > camera->h)))
    {
        gfx_project_ship_on_edge(game_state->state, input_state, nav_state, ship, camera, game_state->game_scale);
    }
    else
        SDL_RenderCopyEx(renderer, ship->projection->texture, &ship->projection->main_img_rect, &ship->projection->rect, ship->projection->angle, &ship->projection->rotation_pt, SDL_FLIP_NONE);

    // Draw galaxy cutoff circle
    int cutoff = nav_state->current_galaxy->cutoff * GALAXY_SCALE * game_state->game_scale;
    int cx = -camera->x * game_state->game_scale;
    int cy = -camera->y * game_state->game_scale;
    gfx_draw_circle_approximation(renderer, camera, cx, cy, cutoff, colors[COLOR_CYAN_70]);

    // Draw cross at position
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);
    SDL_RenderDrawLine(renderer, (camera->w / 2) - 7, camera->h / 2, (camera->w / 2) + 7, camera->h / 2);
    SDL_RenderDrawLine(renderer, camera->w / 2, (camera->h / 2) - 7, camera->w / 2, (camera->h / 2) + 7);

    if (nav_state->selected_star->initialized && nav_state->selected_star->is_selected)
    {
        // Draw star info box
        stars_draw_info_box(nav_state, nav_state->selected_star, camera);

        // Draw planets info box
        stars_draw_planets_info_box(input_state, nav_state, nav_state->selected_star, camera);
    }
    else if (nav_state->current_star->initialized && input_state->is_hovering_star &&
             gfx_is_object_in_camera(camera, nav_state->current_star->position.x, nav_state->current_star->position.y, nav_state->current_star->cutoff, game_state->game_scale))
    {
        // Draw star info box
        stars_draw_info_box(nav_state, nav_state->current_star, camera);
    }

    console_draw_position_console(game_state, nav_state, camera);
    gfx_draw_screen_frame(camera);

    if (game_events->switch_to_map)
        game_events->switch_to_map = false;

    if (game_events->is_entering_map)
        game_events->is_entering_map = false;
}

/**
 * Handles the navigation state of the game, including resetting game elements when exiting the
 * navigation state, zooming in and out, updating the camera position, generating stars, and drawing the galaxy cloud.
 *
 * @param game_state A pointer to the current GameState object.
 * @param input_state A pointer to the current InputState object.
 * @param game_events A pointer to the current GameEvents object.
 * @param nav_state A pointer to the current NavigationState object.
 * @param bstars A pointer to the Bstar structure that holds information about the binary stars.
 * @param ship A pointer to the current ship.
 * @param camera A pointer to the current Camera object.
 *
 * @return void
 */
void game_run_navigate_state(GameState *game_state, InputState *input_state, GameEvents *game_events, NavigationState *nav_state, Bstar *bstars, Ship *ship, Camera *camera)
{
    const double epsilon = ZOOM_EPSILON;

    if (game_events->is_centering_navigate)
    {
        game_state->game_scale = ZOOM_NAVIGATE + ZOOM_STEP;
        game_events->is_centering_navigate = false;
        input_state->zoom_out = true;
    }

    if (game_events->is_exiting_map || game_events->is_exiting_universe)
    {
        // Reset stars and galaxy to current position
        if (!maths_points_equal(nav_state->current_galaxy->position, nav_state->buffer_galaxy->position))
        {
            memcpy(nav_state->current_galaxy, nav_state->buffer_galaxy, sizeof(Galaxy));

            // Reset galaxy_offset
            nav_state->galaxy_offset.current_x = nav_state->galaxy_offset.buffer_x;
            nav_state->galaxy_offset.current_y = nav_state->galaxy_offset.buffer_y;
        }

        // Trigger generation of new stars
        game_events->start_stars_generation = true;

        // Reset ship position
        ship->position.x = ship->previous_position.x;
        ship->position.y = ship->previous_position.y;

        stars_clear_table(nav_state->stars, nav_state, false);

        // Reset saved game_scale
        if (game_state->save_scale)
            game_state->game_scale = game_state->save_scale;
        else
            game_state->game_scale = ZOOM_NAVIGATE;

        game_state->save_scale = false;

        if (input_state->camera_on)
            gfx_update_camera(camera, ship->position, game_state->game_scale);
    }

    // Select current star
    if (nav_state->current_star != NULL && nav_state->selected_star != NULL)
    {
        if (strcmp(nav_state->selected_star->name, nav_state->buffer_star->name) != 0)
            memcpy(nav_state->selected_star, nav_state->buffer_star, sizeof(Star));

        nav_state->selected_star->is_selected = true;
    }

    // Zoom in
    if (input_state->zoom_in)
    {
        if (game_state->game_scale + ZOOM_STEP <= ZOOM_MAX + epsilon)
        {
            // Reset scale
            game_state->game_scale += ZOOM_STEP;
        }

        input_state->zoom_in = false;
    }

    // Zoom out
    if (input_state->zoom_out)
    {
        if (game_state->game_scale - ZOOM_STEP >= ZOOM_NAVIGATE_MIN - epsilon)
        {
            // Reset scale
            game_state->game_scale -= ZOOM_STEP;
        }

        input_state->zoom_out = false;
    }

    if (input_state->camera_on)
        stars_generate(game_state, game_events, nav_state, bstars, ship);

    if (input_state->camera_on)
        gfx_update_camera(camera, nav_state->navigate_offset, game_state->game_scale);

    // Get distance from galaxy center
    double distance_galaxy_center = maths_distance_between_points(ship->position.x, ship->position.y, 0, 0);

    if (BSTARS_ON || GSTARS_ON || SPEED_LINES_ON)
    {
        Speed speed = {.vx = ship->vx, .vy = ship->vy};

        // Draw galaxy cloud
        if (GSTARS_ON)
        {
            Point ship_position_current = {.x = ship->position.x, .y = ship->position.y};
            static double limit_current;

            if (limit_current == 0.0)
                limit_current = 2 * nav_state->current_galaxy->radius * GALAXY_SCALE;

            if (game_events->found_galaxy)
            {
                limit_current = distance_galaxy_center;
                game_events->found_galaxy = false;
            }

            gfx_update_gstars_position(nav_state->current_galaxy, ship_position_current, camera, distance_galaxy_center, limit_current);

            if (game_events->has_exited_galaxy && nav_state->previous_galaxy != NULL && nav_state->previous_galaxy->initialized_hd)
            {
                // Convert ship position to universe position
                Point universe_position;
                universe_position.x = nav_state->current_galaxy->position.x + ship->position.x / GALAXY_SCALE;
                universe_position.y = nav_state->current_galaxy->position.y + ship->position.y / GALAXY_SCALE;

                // Convert universe position to ship position relative to previous galaxy
                Point ship_position_previous = {
                    .x = (universe_position.x - nav_state->previous_galaxy->position.x) * GALAXY_SCALE,
                    .y = (universe_position.y - nav_state->previous_galaxy->position.y) * GALAXY_SCALE};

                double distance_previous = maths_distance_between_points(universe_position.x, universe_position.y, nav_state->previous_galaxy->position.x, nav_state->previous_galaxy->position.y);
                distance_previous *= GALAXY_SCALE;
                double limit_previous = 2 * nav_state->previous_galaxy->radius * GALAXY_SCALE;

                gfx_update_gstars_position(nav_state->previous_galaxy, ship_position_previous, camera, distance_previous, limit_previous);
            }
        }

        // Draw background stars
        if (BSTARS_ON)
        {
            if (game_events->generate_bstars)
                gfx_generate_bstars(game_events, nav_state, bstars, camera, true);
            else
                gfx_update_bstars_position(game_state->state, input_state->camera_on, nav_state, bstars, camera, speed, distance_galaxy_center);
        }

        if (SPEED_LINES_ON && input_state->camera_on)
            gfx_draw_speed_lines(nav_state->velocity.magnitude, camera, speed);
    }

    if (nav_state->velocity.magnitude > GALAXY_SPEED_LIMIT)
        gfx_draw_speed_arc(ship, camera, game_state->game_scale);

    // Process star system
    if ((!game_events->is_exiting_map && !game_events->is_exiting_universe && !input_state->zoom_in && !input_state->zoom_out) || game_state->game_scale > ZOOM_NAVIGATE_MIN)
    {
        for (int i = 0; i < MAX_STARS; i++)
        {
            if (nav_state->stars[i] != NULL)
            {
                // Each index can have many entries, loop through all of them
                StarEntry *entry = nav_state->stars[i];

                while (entry != NULL)
                {
                    stars_update_orbital_positions(game_state, input_state, nav_state, entry->star, ship, camera, entry->star->class);
                    stars_draw_star_system(game_state, input_state, nav_state, entry->star, camera);
                    entry = entry->next;
                }
            }
        }
    }

    if (game_events->arrived_at_waypoint)
    {
        CelestialBody *body = NULL;

        // Star
        if (nav_state->waypoint_planet_index < 0)
        {
            body = nav_state->waypoint_star;
        }
        // Planet
        else if (nav_state->waypoint_planet_index >= 0)
        {
            body = nav_state->waypoint_star->planets[nav_state->waypoint_planet_index];
        }

        game_put_ship_in_orbit(body, ship, WAYPOINT_ORBIT_RADII);

        // Clean up waypoint
        free(nav_state->waypoint_star->waypoint_path);
        nav_state->waypoint_star->waypoint_path = NULL;
        nav_state->waypoint_star->waypoint_points = 0;
        nav_state->waypoint_planet_index = -1;
        stars_initialize_star(nav_state->waypoint_star);

        game_events->arrived_at_waypoint = false;
    }

    bool waypoint_path_exists = nav_state->waypoint_star->initialized &&
                                nav_state->waypoint_star->waypoint_points > 0 &&
                                !input_state->zoom_in && !input_state->zoom_out &&
                                strcmp(nav_state->current_galaxy->name, nav_state->waypoint_star->galaxy_name) == 0;

    if (waypoint_path_exists)
    {
        bool ship_in_waypoint_cutoff = maths_is_point_in_circle(ship->position,
                                                                nav_state->waypoint_star->position,
                                                                nav_state->waypoint_star->cutoff);

        if (ship_in_waypoint_cutoff)
            gfx_calculate_waypoint_path(nav_state);

        // Draw waypoint path
        gfx_draw_waypoint_path(game_state, nav_state, camera);

        double distance_ship_to_waypoint = maths_distance_between_points(ship->position.x, ship->position.y,
                                                                         nav_state->waypoint_star->waypoint_path[nav_state->waypoint_star->waypoint_points - 1].position.x,
                                                                         nav_state->waypoint_star->waypoint_path[nav_state->waypoint_star->waypoint_points - 1].position.y);

        // Autopilot
        if (input_state->autopilot_on)
            game_engage_autopilot(input_state, game_events, nav_state, ship, distance_ship_to_waypoint);
        else
            game_events->autopilot_rotated_ship = false;

        // Increment next point in path
        double distance_ship_to_next_point = maths_distance_between_points(nav_state->waypoint_star->waypoint_path[nav_state->next_path_point].position.x,
                                                                           nav_state->waypoint_star->waypoint_path[nav_state->next_path_point].position.y,
                                                                           ship->position.x,
                                                                           ship->position.y);

        if (distance_ship_to_next_point < WAYPOINT_CIRCLE_RADIUS)
            nav_state->next_path_point++;

        // Make sure that the closest point to ship is set as next_path_point
        if (distance_ship_to_waypoint > nav_state->buffer_star->cutoff)
        {
            double min_dist = INFINITY;
            int closest_point_index = -1;

            for (int i = nav_state->next_path_point; i < nav_state->waypoint_star->waypoint_points; i++)
            {
                double distance = sqrt(pow(nav_state->waypoint_star->waypoint_path[i].position.x - ship->position.x, 2) +
                                       pow(nav_state->waypoint_star->waypoint_path[i].position.y - ship->position.y, 2));

                if (distance < min_dist)
                {
                    min_dist = distance;
                    closest_point_index = i;
                }
            }

            if (closest_point_index > nav_state->next_path_point)
                nav_state->next_path_point = closest_point_index;
        }
    }
    else
    {
        if (input_state->autopilot_on)
            input_state->autopilot_on = false;
    }

    // Enforce speed limits
    if (distance_galaxy_center < nav_state->current_galaxy->radius * GALAXY_SCALE)
    {
        if (nav_state->velocity.magnitude >= GALAXY_SPEED_LIMIT)
        {
            ship->vx = GALAXY_SPEED_LIMIT * ship->vx / nav_state->velocity.magnitude;
            ship->vy = GALAXY_SPEED_LIMIT * ship->vy / nav_state->velocity.magnitude;
        }
    }
    else
    {
        if (nav_state->velocity.magnitude >= UNIVERSE_SPEED_LIMIT)
        {
            ship->vx = UNIVERSE_SPEED_LIMIT * ship->vx / nav_state->velocity.magnitude;
            ship->vy = UNIVERSE_SPEED_LIMIT * ship->vy / nav_state->velocity.magnitude;
        }
    }

    phys_update_velocity(&nav_state->velocity, ship);
    game_update_ship_position(game_state, input_state, ship, camera);

    // Update position
    nav_state->navigate_offset.x = ship->position.x;
    nav_state->navigate_offset.y = ship->position.y;

    game_draw_ship(game_state, input_state, nav_state, ship, camera);

    // Check for nearest galaxy, excluding current galaxy
    if (game_events->has_exited_galaxy && PROJECTIONS_ON)
    {
        // Convert offset to universe position
        Point universe_position;
        universe_position.x = nav_state->current_galaxy->position.x + ship->position.x / GALAXY_SCALE;
        universe_position.y = nav_state->current_galaxy->position.y + ship->position.y / GALAXY_SCALE;

        // Calculate camera position in universe scale
        Camera universe_camera = {
            .x = nav_state->current_galaxy->position.x * GALAXY_SCALE + camera->x,
            .y = nav_state->current_galaxy->position.y * GALAXY_SCALE + camera->y,
            .w = camera->w,
            .h = camera->h};

        Galaxy *nearest_galaxy = galaxies_nearest_circumference(nav_state, universe_position, true);

        if (nearest_galaxy != NULL && !maths_points_equal(nearest_galaxy->position, nav_state->current_galaxy->position))
        {
            // Project nearest galaxy
            gfx_project_galaxy_on_edge(MAP, nav_state, nearest_galaxy, &universe_camera, game_state->game_scale);
        }

        // Project current galaxy
        gfx_project_galaxy_on_edge(MAP, nav_state, nav_state->current_galaxy, &universe_camera, game_state->game_scale);
    }

    // Create galaxy cloud
    if (!nav_state->current_galaxy->initialized_hd || nav_state->current_galaxy->initialized_hd < nav_state->current_galaxy->total_groups_hd)
        gfx_generate_gstars(nav_state->current_galaxy, true);

    // Draw star console
    if (nav_state->current_star != NULL)
    {
        // Get distance from current_star
        double distance_star = maths_distance_between_points(nav_state->current_star->position.x, nav_state->current_star->position.y, nav_state->navigate_offset.x, nav_state->navigate_offset.y);

        if (nav_state->waypoint_star->initialized && nav_state->waypoint_star->waypoint_points > 0)
            console_draw_waypoint_console(nav_state, ship, camera);
        else if (distance_star < nav_state->current_star->cutoff)
            console_draw_star_console(nav_state->current_star, camera);
    }

    console_draw_ship_console(game_state, input_state, nav_state, ship, camera);
    gfx_draw_screen_frame(camera);

    if (game_events->is_exiting_map)
        game_events->is_exiting_map = false;

    if (game_events->is_exiting_universe)
        game_events->is_exiting_universe = false;
}

/**
 * Updates the state of the game universe based on player input and events. This function generates galaxies and
 * stars within the game universe, updates the position of the ship and camera, and triggers the generation of
 * a stars preview. It also handles resetting certain game elements when the player exits or enters the universe,
 * and adjusts the game scale based on the current galaxy class.
 *
 * @param game_state A pointer to the current GameState object.
 * @param input_state A pointer to the current InputState object.
 * @param game_events A pointer to the current GameEvents object.
 * @param nav_state A pointer to the current NavigationState object.
 * @param ship A pointer to the Ship struct representing the player's ship.
 * @param camera A pointer to the current Camera object.
 *
 * @return void
 */
void game_run_universe_state(GameState *game_state, InputState *input_state, GameEvents *game_events, NavigationState *nav_state, Ship *ship, Camera *camera)
{
    const double epsilon = ZOOM_EPSILON / GALAXY_SCALE;

    if (game_events->is_exiting_map || game_events->switch_to_universe)
    {
        // Reset stars
        if (!maths_points_equal(nav_state->current_galaxy->position, nav_state->buffer_galaxy->position))
            stars_clear_table(nav_state->stars, nav_state, false);

        // Reset galaxy_offset
        nav_state->galaxy_offset.current_x = nav_state->galaxy_offset.buffer_x;
        nav_state->galaxy_offset.current_y = nav_state->galaxy_offset.buffer_y;

        // Reset ship position
        ship->position.x = ship->previous_position.x;
        ship->position.y = ship->previous_position.y;

        // Delete stars that end up outside the region
        double bx = maths_get_nearest_section_line(ship->position.x, GALAXY_SECTION_SIZE);
        double by = maths_get_nearest_section_line(ship->position.y, GALAXY_SECTION_SIZE);

        stars_delete_outside_region(nav_state->stars, nav_state, bx, by, GALAXY_REGION_SIZE);
    }

    if (game_events->is_entering_universe || game_events->is_centering_universe)
    {
        // Save ship position
        ship->previous_position.x = ship->position.x;
        ship->previous_position.y = ship->position.y;

        // Reset current_galaxy
        if (!game_events->switch_to_universe)
        {
            if (strcmp(nav_state->current_galaxy->name, nav_state->buffer_galaxy->name) != 0)
                memcpy(nav_state->current_galaxy, nav_state->buffer_galaxy, sizeof(Galaxy));
        }

        if (game_events->is_centering_universe)
            stars_clear_table(nav_state->stars, nav_state, false);

        nav_state->current_galaxy->is_selected = true;

        // Initialize cross lines for stars preview
        // Add GALAXY_SECTION_SIZE to map_offset so that stars generation is triggered on startup
        if (nav_state->cross_line.x == 0.0)
            nav_state->cross_line.x = maths_get_nearest_section_line(nav_state->map_offset.x + GALAXY_SECTION_SIZE, GALAXY_SECTION_SIZE);

        if (nav_state->cross_line.y == 0.0)
            nav_state->cross_line.y = maths_get_nearest_section_line(nav_state->map_offset.y + GALAXY_SECTION_SIZE, GALAXY_SECTION_SIZE);

        // Generate galaxies
        Point offset = {.x = nav_state->galaxy_offset.current_x, .y = nav_state->galaxy_offset.current_y};
        galaxies_generate(game_events, nav_state, offset);

        if (game_events->is_entering_universe && !game_state->save_scale)
            game_state->save_scale = game_state->game_scale;

        if (!game_events->switch_to_universe)
        {
            double zoom_universe;

            switch (nav_state->buffer_galaxy->class)
            {
            case 1:
                zoom_universe = ZOOM_UNIVERSE * 10;
                break;
            case 2:
                zoom_universe = ZOOM_UNIVERSE * 5;
                break;
            case 3:
                zoom_universe = ZOOM_UNIVERSE * 3;
                break;
            case 4:
                zoom_universe = ZOOM_UNIVERSE * 2;
                break;
            default:
                zoom_universe = ZOOM_UNIVERSE;
                break;
            }

            game_state->game_scale = zoom_universe / GALAXY_SCALE;

            // Reset universe_offset
            nav_state->universe_offset.x = nav_state->galaxy_offset.current_x + ship->position.x / GALAXY_SCALE;
            nav_state->universe_offset.y = nav_state->galaxy_offset.current_y + ship->position.y / GALAXY_SCALE;
        }

        gfx_update_camera(camera, nav_state->universe_offset, game_state->game_scale * GALAXY_SCALE);

        // Trigger generation of stars preview
        game_events->start_stars_preview = true;
    }
    else
        galaxies_generate(game_events, nav_state, nav_state->universe_offset);

    gfx_draw_section_lines(camera, game_state->state, colors[COLOR_ORANGE_32], game_state->game_scale * GALAXY_SCALE);

    // Generate stars preview
    long double zoom_generate_preview_stars = game_zoom_generate_preview_stars(nav_state->current_galaxy->class);

    if (game_state->game_scale >= zoom_generate_preview_stars - epsilon)
    {
        if (game_events->start_stars_preview || game_events->lazy_load_started)
        {
            // Update map_offset
            nav_state->map_offset.x = (nav_state->universe_offset.x - nav_state->current_galaxy->position.x) * GALAXY_SCALE;
            nav_state->map_offset.y = (nav_state->universe_offset.y - nav_state->current_galaxy->position.y) * GALAXY_SCALE;

            stars_generate_preview(game_events, nav_state, camera, game_state->game_scale);
            game_events->start_stars_preview = false;
            game_events->zoom_preview = false;
        }
    }

    // Draw galaxies
    if (!game_events->is_entering_universe && !input_state->is_mouse_double_clicked)
    {
        for (int i = 0; i < MAX_GALAXIES; i++)
        {
            if (nav_state->galaxies[i] != NULL)
            {
                // Each index can have many entries, loop through all of them
                GalaxyEntry *entry = nav_state->galaxies[i];

                while (entry != NULL)
                {
                    galaxies_draw_galaxy(input_state, nav_state, entry->galaxy, camera, game_state->state, game_state->game_scale);
                    entry = entry->next;
                }
            }
        }
    }

    // Check if mouse is over current galaxy
    gfx_toggle_galaxy_hover(input_state, nav_state, camera, game_state->game_scale);

    game_scroll_universe(game_state, input_state, game_events, nav_state, camera);
    game_zoom_universe(game_state, input_state, game_events, nav_state);
    gfx_update_camera(camera, nav_state->universe_offset, game_state->game_scale * GALAXY_SCALE);

    // Draw cross at position
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);
    SDL_RenderDrawLine(renderer, (camera->w / 2) - 7, camera->h / 2, (camera->w / 2) + 7, camera->h / 2);
    SDL_RenderDrawLine(renderer, camera->w / 2, (camera->h / 2) - 7, camera->w / 2, (camera->h / 2) + 7);

    // Draw stars and star systems
    if (game_state->game_scale >= zoom_generate_preview_stars - epsilon)
    {
        for (int i = 0; i < MAX_STARS; i++)
        {
            if (nav_state->stars[i] != NULL)
            {
                // Each index can have many entries, loop through all of them
                StarEntry *entry = nav_state->stars[i];

                while (entry != NULL)
                {
                    // Skip buffer_star if in another galaxy
                    if (strcmp(nav_state->current_galaxy->name, entry->star->galaxy_name) != 0)
                    {
                        entry = entry->next;
                        continue;
                    }

                    // Calculate opacity
                    float opacity = entry->star->class * (255 / 6);

                    if (opacity < 120)
                        opacity = 120.0f;

                    double zoom_threshold;

                    switch (entry->star->class)
                    {
                    case 1:
                        zoom_threshold = ZOOM_UNIVERSE_STAR_SYSTEMS;
                        break;
                    case 2:
                        zoom_threshold = ZOOM_UNIVERSE_STAR_SYSTEMS - 0.0002;
                        break;
                    case 3:
                    case 4:
                        zoom_threshold = ZOOM_UNIVERSE_STAR_SYSTEMS - 0.0003;
                        break;
                    case 5:
                    case 6:
                        zoom_threshold = ZOOM_UNIVERSE_STAR_SYSTEMS - 0.0004;
                        break;
                    default:
                        zoom_threshold = ZOOM_UNIVERSE_STAR_SYSTEMS;
                        break;
                    }

                    opacity = (((game_state->game_scale - zoom_generate_preview_stars) / (zoom_threshold - zoom_generate_preview_stars)) * (255.0f - opacity) + opacity);

                    if (game_state->game_scale >= zoom_threshold)
                        opacity = 255.0f;

                    // Check if mouse is inside star cutoff
                    int x = (nav_state->current_galaxy->position.x - camera->x + entry->star->position.x / GALAXY_SCALE) * game_state->game_scale * GALAXY_SCALE;
                    int y = (nav_state->current_galaxy->position.y - camera->y + entry->star->position.y / GALAXY_SCALE) * game_state->game_scale * GALAXY_SCALE;
                    double distance_star = maths_distance_between_points(input_state->mouse_position.x, input_state->mouse_position.y, x, y);
                    double star_cutoff = entry->star->cutoff * game_state->game_scale;

                    // Check if mouse is over star info box
                    input_state->is_hovering_star_info = gfx_toggle_star_info_hover(input_state, nav_state, camera);

                    bool star_is_selected = strcmp(nav_state->selected_star->name, entry->star->name) == 0 && nav_state->selected_star->is_selected;

                    if (star_is_selected || (!input_state->is_hovering_star_info && distance_star <= star_cutoff))
                    {
                        // Use a local rng
                        pcg32_random_t rng;

                        // Create rng seed by combining x,y values
                        uint64_t seed = maths_hash_position_to_uint64(entry->star->position);

                        // Seed with a fixed constant
                        pcg32_srandom_r(&rng, seed, seed);

                        // Draw star cutoff circle
                        if ((strcmp(nav_state->selected_star->name, entry->star->name) != 0 || !nav_state->selected_star->is_selected) &&
                            strcmp(nav_state->waypoint_star->name, entry->star->name) != 0)
                            gfx_draw_circle(renderer, camera, x, y, star_cutoff, colors[COLOR_MAGENTA_120]);

                        stars_populate_body(entry->star, entry->star->position, rng, game_state->game_scale);

                        // Draw star systems
                        if (game_state->game_scale >= zoom_threshold - epsilon)
                        {
                            stars_draw_universe_star_system(game_state, input_state, nav_state, entry->star, camera);
                        }
                        else
                        {
                            SDL_SetRenderDrawColor(renderer, entry->star->color.r, entry->star->color.g, entry->star->color.b, (int)opacity);
                            SDL_RenderDrawPoint(renderer, x, y);
                        }

                        // Set star as current_star
                        if (nav_state->current_star != NULL && distance_star <= star_cutoff)
                        {
                            if (strcmp(nav_state->current_star->name, entry->star->name) != 0)
                                memcpy(nav_state->current_star, entry->star, sizeof(Star));
                        }
                    }
                    else
                    {
                        // Draw points
                        SDL_SetRenderDrawColor(renderer, entry->star->color.r, entry->star->color.g, entry->star->color.b, (int)opacity);
                        SDL_RenderDrawPoint(renderer, x, y);
                    }

                    entry = entry->next;
                }
            }
        }
    }

    // Calculate waypoint path
    if (nav_state->waypoint_star->initialized &&
        game_state->game_scale >= zoom_generate_preview_stars - epsilon &&
        !game_events->start_stars_preview && !game_events->lazy_load_started &&
        !input_state->zoom_in && !input_state->zoom_out &&
        strcmp(nav_state->current_galaxy->name, nav_state->waypoint_star->galaxy_name) == 0)
    {
        gfx_calculate_waypoint_path(nav_state);

        if (nav_state->waypoint_star->waypoint_points > 0)
            gfx_draw_waypoint_path(game_state, nav_state, camera);
    }

    // Draw ship projection
    if (!game_events->switch_to_map)
    {
        ship->projection->rect.x = (nav_state->galaxy_offset.current_x + ship->position.x / GALAXY_SCALE - camera->x) * (game_state->game_scale * GALAXY_SCALE) - SHIP_PROJECTION_RADIUS;
        ship->projection->rect.y = (nav_state->galaxy_offset.current_y + ship->position.y / GALAXY_SCALE - camera->y) * (game_state->game_scale * GALAXY_SCALE) - SHIP_PROJECTION_RADIUS;
        ship->projection->angle = ship->angle;

        if ((ship->projection->rect.x + ship->projection->radius < 0 ||
             ship->projection->rect.x + ship->projection->radius > camera->w ||
             ship->projection->rect.y + ship->projection->radius < 0 ||
             ship->projection->rect.y + ship->projection->radius > camera->h))
        {
            gfx_project_ship_on_edge(game_state->state, input_state, nav_state, ship, camera, game_state->game_scale);
        }
        else
            SDL_RenderCopyEx(renderer, ship->projection->texture, &ship->projection->main_img_rect, &ship->projection->rect, ship->projection->angle, &ship->projection->rotation_pt, SDL_FLIP_NONE);
    }

    // Toggle star hover / Draw star info box
    if (game_state->game_scale >= zoom_generate_preview_stars - epsilon)
    {
        for (int i = 0; i < MAX_STARS; i++)
        {
            if (nav_state->stars[i] != NULL)
            {
                // Each index can have many entries, loop through all of them
                StarEntry *entry = nav_state->stars[i];

                while (entry != NULL)
                {
                    // Skip buffer_star if in another galaxy
                    if (strcmp(nav_state->current_galaxy->name, entry->star->galaxy_name) != 0)
                    {
                        entry = entry->next;
                        continue;
                    }

                    int x = (nav_state->current_galaxy->position.x - camera->x + entry->star->position.x / GALAXY_SCALE) * game_state->game_scale * GALAXY_SCALE;
                    int y = (nav_state->current_galaxy->position.y - camera->y + entry->star->position.y / GALAXY_SCALE) * game_state->game_scale * GALAXY_SCALE;

                    // Check if mouse is inside star cutoff
                    double distance_star = maths_distance_between_points(input_state->mouse_position.x, input_state->mouse_position.y, x, y);
                    double star_cutoff = entry->star->cutoff * game_state->game_scale;

                    // Check if mouse is over star info box
                    input_state->is_hovering_star_info = gfx_toggle_star_info_hover(input_state, nav_state, camera);

                    if (!input_state->is_hovering_star_info && distance_star <= star_cutoff)
                    {
                        // Draw star info box
                        if (!nav_state->selected_star->is_selected)
                            stars_draw_info_box(nav_state, entry->star, camera);
                    }

                    entry = entry->next;
                }
            }
        }

        // Check if mouse is over current star (enables click on hovered star)
        if (!input_state->is_hovering_star_info)
            gfx_toggle_star_hover(input_state, nav_state, camera, game_state->game_scale, UNIVERSE);
    }
    else
        input_state->is_hovering_star = false;

    // Waypoint star
    if (nav_state->waypoint_star->initialized && !game_events->switch_to_map &&
        game_state->game_scale > zoom_generate_preview_stars - epsilon &&
        !input_state->zoom_in && !input_state->zoom_out)
    {
        // Don't show waypoint_star if in another galaxy
        if (strcmp(nav_state->current_galaxy->name, nav_state->waypoint_star->galaxy_name) == 0)
        {
            // Draw waypoint circle
            int waypoint_x = (nav_state->current_galaxy->position.x - camera->x + nav_state->waypoint_star->position.x / GALAXY_SCALE) * game_state->game_scale * GALAXY_SCALE;
            int waypoint_y = (nav_state->current_galaxy->position.y - camera->y + nav_state->waypoint_star->position.y / GALAXY_SCALE) * game_state->game_scale * GALAXY_SCALE;
            double star_waypoint_radius = nav_state->waypoint_star->cutoff * game_state->game_scale;
            SDL_Color waypoint_circle_color = nav_state->waypoint_star->color;
            waypoint_circle_color.a = 150;
            gfx_draw_circle(renderer, camera, waypoint_x, waypoint_y, star_waypoint_radius, waypoint_circle_color);
        }
    }

    // Selected star
    if (game_state->game_scale > zoom_generate_preview_stars - epsilon &&
        nav_state->selected_star->is_selected &&
        !input_state->zoom_in && !input_state->zoom_out)
    {
        // Don't show selected_star if in another galaxy
        // Don't show selected_star if is waypoint_star
        if (strcmp(nav_state->current_galaxy->name, nav_state->selected_star->galaxy_name) == 0)
        {
            // Draw star cutoff circle
            if (strcmp(nav_state->selected_star->name, nav_state->waypoint_star->name) != 0)
            {
                int x = (nav_state->current_galaxy->position.x - camera->x + nav_state->selected_star->position.x / GALAXY_SCALE) * game_state->game_scale * GALAXY_SCALE;
                int y = (nav_state->current_galaxy->position.y - camera->y + nav_state->selected_star->position.y / GALAXY_SCALE) * game_state->game_scale * GALAXY_SCALE;
                double star_cutoff = nav_state->selected_star->cutoff * game_state->game_scale;
                gfx_draw_circle(renderer, camera, x, y, star_cutoff, colors[COLOR_CYAN_100]);
            }

            stars_draw_info_box(nav_state, nav_state->selected_star, camera);
            stars_draw_planets_info_box(input_state, nav_state, nav_state->selected_star, camera);
        }
    }

    // Define zoom threshold (for displaying galaxy info box)
    double zoom_threshold;

    switch (nav_state->current_galaxy->class)
    {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        zoom_threshold = zoom_generate_preview_stars - 0.00001;
        break;
    default:
        zoom_threshold = zoom_generate_preview_stars - 0.000001;
        break;
    }

    // Draw galaxy info box
    if (nav_state->current_galaxy->is_selected)
    {
        if (!nav_state->current_galaxy->initialized || nav_state->current_galaxy->initialized < nav_state->current_galaxy->total_groups)
            gfx_generate_gstars(nav_state->current_galaxy, false);

        if (game_state->game_scale <= zoom_threshold + epsilon)
            galaxies_draw_info_box(nav_state->current_galaxy, camera);
    }

    console_draw_position_console(game_state, nav_state, camera);
    gfx_draw_screen_frame(camera);

    if (game_events->is_exiting_map)
        game_events->is_exiting_map = false;

    if (game_events->switch_to_universe)
        game_events->switch_to_universe = false;

    if (game_events->is_entering_universe)
        game_events->is_entering_universe = false;

    if (game_events->is_centering_universe)
        game_events->is_centering_universe = false;

    if (input_state->is_mouse_double_clicked)
        input_state->is_mouse_double_clicked = false;
}

/**
 * Moves the map by a given speed based on the input state and camera parameters.
 *
 * @param game_state A pointer to the current GameState object.
 * @param input_state A pointer to the current InputState object.
 * @param nav_state A pointer to the current NavigationState object.
 * @param camera A pointer to the current Camera object.
 *
 * @return void
 */
static void game_scroll_map(const GameState *game_state, const InputState *input_state, NavigationState *nav_state, const Camera *camera)
{
    double rate_x = 0, rate_y = 0;

    if (input_state->right_on)
        rate_x = MAP_SPEED_MIN + (MAP_SPEED_MAX - MAP_SPEED_MIN) * (camera->w / 1000) / game_state->game_scale;
    else if (input_state->left_on)
        rate_x = -(MAP_SPEED_MIN + (MAP_SPEED_MAX - MAP_SPEED_MIN) * (camera->w / 1000) / game_state->game_scale);

    nav_state->map_offset.x += rate_x;

    if (input_state->down_on)
        rate_y = MAP_SPEED_MIN + (MAP_SPEED_MAX - MAP_SPEED_MIN) * (camera->w / 1000) / game_state->game_scale;
    else if (input_state->up_on)
        rate_y = -(MAP_SPEED_MIN + (MAP_SPEED_MAX - MAP_SPEED_MIN) * (camera->w / 1000) / game_state->game_scale);

    nav_state->map_offset.y += rate_y;
}

/**
 * Moves the universe by a given speed based on the input state and camera parameters.
 *
 * @param game_state A pointer to the current GameState object.
 * @param input_state A pointer to the current InputState object.
 * @param game_events A pointer to the current GameEvents object.
 * @param nav_state A pointer to the current NavigationState object.
 * @param camera A pointer to the current Camera object.
 *
 * @return void
 */
static void game_scroll_universe(const GameState *game_state, const InputState *input_state, GameEvents *game_events, NavigationState *nav_state, const Camera *camera)
{
    const double epsilon = ZOOM_EPSILON / GALAXY_SCALE;
    double rate_x = 0, rate_y = 0;
    double speed_universe_step = 700;

    if (game_state->game_scale >= 0.004 - epsilon)
        speed_universe_step = 10000;
    else if (game_state->game_scale >= 0.003 - epsilon)
        speed_universe_step = 3000;
    else if (game_state->game_scale >= 0.002 - epsilon)
        speed_universe_step = 1000;
    else if (game_state->game_scale >= 0.001 - epsilon)
        speed_universe_step = 800;
    else if (game_state->game_scale >= 0.00001 - epsilon)
        speed_universe_step = 700;

    if (input_state->right_on || input_state->left_on || input_state->down_on || input_state->up_on)
        game_events->start_stars_preview = true;

    if (input_state->right_on)
        rate_x = UNIVERSE_SPEED_MIN + (UNIVERSE_SPEED_MAX - UNIVERSE_SPEED_MIN) * (camera->w / 1000) / (game_state->game_scale * speed_universe_step);
    else if (input_state->left_on)
        rate_x = -(UNIVERSE_SPEED_MIN + (UNIVERSE_SPEED_MAX - UNIVERSE_SPEED_MIN) * (camera->w / 1000) / (game_state->game_scale * speed_universe_step));

    rate_x /= (double)GALAXY_SCALE / 1000;
    nav_state->universe_offset.x += rate_x;

    if (input_state->down_on)
        rate_y = UNIVERSE_SPEED_MIN + (UNIVERSE_SPEED_MAX - UNIVERSE_SPEED_MIN) * (camera->w / 1000) / (game_state->game_scale * speed_universe_step);
    else if (input_state->up_on)
        rate_y = -(UNIVERSE_SPEED_MIN + (UNIVERSE_SPEED_MAX - UNIVERSE_SPEED_MIN) * (camera->w / 1000) / (game_state->game_scale * speed_universe_step));

    rate_y /= (double)GALAXY_SCALE / 1000;
    nav_state->universe_offset.y += rate_y;

    // Wrap around position (rectangle defines boundaries)
    if (nav_state->universe_offset.x > UNIVERSE_X_LIMIT)
        nav_state->universe_offset.x -= UNIVERSE_X_LIMIT * 2;
    else if (nav_state->universe_offset.x < -UNIVERSE_X_LIMIT)
        nav_state->universe_offset.x += UNIVERSE_X_LIMIT * 2;

    if (nav_state->universe_offset.y > UNIVERSE_Y_LIMIT)
        nav_state->universe_offset.y -= UNIVERSE_Y_LIMIT * 2;
    else if (nav_state->universe_offset.y < -UNIVERSE_Y_LIMIT)
        nav_state->universe_offset.y += UNIVERSE_Y_LIMIT * 2;
}

/**
 * Updates the position of a ship based on input state and camera position.
 *
 * @param game_state A pointer to the current GameState object.
 * @param input_state A pointer to the current InputState object.
 * @param ship A pointer to the ship to update.
 * @param camera A pointer to the current Camera object.
 *
 * @return void
 */
static void game_update_ship_position(GameState *game_state, const InputState *input_state, Ship *ship, const Camera *camera)
{
    float radians;

    // Update ship angle
    if (input_state->right_on && !input_state->left_on && game_state->landing_stage == STAGE_OFF)
        ship->angle += 3;

    if (input_state->left_on && !input_state->right_on && game_state->landing_stage == STAGE_OFF)
        ship->angle -= 3;

    if (ship->angle > 360)
        ship->angle -= 360;
    else if (ship->angle < 0)
        ship->angle += 360;

    // Apply thrust
    if (input_state->thrust_on)
    {
        game_state->landing_stage = STAGE_OFF;
        radians = ship->angle * M_PI / 180;

        ship->vx += G_THRUST * sin(radians);
        ship->vy -= G_THRUST * cos(radians);
    }

    // Apply reverse
    if (input_state->reverse_on)
    {
        radians = ship->angle * M_PI / 180;

        ship->vx -= G_THRUST * sin(radians);
        ship->vy += G_THRUST * cos(radians);
    }

    // Stop ship
    if (input_state->stop_on)
    {
        ship->vx = 0;
        ship->vy = 0;
    }

    // Update ship position
    ship->position.x += (float)ship->vx / FPS;
    ship->position.y += (float)ship->vy / FPS;

    if (input_state->camera_on)
    {
        // Static rect position at center of screen fixes flickering caused by float-to-int inaccuracies
        ship->rect.x = (camera->w / 2) - ship->radius;
        ship->rect.y = (camera->h / 2) - ship->radius;
    }
    else
    {
        // Dynamic rect position based on ship position
        ship->rect.x = (int)((ship->position.x - camera->x) * game_state->game_scale - ship->radius);
        ship->rect.y = (int)((ship->position.y - camera->y) * game_state->game_scale - ship->radius);
    }
}

/**
 * Calculates the game scale above which stars are generated for the given galaxy class.
 *
 * @param galaxy_class The class of the galaxy for which the preview stars are to be generated.
 *
 * @return Returns the game scale as a long double.
 */
long double game_zoom_generate_preview_stars(unsigned short galaxy_class)
{
    long double zoom_generate_preview_stars;

    switch (galaxy_class)
    {
    case 1:
        zoom_generate_preview_stars = ZOOM_STAR_1_PREVIEW_STARS;
        break;
    case 2:
        zoom_generate_preview_stars = ZOOM_STAR_2_PREVIEW_STARS;
        break;
    case 3:
        zoom_generate_preview_stars = ZOOM_STAR_3_PREVIEW_STARS;
        break;
    case 4:
        zoom_generate_preview_stars = ZOOM_STAR_4_PREVIEW_STARS;
        break;
    case 5:
        zoom_generate_preview_stars = ZOOM_STAR_5_PREVIEW_STARS;
        break;
    case 6:
        zoom_generate_preview_stars = ZOOM_STAR_6_PREVIEW_STARS;
        break;
    default:
        zoom_generate_preview_stars = ZOOM_STAR_1_PREVIEW_STARS;
        break;
    }

    return zoom_generate_preview_stars;
}

/**
 * Zoom in/out the map, resets the region size, and zooms in/out on each star system.
 *
 * @param game_state A pointer to the current GameState object.
 * @param input_state A pointer to the current InputState object.
 * @param game_events A pointer to the current GameEvents object.
 * @param nav_state A pointer to the current NavigationState object.
 *
 * @return void
 */
static void game_zoom_map(GameState *game_state, InputState *input_state, GameEvents *game_events, NavigationState *nav_state)
{
    const double epsilon = ZOOM_EPSILON;
    double zoom_step = ZOOM_STEP;

    // Zoom in
    if (input_state->zoom_in)
    {
        if (game_state->game_scale <= ZOOM_MAP_STEP_SWITCH - epsilon)
            zoom_step /= 10;

        if (game_state->game_scale + zoom_step <= ZOOM_MAX + epsilon)
        {
            // Reset scale
            if (game_state->game_scale_override)
                game_state->game_scale = game_state->game_scale_override;
            else
                game_state->game_scale += zoom_step;

            // Delete stars that end up outside the region
            if (game_state->game_scale <= ZOOM_MAP_STEP_SWITCH + zoom_step + epsilon)
            {
                double bx = maths_get_nearest_section_line(nav_state->map_offset.x, GALAXY_SECTION_SIZE);
                double by = maths_get_nearest_section_line(nav_state->map_offset.y, GALAXY_SECTION_SIZE);

                stars_delete_outside_region(nav_state->stars, nav_state, bx, by, GALAXY_REGION_SIZE);
            }
        }
        else if (game_state->game_scale_override)
            game_state->game_scale = game_state->game_scale_override;

        input_state->zoom_in = false;
        game_state->game_scale_override = 0;
    }

    // Zoom out
    if (input_state->zoom_out)
    {
        if (game_state->game_scale <= ZOOM_MAP_STEP_SWITCH + epsilon)
            zoom_step = ZOOM_STEP / 10;

        if (game_state->game_scale - zoom_step >= ZOOM_MAP_MIN - epsilon)
        {
            // Reset scale
            if (game_state->game_scale_override)
                game_state->game_scale = game_state->game_scale_override;
            else
                game_state->game_scale -= zoom_step;

            // Switch to Universe mode
            if (game_state->game_scale <= ZOOM_MAP_SWITCH - epsilon)
            {
                game_events->switch_to_universe = true;
                input_state->click_count = 0;
                game_events->is_entering_universe = true;
                game_change_state(game_state, game_events, UNIVERSE);

                // Update universe_offset
                nav_state->universe_offset.x = nav_state->current_galaxy->position.x + nav_state->map_offset.x / GALAXY_SCALE;
                nav_state->universe_offset.y = nav_state->current_galaxy->position.y + nav_state->map_offset.y / GALAXY_SCALE;
            }
        }
        else if (game_state->game_scale_override)
            game_state->game_scale = game_state->game_scale_override;

        input_state->zoom_out = false;
        game_state->game_scale_override = 0;
    }
}

/**
 * Zoom in/out the universe, resets the region size, and zooms in/out on each star system.
 *
 * @param game_state A pointer to the current GameState object.
 * @param input_state A pointer to the current InputState object.
 * @param game_events A pointer to the current GameEvents object.
 * @param nav_state A pointer to the current NavigationState object.
 *
 * @return void
 */
static void game_zoom_universe(GameState *game_state, InputState *input_state, GameEvents *game_events, NavigationState *nav_state)
{
    const double epsilon = ZOOM_EPSILON / GALAXY_SCALE;
    double zoom_universe_step = ZOOM_UNIVERSE_STEP;

    // Zoom in
    if (input_state->zoom_in)
    {
        if (game_state->game_scale >= 0.001 - epsilon)
            zoom_universe_step = ZOOM_UNIVERSE_STEP;
        else if (game_state->game_scale >= 0.0001 - epsilon)
            zoom_universe_step = ZOOM_UNIVERSE_STEP / 10;
        else if (game_state->game_scale >= 0.00001 - epsilon)
            zoom_universe_step = ZOOM_UNIVERSE_STEP / 100;
        else if (game_state->game_scale > 0)
            zoom_universe_step = ZOOM_UNIVERSE_STEP / 1000;

        if (game_state->game_scale + zoom_universe_step <= ZOOM_MAP_SWITCH + epsilon)
        {
            // Reset scale
            if (game_state->game_scale_override)
                game_state->game_scale = game_state->game_scale_override;
            else
                game_state->game_scale += zoom_universe_step;

            // Switch to Map mode
            if (game_state->game_scale >= ZOOM_MAP_SWITCH - epsilon)
            {
                game_events->switch_to_map = true;
                input_state->click_count = 0;
                game_events->is_entering_map = true;
                game_change_state(game_state, game_events, MAP);

                stars_clear_table(nav_state->stars, nav_state, false);

                // Update map_offset
                nav_state->map_offset.x = (nav_state->universe_offset.x - nav_state->current_galaxy->position.x) * GALAXY_SCALE;
                nav_state->map_offset.y = (nav_state->universe_offset.y - nav_state->current_galaxy->position.y) * GALAXY_SCALE;
            }
        }
        else if (game_state->game_scale_override)
            game_state->game_scale = game_state->game_scale_override;

        game_events->zoom_preview = true;
        input_state->zoom_in = false;
        game_state->game_scale_override = 0;
        game_events->start_stars_preview = true;
        nav_state->cross_line.x += GALAXY_SCALE; // fake increment so that it triggers star preview generation
        nav_state->cross_line.y += GALAXY_SCALE;
    }

    // Zoom out
    if (input_state->zoom_out)
    {
        if (game_state->game_scale <= 0.00001 + epsilon)
            zoom_universe_step = ZOOM_UNIVERSE_STEP / 1000;
        else if (game_state->game_scale <= 0.0001 + epsilon)
            zoom_universe_step = ZOOM_UNIVERSE_STEP / 100;
        else if (game_state->game_scale <= 0.001 + epsilon)
            zoom_universe_step = ZOOM_UNIVERSE_STEP / 10;

        if (game_state->game_scale - zoom_universe_step >= ZOOM_UNIVERSE_MIN / GALAXY_SCALE - epsilon)
        {
            // Reset scale
            if (game_state->game_scale_override)
                game_state->game_scale = game_state->game_scale_override;
            else
                game_state->game_scale -= zoom_universe_step;
        }
        else if (game_state->game_scale_override)
            game_state->game_scale = game_state->game_scale_override;

        stars_clear_table(nav_state->stars, nav_state, false);

        game_events->zoom_preview = true;
        input_state->zoom_out = false;
        game_state->game_scale_override = 0;
        game_events->start_stars_preview = true;
        nav_state->cross_line.x += GALAXY_SCALE; // fake increment so that it triggers star preview generation
        nav_state->cross_line.y += GALAXY_SCALE;
    }
}
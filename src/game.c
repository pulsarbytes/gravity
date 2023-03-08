/*
 * game.c
 */

#include <stdlib.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

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
static void game_update_ship_position(GameState *, const InputState *, Ship *, const Camera *);

/**
 * Changes the state of the game to a new state and updates relevant game events.
 *
 * @param game_state A pointer to the current game state.
 * @param game_events A pointer to the current game events.
 * @param new_state An integer representing the new state to transition to.
 *
 * @return void
 */
void game_change_state(GameState *game_state, GameEvents *game_events, int new_state)
{
    game_state->state = new_state;

    if (game_state->state == NAVIGATE)
        game_events->game_started = ON;

    if (game_events->game_started)
        menu_update_menu_entries(game_state);
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
 * @param game_state A pointer to the current game state.
 * @param input_state A pointer to the current input state.
 * @param nav_state A pointer to the current navigation state.
 * @param ship A pointer to the ship to be drawn.
 * @param camera A pointer to the current camera state.
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
    if (input_state->thrust)
        SDL_RenderCopyEx(renderer, ship->texture, &ship->thrust_img_rect, &ship->rect, ship->angle, &ship->rotation_pt, SDL_FLIP_NONE);

    // Draw reverse thrust
    if (input_state->reverse)
        SDL_RenderCopyEx(renderer, ship->texture, &ship->reverse_img_rect, &ship->rect, ship->angle, &ship->rotation_pt, SDL_FLIP_NONE);
}

/**
 * Resets the game state to the initial state.
 *
 * @param game_state A pointer to the game state.
 * @param input_state A pointer to the input state.
 * @param game_events A pointer to the game events.
 * @param nav_state A pointer to the navigation state.
 * @param bstars A pointer to the bstars.
 * @param ship A pointer to the ship.
 * @param camera A pointer to the camera.
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
    game_state->speed_limit = BASE_SPEED_LIMIT;
    game_state->landing_stage = STAGE_OFF;
    game_state->game_scale = ZOOM_NAVIGATE;
    game_state->save_scale = OFF;
    game_state->galaxy_region_size = GALAXY_REGION_SIZE;

    // InputState
    input_state->mouse_position.x = 0;
    input_state->mouse_position.y = 0;
    input_state->mouse_down_position.x = 0;
    input_state->mouse_down_position.y = 0;
    input_state->mouse_drag = OFF;
    input_state->left = OFF;
    input_state->right = OFF;
    input_state->up = OFF;
    input_state->down = OFF;
    input_state->thrust = OFF;
    input_state->reverse = OFF;
    input_state->camera_on = CAMERA_ON;
    input_state->stop = OFF;
    input_state->zoom_in = OFF;
    input_state->zoom_out = OFF;
    input_state->console = ON;
    input_state->orbits_on = SHOW_ORBITS;
    input_state->selected_button = 0;
    input_state->galaxy_hover = OFF;

    // GameEvents
    if (reset)
        game_events->game_started = ON;
    else
        game_events->game_started = OFF;

    game_events->stars_start = ON;
    game_events->stars_preview_start = ON;
    game_events->galaxies_start = ON;
    game_events->map_enter = OFF;
    game_events->map_exit = OFF;
    game_events->map_center = OFF;
    game_events->map_switch = OFF;
    game_events->universe_enter = OFF;
    game_events->universe_exit = OFF;
    game_events->universe_center = OFF;
    game_events->universe_switch = OFF;
    game_events->exited_galaxy = OFF;
    game_events->galaxy_found = OFF;
    game_events->generate_bstars = OFF;

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
    stars_clear_table(nav_state->stars);

    for (int i = 0; i < MAX_STARS; i++)
    {
        nav_state->stars[i] = NULL;
    }

    // Initialize galaxies hash table to NULL pointers
    galaxies_clear_table(nav_state->galaxies);

    for (int i = 0; i < MAX_GALAXIES; i++)
    {
        nav_state->galaxies[i] = NULL;
    }

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
        // Allocate memory for galaxies
        nav_state->current_galaxy = (Galaxy *)malloc(sizeof(Galaxy));

        if (nav_state->current_galaxy == NULL)
        {
            fprintf(stderr, "Error: Could not allocate memory for current_galaxy.\n");
            return;
        }

        nav_state->buffer_galaxy = (Galaxy *)malloc(sizeof(Galaxy));

        if (nav_state->buffer_galaxy == NULL)
        {
            fprintf(stderr, "Error: Could not allocate memory for buffer_galaxy.\n");
            return;
        }

        nav_state->previous_galaxy = (Galaxy *)malloc(sizeof(Galaxy));

        if (nav_state->previous_galaxy == NULL)
        {
            fprintf(stderr, "Error: Could not allocate memory for previous_galaxy.\n");
            return;
        }
    }

    // Copy current_galaxy_copy to current_galaxy
    memcpy(nav_state->current_galaxy, current_galaxy_copy, sizeof(Galaxy));

    // Set current galaxy as selected
    nav_state->current_galaxy->is_selected = ON;

    // Copy current_galaxy to buffer_galaxy
    memcpy(nav_state->buffer_galaxy, nav_state->current_galaxy, sizeof(Galaxy));

    // Generate background stars
    gfx_generate_bstars(game_events, nav_state, bstars, camera, false);
}

/**
 * Updates the game state and graphics for the map mode. This includes handling user input for zooming and
 * centering the view on the player's ship, as well as generating and rendering stars and galaxies.
 *
 * @param game_state A pointer to the current game state.
 * @param input_state A pointer to the current input state.
 * @param game_events A pointer to the current game events.
 * @param nav_state A pointer to the current navigation state.
 * @param bstars A pointer to the Bstar structure that holds information about the binary stars.
 * @param ship A pointer to the current ship.
 * @param camera A pointer to the current camera position.
 *
 * @return void
 */
void game_run_map_state(GameState *game_state, InputState *input_state, GameEvents *game_events, NavigationState *nav_state, Bstar *bstars, Ship *ship, Camera *camera)
{
    // Add small tolerance to account for floating-point precision errors
    const double epsilon = ZOOM_EPSILON;

    double zoom_step = ZOOM_STEP;

    if (game_events->universe_switch)
        stars_clear_table(nav_state->stars);

    // Reset region_size
    if (game_state->game_scale < ZOOM_MAP_REGION_SWITCH - epsilon)
    {
        if (game_events->universe_switch)
            game_state->galaxy_region_size = GALAXY_REGION_SIZE_MAX;

        // Trigger generation of new stars for new region size
        game_events->stars_start = ON;
    }

    if (game_events->map_enter)
    {
        // Save ship position
        ship->previous_position.x = ship->position.x;
        ship->previous_position.y = ship->position.y;
    }

    if (game_events->map_enter || game_events->map_center)
    {
        if (game_events->map_enter && !game_state->save_scale)
            game_state->save_scale = game_state->game_scale;

        if (!game_events->universe_switch)
        {
            game_state->game_scale = ZOOM_MAP;

            // Reset map_offset
            nav_state->map_offset.x = ship->previous_position.x;
            nav_state->map_offset.y = ship->previous_position.y;
        }

        // Zoom in
        for (int s = 0; s < MAX_STARS; s++)
        {
            if (nav_state->stars[s] != NULL && nav_state->stars[s]->star != NULL)
                gfx_zoom_star_system(nav_state->stars[s]->star, game_state->game_scale);
        }

        gfx_update_camera(camera, nav_state->map_offset, game_state->game_scale);
    }

    if (game_events->map_center)
    {
        // Reset stars and galaxy to current position
        if (nav_state->current_galaxy->position.x != nav_state->buffer_galaxy->position.x ||
            nav_state->current_galaxy->position.y != nav_state->buffer_galaxy->position.y)
        {
            stars_clear_table(nav_state->stars);
            memcpy(nav_state->current_galaxy, nav_state->buffer_galaxy, sizeof(Galaxy));

            // Reset galaxy_offset
            nav_state->galaxy_offset.current_x = nav_state->galaxy_offset.buffer_x;
            nav_state->galaxy_offset.current_y = nav_state->galaxy_offset.buffer_y;
        }

        // Reset ship position
        ship->position.x = ship->previous_position.x;
        ship->position.y = ship->previous_position.y;

        // Reset region size
        game_state->galaxy_region_size = GALAXY_REGION_SIZE;

        // Delete stars that end up outside the region
        if (game_state->game_scale - zoom_step <= ZOOM_MAP_REGION_SWITCH + epsilon)
        {
            double bx = maths_get_nearest_section_line(nav_state->map_offset.x, GALAXY_SECTION_SIZE);
            double by = maths_get_nearest_section_line(nav_state->map_offset.y, GALAXY_SECTION_SIZE);

            stars_delete_outside_region(nav_state->stars, bx, by, game_state->galaxy_region_size);
        }

        game_events->map_center = OFF;
    }

    // Zoom
    if (input_state->zoom_in)
    {
        if (game_state->game_scale <= ZOOM_MAP_REGION_SWITCH - epsilon)
            zoom_step /= 10;

        if (game_state->game_scale + zoom_step <= ZOOM_MAX + epsilon)
        {
            // Reset scale
            game_state->game_scale += zoom_step;

            // Reset region_size
            if (game_state->game_scale >= ZOOM_MAP_REGION_SWITCH - epsilon)
            {
                game_state->galaxy_region_size = GALAXY_REGION_SIZE;

                // Delete stars that end up outside the region
                if (game_state->game_scale <= ZOOM_MAP_REGION_SWITCH + zoom_step + epsilon)
                {
                    double bx = maths_get_nearest_section_line(nav_state->map_offset.x, GALAXY_SECTION_SIZE);
                    double by = maths_get_nearest_section_line(nav_state->map_offset.y, GALAXY_SECTION_SIZE);

                    stars_delete_outside_region(nav_state->stars, bx, by, game_state->galaxy_region_size);
                }
            }

            // Zoom in
            for (int s = 0; s < MAX_STARS; s++)
            {
                if (nav_state->stars[s] != NULL && nav_state->stars[s]->star != NULL)
                    gfx_zoom_star_system(nav_state->stars[s]->star, game_state->game_scale);
            }
        }

        input_state->zoom_in = OFF;
    }

    if (input_state->zoom_out)
    {
        if (game_state->game_scale <= ZOOM_MAP_REGION_SWITCH + epsilon)
            zoom_step = ZOOM_STEP / 10;

        if (game_state->game_scale - zoom_step >= ZOOM_MAP_MIN - epsilon)
        {
            // Reset scale
            game_state->game_scale -= zoom_step;

            // Reset region_size
            if (game_state->game_scale < ZOOM_MAP_REGION_SWITCH - epsilon)
            {
                game_state->galaxy_region_size = GALAXY_REGION_SIZE_MAX;

                // Trigger generation of new stars for new region size
                game_events->stars_start = ON;
            }

            // Switch to Universe mode
            if (game_state->game_scale <= ZOOM_MAP_SWITCH - epsilon)
            {
                game_events->map_exit = ON;
                game_events->map_switch = ON;
                game_events->universe_enter = ON;
                game_change_state(game_state, game_events, UNIVERSE);

                // Update universe_offset
                nav_state->universe_offset.x = nav_state->current_galaxy->position.x + nav_state->map_offset.x / GALAXY_SCALE;
                nav_state->universe_offset.y = nav_state->current_galaxy->position.y + nav_state->map_offset.y / GALAXY_SCALE;
            }

            // Zoom out
            for (int s = 0; s < MAX_STARS; s++)
            {
                if (nav_state->stars[s] != NULL && nav_state->stars[s]->star != NULL)
                    gfx_zoom_star_system(nav_state->stars[s]->star, game_state->game_scale);
            }
        }

        input_state->zoom_out = OFF;
    }

    stars_generate(game_state, game_events, nav_state, bstars, ship);

    gfx_update_camera(camera, nav_state->map_offset, game_state->game_scale);

    gfx_draw_section_lines(camera, game_state->state, colors[COLOR_ORANGE_32], game_state->game_scale);

    // Check for nearest galaxy, excluding current galaxy
    if (game_events->exited_galaxy && PROJECTIONS_ON)
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

        if (nearest_galaxy != NULL &&
            (nearest_galaxy->position.x != nav_state->current_galaxy->position.x ||
             nearest_galaxy->position.y != nav_state->current_galaxy->position.y))
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

    // Move through map
    double rate_x = 0, rate_y = 0;

    if (input_state->right)
        rate_x = MAP_SPEED_MIN + (MAP_SPEED_MAX - MAP_SPEED_MIN) * (camera->w / 1000) / game_state->game_scale;
    else if (input_state->left)
        rate_x = -(MAP_SPEED_MIN + (MAP_SPEED_MAX - MAP_SPEED_MIN) * (camera->w / 1000) / game_state->game_scale);

    nav_state->map_offset.x += rate_x;

    if (input_state->down)
        rate_y = MAP_SPEED_MIN + (MAP_SPEED_MAX - MAP_SPEED_MIN) * (camera->w / 1000) / game_state->game_scale;
    else if (input_state->up)
        rate_y = -(MAP_SPEED_MIN + (MAP_SPEED_MAX - MAP_SPEED_MIN) * (camera->w / 1000) / game_state->game_scale);

    nav_state->map_offset.y += rate_y;

    // Process star system
    if (!game_events->map_switch && !game_events->map_enter && !input_state->zoom_in && !input_state->zoom_out)
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

    // Draw ship projection
    ship->projection->rect.x = (ship->position.x - nav_state->map_offset.x) * game_state->game_scale + (camera->w / 2 - ship->projection->radius);
    ship->projection->rect.y = (ship->position.y - nav_state->map_offset.y) * game_state->game_scale + (camera->h / 2 - ship->projection->radius);
    ship->projection->angle = ship->angle;

    if (!game_events->map_switch && (ship->projection->rect.x + ship->projection->radius < 0 ||
                                     ship->projection->rect.x + ship->projection->radius > camera->w ||
                                     ship->projection->rect.y + ship->projection->radius < 0 ||
                                     ship->projection->rect.y + ship->projection->radius > camera->h))
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

    gfx_draw_screen_frame(camera);

    if (game_events->universe_switch)
        game_events->universe_switch = OFF;

    if (game_events->map_enter)
        game_events->map_enter = OFF;
}

/**
 * This function handles the navigation state of the game, including resetting game elements when exiting the
 * navigation state, zooming in and out, updating the camera position, generating stars, and drawing the galaxy cloud.
 *
 * @param game_state A pointer to the current game state.
 * @param input_state A pointer to the current input state.
 * @param game_events A pointer to the current game events.
 * @param nav_state A pointer to the current navigation state.
 * @param bstars A pointer to the Bstar structure that holds information about the binary stars.
 * @param ship A pointer to the current ship.
 * @param camera A pointer to the current camera position.
 *
 * @return void
 */
void game_run_navigate_state(GameState *game_state, InputState *input_state, GameEvents *game_events, NavigationState *nav_state, Bstar *bstars, Ship *ship, Camera *camera)
{
    if (game_events->map_exit || game_events->universe_exit)
    {
        // Reset stars and galaxy to current position
        if (nav_state->current_galaxy->position.x != nav_state->buffer_galaxy->position.x ||
            nav_state->current_galaxy->position.y != nav_state->buffer_galaxy->position.y)
        {
            stars_clear_table(nav_state->stars);
            memcpy(nav_state->current_galaxy, nav_state->buffer_galaxy, sizeof(Galaxy));

            // Reset galaxy_offset
            nav_state->galaxy_offset.current_x = nav_state->galaxy_offset.buffer_x;
            nav_state->galaxy_offset.current_y = nav_state->galaxy_offset.buffer_y;
        }

        // Trigger generation of new stars
        game_events->stars_start = ON;

        // Reset ship position
        ship->position.x = ship->previous_position.x;
        ship->position.y = ship->previous_position.y;

        // Reset region size
        game_state->galaxy_region_size = GALAXY_REGION_SIZE;

        // Delete stars that end up outside the region
        double bx = maths_get_nearest_section_line(ship->position.x, GALAXY_SECTION_SIZE);
        double by = maths_get_nearest_section_line(ship->position.y, GALAXY_SECTION_SIZE);

        stars_delete_outside_region(nav_state->stars, bx, by, game_state->galaxy_region_size);

        // Reset saved game_scale
        if (game_state->save_scale)
            game_state->game_scale = game_state->save_scale;
        else
            game_state->game_scale = ZOOM_NAVIGATE;

        game_state->save_scale = OFF;

        if (input_state->camera_on)
            gfx_update_camera(camera, ship->position, game_state->game_scale);

        // Zoom in
        for (int s = 0; s < MAX_STARS; s++)
        {
            if (nav_state->stars[s] != NULL && nav_state->stars[s]->star != NULL)
                gfx_zoom_star_system(nav_state->stars[s]->star, game_state->game_scale);
        }
    }

    // Add small tolerance to account for floating-point precision errors
    const double epsilon = ZOOM_EPSILON;

    if (input_state->zoom_in)
    {
        if (game_state->game_scale + ZOOM_STEP <= ZOOM_MAX + epsilon)
        {
            // Reset scale
            game_state->game_scale += ZOOM_STEP;

            // Zoom in
            for (int s = 0; s < MAX_STARS; s++)
            {
                if (nav_state->stars[s] != NULL && nav_state->stars[s]->star != NULL)
                    gfx_zoom_star_system(nav_state->stars[s]->star, game_state->game_scale);
            }
        }

        input_state->zoom_in = OFF;
    }

    if (input_state->zoom_out)
    {
        if (game_state->game_scale - ZOOM_STEP >= ZOOM_NAVIGATE_MIN - epsilon)
        {
            // Reset scale
            game_state->game_scale -= ZOOM_STEP;

            // Zoom out
            for (int s = 0; s < MAX_STARS; s++)
            {
                if (nav_state->stars[s] != NULL && nav_state->stars[s]->star != NULL)
                    gfx_zoom_star_system(nav_state->stars[s]->star, game_state->game_scale);
            }
        }

        input_state->zoom_out = OFF;
    }

    if (input_state->camera_on)
        stars_generate(game_state, game_events, nav_state, bstars, ship);

    if (input_state->camera_on)
        gfx_update_camera(camera, nav_state->navigate_offset, game_state->game_scale);

    // Get distance from galaxy center
    double distance_current = maths_distance_between_points(ship->position.x, ship->position.y, 0, 0);

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

            if (game_events->galaxy_found)
            {
                limit_current = distance_current;
                game_events->galaxy_found = OFF;
            }

            gfx_update_gstars_position(nav_state->current_galaxy, ship_position_current, camera, distance_current, limit_current);

            if (game_events->exited_galaxy && nav_state->previous_galaxy != NULL && nav_state->previous_galaxy->initialized_hd)
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
                gfx_update_bstars_position(game_state->state, input_state->camera_on, nav_state, bstars, camera, speed, distance_current);
        }

        if (SPEED_LINES_ON && input_state->camera_on)
            gfx_draw_speed_lines(nav_state->velocity.magnitude, camera, speed);
    }

    if (nav_state->velocity.magnitude > GALAXY_SPEED_LIMIT)
        gfx_draw_speed_arc(ship, camera, game_state->game_scale);

    // Process star system
    if ((!game_events->map_exit && !game_events->universe_exit && !input_state->zoom_in && !input_state->zoom_out) || game_state->game_scale > ZOOM_NAVIGATE_MIN)
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

    // Enforce speed limits
    if (distance_current < nav_state->current_galaxy->radius * GALAXY_SCALE)
    {
        if (nav_state->velocity.magnitude > GALAXY_SPEED_LIMIT)
        {
            ship->vx = GALAXY_SPEED_LIMIT * ship->vx / nav_state->velocity.magnitude;
            ship->vy = GALAXY_SPEED_LIMIT * ship->vy / nav_state->velocity.magnitude;
        }
    }
    else
    {
        if (nav_state->velocity.magnitude > UNIVERSE_SPEED_LIMIT)
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
    if (game_events->exited_galaxy && PROJECTIONS_ON)
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

        if (nearest_galaxy != NULL &&
            (nearest_galaxy->position.x != nav_state->current_galaxy->position.x ||
             nearest_galaxy->position.y != nav_state->current_galaxy->position.y))
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

    gfx_draw_screen_frame(camera);

    // Update velocity in console
    console_update_entry(game_state->console_entries, CONSOLE_V, nav_state->velocity.magnitude);

    if (game_events->map_exit)
        game_events->map_exit = OFF;

    if (game_events->universe_exit)
        game_events->universe_exit = OFF;
}

/**
 * Updates the state of the game universe based on player input and events. This function generates galaxies and
 * stars within the game universe, updates the position of the ship and camera, and triggers the generation of
 * a stars preview. It also handles resetting certain game elements when the player exits or enters the universe,
 * and adjusts the game scale based on the current galaxy class.
 *
 * @param game_state A pointer to the GameState struct containing the current state of the game.
 * @param input_state A pointer to the InputState struct containing the current user input state.
 * @param game_events A pointer to the GameEvents struct containing the current events in the game.
 * @param nav_state A pointer to the NavigationState struct containing the current navigation state of the game.
 * @param ship A pointer to the Ship struct representing the player's ship.
 * @param camera A pointer to the Camera struct representing the current camera position.
 *
 * @return void
 */
void game_run_universe_state(GameState *game_state, InputState *input_state, GameEvents *game_events, NavigationState *nav_state, Ship *ship, Camera *camera)
{
    static int zoom_preview = OFF;
    static Point cross_point = {0.0, 0.0};

    if (game_events->map_exit)
    {
        // Reset stars
        if (nav_state->current_galaxy->position.x != nav_state->buffer_galaxy->position.x ||
            nav_state->current_galaxy->position.y != nav_state->buffer_galaxy->position.y)
            stars_clear_table(nav_state->stars);

        // Reset galaxy_offset
        nav_state->galaxy_offset.current_x = nav_state->galaxy_offset.buffer_x;
        nav_state->galaxy_offset.current_y = nav_state->galaxy_offset.buffer_y;

        // Reset ship position
        ship->position.x = ship->previous_position.x;
        ship->position.y = ship->previous_position.y;

        // Reset region size
        game_state->galaxy_region_size = GALAXY_REGION_SIZE;

        // Delete stars that end up outside the region
        double bx = maths_get_nearest_section_line(ship->position.x, GALAXY_SECTION_SIZE);
        double by = maths_get_nearest_section_line(ship->position.y, GALAXY_SECTION_SIZE);

        stars_delete_outside_region(nav_state->stars, bx, by, game_state->galaxy_region_size);
    }

    if (game_events->universe_enter || game_events->universe_center)
    {
        // Save ship position
        ship->previous_position.x = ship->position.x;
        ship->previous_position.y = ship->position.y;

        // Initialize cross lines for stars preview
        // Add GALAXY_SECTION_SIZE to map_offset so that stars generation is triggered on startup
        if (cross_point.x == 0.0)
            cross_point.x = maths_get_nearest_section_line(nav_state->map_offset.x + GALAXY_SECTION_SIZE, GALAXY_SECTION_SIZE);

        if (cross_point.y == 0.0)
            cross_point.y = maths_get_nearest_section_line(nav_state->map_offset.y + GALAXY_SECTION_SIZE, GALAXY_SECTION_SIZE);

        // Generate galaxies
        Point offset = {.x = nav_state->galaxy_offset.current_x, .y = nav_state->galaxy_offset.current_y};
        galaxies_generate(game_events, nav_state, offset);

        if (game_events->universe_enter && !game_state->save_scale)
            game_state->save_scale = game_state->game_scale;

        if (!game_events->map_switch)
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
        game_events->stars_preview_start = ON;

        if (game_events->universe_center)
            game_events->universe_center = OFF;
    }
    else
        galaxies_generate(game_events, nav_state, nav_state->universe_offset);

    gfx_draw_section_lines(camera, game_state->state, colors[COLOR_ORANGE_32], game_state->game_scale * GALAXY_SCALE);

    // Generate stars preview
    double zoom_universe_stars = ZOOM_UNIVERSE_STARS;

    switch (nav_state->current_galaxy->class)
    {
    case 1:
        zoom_universe_stars = 0.00005;
        break;
    default:
        zoom_universe_stars = ZOOM_UNIVERSE_STARS;
        break;
    }

    // Add small tolerance to account for floating-point precision errors
    const double epsilon = ZOOM_EPSILON / GALAXY_SCALE;

    if (game_state->game_scale >= zoom_universe_stars - epsilon)
    {
        if (game_events->stars_preview_start)
        {
            // Update map_offset
            nav_state->map_offset.x = (nav_state->universe_offset.x - nav_state->current_galaxy->position.x) * GALAXY_SCALE;
            nav_state->map_offset.y = (nav_state->universe_offset.y - nav_state->current_galaxy->position.y) * GALAXY_SCALE;

            stars_generate_preview(nav_state, camera, &cross_point, zoom_preview, game_state->game_scale);
            game_events->stars_preview_start = OFF;
            zoom_preview = OFF;
        }

        for (int i = 0; i < MAX_STARS; i++)
        {
            if (nav_state->stars[i] != NULL)
            {
                // Each index can have many entries, loop through all of them
                StarEntry *entry = nav_state->stars[i];

                while (entry != NULL)
                {
                    int x = (nav_state->current_galaxy->position.x - camera->x + entry->star->position.x / GALAXY_SCALE) * game_state->game_scale * GALAXY_SCALE;
                    int y = (nav_state->current_galaxy->position.y - camera->y + entry->star->position.y / GALAXY_SCALE) * game_state->game_scale * GALAXY_SCALE;
                    int opacity = entry->star->class * (255 / 6);
                    SDL_SetRenderDrawColor(renderer, entry->star->color.r, entry->star->color.g, entry->star->color.b, opacity);
                    SDL_RenderDrawPoint(renderer, x, y);

                    entry = entry->next;
                }
            }
        }
    }

    // Draw galaxies
    if (!game_events->universe_enter)
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

    // Move through map
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

    if (input_state->right || input_state->left || input_state->down || input_state->up)
        game_events->stars_preview_start = ON;

    if (input_state->right)
        rate_x = UNIVERSE_SPEED_MIN + (UNIVERSE_SPEED_MAX - UNIVERSE_SPEED_MIN) * (camera->w / 1000) / (game_state->game_scale * speed_universe_step);
    else if (input_state->left)
        rate_x = -(UNIVERSE_SPEED_MIN + (UNIVERSE_SPEED_MAX - UNIVERSE_SPEED_MIN) * (camera->w / 1000) / (game_state->game_scale * speed_universe_step));

    rate_x /= (double)GALAXY_SCALE / 1000;
    nav_state->universe_offset.x += rate_x;

    if (input_state->down)
        rate_y = UNIVERSE_SPEED_MIN + (UNIVERSE_SPEED_MAX - UNIVERSE_SPEED_MIN) * (camera->w / 1000) / (game_state->game_scale * speed_universe_step);
    else if (input_state->up)
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

    // Zoom
    double zoom_universe_step = ZOOM_UNIVERSE_STEP;

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
            game_state->game_scale += zoom_universe_step;

            // Switch to Map mode
            if (game_state->game_scale >= ZOOM_MAP_SWITCH - epsilon)
            {
                game_events->universe_switch = ON;
                game_events->map_enter = ON;
                game_change_state(game_state, game_events, MAP);

                stars_clear_table(nav_state->stars);

                // Update map_offset
                nav_state->map_offset.x = (nav_state->universe_offset.x - nav_state->current_galaxy->position.x) * GALAXY_SCALE;
                nav_state->map_offset.y = (nav_state->universe_offset.y - nav_state->current_galaxy->position.y) * GALAXY_SCALE;
            }
        }

        zoom_preview = ON;
        input_state->zoom_in = OFF;
        game_events->stars_preview_start = ON;
        cross_point.x += GALAXY_SCALE; // fake increment so that it triggers star preview generation
        cross_point.y += GALAXY_SCALE;
    }

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
            game_state->game_scale -= zoom_universe_step;
        }

        stars_clear_table(nav_state->stars);

        zoom_preview = ON;
        input_state->zoom_out = OFF;
        game_events->stars_preview_start = ON;
        cross_point.x += GALAXY_SCALE; // fake increment so that it triggers star preview generation
        cross_point.y += GALAXY_SCALE;
    }

    gfx_update_camera(camera, nav_state->universe_offset, game_state->game_scale * GALAXY_SCALE);

    // Draw ship projection
    ship->projection->rect.x = (nav_state->galaxy_offset.current_x + ship->position.x / GALAXY_SCALE - camera->x) * (game_state->game_scale * GALAXY_SCALE) - SHIP_PROJECTION_RADIUS;
    ship->projection->rect.y = (nav_state->galaxy_offset.current_y + ship->position.y / GALAXY_SCALE - camera->y) * (game_state->game_scale * GALAXY_SCALE) - SHIP_PROJECTION_RADIUS;
    ship->projection->angle = ship->angle;

    if (!game_events->universe_switch && (ship->projection->rect.x + ship->projection->radius < 0 ||
                                          ship->projection->rect.x + ship->projection->radius > camera->w ||
                                          ship->projection->rect.y + ship->projection->radius < 0 ||
                                          ship->projection->rect.y + ship->projection->radius > camera->h))
    {
        gfx_project_ship_on_edge(game_state->state, input_state, nav_state, ship, camera, game_state->game_scale);
    }
    else
        SDL_RenderCopyEx(renderer, ship->projection->texture, &ship->projection->main_img_rect, &ship->projection->rect, ship->projection->angle, &ship->projection->rotation_pt, SDL_FLIP_NONE);

    // Draw cross at position
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);
    SDL_RenderDrawLine(renderer, (camera->w / 2) - 7, camera->h / 2, (camera->w / 2) + 7, camera->h / 2);
    SDL_RenderDrawLine(renderer, camera->w / 2, (camera->h / 2) - 7, camera->w / 2, (camera->h / 2) + 7);

    // Draw galaxy info box
    if (input_state->galaxy_hover || nav_state->current_galaxy->is_selected)
    {
        if (!nav_state->current_galaxy->initialized || nav_state->current_galaxy->initialized < nav_state->current_galaxy->total_groups)
            gfx_generate_gstars(nav_state->current_galaxy, false);

        galaxies_draw_info_box(nav_state->current_galaxy, camera);
    }

    gfx_draw_screen_frame(camera);

    if (game_events->map_exit)
        game_events->map_exit = OFF;

    if (game_events->map_switch)
        game_events->map_switch = OFF;

    if (game_events->universe_enter)
        game_events->universe_enter = OFF;
}

/**
 * Updates the position of a ship based on input state and camera position.
 *
 * @param game_state A pointer to the current game state.
 * @param input_state A pointer to the current input state.
 * @param ship A pointer to the ship to update.
 * @param camera A pointer to the camera used to render the game.
 *
 * @return void
 */
static void game_update_ship_position(GameState *game_state, const InputState *input_state, Ship *ship, const Camera *camera)
{
    float radians;

    // Update ship angle
    if (input_state->right && !input_state->left && game_state->landing_stage == STAGE_OFF)
        ship->angle += 3;

    if (input_state->left && !input_state->right && game_state->landing_stage == STAGE_OFF)
        ship->angle -= 3;

    if (ship->angle > 360)
        ship->angle -= 360;

    // Apply thrust
    if (input_state->thrust)
    {
        game_state->landing_stage = STAGE_OFF;
        radians = ship->angle * M_PI / 180;

        ship->vx += G_THRUST * sin(radians);
        ship->vy -= G_THRUST * cos(radians);
    }

    // Apply reverse
    if (input_state->reverse)
    {
        radians = ship->angle * M_PI / 180;

        ship->vx -= G_THRUST * sin(radians);
        ship->vy += G_THRUST * cos(radians);
    }

    // Stop ship
    if (input_state->stop)
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
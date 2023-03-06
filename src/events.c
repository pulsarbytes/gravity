/*
 * events.c
 */

#include <SDL2/SDL.h>

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"
#include "../include/events.h"

// Store the initial mouse position for dragging
SDL_Point mouseDownPos = {0, 0};

/**
 * The events_loop function handles the events that occur while the game is running.
 *
 * @param game_state A pointer to the current game state.
 * @param input_state A pointer to the current input state.
 * @param game_events A pointer to the current game events.
 * @param nav_events A pointer to the current navigation state.
 * @param camera A pointer to the camera.
 *
 * @return void
 */
void events_loop(GameState *game_state, InputState *input_state, GameEvents *game_events, NavigationState *nav_state, const Camera *camera)
{
    SDL_Event event;
    static int save_state;
    const double epsilon = ZOOM_EPSILON / GALAXY_SCALE;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            game_change_state(game_state, game_events, QUIT);
            break;
        case SDL_MOUSEBUTTONDOWN:
            // If left mouse button is pressed, record the initial mouse position
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                if (game_state->state == UNIVERSE || game_state->state == MAP)
                {
                    mouseDownPos.x = event.button.x;
                    mouseDownPos.y = event.button.y;
                }
                else if (game_state->state == MENU)
                {
                    input_state->mouse_x = event.button.x;
                    input_state->mouse_y = event.button.y;

                    for (int i = 0; i < MENU_BUTTON_COUNT; i++)
                    {
                        if (!game_state->menu[i].disabled &&
                            input_state->mouse_x >= game_state->menu[i].rect.x &&
                            input_state->mouse_x <= game_state->menu[i].rect.x + game_state->menu[i].rect.w &&
                            input_state->mouse_y >= game_state->menu[i].rect.y &&
                            input_state->mouse_y <= game_state->menu[i].rect.y + game_state->menu[i].rect.h)
                        {
                            input_state->selected_button = i;

                            if (game_state->menu[i].state == RESUME)
                                game_change_state(game_state, game_events, save_state);
                            else
                                game_change_state(game_state, game_events, game_state->menu[i].state);
                            break;
                        }
                    }
                }
            }
            break;
        case SDL_MOUSEMOTION:
            if (game_state->state == MENU)
            {
                input_state->mouse_x = event.motion.x;
                input_state->mouse_y = event.motion.y;
            }
            else if (game_state->state == MAP || game_state->state == UNIVERSE)
            {
                input_state->mouse_x = event.motion.x;
                input_state->mouse_y = event.motion.y;

                // If left mouse button is held down, update the position
                if (event.motion.state & SDL_BUTTON_LMASK)
                {
                    // Calculate the difference between the current mouse position and the initial mouse position
                    int delta_x = input_state->mouse_x - mouseDownPos.x;
                    int delta_y = input_state->mouse_y - mouseDownPos.y;

                    if (game_state->state == UNIVERSE)
                    {
                        // Trigger star generation
                        game_events->stars_preview_start = ON;

                        // Calculate drag speed
                        double speed_universe_step = 7000;

                        if (game_state->game_scale >= 0.002 - epsilon)
                            speed_universe_step = 10000;
                        else if (game_state->game_scale >= 0.001 - epsilon)
                            speed_universe_step = 8000;
                        else if (game_state->game_scale >= 0.00001 - epsilon)
                            speed_universe_step = 7000;

                        // Update the position
                        nav_state->universe_offset.x -= delta_x / (game_state->game_scale * speed_universe_step);
                        nav_state->universe_offset.y -= delta_y / (game_state->game_scale * speed_universe_step);
                    }
                    else if (game_state->state == MAP)
                    {
                        nav_state->map_offset.x -= delta_x / game_state->game_scale;
                        nav_state->map_offset.y -= delta_y / game_state->game_scale;
                    }

                    // Update the initial mouse position to the current mouse position for the next iteration
                    mouseDownPos.x = input_state->mouse_x;
                    mouseDownPos.y = input_state->mouse_y;
                }
                else
                {
                    // Scroll left
                    if (input_state->mouse_x < MOUSE_SCROLL_DISTANCE)
                    {
                        input_state->right = OFF;
                        input_state->left = ON;
                    }
                    else
                        input_state->left = OFF;

                    // Scroll right
                    if (input_state->mouse_x > camera->w - MOUSE_SCROLL_DISTANCE)
                    {
                        input_state->left = OFF;
                        input_state->right = ON;
                    }
                    else
                        input_state->right = OFF;

                    // Scroll up
                    if (input_state->mouse_y < MOUSE_SCROLL_DISTANCE)
                    {
                        input_state->down = OFF;
                        input_state->up = ON;
                    }
                    else
                        input_state->up = OFF;

                    // Scroll down
                    if (input_state->mouse_y > camera->h - MOUSE_SCROLL_DISTANCE)
                    {
                        input_state->up = OFF;
                        input_state->down = ON;
                    }
                    else
                        input_state->down = OFF;
                }
            }
            break;
        case SDL_MOUSEWHEEL:
            // Wait until previous zoom has ended
            if (input_state->zoom_in || input_state->zoom_out)
                return;

            // Don't continue below min zoom
            if (event.wheel.y < 0 && game_state->game_scale <= (ZOOM_UNIVERSE_MIN / GALAXY_SCALE) + epsilon)
                return;

            // Get the current mouse position
            int mouse_x, mouse_y;
            SDL_GetMouseState(&mouse_x, &mouse_y);

            double zoom_universe_step = ZOOM_UNIVERSE_STEP;
            double zoom_step = ZOOM_STEP;

            // Handle mouse wheel
            if (event.wheel.y > 0)
            {
                // Zoom in
                input_state->zoom_in = ON;
                input_state->zoom_out = OFF;

                if (game_state->state == UNIVERSE)
                {
                    if (game_state->game_scale >= 0.001 - epsilon)
                        zoom_universe_step = ZOOM_UNIVERSE_STEP;
                    else if (game_state->game_scale >= 0.0001 - epsilon)
                        zoom_universe_step = ZOOM_UNIVERSE_STEP / 10;
                    else if (game_state->game_scale >= 0.00001 - epsilon)
                        zoom_universe_step = ZOOM_UNIVERSE_STEP / 100;
                    else if (game_state->game_scale > 0)
                        zoom_universe_step = ZOOM_UNIVERSE_STEP / 1000;
                }
                else if (game_state->state == MAP)
                {
                    if (game_state->game_scale <= ZOOM_MAP_REGION_SWITCH - epsilon)
                        zoom_step = zoom_step / 10;
                }
            }
            else if (event.wheel.y < 0)
            {
                // Zoom out
                input_state->zoom_in = OFF;
                input_state->zoom_out = ON;

                if (game_state->state == UNIVERSE)
                {
                    if (game_state->game_scale <= 0.00001 + epsilon)
                        zoom_universe_step = -(ZOOM_UNIVERSE_STEP / 1000);
                    else if (game_state->game_scale <= 0.0001 + epsilon)
                        zoom_universe_step = -(ZOOM_UNIVERSE_STEP / 100);
                    else if (game_state->game_scale <= 0.001 + epsilon)
                        zoom_universe_step = -(ZOOM_UNIVERSE_STEP / 10);
                    else
                        zoom_universe_step = -ZOOM_UNIVERSE_STEP;
                }
                else if (game_state->state == MAP)
                {
                    if (game_state->game_scale <= ZOOM_MAP_REGION_SWITCH + epsilon)
                        zoom_step = -(zoom_step / 10);
                    else
                        zoom_step = -ZOOM_STEP;
                }
            }

            // Set position so that zoom is centered around mouse position
            if (game_state->state == UNIVERSE)
            {
                nav_state->universe_offset.x += ((mouse_x - camera->w / 2)) / (game_state->game_scale * GALAXY_SCALE) -
                                                ((mouse_x - camera->w / 2)) / (((game_state->game_scale) + zoom_universe_step) * GALAXY_SCALE);
                nav_state->universe_offset.y += ((mouse_y - camera->h / 2)) / (game_state->game_scale * GALAXY_SCALE) -
                                                ((mouse_y - camera->h / 2)) / (((game_state->game_scale) + zoom_universe_step) * GALAXY_SCALE);
            }
            else if (game_state->state == MAP)
            {
                nav_state->map_offset.x += ((mouse_x - camera->w / 2)) / (game_state->game_scale) -
                                           ((mouse_x - camera->w / 2)) / (((game_state->game_scale) + zoom_step));
                nav_state->map_offset.y += ((mouse_y - camera->h / 2)) / (game_state->game_scale) -
                                           ((mouse_y - camera->h / 2)) / (((game_state->game_scale) + zoom_step));
            }

            break;
        case SDL_KEYDOWN:
            switch (event.key.keysym.scancode)
            {
            case SDL_SCANCODE_C:
                // Toggle camera
                if (game_state->state == NAVIGATE)
                    input_state->camera_on = !input_state->camera_on;
                else if (game_state->state == MAP || game_state->state == UNIVERSE)
                    input_state->camera_on = ON;
                break;
            case SDL_SCANCODE_K:
                // Toggle console
                input_state->console = !input_state->console;
                break;
            case SDL_SCANCODE_M:
                // Enter Map
                if (game_state->state == UNIVERSE)
                    game_events->universe_exit = ON;
                if (game_state->state == NAVIGATE || game_state->state == UNIVERSE)
                {
                    game_change_state(game_state, game_events, MAP);
                    game_events->map_enter = ON;
                    input_state->camera_on = ON;
                }
                break;
            case SDL_SCANCODE_N:
                // Enter Navigate
                if (game_state->state == MAP)
                {
                    game_change_state(game_state, game_events, NAVIGATE);
                    game_events->map_exit = ON;
                }
                else if (game_state->state == UNIVERSE)
                {
                    game_change_state(game_state, game_events, NAVIGATE);
                    game_events->universe_exit = ON;
                }
                break;
            case SDL_SCANCODE_O:
                // Toggle orbits
                input_state->orbits_on = !input_state->orbits_on;
                break;
            case SDL_SCANCODE_S:
                // Stop ship
                if (game_state->state == NAVIGATE)
                    input_state->stop = ON;
                break;
            case SDL_SCANCODE_U:
                // Enter Universe
                if (game_state->state == MAP)
                    game_events->map_exit = ON;
                if (game_state->state == NAVIGATE || game_state->state == MAP)
                {
                    game_change_state(game_state, game_events, UNIVERSE);
                    game_events->universe_enter = ON;
                    input_state->camera_on = ON;
                }
                break;
            case SDL_SCANCODE_LEFT:
                // Scroll left / Rotate left
                input_state->right = OFF;
                input_state->left = ON;
                break;
            case SDL_SCANCODE_RIGHT:
                // Scroll right / Rotate right
                input_state->left = OFF;
                input_state->right = ON;
                break;
            case SDL_SCANCODE_UP:
                // Menu up
                if (game_state->state == MENU)
                {
                    input_state->mouse_x = 0;
                    input_state->mouse_y = 0;

                    do
                    {
                        input_state->selected_button = (input_state->selected_button + MENU_BUTTON_COUNT - 1) % MENU_BUTTON_COUNT;
                    } while (game_state->menu[input_state->selected_button].disabled);
                }
                // Scroll up / Thrust
                else
                {
                    input_state->reverse = OFF;
                    input_state->down = OFF;
                    input_state->thrust = ON;
                    input_state->up = ON;
                }
                break;
            case SDL_SCANCODE_DOWN:
                // Menu down
                if (game_state->state == MENU)
                {
                    input_state->mouse_x = 0;
                    input_state->mouse_y = 0;

                    do
                    {
                        input_state->selected_button = (input_state->selected_button + 1) % MENU_BUTTON_COUNT;
                    } while (game_state->menu[input_state->selected_button].disabled);
                }
                // Scroll down / Reverse
                else
                {
                    input_state->thrust = OFF;
                    input_state->up = OFF;
                    input_state->reverse = ON;
                    input_state->down = ON;
                }
                break;
            case SDL_SCANCODE_RETURN:
                // Select menu button
                if (game_state->state == MENU)
                {
                    if (game_state->menu[input_state->selected_button].state == RESUME)
                        game_change_state(game_state, game_events, save_state);
                    else
                        game_change_state(game_state, game_events, game_state->menu[input_state->selected_button].state);
                }
                break;
            case SDL_SCANCODE_SPACE:
                // Center Map / Center Universe
                if (game_state->state == MAP)
                    game_events->map_center = ON;
                else if (game_state->state == UNIVERSE)
                    game_events->universe_center = ON;
                break;
            case SDL_SCANCODE_ESCAPE:
                // Toggle Menu
                if (game_state->state != MENU)
                {
                    save_state = game_state->state;
                    game_change_state(game_state, game_events, MENU);
                }
                else if (game_state->state == MENU && game_events->game_started)
                    game_change_state(game_state, game_events, save_state);
                break;
            case SDL_SCANCODE_LEFTBRACKET:
                // Zoom out
                input_state->zoom_in = OFF;
                input_state->zoom_out = ON;
                break;
            case SDL_SCANCODE_RIGHTBRACKET:
                // Zoom in
                input_state->zoom_in = ON;
                input_state->zoom_out = OFF;
                break;
            default:
                break;
            }
            break;
        case SDL_KEYUP:
            switch (event.key.keysym.scancode)
            {
            case SDL_SCANCODE_S:
                input_state->stop = OFF;
                break;
            case SDL_SCANCODE_LEFT:
                input_state->left = OFF;
                break;
            case SDL_SCANCODE_RIGHT:
                input_state->right = OFF;
                break;
            case SDL_SCANCODE_UP:
                input_state->thrust = OFF;
                input_state->up = OFF;
                break;
            case SDL_SCANCODE_DOWN:
                input_state->reverse = OFF;
                input_state->down = OFF;
                break;
            case SDL_SCANCODE_LEFTBRACKET:
                input_state->zoom_out = OFF;
                break;
            case SDL_SCANCODE_RIGHTBRACKET:
                input_state->zoom_in = OFF;
                break;
            default:
                break;
            }
            break;
        }
    }
}

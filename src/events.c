/*
 * events.c
 */

#include <stdbool.h>

#include <SDL2/SDL.h>

#include "../include/constants.h"
#include "../include/enums.h"
#include "../include/structs.h"
#include "../include/events.h"

/**
 * The events_loop function handles the events that occur while the game is running.
 *
 * @param game_state A pointer to the current GameState object.
 * @param input_state A pointer to the current InputState object.
 * @param game_events A pointer to the current GameEvents object.
 * @param nav_state A pointer to the current NavigationState object.
 * @param camera A pointer to the current Camera object.
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
            // Reset is_mouse_dragging
            input_state->is_mouse_dragging = false;

            // Record the mouse click position
            if (game_state->state == UNIVERSE || game_state->state == MAP)
            {
                input_state->mouse_down_position.x = event.button.x;
                input_state->mouse_down_position.y = event.button.y;
            }

            // Left click down
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                if (game_state->state == UNIVERSE)
                {
                    if (input_state->is_hovering_star)
                        input_state->clicked_inside_star = true;
                    else if (input_state->is_hovering_galaxy)
                        input_state->clicked_inside_galaxy = true;
                }
                else if (game_state->state == MAP && input_state->is_hovering_star)
                    input_state->clicked_inside_star = true;

                Uint32 current_time = SDL_GetTicks();

                // Detect double-clicks
                if (current_time - input_state->last_click_time < DOUBLE_CLICK_INTERVAL)
                {
                    if (game_state->state == UNIVERSE)
                    {
                        if (input_state->is_hovering_star || input_state->is_hovering_galaxy)
                            input_state->click_count++;
                    }
                    else if (game_state->state == MAP && input_state->is_hovering_star)
                        input_state->click_count++;

                    if (input_state->click_count == 2)
                    {
                        if (game_state->state == UNIVERSE)
                        {
                            // Center galaxy
                            if (input_state->is_hovering_galaxy)
                            {
                                nav_state->universe_offset.x = nav_state->current_galaxy->position.x;
                                nav_state->universe_offset.y = nav_state->current_galaxy->position.y;

                                input_state->is_mouse_double_clicked = true;

                                // Adjust game_scale to new galaxy
                                double new_scale;

                                switch (nav_state->current_galaxy->class)
                                {
                                case 1:
                                    new_scale = ZOOM_UNIVERSE * 10;
                                    break;
                                case 2:
                                    new_scale = ZOOM_UNIVERSE * 5;
                                    break;
                                case 3:
                                    new_scale = ZOOM_UNIVERSE * 3;
                                    break;
                                case 4:
                                case 5:
                                case 6:
                                    new_scale = ZOOM_UNIVERSE * 2;
                                    break;
                                default:
                                    new_scale = ZOOM_UNIVERSE * 2;
                                    break;
                                }

                                if (game_state->game_scale > (new_scale / GALAXY_SCALE) - epsilon)
                                {
                                    input_state->zoom_out = true;
                                    input_state->zoom_in = false;
                                }
                                else
                                {
                                    input_state->zoom_in = true;
                                    input_state->zoom_out = false;
                                }

                                game_state->game_scale_override = new_scale / GALAXY_SCALE;
                            }
                            // Center star
                            else if (input_state->is_hovering_star)
                            {
                                nav_state->map_offset.x = nav_state->current_star->position.x;
                                nav_state->map_offset.y = nav_state->current_star->position.y;

                                // Adjust game_scale to new star
                                double new_scale;

                                switch (nav_state->current_star->class)
                                {
                                case 1:
                                    new_scale = ZOOM_MAP * 5;
                                    break;
                                case 2:
                                    new_scale = ZOOM_MAP * 2 + 0.001;
                                    break;
                                case 3:
                                case 4:
                                    new_scale = ZOOM_MAP + 0.001;
                                    break;
                                default:
                                    new_scale = ZOOM_MAP;
                                    break;
                                }

                                if (game_state->game_scale > new_scale - epsilon)
                                {
                                    input_state->zoom_in = false;
                                    input_state->zoom_out = true;
                                }
                                else
                                {
                                    input_state->zoom_in = true;
                                    input_state->zoom_out = false;
                                }

                                game_state->game_scale_override = new_scale;

                                game_events->switch_to_map = true;
                                game_events->is_entering_map = true;
                                game_change_state(game_state, game_events, MAP);
                            }
                        }
                        // Center star
                        else if (game_state->state == MAP)
                        {
                            if (input_state->is_hovering_star)
                            {
                                nav_state->map_offset.x = nav_state->current_star->position.x;
                                nav_state->map_offset.y = nav_state->current_star->position.y;

                                // Adjust game_scale to new star
                                double new_scale;

                                switch (nav_state->current_star->class)
                                {
                                case 1:
                                    new_scale = ZOOM_MAP * 5;
                                    break;
                                case 2:
                                    new_scale = ZOOM_MAP * 2 + 0.001;
                                    break;
                                case 3:
                                case 4:
                                    new_scale = ZOOM_MAP + 0.001;
                                    break;
                                default:
                                    new_scale = ZOOM_MAP;
                                    break;
                                }

                                if (game_state->game_scale > new_scale - epsilon)
                                {
                                    input_state->zoom_in = false;
                                    input_state->zoom_out = true;
                                }
                                else
                                {
                                    input_state->zoom_in = true;
                                    input_state->zoom_out = false;
                                }

                                game_state->game_scale_override = new_scale;
                            }
                        }

                        input_state->click_count = 0;
                    }
                }
                else
                {
                    if (game_state->state == UNIVERSE)
                    {
                        if (input_state->is_hovering_star || input_state->is_hovering_galaxy)
                            input_state->click_count = 1;
                        else
                            input_state->click_count = 0;
                    }
                    else if (game_state->state == MAP)
                    {
                        if (input_state->is_hovering_star)
                            input_state->click_count = 1;
                        else
                            input_state->click_count = 0;
                    }
                    else if (game_state->state == MENU || game_state->state == CONTROLS)
                    {
                        // Select menu button
                        input_state->mouse_position.x = event.button.x;
                        input_state->mouse_position.y = event.button.y;

                        for (int i = 0; i < MENU_BUTTON_COUNT; i++)
                        {
                            if (!game_state->menu[i].disabled &&
                                input_state->mouse_position.x >= game_state->menu[i].rect.x &&
                                input_state->mouse_position.x <= game_state->menu[i].rect.x + game_state->menu[i].rect.w &&
                                input_state->mouse_position.y >= game_state->menu[i].rect.y &&
                                input_state->mouse_position.y <= game_state->menu[i].rect.y + game_state->menu[i].rect.h)
                            {
                                input_state->selected_button_index = i;

                                if (game_state->menu[i].state == RESUME)
                                    game_change_state(game_state, game_events, save_state);
                                else
                                    game_change_state(game_state, game_events, game_state->menu[i].state);
                                break;
                            }
                        }
                    }
                }

                input_state->last_click_time = current_time;
            }

            if (event.button.button == SDL_BUTTON_RIGHT)
            {
                // Scroll left
                if (input_state->mouse_position.x < MOUSE_SCROLL_DISTANCE)
                {
                    input_state->right_on = false;
                    input_state->left_on = true;
                    nav_state->current_galaxy->is_selected = false;
                }
                else
                    input_state->left_on = false;

                // Scroll right
                if (input_state->mouse_position.x > camera->w - MOUSE_SCROLL_DISTANCE)
                {
                    input_state->left_on = false;
                    input_state->right_on = true;
                    nav_state->current_galaxy->is_selected = false;
                }
                else
                    input_state->right_on = false;

                // Scroll up
                if (input_state->mouse_position.y < MOUSE_SCROLL_DISTANCE)
                {
                    input_state->down_on = false;
                    input_state->up_on = true;
                    nav_state->current_galaxy->is_selected = false;
                }
                else
                    input_state->up_on = false;

                // Scroll down
                if (input_state->mouse_position.y > camera->h - MOUSE_SCROLL_DISTANCE)
                {
                    input_state->up_on = false;
                    input_state->down_on = true;
                    nav_state->current_galaxy->is_selected = false;
                }
            }
            else
                input_state->down_on = false;
            break;
        case SDL_MOUSEBUTTONUP:
            if (!input_state->is_mouse_dragging)
            {
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    if (game_state->state == UNIVERSE)
                    {
                        // Deselect galaxy
                        if (!input_state->is_hovering_galaxy && !input_state->clicked_inside_galaxy && !input_state->clicked_inside_star)
                            nav_state->current_galaxy->is_selected = false;

                        // Set galaxy as selected
                        if (input_state->is_hovering_galaxy)
                            nav_state->current_galaxy->is_selected = true;

                        input_state->clicked_inside_galaxy = false;

                        // Deselect star
                        if (!input_state->is_hovering_star && !input_state->clicked_inside_star && !input_state->is_hovering_star_info)
                            nav_state->selected_star->is_selected = false;

                        // Set star as selected
                        if (input_state->is_hovering_star)
                        {
                            if (strcmp(nav_state->selected_star->name, nav_state->current_star->name) != 0)
                                memcpy(nav_state->selected_star, nav_state->current_star, sizeof(Star));

                            nav_state->selected_star->is_selected = true;
                        }

                        input_state->clicked_inside_star = false;
                    }
                    else if (game_state->state == MAP)
                    {
                        // Deselect star
                        if (!input_state->is_hovering_star && !input_state->clicked_inside_star && !input_state->is_hovering_star_info)
                            nav_state->selected_star->is_selected = false;

                        // Set star as selected
                        if (input_state->is_hovering_star)
                        {
                            if (strcmp(nav_state->selected_star->name, nav_state->current_star->name) != 0)
                                memcpy(nav_state->selected_star, nav_state->current_star, sizeof(Star));

                            nav_state->selected_star->is_selected = true;
                        }

                        input_state->clicked_inside_star = false;
                    }
                }

                if (event.button.button == SDL_BUTTON_RIGHT)
                {
                    // Stop scrolling
                    input_state->right_on = false;
                    input_state->left_on = false;
                    input_state->down_on = false;
                    input_state->up_on = false;
                }
            }

            SDL_SetCursor(input_state->previous_cursor);
            input_state->previous_cursor = NULL;
            input_state->is_mouse_dragging = false;
            break;
        case SDL_MOUSEMOTION:
            if (game_state->state == MENU || game_state->state == CONTROLS)
            {
                input_state->mouse_position.x = event.motion.x;
                input_state->mouse_position.y = event.motion.y;
            }
            else if (game_state->state == MAP || game_state->state == UNIVERSE)
            {
                input_state->mouse_position.x = event.motion.x;
                input_state->mouse_position.y = event.motion.y;

                // If left mouse button is held down, update the position
                if (event.motion.state & SDL_BUTTON_LMASK && !input_state->is_hovering_star_info)
                {
                    input_state->is_mouse_dragging = true;

                    input_state->previous_cursor = SDL_GetCursor();
                    SDL_SetCursor(input_state->drag_cursor);
                    input_state->click_count = 0;
                    input_state->clicked_inside_galaxy = false;
                    input_state->clicked_inside_star = false;

                    // Calculate the difference between the current mouse position and the initial mouse position
                    int delta_x = input_state->mouse_position.x - input_state->mouse_down_position.x;
                    int delta_y = input_state->mouse_position.y - input_state->mouse_down_position.y;

                    if (game_state->state == UNIVERSE)
                    {
                        // Trigger star generation
                        game_events->start_stars_preview = true;

                        // Update the position
                        nav_state->universe_offset.x -= delta_x / (game_state->game_scale * GALAXY_SCALE);
                        nav_state->universe_offset.y -= delta_y / (game_state->game_scale * GALAXY_SCALE);
                    }
                    else if (game_state->state == MAP)
                    {
                        nav_state->map_offset.x -= delta_x / game_state->game_scale;
                        nav_state->map_offset.y -= delta_y / game_state->game_scale;
                    }

                    // Update the initial mouse position to the current mouse position for the next iteration
                    input_state->mouse_down_position.x = input_state->mouse_position.x;
                    input_state->mouse_down_position.y = input_state->mouse_position.y;
                }
            }
            break;
        case SDL_MOUSEWHEEL:
            // Get the current mouse position
            int mouse_x, mouse_y;
            SDL_GetMouseState(&mouse_x, &mouse_y);

            double zoom_universe_step = ZOOM_UNIVERSE_STEP;
            double zoom_step = ZOOM_STEP;

            if (game_state->state == UNIVERSE || game_state->state == MAP || game_state->state == NAVIGATE)
            {
                // Wait until previous zoom has ended
                if (input_state->zoom_in || input_state->zoom_out)
                    return;

                // Don't continue below min zoom
                if (event.wheel.y < 0 && game_state->game_scale <= (ZOOM_UNIVERSE_MIN / GALAXY_SCALE) + epsilon)
                    return;
            }

            // Handle mouse wheel
            if (event.wheel.y > 0)
            {
                if (game_state->state == UNIVERSE || game_state->state == MAP || game_state->state == NAVIGATE)
                {
                    // Zoom in
                    input_state->zoom_in = true;
                    input_state->zoom_out = false;

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
                        if (game_state->game_scale <= ZOOM_MAP_STEP_SWITCH - epsilon)
                            zoom_step = zoom_step / 10;
                    }
                }
                else if (game_state->state == CONTROLS)
                {
                    if (game_state->table_top_row > 0)
                    {
                        game_state->table_top_row--;
                    }
                }
            }
            else if (event.wheel.y < 0)
            {
                if (game_state->state == UNIVERSE || game_state->state == MAP || game_state->state == NAVIGATE)
                {
                    // Zoom out
                    input_state->zoom_in = false;
                    input_state->zoom_out = true;

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
                        if (game_state->game_scale <= ZOOM_MAP_STEP_SWITCH + epsilon)
                            zoom_step = -(zoom_step / 10);
                        else
                            zoom_step = -ZOOM_STEP;
                    }
                }
                else if (game_state->state == CONTROLS)
                {
                    if (game_state->table_top_row + game_state->table_num_rows_displayed < game_state->table_num_rows)
                    {
                        game_state->table_top_row++;
                    }
                }
            }

            if (game_state->state == UNIVERSE || game_state->state == MAP)
            {
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
                    if (game_state->game_scale + zoom_step <= ZOOM_MAX + epsilon)
                    {
                        nav_state->map_offset.x += ((mouse_x - camera->w / 2)) / (game_state->game_scale) -
                                                   ((mouse_x - camera->w / 2)) / (((game_state->game_scale) + zoom_step));
                        nav_state->map_offset.y += ((mouse_y - camera->h / 2)) / (game_state->game_scale) -
                                                   ((mouse_y - camera->h / 2)) / (((game_state->game_scale) + zoom_step));
                    }
                }
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
                    input_state->camera_on = true;
                break;
            case SDL_SCANCODE_F:
                // Toggle FPS
                input_state->fps_on = !input_state->fps_on;
                break;
            case SDL_SCANCODE_M:
                // Enter Map
                if (game_state->state == UNIVERSE)
                    game_events->is_exiting_universe = true;
                if (game_state->state == NAVIGATE || game_state->state == UNIVERSE)
                {
                    game_change_state(game_state, game_events, MAP);
                    game_events->is_entering_map = true;
                    input_state->camera_on = true;
                }
                break;
            case SDL_SCANCODE_N:
                // Enter Navigate
                if (game_state->state == MAP)
                {
                    game_change_state(game_state, game_events, NAVIGATE);
                    game_events->is_exiting_map = true;
                }
                else if (game_state->state == UNIVERSE)
                {
                    game_change_state(game_state, game_events, NAVIGATE);
                    game_events->is_exiting_universe = true;
                }
                break;
            case SDL_SCANCODE_O:
                // Toggle orbits
                input_state->orbits_on = !input_state->orbits_on;
                break;
            case SDL_SCANCODE_S:
                // Stop ship
                if (game_state->state == NAVIGATE)
                    input_state->stop_on = true;
                break;
            case SDL_SCANCODE_U:
                // Enter Universe
                if (game_state->state == MAP)
                    game_events->is_exiting_map = true;
                if (game_state->state == NAVIGATE || game_state->state == MAP)
                {
                    game_change_state(game_state, game_events, UNIVERSE);
                    game_events->is_entering_universe = true;
                    input_state->camera_on = true;
                }
                break;
            case SDL_SCANCODE_LEFT:
                // Scroll left / Rotate left
                input_state->right_on = false;
                input_state->left_on = true;
                break;
            case SDL_SCANCODE_RIGHT:
                // Scroll right / Rotate right
                input_state->left_on = false;
                input_state->right_on = true;
                break;
            case SDL_SCANCODE_UP:
                // Menu up
                if (game_state->state == MENU)
                {
                    input_state->mouse_position.x = 0;
                    input_state->mouse_position.y = 0;

                    do
                    {
                        input_state->selected_button_index = (input_state->selected_button_index + MENU_BUTTON_COUNT - 1) % MENU_BUTTON_COUNT;
                    } while (game_state->menu[input_state->selected_button_index].disabled);
                }
                else if (game_state->state == CONTROLS)
                {
                    if (game_state->table_top_row > 0)
                    {
                        game_state->table_top_row--;
                    }
                }
                // Scroll up / Thrust
                else
                {
                    input_state->reverse_on = false;
                    input_state->down_on = false;
                    input_state->thrust_on = true;
                    input_state->up_on = true;
                }
                break;
            case SDL_SCANCODE_DOWN:
                // Menu down
                if (game_state->state == MENU)
                {
                    input_state->mouse_position.x = 0;
                    input_state->mouse_position.y = 0;

                    do
                    {
                        input_state->selected_button_index = (input_state->selected_button_index + 1) % MENU_BUTTON_COUNT;
                    } while (game_state->menu[input_state->selected_button_index].disabled);
                }
                else if (game_state->state == CONTROLS)
                {
                    if (game_state->table_top_row + game_state->table_num_rows_displayed < game_state->table_num_rows)
                    {
                        game_state->table_top_row++;
                    }
                }
                // Scroll down / Reverse
                else
                {
                    input_state->thrust_on = false;
                    input_state->up_on = false;
                    input_state->reverse_on = true;
                    input_state->down_on = true;
                }
                break;
            case SDL_SCANCODE_RETURN:
                // Select menu button
                if (game_state->state == MENU || game_state->state == CONTROLS)
                {
                    if (game_state->menu[input_state->selected_button_index].state == RESUME)
                        game_change_state(game_state, game_events, save_state);
                    else
                        game_change_state(game_state, game_events, game_state->menu[input_state->selected_button_index].state);
                }
                break;
            case SDL_SCANCODE_SPACE:
                // Center Map / Center Universe
                if (game_state->state == NAVIGATE)
                    game_events->is_centering_navigate = true;
                else
                {
                    long double zoom_generate_preview_stars = game_zoom_generate_preview_stars(nav_state->current_galaxy->class);

                    if (game_state->game_scale >= zoom_generate_preview_stars - epsilon)
                    {
                        game_events->is_centering_map = true;

                        if (game_state->state == UNIVERSE)
                        {
                            input_state->click_count = 0;
                            game_change_state(game_state, game_events, MAP);
                        }
                    }
                    else
                        game_events->is_centering_universe = true;
                }
                break;
            case SDL_SCANCODE_ESCAPE:
                // Toggle Menu
                if (game_state->state != MENU)
                {
                    save_state = game_state->state;
                    game_change_state(game_state, game_events, MENU);
                }
                else if (game_state->state == MENU && game_events->is_game_started)
                    game_change_state(game_state, game_events, save_state);
                break;
            case SDL_SCANCODE_LEFTBRACKET:
                // Zoom out
                input_state->zoom_in = false;
                input_state->zoom_out = true;
                break;
            case SDL_SCANCODE_RIGHTBRACKET:
                // Zoom in
                input_state->zoom_in = true;
                input_state->zoom_out = false;
                break;
            default:
                break;
            }
            break;
        case SDL_KEYUP:
            switch (event.key.keysym.scancode)
            {
            case SDL_SCANCODE_S:
                input_state->stop_on = false;
                break;
            case SDL_SCANCODE_LEFT:
                input_state->left_on = false;
                break;
            case SDL_SCANCODE_RIGHT:
                input_state->right_on = false;
                break;
            case SDL_SCANCODE_UP:
                input_state->thrust_on = false;
                input_state->up_on = false;
                break;
            case SDL_SCANCODE_DOWN:
                input_state->reverse_on = false;
                input_state->down_on = false;
                break;
            case SDL_SCANCODE_LEFTBRACKET:
                input_state->zoom_out = false;
                break;
            case SDL_SCANCODE_RIGHTBRACKET:
                input_state->zoom_in = false;
                break;
            default:
                break;
            }
            break;
        }
    }
}

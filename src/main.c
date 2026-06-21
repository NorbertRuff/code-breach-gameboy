#include <gb/gb.h>
#include <stdint.h>
#include <stdio.h>

#include "./graphics/background.h"
#include "./graphics/programmer.h"
#include "./utils.h"
#include "./level/level_manager.h"
#include "./player/player.h"
#include "./input/input.h"

#include "./game_state/core_game_loop.h"
#include "./game_state/game_over.h"
#include "./game_state/intro_credits.h"
#include "./game_state/prepare_gameboy.h"
#include "./game_state/start_screen.h"

// Game state variables
uint8_t joypad_current = 0;
uint8_t joypad_previous = 0;
uint16_t scroll_value = 0;
int16_t total_scroll_x = 0;  // Accumulated scroll offset for collision detection
uint16_t text_animation_state = ANIMATION_COUNTER_TIME;
uint8_t current_game_state = INTRO_CREDITS_STATE;

// Player instance
Player player;

// Level manager instance
LevelManager level_manager;

// Main game loop
void main_game_loop()
{
    // Handle input (movement_force only; camera handles scrolling)
    input_handle_joystick_input(&player, joypad_current, joypad_previous, &scroll_value);

    // Update player physics in world coordinates (scroll_x=0: player->x is world X)
    player_handle_movement_physics(&player, &level_manager.current_level, 0);
    player_handle_jumping_physics(&player, &level_manager.current_level, 0);

    // Camera follow: center player on screen, clamped to level bounds
    {
        int16_t cam = (int16_t)player.x - 80;
        if (cam < 0) cam = 0;
        if (cam > (int16_t)MAX_SCROLL_X) cam = (int16_t)MAX_SCROLL_X;
        total_scroll_x = cam;
    }
    move_bkg((uint8_t)total_scroll_x, 0);

    // Update and render level
    level_update(&level_manager);
    level_render(&level_manager);

    // Check collectibles (world coords, no scroll offset needed)
    player_check_collectibles(&player, &level_manager.current_level, 0);

    // Update HUD counter when count changes
    {
        static uint8_t prev_collected = 0xFF;
        uint8_t cur = level_manager.current_level.collected_count;
        if (cur != prev_collected) {
            hud_update_count(cur, level_manager.current_level.collectible_count);
            prev_collected = cur;
        }
    }

    // Compute screen-space draw position from world position then draw
    player.draw_x = (uint16_t)((int16_t)player.x - total_scroll_x);
    player_draw(&player);
}

void main(void)
{
    prepare_gameboy();

    // Feature flag: Skip intro and start screen for development/testing
    #if SKIP_INTRO_AND_START
        // Skip directly to gameplay
        current_game_state = GAMEPLAY_START;
    #endif

    while (1)
    {
        // Update input state
        input_update_joypad_state(&joypad_current, &joypad_previous);
        
        switch (current_game_state)
        {
        case INTRO_CREDITS_STATE:
            handle_intro_credits();
            current_game_state = TITLE_SCREEN_STATE;
            break;
        case TITLE_SCREEN_STATE:
            init_splash_screen();
            // Wait for the player to press start
            while (joypad() != J_START)
            {
                update_splash_screen(&text_animation_state);
            }
            fade_out_screen(10);
            current_game_state = GAMEPLAY_START;
            break;
        case GAMEPLAY_START:
            fade_in_screen(10);
            // Initialize level system (includes HUD and sprite setup)
            level_manager_init(&level_manager);
            level_load_level(&level_manager, 1);  // Load Level 1: Server Room
            // Initialize player at level start position
            player_init(&player, level_manager.current_level.player_start_x, level_manager.current_level.player_start_y);
            current_game_state = COREGAMELOOP;
            break;
        case COREGAMELOOP:
            main_game_loop();
            // current_game_state = GAMEPLAY_END;
            // fade_out_screen(10);
            break;
        case GAMEPLAY_END:
            game_over();
            fade_out_screen(10);
            current_game_state = GAMEPLAY_START;
            break;
        default:
            break;
        }

        // Done processing, yield CPU and wait for start of next frame
        wait_vbl_done();
    }
}

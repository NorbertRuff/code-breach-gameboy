#include <gb/gb.h>
#include <stdint.h>
#include <stdio.h>

#include "./graphics/background.h"
#include "./graphics/programmer.h"
#include "./utils.h"
#include "./player/player.h"
#include "./input/input.h"

#include "./game_state/core_game_loop.h"
#include "./game_state/game_over.h"
#include "./game_state/intro_credits.h"
#include "./game_state/prepare_core_game_loop.h"
#include "./game_state/prepare_gameboy.h"
#include "./game_state/start_screen.h"

// Game state variables
uint8_t joypad_current = 0;
uint8_t joypad_previous = 0;
uint16_t scroll_value = 0;
uint16_t text_animation_state = ANIMATION_COUNTER_TIME;
uint8_t current_game_state = INTRO_CREDITS_STATE;

// Player instance
Player player;

void handle_background_scrolling()
{
    #if DISABLE_BACKGROUND_SCROLLING
        // Background scrolling is disabled - do nothing
        scroll_value = 0;
    #else
        // Normal background scrolling behavior
        scroll_bkg(scroll_value, 0);
        scroll_value = 0;
    #endif
}

// Main game loop
void main_game_loop()
{
    handle_background_scrolling();
    
    // Handle input and update player
    int16_t movement = input_handle_joystick_input(&player, joypad_current, joypad_previous, &scroll_value);
    
    // Update player physics
    player_handle_movement_physics(&player);
    player_handle_jumping_physics(&player);
    
    // Draw player
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
            prepare_core_game_loop();
            fade_in_screen(10);
            // Initialize player
            player_init(&player, 10, 120);
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

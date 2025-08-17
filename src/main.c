#include <gb/gb.h>
#include <stdint.h>
#include <stdio.h>

#include "./graphics/background.h"
#include "./graphics/programmer.h"
#include "./utils.h"

#include "./game_state/core_game_loop.h"
#include "./game_state/game_over.h"
#include "./game_state/intro_credits.h"
#include "./game_state/prepare_core_game_loop.h"
#include "./game_state/prepare_gameboy.h"
#include "./game_state/start_screen.h"

// Feature flag: Set SKIP_INTRO_AND_START to 1 in utils.h to skip intro and start screen
// This is useful for development and testing to get directly to gameplay
// Feature flag: Set EMPTY_BACKGROUND to 1 in utils.h to draw empty background for debugging
// Feature flag: Set DISABLE_BACKGROUND_SCROLLING to 1 in utils.h to disable background scrolling

#define FLOOR_Y 120
#define PROGRAMMER_RUN_FRAMECOUNT 4
#define PROGRAMMER_WALKING_SPEED 20

// Jumping mechanics constants
#define MAX_JUMP_FRAMES 15      // Maximum frames to hold jump
#define JUMP_UPWARD_FORCE -2    // Upward movement per frame while jumping

uint16_t programmer_x;
uint16_t programmer_y;          // Vertical position
uint8_t programmer_is_jumping;  // Jump state flag
uint8_t programmer_jump_frames; // How long we've been jumping

uint16_t programmer_draw_x;
uint16_t programmer_draw_y;     // Drawing Y position
uint16_t programmer_draw_frame;
uint8_t programmer_direction = 0;

uint8_t programmer_frame = 0;

uint8_t joypad_current = 0;
uint8_t joypad_previous = 0;

uint16_t scroll_value = 0;
uint16_t text_animation_state = ANIMATION_COUNTER_TIME;

uint8_t current_game_state = INTRO_CREDITS_STATE;
// uint8_t current_game_state = GAMEPLAY_START;

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

int8_t handle_joystick_input()
{

    int8_t movement = 0;
    if (joypad_current & J_LEFT)
    {
        movement = -1;

        programmer_direction = 0;
        #if !DISABLE_BACKGROUND_SCROLLING
            scroll_value -= 2;
        #endif
    }
    else if (joypad_current & J_RIGHT)
    {
        movement = 1;
        programmer_direction = 1;
        #if !DISABLE_BACKGROUND_SCROLLING
            scroll_value += 2;
        #endif
    }

    // Jump when A button is pressed and we're on the ground
    if ((joypad_current & J_A) && !(joypad_previous & J_A) && !programmer_is_jumping)
    {
        programmer_is_jumping = 1;
        programmer_jump_frames = 0;  // Start jump frame counter
    }
    
    return movement;
}

void handle_jumping_physics()
{
    // If we're jumping and still have frames left
    if (programmer_is_jumping && programmer_jump_frames < MAX_JUMP_FRAMES)
    {
        // Move upward
        programmer_y += JUMP_UPWARD_FORCE;
        programmer_jump_frames++;
    }
    // If we're not jumping or ran out of frames, fall down
    else
    {
        // Simple gravity - fall down
        if (programmer_y < 120)  // Ground level
        {
            programmer_y += 2;  // Fall down
        }
        
        // Check if we've landed
        if (programmer_y >= 120)
        {
            programmer_y = 120;  // Snap to ground
            programmer_is_jumping = 0;  // No longer jumping
            programmer_jump_frames = 0; // Reset jump frames
        }
    }
    
    // Update drawing Y position
    programmer_draw_y = programmer_y;
}

void handle_programmer_animation(int8_t *movement)
{

    programmer_draw_x = programmer_x;
    programmer_draw_frame = programmer_frame;

    // Increase/Decrease the "face" value
    programmer_x += *movement * PROGRAMMER_WALKING_SPEED;

    // Draw programmer at the true value
    programmer_draw_x = programmer_x >> 4;

    // If the player is moving
    if (*movement != 0)
    {

        // Increase programmer's frame by 2
        // This is a sub-pixel resolution value
        programmer_frame += 2;

        // Increase programmer's frame
        // Loop background after 3
        // Shift right 4 bits to get it's true value
        if ((programmer_frame >> 4) >= PROGRAMMER_RUN_FRAMECOUNT)
            programmer_frame = 0;

        // Frame 0 is or standing
        // Increase by one for running frames
        // Shift right 4 bits to get it's true value
        programmer_draw_frame = 1 + programmer_frame >> 4;
    }
    else
    {

        // Draw with our standing frame 0
        programmer_frame = 0;
        programmer_draw_frame = 0;
    }

    // Draw the programmer metasprite
    // Flip the metasprite horizontally (using vflip) if programmer isn' facing right
    if (programmer_direction == 1)
    {
        move_metasprite(programmer_metasprites[programmer_draw_frame], 0, 0, programmer_draw_x, programmer_draw_y);
    }
    else
    {
        move_metasprite_vflip(programmer_metasprites[programmer_draw_frame], 0, 0, programmer_draw_x + 4, programmer_draw_y);
    }
}

// Main game loop
void main_game_loop()
{
    handle_background_scrolling();
    int16_t movement = handle_joystick_input();

    handle_jumping_physics();
    handle_programmer_animation(&movement);
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
        joypad_previous = joypad_current;
        joypad_current = joypad();
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
            // Put programmer in the middle
            programmer_x = 10;
            // Initialize jumping variables
            programmer_y = 120;           // Start on ground
            programmer_is_jumping = 0;    // Not jumping initially
            programmer_jump_frames = 0;   // No jump frames
            programmer_draw_y = 120;      // Drawing Y position
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

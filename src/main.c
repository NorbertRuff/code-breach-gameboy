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
#define PROGRAMMER_RUN_FRAMECOUNT 5      // Updated to match actual available frames (0-4)
#define PROGRAMMER_WALKING_SPEED 20

// Animation system constants
#define ANIMATION_FRAME_LENGTH 4         // How often to change animation frames
#define MOVEMENT_FRAME_LENGTH 8          // Movement timing for smooth acceleration

// Underfoot detection constants
#define FOOT_ON_LAND    0x01U
#define FOOT_IN_AIR     0x02U

// Jumping mechanics constants
#define MAX_JUMP_FRAMES 15      // Maximum frames to hold jump
#define JUMP_UPWARD_FORCE -2    // Upward movement per frame while jumping

uint16_t programmer_x;
uint16_t programmer_y;          // Vertical position
uint8_t programmer_is_jumping;  // Jump state flag
uint8_t programmer_jump_frames; // How long we've been jumping

// Animation system variables
uint8_t programmer_animation_frame_count;  // Counter for animation timing
uint8_t programmer_movement_frame_count;   // Counter for movement timing
uint8_t programmer_last_movement_x;        // Previous movement direction for smooth deceleration

// Underfoot detection variables
uint8_t programmer_underfoot_state;       // Current ground state

// Movement force system variables
int8_t programmer_movement_force_x;       // Desired movement direction (-1, 0, 1)
int8_t programmer_velocity_x;             // Actual movement velocity
uint8_t programmer_slowing_x;             // Deceleration direction for smooth stopping

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
    // Reset movement force
    programmer_movement_force_x = 0;

    if (joypad_current & J_LEFT)
    {
        programmer_movement_force_x = -1;
        programmer_direction = 0;
        #if !DISABLE_BACKGROUND_SCROLLING
            scroll_value -= 2;
        #endif
    }
    else if (joypad_current & J_RIGHT)
    {
        programmer_movement_force_x = 1;
        programmer_direction = 1;
        #if !DISABLE_BACKGROUND_SCROLLING
            scroll_value += 2;
        #endif
    }

    // Jump when A button is pressed and we're on the ground
    if ((joypad_current & J_A) && !(joypad_previous & J_A) && (programmer_underfoot_state & FOOT_ON_LAND))
    {
        programmer_is_jumping = 1;
        programmer_jump_frames = 0;  // Start jump frame counter
    }
    
    return programmer_movement_force_x;
}

void handle_movement_physics()
{
    // Reset frame counters when movement changes
    if ((programmer_last_movement_x != 0 && programmer_movement_force_x == 0) ||
        (programmer_last_movement_x == 0 && programmer_movement_force_x != 0))
    {
        programmer_movement_frame_count = 0;
        programmer_animation_frame_count = 0;
    }

    // Handle deceleration when stopping
    if (programmer_last_movement_x != 0 && programmer_movement_force_x == 0)
    {
        programmer_slowing_x = programmer_last_movement_x > 0 ? 1 : -1;
    }

    // Apply movement force to velocity
    if (programmer_movement_force_x != 0)
    {
        // Direct movement when force is applied
        programmer_velocity_x = programmer_movement_force_x;
        programmer_movement_frame_count++;
        
        // Progress animation counter
        programmer_animation_frame_count++;
        
        // Change animation frame every ANIMATION_FRAME_LENGTH frames
        if (programmer_animation_frame_count % ANIMATION_FRAME_LENGTH == 0)
        {
            // Progress to next animation frame
            // Use all available frames: 0=standing, 1-4=running frames
            if (programmer_draw_frame < 4)  // Use frames 0-4 (5 total frames)
                programmer_draw_frame++;
            else
                programmer_draw_frame = 1; // Start from frame 1 (skip standing frame)
            
        }
    }
    else
    {
        // Handle deceleration
        if (programmer_slowing_x && programmer_movement_frame_count < 3)
        {
            programmer_movement_frame_count++;
            
            // Allow one more frame of movement before stopping
            if (programmer_movement_frame_count == 2)
                programmer_velocity_x = 0;
            else
                programmer_velocity_x = programmer_slowing_x;
        }
        else
        {
            programmer_velocity_x = 0;
            programmer_slowing_x = 0;
        }
        
        // Reset to standing frame when stopped
        programmer_draw_frame = 0;
    }
    
    // Update position based on velocity
    programmer_x += programmer_velocity_x * PROGRAMMER_WALKING_SPEED;
    
    // Update drawing position
    programmer_draw_x = programmer_x >> 4;
    
    // Remember last movement for next frame
    programmer_last_movement_x = programmer_movement_force_x;
}

void handle_jumping_physics()
{
    // Update underfoot state based on Y position
    programmer_underfoot_state = (programmer_y >= 120) ? FOOT_ON_LAND : FOOT_IN_AIR;
    
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
    // Animation is now handled in handle_movement_physics()
    // This function is kept for compatibility but simplified
    
    // Update drawing position (already handled in movement physics)
    // programmer_draw_x and programmer_draw_frame are set in handle_movement_physics()
    
    // Draw the programmer metasprite
    // Flip the metasprite horizontally (using vflip) if programmer isn' facing right
    
    // Safety check: ensure frame is within valid range
    uint8_t safe_frame = programmer_draw_frame;
    if (safe_frame > 4) safe_frame = 0;  // Clamp to valid range 0-4
    
    if (programmer_direction == 1)
    {
        move_metasprite(programmer_metasprites[safe_frame], 0, 0, programmer_draw_x, programmer_draw_y);
    }
    else
    {
        move_metasprite_vflip(programmer_metasprites[safe_frame], 0, 0, programmer_draw_x + 4, programmer_draw_y);
    }
}

// Main game loop
void main_game_loop()
{
    handle_background_scrolling();
    int16_t movement = handle_joystick_input();

    handle_movement_physics();
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
            
            // Initialize animation system
            programmer_animation_frame_count = 0;
            programmer_movement_frame_count = 0;
            programmer_last_movement_x = 0;
            programmer_draw_frame = 0;
            
            // Initialize movement system
            programmer_movement_force_x = 0;
            programmer_velocity_x = 0;
            programmer_slowing_x = 0;
            
            // Initialize underfoot detection
            programmer_underfoot_state = FOOT_ON_LAND;
            
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

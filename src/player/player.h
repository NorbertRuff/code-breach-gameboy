#ifndef PLAYER_H
#define PLAYER_H

#include <gb/gb.h>
#include <stdint.h>

// Player character constants
#define FLOOR_Y 120
#define PROGRAMMER_RUN_FRAMECOUNT 5      // Updated to match actual available frames (0-4)
#define PROGRAMMER_WALKING_SPEED 2       // Increased for faster horizontal movement

// Animation system constants
#define ANIMATION_FRAME_LENGTH 4         // How often to change animation frames (slowed down)
#define MOVEMENT_FRAME_LENGTH 4          // Reduced for more responsive movement
#define DECELERATION_FRAMES 3            // Frames to continue moving after releasing button

// Underfoot detection constants
#define FOOT_ON_LAND    0x01U
#define FOOT_IN_AIR     0x02U

// Jumping mechanics constants (improved from example)
#define MAX_JUMP_FRAMES 15      // Maximum frames to hold jump
#define JUMP_UPWARD_FORCE -1    // Upward movement per frame while jumping
#define MAX_VELOCITY_Y 3        // Maximum vertical velocity
#define MIN_VELOCITY_Y -3       // Minimum vertical velocity

// Player character structure
typedef struct {
    // Position
    uint16_t x;
    uint16_t y;
    uint16_t draw_x;
    uint16_t draw_y;
    
    // Animation
    uint8_t draw_frame;
    uint8_t direction;
    uint8_t animation_frame_count;
    uint8_t movement_frame_count;
    uint8_t last_movement_x;
    
    // Movement
    int8_t movement_force_x;
    int8_t movement_force_y;    // Vertical movement force for jumping
    int8_t velocity_x;
    uint8_t slowing_x;
    
    // Jumping (improved physics)
    uint8_t is_jumping;
    uint8_t jump_frames;
    int8_t velocity_y;          // Vertical velocity for smooth falling
    uint8_t jump_frame_count;   // Counter for jump physics
    
    // State
    uint8_t underfoot_state;
} Player;

// Function declarations
void player_init(Player* player, uint16_t start_x, uint16_t start_y);
void player_handle_movement_physics(Player* player);
void player_handle_jumping_physics(Player* player);
void player_draw(Player* player);
void player_update_underfoot_state(Player* player);
void player_detect_collisions(Player* player, uint16_t* predicted_x, uint16_t* predicted_y);

#endif // PLAYER_H

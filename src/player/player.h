#ifndef PLAYER_H
#define PLAYER_H

#include <gb/gb.h>
#include <stdint.h>

// Player character constants
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
    int8_t velocity_x;
    uint8_t slowing_x;
    
    // Jumping
    uint8_t is_jumping;
    uint8_t jump_frames;
    
    // State
    uint8_t underfoot_state;
} Player;

// Function declarations
void player_init(Player* player, uint16_t start_x, uint16_t start_y);
void player_handle_movement_physics(Player* player);
void player_handle_jumping_physics(Player* player);
void player_draw(Player* player);
void player_update_underfoot_state(Player* player);

#endif // PLAYER_H

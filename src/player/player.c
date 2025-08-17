#include "player.h"
#include "../graphics/programmer.h"
#include <stdio.h>

void player_init(Player* player, uint16_t start_x, uint16_t start_y)
{
    // Initialize position
    player->x = start_x;
    player->y = start_y;
    player->draw_x = start_x;
    player->draw_y = start_y;
    
    // Initialize animation
    player->draw_frame = 0;
    player->direction = 0;
    player->animation_frame_count = 0;
    player->movement_frame_count = 0;
    player->last_movement_x = 0;
    
    // Initialize movement
    player->movement_force_x = 0;
    player->velocity_x = 0;
    player->slowing_x = 0;
    
    // Initialize jumping
    player->is_jumping = 0;
    player->jump_frames = 0;
    
    // Initialize state
    player->underfoot_state = FOOT_ON_LAND;
}

void player_update_underfoot_state(Player* player)
{
    player->underfoot_state = (player->y >= 120) ? FOOT_ON_LAND : FOOT_IN_AIR;
}

void player_handle_movement_physics(Player* player)
{
    // Reset frame counters when movement changes
    if ((player->last_movement_x != 0 && player->movement_force_x == 0) ||
        (player->last_movement_x == 0 && player->movement_force_x != 0))
    {
        player->movement_frame_count = 0;
        player->animation_frame_count = 0;
    }

    // Handle deceleration when stopping
    if (player->last_movement_x != 0 && player->movement_force_x == 0)
    {
        player->slowing_x = player->last_movement_x > 0 ? 1 : -1;
    }

    // Apply movement force to velocity
    if (player->movement_force_x != 0)
    {
        // Direct movement when force is applied
        player->velocity_x = player->movement_force_x;
        player->movement_frame_count++;
        
        // Progress animation counter
        player->animation_frame_count++;
        
        // Change animation frame every ANIMATION_FRAME_LENGTH frames
        if (player->animation_frame_count % ANIMATION_FRAME_LENGTH == 0)
        {
            // Progress to next animation frame
            // Use all available frames: 0=standing, 1-4=running frames
            if (player->draw_frame < 4)  // Use frames 0-4 (5 total frames)
                player->draw_frame++;
            else
                player->draw_frame = 1; // Start from frame 1 (skip standing frame)
        }
    }
    else
    {
        // Handle deceleration
        if (player->slowing_x && player->movement_frame_count < 3)
        {
            player->movement_frame_count++;
            
            // Allow one more frame of movement before stopping
            if (player->movement_frame_count == 2)
                player->velocity_x = 0;
            else
                player->velocity_x = player->slowing_x;
        }
        else
        {
            player->velocity_x = 0;
            player->slowing_x = 0;
        }
        
        // Reset to standing frame when stopped
        player->draw_frame = 0;
    }
    
    // Update position based on velocity
    player->x += player->velocity_x * PROGRAMMER_WALKING_SPEED;
    
    // Update drawing position
    player->draw_x = player->x >> 4;
    
    // Remember last movement for next frame
    player->last_movement_x = player->movement_force_x;
}

void player_handle_jumping_physics(Player* player)
{
    // Update underfoot state based on Y position
    player_update_underfoot_state(player);
    
    // If we're jumping and still have frames left
    if (player->is_jumping && player->jump_frames < MAX_JUMP_FRAMES)
    {
        // Move upward
        player->y += JUMP_UPWARD_FORCE;
        player->jump_frames++;
    }
    // If we're not jumping or ran out of frames, fall down
    else
    {
        // Simple gravity - fall down
        if (player->y < 120)  // Ground level
        {
            player->y += 2;  // Fall down
        }
        
        // Check if we've landed
        if (player->y >= 120)
        {
            player->y = 120;  // Snap to ground
            player->is_jumping = 0;  // No longer jumping
            player->jump_frames = 0; // Reset jump frames
        }
    }
    
    // Update drawing Y position
    player->draw_y = player->y;
}

void player_draw(Player* player)
{
    // Safety check: ensure frame is within valid range
    uint8_t safe_frame = player->draw_frame;
    if (safe_frame > 4) safe_frame = 0;  // Clamp to valid range 0-4
    
    // Draw the programmer metasprite
    // Flip the metasprite horizontally (using vflip) if programmer isn' facing right
    if (player->direction == 1)
    {
        move_metasprite(programmer_metasprites[safe_frame], 0, 0, player->draw_x, player->draw_y);
    }
    else
    {
        move_metasprite_vflip(programmer_metasprites[safe_frame], 0, 0, player->draw_x + 4, player->draw_y);
    }
}

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
    player->movement_force_y = 0;  // Initialize vertical movement force
    player->velocity_x = 0;
    player->slowing_x = 0;
    
    // Initialize jumping (improved physics)
    player->is_jumping = 0;
    player->jump_frames = 0;
    player->velocity_y = 0;
    player->jump_frame_count = 0;
    
    // Initialize state
    player->underfoot_state = FOOT_ON_LAND;
}

void player_update_underfoot_state(Player* player)
{
    player->underfoot_state = (player->y >= FLOOR_Y) ? FOOT_ON_LAND : FOOT_IN_AIR;
}

void player_detect_collisions(Player* player, uint16_t* predicted_x, uint16_t* predicted_y)
{
    // If we hit ground...
    if (*predicted_y >= FLOOR_Y)
    {
        // Snap to ground
        *predicted_y = FLOOR_Y; 

        // Remove velocity
        player->velocity_y = 0;
    }
}

void player_handle_movement_physics(Player* player)
{
    // Reset frame counters when starting movement (but NOT when stopping)
    if (player->last_movement_x == 0 && player->movement_force_x != 0)
    {
        player->movement_frame_count = 0;
        player->animation_frame_count = 0;
    }

    // Flag true when we release from movement (from example)
    if (player->last_movement_x != 0 && player->movement_force_x == 0)
    {
        player->slowing_x = player->last_movement_x > 0 ? 1 : -1;
    }

    // Handle sprite flipping for direction
    if (player->last_movement_x <= 0 && player->movement_force_x > 0)
    {
        player->direction = 1;  // Facing right
    }
    else if (player->last_movement_x >= 0 && player->movement_force_x < 0)
    {
        player->direction = 0;  // Facing left
    }

    // While moving...
    if (player->movement_force_x != 0)
    {
        // Move character by movement force (from example)
        player->velocity_x = player->movement_force_x;

        // Progress movement counter
        player->movement_frame_count++;
        
        // Roll around after max frame
        if (player->movement_frame_count == MOVEMENT_FRAME_LENGTH)
        {
            player->movement_frame_count = 0;
        }
        
        // Progress animation counter
        player->animation_frame_count++;
        
        // Roll around after max frame
        if (player->animation_frame_count == ANIMATION_FRAME_LENGTH)
        {
            player->animation_frame_count = 0;

            // Move to next animation frame
            if (player->draw_frame < 4)  // Use frames 0-4 (5 total frames)
                player->draw_frame++;
            else
                player->draw_frame = 1; // Start from frame 1 (skip standing frame)
        }
    }
    // When no longer moving
    else
    {
        if (player->slowing_x && player->movement_frame_count < DECELERATION_FRAMES)
        {
            player->movement_frame_count++;

            // Allow character to move one frame before stopping (like example)
            if (player->movement_frame_count == DECELERATION_FRAMES - 1)
            {
                player->velocity_x = 0;
                player->slowing_x = 0;
            }
            else
                // Continue to move in the direction we were
                player->velocity_x = player->slowing_x;
        }
        else
        {
            player->velocity_x = 0;
        }
        
        // Reset to standing frame when fully stopped
        if (player->velocity_x == 0)
        {
            player->draw_frame = 0;
        }
    }
    
    // Update position based on velocity
    player->x += player->velocity_x * PROGRAMMER_WALKING_SPEED;
    
    // Update drawing position (removed bit shifting that was dividing by 16)
    player->draw_x = player->x;
    
    // Remember last movement for next frame
    player->last_movement_x = player->movement_force_x;
}

void player_handle_jumping_physics(Player* player)
{
    // Update underfoot state based on Y position
    player_update_underfoot_state(player);
    
    // While jumping...
    if (player->movement_force_y != 0)
    {
        player->velocity_y += player->movement_force_y;
        player->jump_frame_count = 0;
    }
    // Otherwise, if in the air...
    else if (player->underfoot_state & FOOT_IN_AIR)
    {
        // Jump frame count from 0 to 60
        player->jump_frame_count = (player->jump_frame_count + 1) % 60;

        // Apply gravity (from example - variable gravity for smooth feel)
        if (player->jump_frame_count < 5)
            player->velocity_y = -2;
        else if (player->jump_frame_count < 10)
            player->velocity_y = -1;
        else if (player->jump_frame_count < 14)
            player->velocity_y = 0;
        else if (player->jump_frame_count < 20)
            player->velocity_y = 1;
        else
            player->velocity_y = 2;
    }

    // Velocity limits (from example)
    if (player->velocity_y != 0 && player->velocity_y < MIN_VELOCITY_Y)
        player->velocity_y = MIN_VELOCITY_Y;
    if (player->velocity_y != 0 && player->velocity_y > MAX_VELOCITY_Y)
        player->velocity_y = MAX_VELOCITY_Y;
    
    // Calculate future position
    uint16_t predicted_x = player->x;
    uint16_t predicted_y = player->y + player->velocity_y;

    // Update predictions based on collision
    player_detect_collisions(player, &predicted_x, &predicted_y);

    // Move character to new position
    player->y = predicted_y;
    
    // Update drawing Y position
    player->draw_y = player->y;
}

void player_draw(Player* player)
{
    // Safety check: ensure frame is within valid range
    uint8_t safe_frame = player->draw_frame;
    if (safe_frame > 4) safe_frame = 0;  // Clamp to valid range 0-4
    
    // Draw the programmer metasprite
    // Flip the metasprite horizontally (using vflip) if programmer isn't facing right
    if (player->direction == 1)
    {
        move_metasprite(programmer_metasprites[safe_frame], 0, 0, player->draw_x, player->draw_y);
    }
    else
    {
        move_metasprite_vflip(programmer_metasprites[safe_frame], 0, 0, player->draw_x + 4, player->draw_y);
    }
}

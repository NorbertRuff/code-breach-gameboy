#include "player.h"
#include "../graphics/programmer.h"
#include "../level/level_manager.h"

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
    player->invincibility_frames = 0;
}

void player_update_underfoot_state(Player* player, Level* current_level, uint16_t scroll_x)
{
    // Check one pixel below, at both sprite edges
    uint16_t left_x  = (player->x > SPRITE_LEFT_W) ? player->x - SPRITE_LEFT_W : 0;
    uint16_t right_x = player->x + SPRITE_RIGHT_W;
    uint8_t col_left  = level_check_collision(current_level, left_x,  player->y + 1, scroll_x);
    uint8_t col_right = level_check_collision(current_level, right_x, player->y + 1, scroll_x);
    player->underfoot_state = (col_left == COLLISION_SOLID || col_right == COLLISION_SOLID)
                              ? FOOT_ON_LAND : FOOT_IN_AIR;
}

void player_detect_collisions(Player* player, uint16_t* predicted_x, uint16_t* predicted_y, Level* current_level, uint16_t scroll_x)
{
    uint16_t check_x_left  = (*predicted_x > SPRITE_LEFT_W) ? *predicted_x - SPRITE_LEFT_W : 0;
    uint16_t check_x_right = *predicted_x + SPRITE_RIGHT_W;

    if (player->velocity_y > 0) {
        // Falling — check feet position
        uint8_t col_left  = level_check_collision(current_level, check_x_left,  *predicted_y, scroll_x);
        uint8_t col_right = level_check_collision(current_level, check_x_right, *predicted_y, scroll_x);
        if (col_left == COLLISION_SOLID || col_right == COLLISION_SOLID) {
            uint8_t tile_y = *predicted_y / TILE_SIZE;
            *predicted_y = (tile_y * TILE_SIZE) - 1;
            player->velocity_y = 0;
            player->underfoot_state = FOOT_ON_LAND;
        } else {
            player->underfoot_state = FOOT_IN_AIR;
        }
    } else if (player->velocity_y < 0) {
        // Rising — check head position (SPRITE_HEAD_OFFSET above pivot)
        uint16_t head_y = (*predicted_y > SPRITE_HEAD_OFFSET) ? *predicted_y - SPRITE_HEAD_OFFSET : 0;
        uint8_t col_left  = level_check_collision(current_level, check_x_left,  head_y, scroll_x);
        uint8_t col_right = level_check_collision(current_level, check_x_right, head_y, scroll_x);
        if (col_left == COLLISION_SOLID || col_right == COLLISION_SOLID) {
            uint8_t tile_y = head_y / TILE_SIZE;
            // Push feet down so head sits just below the tile bottom
            *predicted_y = ((tile_y + 1) * TILE_SIZE) + SPRITE_HEAD_OFFSET;
            player->velocity_y = 0;
        }
    }

    // Collectible pickup is handled separately by player_check_collectibles
}

void player_handle_movement_physics(Player* player, Level* current_level, uint16_t scroll_x)
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
        
        // Reset to standing frame when fully stopped (but not when jumping)
        if (player->velocity_x == 0 && !player->is_jumping)
        {
            player->draw_frame = 0;
        }
    }
    
    // Check horizontal collision at body height (not feet — feet are on the ground row)
    int16_t new_x_signed = (int16_t)player->x + (player->velocity_x * PROGRAMMER_WALKING_SPEED);

    // Clamp to level world bounds
    if (new_x_signed < 0) new_x_signed = 0;
    if (new_x_signed > (int16_t)(LEVEL_WIDTH * TILE_SIZE - SPRITE_RIGHT_W)) new_x_signed = (int16_t)(LEVEL_WIDTH * TILE_SIZE - SPRITE_RIGHT_W);

    uint16_t new_x = (uint16_t)new_x_signed;
    uint16_t body_y = (player->y > TILE_SIZE) ? player->y - TILE_SIZE : player->y;
    uint8_t collision_x = level_check_collision(current_level, new_x, body_y, scroll_x);

    if (collision_x == COLLISION_SOLID) {
        player->velocity_x = 0;
    } else {
        player->x = new_x;
    }
    
    // Update drawing position
    player->draw_x = player->x;
    
    // Remember last movement for next frame
    player->last_movement_x = player->movement_force_x;
}

void player_handle_jumping_physics(Player* player, Level* current_level, uint16_t scroll_x)
{
    // Update underfoot state based on Y position
    player_update_underfoot_state(player, current_level, scroll_x);
    
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
    player_detect_collisions(player, &predicted_x, &predicted_y, current_level, scroll_x);

    // Move character to new position
    player->y = predicted_y;

    // Update drawing Y position
    player->draw_y = player->y;
}

static void check_collect_at(Level* level, uint8_t tile_x, uint8_t tile_y) {
    if (tile_x < LEVEL_WIDTH && tile_y < LEVEL_HEIGHT) {
        if (level->collision_map[tile_y][tile_x] == COLLISION_COLLECTIBLE) {
            level_handle_collision(level, COLLISION_COLLECTIBLE, tile_x, tile_y);
        }
    }
}

void player_check_collectibles(Player* player, Level* current_level, uint16_t scroll_x)
{
    // Sprite occupies player->x-8 to player->x+8 horizontally
    // Check left and right tile columns at two vertical positions (body and feet)
    uint8_t tile_x_left  = ((player->x > SPRITE_LEFT_W ? player->x - SPRITE_LEFT_W : 0) + scroll_x) / TILE_SIZE;
    uint8_t tile_x_right = (player->x + SPRITE_RIGHT_W + scroll_x) / TILE_SIZE;

    uint8_t tile_y_feet = player->y / TILE_SIZE;
    check_collect_at(current_level, tile_x_left,  tile_y_feet);
    check_collect_at(current_level, tile_x_right, tile_y_feet);

    if (player->y >= TILE_SIZE) {
        uint8_t tile_y_body = (player->y - TILE_SIZE) / TILE_SIZE;
        check_collect_at(current_level, tile_x_left,  tile_y_body);
        check_collect_at(current_level, tile_x_right, tile_y_body);
    }
}

void player_draw(Player* player)
{
    // Flash sprite while invincible: hide every other 2-frame window
    if (player->invincibility_frames > 0 && (player->invincibility_frames & 2)) {
        move_sprite(0, 0, 0); move_sprite(1, 0, 0);
        move_sprite(2, 0, 0); move_sprite(3, 0, 0);
        return;
    }

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

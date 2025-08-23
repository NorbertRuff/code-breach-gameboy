#include "input.h"
#include "../utils.h"

void input_update_joypad_state(uint8_t* joypad_current, uint8_t* joypad_previous)
{
    *joypad_previous = *joypad_current;
    *joypad_current = joypad();
}

int8_t input_handle_joystick_input(Player* player, uint8_t joypad_current, uint8_t joypad_previous, uint16_t* scroll_value)
{
    // Reset movement force
    player->movement_force_x = 0;
    player->movement_force_y = 0;  // Reset vertical movement force

    if (joypad_current & J_LEFT)
    {
        player->movement_force_x = -1;
        player->direction = 0;
        #if !DISABLE_BACKGROUND_SCROLLING
            *scroll_value -= 2;
        #endif
    }
    else if (joypad_current & J_RIGHT)
    {
        player->movement_force_x = 1;
        player->direction = 1;
        #if !DISABLE_BACKGROUND_SCROLLING
            *scroll_value += 2;
        #endif
    }

    // Jump mechanics (from example code)
    // Have we jumped?
    if ((player->underfoot_state & FOOT_ON_LAND) && (joypad_current & J_A))
    {
        // Mark character as jumped
        player->is_jumping = 1;
        player->jump_frames = 0;
    }
    // If we are on land again after jumping...
    else if (player->underfoot_state & FOOT_ON_LAND)
    {
        // Reset jump height
        player->jump_frames = 0;

        // Reset has jumped
        player->is_jumping = 0;
    }

    // End jumping if not pressing A or gone above frames
    if (!(joypad_current & J_A) || player->jump_frames > MAX_JUMP_FRAMES)
    {
        player->is_jumping = 0;
    }

    // If we have jumped and still have button pressed...
    if (player->is_jumping && (joypad_current & J_A))
    {
        // Apply upward velocity
        player->movement_force_y = -1;

        // Keep track of jumped height
        player->jump_frames++;
    }
    
    return player->movement_force_x;
}

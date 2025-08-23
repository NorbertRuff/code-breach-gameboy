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

    // Jump when A button is pressed and we're on the ground
    if ((joypad_current & J_A) && !(joypad_previous & J_A) && (player->underfoot_state & FOOT_ON_LAND))
    {
        player->is_jumping = 1;
        player->jump_frames = 0;  // Start jump frame counter
    }
    
    return player->movement_force_x;
}

#include "input.h"

// Remembers last frame's buttons and reads this frame's.
void input_update_joypad_state(uint8_t* joypad_current, uint8_t* joypad_previous)
{
    *joypad_previous = *joypad_current;
    *joypad_current = joypad();
}

// Turns the buttons into the player's input fields. player_update() does the rest.
void input_read_player(Player* player, uint8_t joypad_current, uint8_t joypad_previous)
{
    if (joypad_current & J_LEFT) player->input_x = -1;
    else if (joypad_current & J_RIGHT) player->input_x = 1;
    else player->input_x = 0;

    player->down_held = (joypad_current & J_DOWN) != 0;
    player->jump_held = (joypad_current & J_A) != 0;

    // A new press of A (not a held one) is remembered for a few frames: the jump buffer.
    if ((joypad_current & J_A) && !(joypad_previous & J_A)) {
        player->jump_buffer_frames = JUMP_BUFFER_FRAMES;
    }
}

#ifndef INPUT_H
#define INPUT_H

#include <gb/gb.h>
#include <stdint.h>
#include "../player/player.h"

// Jumping mechanics constants
#define MAX_JUMP_FRAMES 15

// Input handling function declarations
void input_update_joypad_state(uint8_t* joypad_current, uint8_t* joypad_previous);
int8_t input_handle_joystick_input(Player* player, uint8_t joypad_current, uint8_t joypad_previous, uint16_t* scroll_value);

#endif // INPUT_H

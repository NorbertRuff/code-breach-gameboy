#ifndef INPUT_H
#define INPUT_H

#include <gb/gb.h>
#include <stdint.h>
#include "../player/player.h"

void input_update_joypad_state(uint8_t* joypad_current, uint8_t* joypad_previous);
void input_read_player(Player* player, uint8_t joypad_current, uint8_t joypad_previous);

#endif // INPUT_H

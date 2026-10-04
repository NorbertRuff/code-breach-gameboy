#ifndef PAUSE_H
#define PAUSE_H

#include <gb/gb.h>
#include <stdint.h>

void pause_load_tiles(void) BANKED;
uint8_t pause_draw(uint8_t first_sprite) BANKED;

#endif // PAUSE_H

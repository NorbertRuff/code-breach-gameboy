// Not per-frame code: in a ROM bank with its tiles, to keep bank 0 free.
#pragma bank 255

#include <gb/gb.h>
#include "pause.h"

#define PAUSE_FIRST_TILE 44   // sprite tiles after the bot's (40-43)
#define PAUSE_LETTERS 5
#define PAUSE_LEFT 60         // screen px: the label's 40 px centered on 160
#define PAUSE_TOP 8           // in the parallax band, where no bot goes: at most 7 sprites a line

// "PAUSE" in the style of the HUD digits. Each letter is an 8x16 sprite: the letter's
// tile, then a blank one.
static const uint8_t pause_tiles[PAUSE_LETTERS * 2 * 16] = {
    // P
    0x7C,0x7C, 0x66,0x66, 0x66,0x66, 0x7C,0x7C, 0x60,0x60, 0x60,0x60, 0x60,0x60, 0x00,0x00,
    0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
    // A
    0x3C,0x3C, 0x66,0x66, 0x66,0x66, 0x7E,0x7E, 0x66,0x66, 0x66,0x66, 0x66,0x66, 0x00,0x00,
    0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
    // U
    0x66,0x66, 0x66,0x66, 0x66,0x66, 0x66,0x66, 0x66,0x66, 0x66,0x66, 0x3C,0x3C, 0x00,0x00,
    0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
    // S
    0x3C,0x3C, 0x66,0x66, 0x60,0x60, 0x3C,0x3C, 0x06,0x06, 0x66,0x66, 0x3C,0x3C, 0x00,0x00,
    0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
    // E
    0x7E,0x7E, 0x60,0x60, 0x60,0x60, 0x7C,0x7C, 0x60,0x60, 0x60,0x60, 0x7E,0x7E, 0x00,0x00,
    0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
};

// Loads the label's letters into sprite VRAM.
void pause_load_tiles(void) BANKED
{
    set_sprite_data(PAUSE_FIRST_TILE, PAUSE_LETTERS * 2, pause_tiles);
}

// Shows "PAUSE" in the middle of the screen with the hardware sprites from first_sprite
// on. Returns the next free sprite.
uint8_t pause_draw(uint8_t first_sprite) BANKED
{
    for (uint8_t i = 0; i < PAUSE_LETTERS; i++, first_sprite++) {
        set_sprite_tile(first_sprite, PAUSE_FIRST_TILE + 2 * i);
        set_sprite_prop(first_sprite, 0);
        move_sprite(first_sprite, PAUSE_LEFT + 8 * i + DEVICE_SPRITE_PX_OFFSET_X,
                    PAUSE_TOP + DEVICE_SPRITE_PX_OFFSET_Y);
    }
    return first_sprite;
}

// Hand-made HUD tiles (src/graphics/hud_digits.c), until the game font (M5).
#ifndef HUD_DIGITS_H
#define HUD_DIGITS_H

#include <stdint.h>
#include <gbdk/platform.h>

#define hud_digits_TILE_COUNT 12   // '0'-'9', '/', heart

BANKREF_EXTERN(hud_digits)
extern const uint8_t hud_digits_tiles[hud_digits_TILE_COUNT * 16];

#endif

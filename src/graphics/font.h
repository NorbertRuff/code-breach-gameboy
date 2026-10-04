// Hand-made font (src/graphics/font.c), until the game font (M5).
#ifndef FONT_H
#define FONT_H

#include <stdint.h>
#include <gbdk/platform.h>

// The characters, in tile order: the font's tile n shows FONT_CHARS[n].
#define FONT_CHARS " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?()/:-#%"
#define font_TILE_COUNT 48

BANKREF_EXTERN(font)
extern const uint8_t font_tiles[font_TILE_COUNT * 16];

#endif

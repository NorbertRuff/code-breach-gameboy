#include <gb/gb.h>
#include "text.h"
#include "banking.h"
#include "../graphics/font.h"

#define MAX_LINE 20   // characters across the screen

static const char font_chars[] = FONT_CHARS;
_Static_assert(sizeof(font_chars) - 1 == FONT_CHAR_COUNT, "FONT_CHAR_COUNT must match FONT_CHARS");

// Loads the font's 1-bit tiles into background tile memory: ink in color 3 (darkest),
// the rest color 0 (lightest).
void text_load_font(void)
{
    set_1bpp_colors(DMG_BLACK, DMG_WHITE);
    WITH_BANK(font, set_bkg_1bpp_data(FONT_FIRST_TILE, FONT_CHAR_COUNT, font_tiles));
}

// The font tile of a character: lowercase shows as uppercase, and anything the font
// doesn't have as a space.
static uint8_t char_tile(char c)
{
    if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
    for (uint8_t i = 0; font_chars[i]; i++) {
        if (font_chars[i] == c) return FONT_FIRST_TILE + i;
    }
    return TEXT_SPACE_TILE;
}

// Writes text into the background map from tile (x, y), on one row.
void text_print(uint8_t x, uint8_t y, const char* text)
{
    uint8_t tiles[MAX_LINE];
    uint8_t length = 0;
    for (; *text && length < MAX_LINE; text++, length++) {
        tiles[length] = char_tile(*text);
    }
    set_bkg_tiles(x, y, length, 1, tiles);
}

// Puts the two digit tiles of a value, capped at 99, into tiles[0] and tiles[1]. Counts
// the tens by subtracting: the CPU has no division, and the library's takes about 6
// screen lines.
void text_two_digit_tiles(uint8_t value, uint8_t* tiles)
{
    uint8_t tens = 0;
    if (value > 99) value = 99;
    while (value >= 10) {
        value -= 10;
        tens++;
    }
    tiles[0] = FONT_DIGIT_TILE + tens;
    tiles[1] = FONT_DIGIT_TILE + value;
}

// Writes a value 0-99 as two digits from tile (x, y).
void text_print_two_digits(uint8_t x, uint8_t y, uint8_t value)
{
    uint8_t tiles[2];
    text_two_digit_tiles(value, tiles);
    set_bkg_tiles(x, y, 2, 1, tiles);
}

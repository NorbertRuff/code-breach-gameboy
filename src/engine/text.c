#include <gb/gb.h>
#include "text.h"
#include "banking.h"
#include "../graphics/font.h"

#define MAX_LINE 20   // characters across the screen

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
    static const char chars[] = FONT_CHARS;
    if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
    for (uint8_t i = 0; chars[i]; i++) {
        if (chars[i] == c) return FONT_FIRST_TILE + i;
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

// Writes a value 0-99 as two digits from tile (x, y). Counts the tens by subtracting: the
// CPU has no division.
void text_print_two_digits(uint8_t x, uint8_t y, uint8_t value)
{
    char digits[3] = { '0', '0', 0 };
    while (value >= 10 && digits[0] < '9') {
        value -= 10;
        digits[0]++;
    }
    digits[1] += value;
    text_print(x, y, digits);
}

#ifndef TEXT_H
#define TEXT_H

#include <stdint.h>

// Screen text in the game's font, one background tile per character. The font's tile n
// shows FONT_CHARS[n] (graphics/font/font.png, in this order).
#define FONT_CHARS " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?()/:-#%>"
#define FONT_CHAR_COUNT 49                     // FONT_CHARS's length, checked in text.c
#define FONT_FIRST_TILE 160                    // background tiles 160-208
#define TEXT_SPACE_TILE FONT_FIRST_TILE
#define FONT_DIGIT_TILE (uint8_t)(FONT_FIRST_TILE + 27)  // '0'; '1' to '9' follow
#define FONT_SLASH_TILE (uint8_t)(FONT_FIRST_TILE + 43)  // '/'

void text_load_font(void);
void text_print(uint8_t x, uint8_t y, const char* text);
void text_print_two_digits(uint8_t x, uint8_t y, uint8_t value);

#endif // TEXT_H

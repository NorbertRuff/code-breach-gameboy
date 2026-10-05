#ifndef TEXT_H
#define TEXT_H

#include <stdint.h>

// Screen text in the game's font, one background tile per character.
#define FONT_FIRST_TILE 160   // background tiles 160-208, after the level's and the HUD's
#define TEXT_SPACE_TILE FONT_FIRST_TILE

void text_load_font(void);
void text_print(uint8_t x, uint8_t y, const char* text);
void text_print_two_digits(uint8_t x, uint8_t y, uint8_t value);

#endif // TEXT_H

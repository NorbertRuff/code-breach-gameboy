#ifndef PALETTE_H
#define PALETTE_H

#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>

// The only module that writes palettes, on both systems (docs/CGB_PLAN.md section 5).
// A fade darkens the background (and the HUD window) in steps from normal to black.
#define PALETTE_NORMAL 0
#define PALETTE_BLACK  3
#define SCREEN_FADE_FRAMES 10   // per fade step between screens

// GBC background palettes: the level's tiles, the parallax band, the HUD window.
#define LEVEL_PALETTE 0
#define BAND_PALETTE  1
#define HUD_PALETTE   2

extern uint8_t palette_fade_step;   // PALETTE_NORMAL to PALETTE_BLACK

void palette_init(void);
void palette_set_bkg(uint8_t first, uint8_t count, const palette_color_t* colors);
void palette_set_sprite(uint8_t first, uint8_t count, const palette_color_t* colors);
void palette_set_pale_sprite(uint8_t target, uint8_t source);
void palette_use_bkg(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t palette);
void palette_use_win(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t palette);
void palette_fade_to(uint8_t step);
void palette_fade_out(uint8_t frames_per_step);
void palette_fade_in(uint8_t frames_per_step);
void palette_flash(uint8_t count, uint8_t hold);

#endif // PALETTE_H

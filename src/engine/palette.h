#ifndef PALETTE_H
#define PALETTE_H

#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>

// The only module that writes palettes, on both systems (docs/ARCHITECTURE.md, "Palettes and fades"):
// palette.c (bank 0) and palette_effects.c (a ROM bank). A fade darkens the background
// (and the HUD window) in steps from normal to black.
#define PALETTE_NORMAL 0
#define PALETTE_BLACK  3
#define SCREEN_FADE_FRAMES 10   // per fade step between screens

// GBC background palettes: the level's tileset from 0 (up to 4), the parallax band base,
// the HUD window chassis, the HUD accents (heart and floppy disk icons), and the band accents.
#define LEVEL_PALETTE       0
#define BAND_PALETTE        4
#define HUD_PALETTE         5
#define HUD_ACCENT_PALETTE  6
#define BAND_ACCENT_PALETTE 7

// GBC sprite palettes: 0 the player, 1 the pale invincible player (player.h), then the
// actors': the enemies' (bots), the harmless objects' (lifts), and the chaser drones.
#define ENEMY_PALETTE  2
#define OBJECT_PALETTE 3
#define CHASER_PALETTE 4

#define PALETTE_COUNT 8    // per kind (background, sprites) on GBC
#define PALETTE_COLORS 4

extern uint8_t palette_fade_step;   // PALETTE_NORMAL to PALETTE_BLACK

// palette.c, bank 0: they read ROM colors inside the caller's WITH_BANK.
void palette_set_bkg(uint8_t first, uint8_t count, const palette_color_t* colors);
void palette_set_sprite(uint8_t first, uint8_t count, const palette_color_t* colors);

// palette_effects.c, in a ROM bank.
void palette_init(void) BANKED;
void palette_set_pale_sprite(uint8_t target, uint8_t source) BANKED;
void palette_use_bkg(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t palette) BANKED;
void palette_use_grays(void) BANKED;
void palette_use_terminal(void) BANKED;
void palette_use_win(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t palette) BANKED;
void palette_fade_to(uint8_t step) BANKED;
void palette_fade_out(uint8_t frames_per_step) BANKED;
void palette_fade_in(uint8_t frames_per_step) BANKED;
void palette_flash(uint8_t count, uint8_t hold) BANKED;

// Shared by the two files only: the palettes as set, before any fade, and the write of
// the background palettes at the current fade step.
extern palette_color_t bkg_colors[PALETTE_COUNT * PALETTE_COLORS];
extern palette_color_t sprite_colors[PALETTE_COUNT * PALETTE_COLORS];
void palette_write_bkg(uint8_t first, uint8_t count);

#endif // PALETTE_H

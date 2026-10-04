// Runs once at boot: in a ROM bank, to keep bank 0 free.
#pragma bank 255

#include <gb/gb.h>

#include "prepare_gameboy.h"
#include "../engine/palette.h"
#include "../engine/platform.h"
#include "../engine/screen.h"

// Turns on the screen, the background and the sprites, in 8x16 mode.
static void setup_display(void)
{
    DISPLAY_ON;
    SHOW_BKG;
    SHOW_SPRITES;
    SPRITES_8x16;
}

// Turns on the sound chip at full volume, all four channels on both speakers.
static void setup_sound(void)
{
    NR52_REG = 0x80;  // sound on: first, the other registers ignore writes while it's off
    NR50_REG = 0x77;  // volume 7 of 7, left and right
    NR51_REG = 0xFF;  // every channel to both speakers
}

// Sets up the hardware at boot: the palettes (before the display turns on, so nothing
// shows white on GBC), the display, the screen splits and the sound.
void prepare_gameboy(void) BANKED
{
    platform_init();
    palette_init();
    setup_display();
    screen_start_splits();
    setup_sound();
}

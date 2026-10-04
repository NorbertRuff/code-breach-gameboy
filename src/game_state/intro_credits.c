// Runs once at boot: in a ROM bank, to keep bank 0 free for the per-frame code.
#pragma bank 255

#include <gb/gb.h>

#include "intro_credits.h"
#include "../engine/text.h"
#include "../engine/palette.h"
#include "../utils.h"

#define CREDITS_SCROLL_STEPS 32   // 4 px each: the text moves 128 px up

// Draws the credits screen: a blank screen in the game's font, and four lines.
static void print_credits(void)
{
    text_load_font();
    fill_bkg_rect(0, 0, DEVICE_SCREEN_WIDTH, DEVICE_SCREEN_HEIGHT, TEXT_SPACE_TILE);
    text_print(4, 2, "Developed By");
    text_print(4, 6, "Norbert Ruff");
    text_print(4, 10, "v0.2.0 (dev)");
    text_print(8, 15, "2026");
}

// Fades the credits in, holds them, scrolls them up off the screen and fades out.
void play_intro_credits(void) BANKED
{
    print_credits();
    palette_fade_in(10);
    wait_frames(200);

    for (uint8_t i = 0; i < CREDITS_SCROLL_STEPS; i++) {
        scroll_bkg(0, 4);
        wait_frames(5);
    }

    wait_frames(50);
    palette_fade_out(10);
    scroll_bkg(0, -4 * CREDITS_SCROLL_STEPS);   // back to the top for the title screen
}

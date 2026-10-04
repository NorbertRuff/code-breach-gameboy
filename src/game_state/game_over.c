// Not per-frame code: in a ROM bank, to keep bank 0 free.
#pragma bank 255

#include <gb/gb.h>
#include "game_over.h"
#include "../engine/palette.h"
#include "../engine/screen.h"
#include "../engine/text.h"
#include "../level/level_manager.h"
#include "../utils.h"

#define MESSAGE_FRAMES 300   // a message stays 5 s, unless START is pressed sooner

// Blanks the play area (the band and the HUD stay) for a message, behind a dark screen.
static void clear_play_area(void)
{
    palette_fade_out(SCREEN_FADE_FRAMES);
    screen_show_sprites(0);
    screen_scroll(0);
    text_load_font();
    fill_bkg_rect(0, LEVEL_MAP_TOP_ROW, DEVICE_SCREEN_WIDTH, LEVEL_MAP_ROWS, TEXT_SPACE_TILE);
}

// Fades the message in and waits for START, or MESSAGE_FRAMES.
static void show_message(void)
{
    text_print(4, 13, "PRESS START");
    palette_fade_in(SCREEN_FADE_FRAMES);
    for (uint16_t frame = 0; frame < MESSAGE_FRAMES && !(joypad() & J_START); frame++) {
        wait_vbl_done();
    }
}

// Game over: 4 slow flashes, then "GAME OVER" until START. The section restarts after.
void game_over(void) BANKED
{
    palette_flash(4, 12);
    clear_play_area();
    text_print(5, 7, "GAME OVER");
    show_message();
}

// Level complete: 6 flashes at 3 per second, then the level's fragments until START.
void level_complete(void) BANKED
{
    uint8_t collected = current_level.section_collected[0] + current_level.section_collected[1]
                      + current_level.section_collected[2];
    palette_flash(6, 10);
    clear_play_area();
    text_print(3, 5, "LEVEL COMPLETE");
    text_print(2, 8, "FRAGMENTS");
    text_print_two_digits(12, 8, collected);
    text_print(14, 8, "/");
    text_print_two_digits(15, 8, current_level.fragment_count);
    if (collected == current_level.fragment_count) text_print(8, 10, "100%");
    show_message();
}

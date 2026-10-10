// Not per-frame code: in a ROM bank, to keep bank 0 free.
#pragma bank 255

#include <gb/gb.h>
#include "game_over.h"
#include "menu.h"
#include "progress.h"
#include "../engine/palette.h"
#include "../engine/screen.h"
#include "../engine/text.h"
#include "../level/level_manager.h"
#include "../sound/music.h"
#include "../utils.h"

#define MESSAGE_FRAMES 300   // a message stays 5 s, unless START is pressed sooner
#define CONTINUE 0           // the game over choices: CONTINUE, then QUIT

// Blanks the play area (the band and the HUD stay) for a message, behind a dark screen.
static void clear_play_area(void)
{
    palette_fade_out(SCREEN_FADE_FRAMES);
    screen_show_sprites(0);
    screen_scroll(0);
    text_load_font();
    fill_bkg_rect(0, LEVEL_MAP_TOP_ROW, DEVICE_SCREEN_WIDTH, LEVEL_MAP_ROWS, TEXT_SPACE_TILE);
    palette_use_bkg(0, LEVEL_MAP_TOP_ROW, DEVICE_SCREEN_WIDTH, LEVEL_MAP_ROWS, LEVEL_PALETTE);
}

// Opens the elevator doors at the exit, its bottom-left tile: draws the tileset's open
// elevator over the closed one in the map. Tilesets without one leave the exit as it is.
static void open_exit(void)
{
    if (!current_tileset.exit_open[0]) return;
    const uint8_t* tile = current_tileset.exit_open;
    for (uint8_t row = current_level.exit_row - 3; row <= current_level.exit_row; row++) {
        level_show_tile(current_level.exit_column, row, *tile++);
        level_show_tile(current_level.exit_column + 1, row, *tile++);
    }
}

// Fades the message in and waits for START, or MESSAGE_FRAMES.
static void show_message(void)
{
    text_print(4, 14, "PRESS START");
    palette_fade_in(SCREEN_FADE_FRAMES);
    for (uint16_t frame = 0; frame < MESSAGE_FRAMES && !(joypad() & J_START); frame++) {
        wait_vbl_done();
    }
}

// Game over: 4 slow flashes, then its song, "GAME OVER", the password, and a choice:
// CONTINUE (returns 1: the section restarts) or QUIT (returns 0: to the title).
uint8_t game_over(void) BANKED
{
    palette_flash(4, 12);
    music_play_game_over();
    clear_play_area();
    text_print(5, 5, "GAME OVER");
    text_print(7, 8, "CONTINUE");
    text_print(7, 10, "QUIT");
    menu_print_password(3, 13);
    return menu_choose(5, 8, 2, CONTINUE) == CONTINUE;
}

// Level complete: notes it in the progress; the elevator opens, 6 flashes at 3 per
// second, then its song, the patch it compiled (game levels only), the level's fragments
// and the new password until START.
void level_complete(void) BANKED
{
    uint8_t collected = current_level.section_collected[0] + current_level.section_collected[1]
                      + current_level.section_collected[2];
    uint8_t perfect = collected == current_level.fragment_count;
    progress_finish_level(level_index, perfect);
    open_exit();
    palette_flash(6, 10);
    music_play_level_complete();
    clear_play_area();
    text_print(3, 4, "LEVEL COMPLETE");
    if (level_index < GAME_LEVEL_COUNT) {
        char patch[] = "PATCH 1/5 COMPILED";
        patch[6] += level_index;
        text_print(1, 6, patch);
    }
    text_print(2, 8, "FRAGMENTS");
    text_print_two_digits(12, 8, collected);
    text_print(14, 8, "/");
    text_print_two_digits(15, 8, current_level.fragment_count);
    if (perfect) text_print(8, 10, "100%");
    menu_print_password(3, 12);
    show_message();
}

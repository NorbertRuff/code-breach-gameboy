#pragma bank 255

#include <gb/gb.h>

#include "start_screen.h"
#include "menu.h"
#include "title_menu.h"
#include "../graphics/title_top.h"
#include "../graphics/title_bottom.h"
#include "../engine/banking.h"
#include "../engine/palette.h"
#include "../engine/text.h"
#include "../sound/music.h"
#include "../utils.h"

#define TITLE_TOP_ROWS (title_top_HEIGHT >> 3)
#define TITLE_BOTTOM_ROWS (title_bottom_HEIGHT >> 3)
#define TITLE_BOTTOM_FIRST_TILE (uint8_t)(FONT_FIRST_TILE + FONT_CHAR_COUNT)   // after the font

// Draws the title picture: the top part's tiles from 0, the bottom part's after the font,
// which the title menu prints with. In bank 0, to read the picture's banks.
static void draw_title_picture(void) NONBANKED
{
    WITH_BANK(title_top, {
        set_bkg_data(0, title_top_TILE_COUNT, title_top_tiles);
        set_bkg_tiles(0, 0, DEVICE_SCREEN_WIDTH, TITLE_TOP_ROWS, title_top_map);
        palette_set_bkg(0, 1, title_top_palettes);
    });
    WITH_BANK(title_bottom, {
        set_bkg_data(TITLE_BOTTOM_FIRST_TILE, title_bottom_TILE_COUNT, title_bottom_tiles);
        set_bkg_based_tiles(0, TITLE_TOP_ROWS, DEVICE_SCREEN_WIDTH, TITLE_BOTTOM_ROWS, title_bottom_map, TITLE_BOTTOM_FIRST_TILE);
    });
}

// Shows the title screen until START is pressed, then its menu; the title again after
// backing out of the password screen. Returns with the level to start chosen.
void run_title_screen(void) BANKED
{
    music_play_title();
    do {
        menu_open_screen();
        draw_title_picture();
        palette_fade_in(10);
        menu_start_input();
        while (!(menu_wait_press() & J_START));
    } while (!title_menu());
}

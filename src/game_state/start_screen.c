#pragma bank 255

#include <gb/gb.h>

#include "start_screen.h"
#include "menu.h"
#include "demo.h"
#include "title_menu.h"
#include "../graphics/title.h"
#include "../graphics/title_warning.h"
#include "../engine/banking.h"
#include "../engine/palette.h"
#include "../engine/platform.h"
#include "../engine/text.h"
#include "../sound/music.h"

#define TITLE_ROWS (title_HEIGHT >> 3)
#define WARNING_FIRST_TILE (uint8_t)(FONT_FIRST_TILE + FONT_CHAR_COUNT)   // after the font
#define WARNING_OFF_TILE WARNING_FIRST_TILE   // the warning's first tile, a dark corner
#define WARNING_X 7                           // the monitor's screen, in tiles
#define WARNING_Y 6
#define WARNING_W (title_warning_WIDTH >> 3)
#define WARNING_H (title_warning_HEIGHT >> 3)
#define WARNING_BLINK_FRAMES 30               // on, then off, for 0.5 s each
#define PRESS_START_X 5                       // in the menu box, which covers it
#define PRESS_START_Y 14

// Draws the title picture, its tiles from 0 and on GBC its palette per map cell, and
// loads the warning's tiles after the font, which the title menu prints with. In bank 0,
// to read the picture's banks.
static void draw_title_picture(void) NONBANKED
{
    WITH_BANK(title, {
        set_bkg_data(0, title_TILE_COUNT, title_tiles);
        set_bkg_tiles(0, 0, DEVICE_SCREEN_WIDTH, TITLE_ROWS, title_map);
        if (is_cgb) set_bkg_attributes(0, 0, DEVICE_SCREEN_WIDTH, TITLE_ROWS, title_map_attributes);
        palette_set_bkg(0, title_PALETTE_COUNT, title_palettes);
    });
    WITH_BANK(title_warning, set_bkg_data(WARNING_FIRST_TILE, title_warning_TILE_COUNT, title_warning_tiles));
}

// Shows the warning triangle on the monitor. In bank 0, to read its map's bank.
static void show_warning(void) NONBANKED
{
    WITH_BANK(title_warning, set_bkg_based_tiles(WARNING_X, WARNING_Y, WARNING_W, WARNING_H,
                                                 title_warning_map, WARNING_FIRST_TILE));
}

// Waits for START with the warning blinking, then leaves it on for the menu. Returns 1,
// or 0 after DEMO_IDLE_FRAMES without a press, for the demo.
static uint8_t wait_for_start(void)
{
    uint8_t blink_frames = 0;
    uint8_t on = 1;
    uint16_t idle_frames = 0;
    uint8_t pressed;
    menu_start_input();
    while (!((pressed = menu_wait_press()) & J_START)) {
        if (pressed) idle_frames = 0;
        else if (++idle_frames == DEMO_IDLE_FRAMES) return 0;
        if (++blink_frames < WARNING_BLINK_FRAMES) continue;
        blink_frames = 0;
        on = !on;
        if (on) show_warning();
        else fill_bkg_rect(WARNING_X, WARNING_Y, WARNING_W, WARNING_H, WARNING_OFF_TILE);
    }
    if (!on) show_warning();
    return 1;
}

// Shows the title screen until START is pressed, then its menu; the title again after
// backing out of the password screen. Returns 1 with the level to start chosen, or 0
// when the demo should play. The screen is faded out either way.
uint8_t run_title_screen(void) BANKED
{
    music_play_title();
    do {
        menu_open_screen();
        draw_title_picture();
        show_warning();
        text_print(PRESS_START_X, PRESS_START_Y, "PRESS START");
        palette_fade_in(SCREEN_FADE_FRAMES);
        if (!wait_for_start()) {
            palette_fade_out(SCREEN_FADE_FRAMES);
            return 0;
        }
    } while (!title_menu());
    return 1;
}

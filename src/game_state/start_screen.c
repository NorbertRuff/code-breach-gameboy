#include <gb/gb.h>

#include "start_screen.h"
#include "menu.h"
#include "title_menu.h"
#include "../graphics/splashscreen.h"
#include "../engine/banking.h"
#include "../engine/palette.h"
#include "../sound/music.h"
#include "../utils.h"

static uint8_t blink_frames;   // frames into the "PRESS START" blink cycle

// Draws the title screen on a blank screen and fades it in.
static void draw_title_screen(void)
{
    menu_open_screen();
    WITH_BANK(splashscreen, {
        set_bkg_data(0, splashscreen_tileset_size, splashscreen_tileset);
        set_bkg_tiles(splashscreen_tileset_X, splashscreen_tileset_Y, splashscreen_tileset_width, splashscreen_tileset_height, splashscreen_tilemap);
    });
    palette_fade_in(10);
}

// Blinks "PRESS START": hides it after PRESS_START_SHOWN_FRAMES, shows it again at the
// start of the next cycle. Once per frame.
static void blink_press_start(void)
{
    blink_frames++;
    if (blink_frames == PRESS_START_SHOWN_FRAMES) {
        WITH_BANK(splashscreen, set_bkg_tiles(press_start_tileset_X, press_start_tileset_Y, press_start_tileset_width, press_start_tileset_height, press_start_blank_tilemap));
    } else if (blink_frames >= PRESS_START_CYCLE_FRAMES) {
        WITH_BANK(splashscreen, set_bkg_tiles(press_start_tileset_X, press_start_tileset_Y, press_start_tileset_width, press_start_tileset_height, press_start_tilemap));
        blink_frames = 0;
    }
}

// Shows the title screen until START is pressed, then its menu; the title again after
// backing out of the password screen. Returns with the level to start chosen.
void run_title_screen(void)
{
    music_play_title();
    do {
        draw_title_screen();
        menu_start_input();
        while (!(menu_wait_press() & J_START)) blink_press_start();
    } while (!title_menu());
}

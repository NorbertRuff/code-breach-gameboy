#include <gb/gb.h>

#include "start_screen.h"
#include "../graphics/splashscreen.h"
#include "../engine/banking.h"
#include "../engine/palette.h"
#include "../sound/music.h"
#include "../utils.h"

static uint8_t blink_frames;   // frames into the "PRESS START" blink cycle

// Draws the title screen and fades it in.
static void draw_title_screen(void)
{
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

// Shows the title screen until START is pressed, then fades it out.
void run_title_screen(void)
{
    music_play_title();
    draw_title_screen();
    while (!(joypad() & J_START)) {
        blink_press_start();
        wait_vbl_done();
    }
    palette_fade_out(10);
}

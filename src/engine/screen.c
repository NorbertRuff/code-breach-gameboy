#include <gb/gb.h>
#include "screen.h"
#include "../sound/music.h"

static uint8_t band_scx, level_scx;   // set during an update, for the next frame
static uint8_t frame_level_scx;       // the level's scroll for the frame being drawn
static uint8_t sprites_on = 1;

// At the start of VBlank: the band's scroll for the frame about to be drawn, the sprites
// back on, and the first split line. The level's scroll waits for the split below the band.
static void on_vblank(void)
{
    SCX_REG = band_scx;
    frame_level_scx = level_scx;
    if (sprites_on) SHOW_SPRITES;
    LYC_REG = BAND_LAST_LINE;
}

// At a split line, in its HBlank, so the change starts on the next line: below the band
// the level's scroll and then the frame's music step, over the HUD no sprites. The music
// runs here, not at VBlank, so it never delays the VBlank work or the game's update.
static void on_split_line(void)
{
    while (STAT_REG & STATF_BUSY);
    if (LYC_REG == BAND_LAST_LINE) {
        SCX_REG = frame_level_scx;
        LYC_REG = HUD_FIRST_LINE - 1;
        music_step();
    } else {
        HIDE_SPRITES;
    }
}

// Starts the line interrupts. They run for the rest of the game.
void screen_start_splits(void)
{
    CRITICAL {
        LYC_REG = BAND_LAST_LINE;
        STAT_REG = STATF_LYC;
        add_VBL(on_vblank);
        add_LCD(on_split_line);
    }
    set_interrupts(IE_REG | LCD_IFLAG);
}

// Sets the scrolls the next frame shows: the band at half the camera's speed, the level
// with the camera. Both change together, so a frame never mixes old and new.
void screen_scroll(int16_t camera_x)
{
    CRITICAL {
        band_scx = camera_x >> 1;
        level_scx = camera_x;
    }
}

// Shows or hides the sprites from now on: hidden while a level is built behind a dark
// screen.
void screen_show_sprites(uint8_t show)
{
    sprites_on = show;
    if (show) SHOW_SPRITES;
    else HIDE_SPRITES;
}

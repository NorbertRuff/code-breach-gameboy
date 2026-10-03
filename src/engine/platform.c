#include <gb/gb.h>
#include <gb/cgb.h>

#include "platform.h"

uint8_t is_cgb;

// The 4 DMG shades as GBC colors, white to black.
static const palette_color_t dmg_grays[4] = {
    RGB(31, 31, 31), RGB(21, 21, 21), RGB(10, 10, 10), RGB(0, 0, 0)
};

// On GBC: no color attributes yet, and gray palettes, so it looks like the DMG.
void platform_init(void)
{
    uint8_t i;

    is_cgb = (_cpu == CGB_TYPE);
    if (!is_cgb) return;

    // Attributes live in VRAM bank 1, which only exists on GBC.
    VBK_REG = VBK_ATTRIBUTES;
    fill_bkg_rect(0, 0, 32, 32, 0);
    fill_win_rect(0, 0, 32, 32, 0);
    VBK_REG = VBK_TILES;

    for (i = 0; i < 8; i++) {
        set_bkg_palette(i, 1, dmg_grays);
        set_sprite_palette(i, 1, dmg_grays);
    }
}

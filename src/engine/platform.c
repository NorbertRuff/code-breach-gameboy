#include <gb/gb.h>
#include <gb/cgb.h>

#include "platform.h"

uint8_t is_cgb;

// The DMG shades as GBC colors: index 0 white ... 3 black, the same order the
// default DMG palettes (BGP_REG and OBP0_REG = 0xE4) use.
static const palette_color_t dmg_grays[4] = {
    RGB(31, 31, 31), RGB(21, 21, 21), RGB(10, 10, 10), RGB(0, 0, 0)
};

void platform_init(void)
{
    uint8_t i;

    is_cgb = (_cpu == CGB_TYPE);
    if (!is_cgb) return;

    // VRAM bank 1 holds a GBC attribute byte per map cell (palette, flips, tile bank).
    // Zero both maps so every cell uses palette 0 and tiles from VRAM bank 0.
    // Never do this on DMG: there VBK_REG is ignored and this would wipe the tile maps.
    VBK_REG = VBK_ATTRIBUTES;
    fill_bkg_rect(0, 0, 32, 32, 0);
    fill_win_rect(0, 0, 32, 32, 0);
    VBK_REG = VBK_TILES;

    for (i = 0; i < 8; i++) {
        set_bkg_palette(i, 1, dmg_grays);
        set_sprite_palette(i, 1, dmg_grays);
    }
}

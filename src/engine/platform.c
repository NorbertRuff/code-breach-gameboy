#include <gb/gb.h>
#include <gb/cgb.h>

#include "platform.h"

uint8_t is_cgb;

// Detects the hardware. On GBC, clears the color attributes: every map cell starts on
// palette 0.
void platform_init(void)
{
    is_cgb = (_cpu == CGB_TYPE);
    if (!is_cgb) return;

    // Attributes live in VRAM bank 1, which only exists on GBC.
    VBK_REG = VBK_ATTRIBUTES;
    fill_bkg_rect(0, 0, 32, 32, 0);
    fill_win_rect(0, 0, 32, 32, 0);
    VBK_REG = VBK_TILES;
}

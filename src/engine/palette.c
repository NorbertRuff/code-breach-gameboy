#include <string.h>
#include "palette.h"
#include "platform.h"

// The palette loads that read ROM colors stay in bank 0, so they run inside the caller's
// WITH_BANK; the fades, flashes and the rest are in palette_effects.c, in a ROM bank.

uint8_t palette_fade_step;
palette_color_t bkg_colors[PALETTE_COUNT * PALETTE_COLORS];
palette_color_t sprite_colors[PALETTE_COUNT * PALETTE_COLORS];

// A GBC color darkened to a fade step: all 3 channels at once, by shifting the whole
// RGB555 word and masking off the bits that slid into the next channel.
static palette_color_t fade_color(palette_color_t color, uint8_t step)
{
    if (step == PALETTE_NORMAL) return color;
    if (step == 1) return ((color >> 1) & 0x3DEF) + ((color >> 3) & 0x0C63);   // 5/8
    if (step == 2) return (color >> 2) & 0x1CE7;                                 // 1/4
    return 0;
}

// Writes the GBC background palettes first to first + count - 1 at the fade step, in
// one call.
void palette_write_bkg(uint8_t first, uint8_t count)
{
    static palette_color_t faded[PALETTE_COUNT * PALETTE_COLORS];
    const palette_color_t* color = &bkg_colors[first * PALETTE_COLORS];
    for (uint8_t i = 0; i < count * PALETTE_COLORS; i++, color++) {
        faded[i] = fade_color(*color, palette_fade_step);
    }
    set_bkg_palette(first, count, faded);
}

// Sets GBC background palettes, shown at the current fade step. Pass ROM colors inside
// their WITH_BANK. Nothing to do on DMG: BGP_REG belongs to the fades.
void palette_set_bkg(uint8_t first, uint8_t count, const palette_color_t* colors)
{
    if (!is_cgb) return;
    memcpy(&bkg_colors[first * PALETTE_COLORS], colors, count * PALETTE_COLORS * sizeof(palette_color_t));
    palette_write_bkg(first, count);
}

// Sets GBC sprite palettes. Fades leave the sprites alone, as on DMG.
void palette_set_sprite(uint8_t first, uint8_t count, const palette_color_t* colors)
{
    if (!is_cgb) return;
    memcpy(&sprite_colors[first * PALETTE_COLORS], colors, count * PALETTE_COLORS * sizeof(palette_color_t));
    set_sprite_palette(first, count, colors);
}

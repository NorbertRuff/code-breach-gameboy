#include <string.h>
#include "palette.h"
#include "platform.h"
#include "../utils.h"

#define PALETTES 8
#define COLORS 4
#define FLASH_MIN_HOLD_FRAMES 10   // at most 3 flashes per second (photosensitivity)

uint8_t palette_fade_step;

// DMG: the background palette at each fade step, one shade darker each.
static const uint8_t dmg_fade_bgp[PALETTE_BLACK + 1] = { 0xE4, 0xF9, 0xFE, 0xFF };
#define DMG_ALL_BLACK 0xFF

// GBC: the palettes as set, before any fade.
static palette_color_t bkg_colors[PALETTES * COLORS];
static palette_color_t sprite_colors[PALETTES * COLORS];

// The 4 DMG shades as GBC colors, white to black: every palette starts as these.
static const palette_color_t grays[COLORS] = {
    RGB(31, 31, 31), RGB(21, 21, 21), RGB(10, 10, 10), RGB(0, 0, 0)
};

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
static void write_bkg(uint8_t first, uint8_t count)
{
    static palette_color_t faded[PALETTES * COLORS];
    const palette_color_t* color = &bkg_colors[first * COLORS];
    for (uint8_t i = 0; i < count * COLORS; i++, color++) {
        faded[i] = fade_color(*color, palette_fade_step);
    }
    set_bkg_palette(first, count, faded);
}

// Sets every palette to the DMG grays, unfaded. Once at boot, before the display is on.
void palette_init(void)
{
    palette_fade_step = PALETTE_NORMAL;
    BGP_REG = dmg_fade_bgp[PALETTE_NORMAL];
    for (uint8_t palette = 0; palette < PALETTES; palette++) {
        palette_set_bkg(palette, 1, grays);
        palette_set_sprite(palette, 1, grays);
    }
}

// Sets GBC background palettes, shown at the current fade step. Pass ROM colors inside
// their WITH_BANK. Nothing to do on DMG: BGP_REG belongs to the fades.
void palette_set_bkg(uint8_t first, uint8_t count, const palette_color_t* colors)
{
    if (!is_cgb) return;
    memcpy(&bkg_colors[first * COLORS], colors, count * COLORS * sizeof(palette_color_t));
    write_bkg(first, count);
}

// Sets GBC sprite palettes. Fades leave the sprites alone, as on DMG.
void palette_set_sprite(uint8_t first, uint8_t count, const palette_color_t* colors)
{
    if (!is_cgb) return;
    memcpy(&sprite_colors[first * COLORS], colors, count * COLORS * sizeof(palette_color_t));
    set_sprite_palette(first, count, colors);
}

// Sets GBC sprite palette target to a pale copy of palette source: each color halfway
// to white, made with the same masked shift as the fades. For the invincible player.
void palette_set_pale_sprite(uint8_t target, uint8_t source)
{
    if (!is_cgb) return;
    const palette_color_t* color = &sprite_colors[source * COLORS];
    palette_color_t* pale = &sprite_colors[target * COLORS];
    for (uint8_t i = 0; i < COLORS; i++) {
        pale[i] = ((color[i] >> 1) & 0x3DEF) + 0x4210;   // (color + white) / 2, rounded up
    }
    set_sprite_palette(target, 1, pale);
}

// GBC: makes a rectangle of background map cells use a palette (their attributes, in
// VRAM bank 1). Nothing on DMG, where bank 1 doesn't exist and the write would land on
// the tile map.
void palette_use_bkg(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t palette)
{
    if (!is_cgb) return;
    VBK_REG = VBK_ATTRIBUTES;
    fill_bkg_rect(x, y, w, h, palette);
    VBK_REG = VBK_TILES;
}

// GBC: the same for window map cells (the HUD).
void palette_use_win(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t palette)
{
    if (!is_cgb) return;
    VBK_REG = VBK_ATTRIBUTES;
    fill_win_rect(x, y, w, h, palette);
    VBK_REG = VBK_TILES;
}

// Shows the background at a fade step, from PALETTE_NORMAL to PALETTE_BLACK.
void palette_fade_to(uint8_t step)
{
    palette_fade_step = step;
    if (is_cgb) write_bkg(0, PALETTES);
    else BGP_REG = dmg_fade_bgp[step];
}

// Darkens the background to black in 4 steps of frames_per_step frames.
void palette_fade_out(uint8_t frames_per_step)
{
    for (uint8_t step = PALETTE_NORMAL; step <= PALETTE_BLACK; step++) {
        palette_fade_to(step);
        wait_frames(frames_per_step);
    }
}

// Brightens the background from black back to normal in 3 steps.
void palette_fade_in(uint8_t frames_per_step)
{
    for (uint8_t step = PALETTE_BLACK; step > PALETTE_NORMAL; step--) {
        palette_fade_to(step - 1);
        wait_frames(frames_per_step);
    }
}

static const palette_color_t black_colors[COLORS] = { 0 };
static uint8_t dmg_obp0;   // the DMG sprite palette during a flash

// Turns the background and the sprites black, or back to how they were.
static void show_black(uint8_t black)
{
    if (is_cgb) {
        for (uint8_t palette = 0; palette < PALETTES; palette++) {
            if (black) set_bkg_palette(palette, 1, black_colors);
            set_sprite_palette(palette, 1, black ? black_colors : &sprite_colors[palette * COLORS]);
        }
        if (!black) write_bkg(0, PALETTES);
    } else if (black) {
        dmg_obp0 = OBP0_REG;
        BGP_REG = DMG_ALL_BLACK;
        OBP0_REG = DMG_ALL_BLACK;
    } else {
        BGP_REG = dmg_fade_bgp[palette_fade_step];
        OBP0_REG = dmg_obp0;
    }
}

// Flashes the screen black count times, hold frames each way (at least
// FLASH_MIN_HOLD_FRAMES), then restores the palettes.
void palette_flash(uint8_t count, uint8_t hold)
{
    if (hold < FLASH_MIN_HOLD_FRAMES) hold = FLASH_MIN_HOLD_FRAMES;
    for (uint8_t i = 0; i < count; i++) {
        show_black(1);
        wait_frames(hold);
        show_black(0);
        wait_frames(hold);
    }
}

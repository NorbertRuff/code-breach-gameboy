// Not per-frame code: in a ROM bank, to keep bank 0 free. The rest of the palette
// module is in palette.c.
#pragma bank 255

#include "palette.h"
#include "platform.h"
#include "../utils.h"

#define FLASH_MIN_HOLD_FRAMES 10   // at most 3 flashes per second (photosensitivity)
#define DMG_ALL_BLACK 0xFF
#define DMG_SPRITE_PALETTE 0xD0    // sprite colors 1-3 as white, light gray, black (0 is transparent)

// DMG: the background palette at each fade step, one shade darker each.
static const uint8_t dmg_fade_bgp[PALETTE_BLACK + 1] = { 0xE4, 0xF9, 0xFE, 0xFF };

// The 4 DMG shades as GBC colors, white to black: every palette starts as these.
static const palette_color_t grays[PALETTE_COLORS] = {
    RGB(31, 31, 31), RGB(21, 21, 21), RGB(10, 10, 10), RGB(0, 0, 0)
};

static const palette_color_t black_colors[PALETTE_COLORS] = { 0 };
static uint8_t dmg_obp0;   // the DMG sprite palette during a flash

// Sets every palette to the DMG grays, unfaded. Once at boot, before the display is on.
// (palette_set_bkg runs in bank 0 and sees this bank, so it can read the grays.)
void palette_init(void) BANKED
{
    palette_fade_step = PALETTE_NORMAL;
    BGP_REG = dmg_fade_bgp[PALETTE_NORMAL];
    OBP0_REG = DMG_SPRITE_PALETTE;
    for (uint8_t palette = 0; palette < PALETTE_COUNT; palette++) {
        palette_set_bkg(palette, 1, grays);
        palette_set_sprite(palette, 1, grays);
    }
}

// Sets GBC sprite palette target to a pale copy of palette source: each color halfway
// to white, made with the same masked shift as the fades. For the invincible player.
void palette_set_pale_sprite(uint8_t target, uint8_t source) BANKED
{
    if (!is_cgb) return;
    const palette_color_t* color = &sprite_colors[source * PALETTE_COLORS];
    palette_color_t* pale = &sprite_colors[target * PALETTE_COLORS];
    for (uint8_t i = 0; i < PALETTE_COLORS; i++) {
        pale[i] = ((color[i] >> 1) & 0x3DEF) + 0x4210;   // (color + white) / 2, rounded up
    }
    set_sprite_palette(target, 1, pale);
}

// GBC: makes a rectangle of background map cells use a palette (their attributes, in
// VRAM bank 1). Nothing on DMG, where bank 1 doesn't exist and the write would land on
// the tile map.
void palette_use_bkg(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t palette) BANKED
{
    if (!is_cgb) return;
    VBK_REG = VBK_ATTRIBUTES;
    fill_bkg_rect(x, y, w, h, palette);
    VBK_REG = VBK_TILES;
}

// The screens without art palettes (title, menus): background palette 0 back to the
// grays, and every cell of the screen on it.
void palette_use_grays(void) BANKED
{
    palette_set_bkg(LEVEL_PALETTE, 1, grays);
    palette_use_bkg(0, 0, DEVICE_SCREEN_WIDTH, DEVICE_SCREEN_HEIGHT, LEVEL_PALETTE);
}

// GBC: the same for window map cells (the HUD).
void palette_use_win(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t palette) BANKED
{
    if (!is_cgb) return;
    VBK_REG = VBK_ATTRIBUTES;
    fill_win_rect(x, y, w, h, palette);
    VBK_REG = VBK_TILES;
}

// Shows the background at a fade step, from PALETTE_NORMAL to PALETTE_BLACK.
void palette_fade_to(uint8_t step) BANKED
{
    palette_fade_step = step;
    if (is_cgb) palette_write_bkg(0, PALETTE_COUNT);
    else BGP_REG = dmg_fade_bgp[step];
}

// Darkens the background to black in 4 steps of frames_per_step frames.
void palette_fade_out(uint8_t frames_per_step) BANKED
{
    for (uint8_t step = PALETTE_NORMAL; step <= PALETTE_BLACK; step++) {
        palette_fade_to(step);
        wait_frames(frames_per_step);
    }
}

// Brightens the background from black back to normal in 3 steps.
void palette_fade_in(uint8_t frames_per_step) BANKED
{
    for (uint8_t step = PALETTE_BLACK; step > PALETTE_NORMAL; step--) {
        palette_fade_to(step - 1);
        wait_frames(frames_per_step);
    }
}

// Turns the background and the sprites black, or back to how they were.
static void show_black(uint8_t black)
{
    if (is_cgb) {
        for (uint8_t palette = 0; palette < PALETTE_COUNT; palette++) {
            if (black) set_bkg_palette(palette, 1, black_colors);
            set_sprite_palette(palette, 1, black ? black_colors : &sprite_colors[palette * PALETTE_COLORS]);
        }
        if (!black) palette_write_bkg(0, PALETTE_COUNT);
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
void palette_flash(uint8_t count, uint8_t hold) BANKED
{
    if (hold < FLASH_MIN_HOLD_FRAMES) hold = FLASH_MIN_HOLD_FRAMES;
    for (uint8_t i = 0; i < count; i++) {
        show_black(1);
        wait_frames(hold);
        show_black(0);
        wait_frames(hold);
    }
}

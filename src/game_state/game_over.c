#include <gb/gb.h>
#include "../utils.h"

// At most 3 flashes per second, for photosensitive players (WCAG 2.3.1): each half
// of a flash lasts at least 10 frames, so one flash takes at least 20 frames.
#define FLASH_MIN_HOLD_FRAMES 10

// Flash by toggling the BG palette to all-black and back.
// This darkens the screen without touching LCDC layer bits,
// so the HUD window position is never disturbed.
// DMG only: GBC ignores BGP_REG/OBP0_REG (palette module in roadmap M4).
static void palette_flash(uint8_t count, uint8_t hold)
{
    uint8_t i;
    uint8_t bgp = BGP_REG;    // restore exactly what was set before the flash
    uint8_t obp0 = OBP0_REG;
    if (hold < FLASH_MIN_HOLD_FRAMES) hold = FLASH_MIN_HOLD_FRAMES;
    for (i = 0; i < count; i++) {
        BGP_REG = 0xFF;   // all BG colours → black
        OBP0_REG = 0xFF;  // sprites too
        performant_delay(hold);
        BGP_REG = bgp;
        OBP0_REG = obp0;
        performant_delay(hold);
    }
}

void game_over()
{
    palette_flash(4, 12);    // 4 flashes — signal "you lost"
    performant_delay(90);    // hold ~2 s then let fade_out take over
}

void level_complete()
{
    palette_flash(6, FLASH_MIN_HOLD_FRAMES);  // 6 quick flashes (3 per second) — celebrate
    performant_delay(60);
}

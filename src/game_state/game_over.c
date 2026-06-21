#include <gb/gb.h>
#include "../utils.h"

// Flash by toggling the BG palette to all-black and back.
// This darkens the screen without touching LCDC layer bits,
// so the HUD window position is never disturbed.
static void palette_flash(uint8_t count, uint8_t hold)
{
    uint8_t i;
    for (i = 0; i < count; i++) {
        BGP_REG = 0xFF;   // all BG colours → black
        OBP0_REG = 0xFF;  // sprites too
        performant_delay(hold);
        BGP_REG = 0xE4;   // restore default BG palette
        OBP0_REG = 0xD0;  // restore default sprite palette
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
    palette_flash(6, 8);     // 6 faster flashes — celebrate
    performant_delay(60);
}

#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

// 1 when running on a Game Boy Color (or a GBA in GBC mode), 0 on DMG.
// Set by platform_init(). See docs/CGB_PLAN.md.
extern uint8_t is_cgb;

// Detect the hardware. On GBC, also clear the attribute maps and load gray palettes
// that match the DMG shades, so the game looks the same on both until real colors
// arrive (roadmap M4). Call once at boot, before anything is shown.
void platform_init(void);

#endif // PLATFORM_H

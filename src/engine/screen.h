#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

// Each frame is split by line: the parallax band (screen rows 0-2) scrolls at half the
// camera's speed, the level below it with the camera, and sprites are off over the HUD.
#define BAND_LAST_LINE 23     // rows 0-2: lines 0-23
#define HUD_FIRST_LINE 128    // the HUD window's top (WY)

void screen_start_splits(void);
void screen_scroll(int16_t camera_x);
void screen_show_sprites(uint8_t show);

#endif // SCREEN_H

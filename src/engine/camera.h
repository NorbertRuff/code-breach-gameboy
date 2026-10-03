#ifndef CAMERA_H
#define CAMERA_H

#include <stdint.h>

#define CAMERA_CENTER 80      // screen x the camera keeps the focus point near
#define CAMERA_DEADZONE 8     // the focus may drift this far either way before the camera moves
#define LOOK_AHEAD 24         // px the focus leads the player, toward where they run
#define LOOK_AHEAD_TURN 16    // px of running the other way before the lead flips

extern int16_t camera_x;            // world x of the screen's left edge
extern int16_t camera_left_limit;   // neither the camera nor the player goes left of it

void camera_lock_left(int16_t x);
void camera_reset(uint16_t player_px, uint16_t level_width_px);
void camera_update(uint16_t player_px, uint16_t level_width_px);

#endif // CAMERA_H

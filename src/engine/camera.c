#include <gb/gb.h>
#include "camera.h"

int16_t camera_x;
int16_t camera_left_limit;

static int16_t next_left_limit;  // becomes camera_left_limit once the camera gets there or to the end
static int8_t look_ahead;        // current lead in px, eases toward look_direction * LOOK_AHEAD
static int8_t look_direction;    // 1 right, -1 left
static uint8_t turn_distance;    // px run against look_direction since the last flip
static uint16_t last_player_px;

// Keeps the focus point inside the deadzone, and the screen right of the left limit and
// inside the level. A new left limit takes force once the camera gets there, or gets to
// the level's end; near the end, the end wins, so the screen never shows past it.
static void follow_focus(int16_t focus, uint16_t level_width_px)
{
    int16_t last_x = level_width_px - DEVICE_SCREEN_PX_WIDTH;   // the screen at the level's end
    if (focus < camera_x + CAMERA_CENTER - CAMERA_DEADZONE) camera_x = focus - (CAMERA_CENTER - CAMERA_DEADZONE);
    if (focus > camera_x + CAMERA_CENTER + CAMERA_DEADZONE) camera_x = focus - (CAMERA_CENTER + CAMERA_DEADZONE);
    if (camera_x >= next_left_limit || camera_x >= last_x) camera_left_limit = next_left_limit;
    if (camera_x < camera_left_limit) camera_x = camera_left_limit;
    if (camera_x > last_x) camera_x = last_x;
}

// Sets a new left limit, behind an opened gate. It takes force once the camera has
// scrolled to it, so the camera never jumps.
void camera_lock_left(int16_t x)
{
    next_left_limit = x;
}

// Puts the camera on the player at once, looking right, with the left limit in force:
// for a level start or a restart.
void camera_reset(uint16_t player_px, uint16_t level_width_px)
{
    camera_left_limit = next_left_limit;
    look_ahead = 0;
    look_direction = 1;
    turn_distance = 0;
    last_player_px = player_px;
    camera_x = (int16_t)player_px - CAMERA_CENTER;
    follow_focus(player_px, level_width_px);
}

// Follows the player with a lead in the running direction. The lead flips only after
// LOOK_AHEAD_TURN px the other way, and moves 1 px per frame, so the camera never moves
// more than 1 px per frame faster than the player.
void camera_update(uint16_t player_px, uint16_t level_width_px)
{
    int8_t moved = (int8_t)(player_px - last_player_px);
    last_player_px = player_px;

    // Running against the lead? Flip it after LOOK_AHEAD_TURN px.
    if (moved && (moved > 0) != (look_direction > 0)) {
        turn_distance += (moved > 0) ? moved : -moved;
        if (turn_distance >= LOOK_AHEAD_TURN) {
            look_direction = -look_direction;
            turn_distance = 0;
        }
    } else if (moved) {
        turn_distance = 0;
    }

    if (look_ahead < look_direction * LOOK_AHEAD) look_ahead++;
    if (look_ahead > look_direction * LOOK_AHEAD) look_ahead--;

    follow_focus((int16_t)player_px + look_ahead, level_width_px);
}

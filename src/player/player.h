#ifndef PLAYER_H
#define PLAYER_H

#include <gb/gb.h>
#include <stdint.h>
#include "../level/level_manager.h"

// Positions are in 1/32 px: pixel = pos >> 5, tile = pos >> 8.
#define SUBPIXELS 32
#define TO_PIXELS(pos) ((pos) >> 5)

// Body box around the pivot (x = center, y = lowest body row), 16x32 px.
#define SPRITE_LEFT_W   8   // pixels from x to the left edge
#define SPRITE_RIGHT_W  7   // pixels from x to the right edge
#define SPRITE_HEAD_OFFSET 31  // pixels from y up to the top row

// Running, in 1/32 px per frame (RESEARCH.md 2.2).
#define RUN_SPEED_MAX     48   // 1.5 px/f
#define RUN_ACCEL          8   // full speed in 6 frames
#define RUN_DECEL          6   // stop in 8 frames
#define AIR_ACCEL          5   // 0.65x control in the air
#define AIR_DECEL          4

// Jumping, in 1/32 px per frame.
#define JUMP_SPEED        96   // 3 px/f upward at take-off
#define GRAVITY_RISE       4   // 0.125 px/f2 while rising with A held
#define GRAVITY_FALL      12   // 0.375 px/f2 otherwise: falling, or A released (jump cut)
#define APEX_SPEED        16   // below 0.5 px/f with A held, gravity is halved (apex hang)
#define FALL_SPEED_MAX   112   // 3.5 px/f, under 8 px/f so no tile is ever skipped
#define COYOTE_FRAMES      6   // a jump still works this long after walking off a ledge
#define JUMP_BUFFER_FRAMES 5   // an A press this long before landing still jumps
#define CORNER_CORRECTION  4   // px the head may slide sideways past a ceiling corner

#define PLAYER_PALE_PALETTE 1      // GBC sprite palette while invincible

#define ANIMATION_FRAME_LENGTH 4   // frames per run animation frame
#define JUMP_POSE_FRAME 3

typedef struct {
    uint16_t x;                  // body center, world, 1/32 px
    uint16_t y;                  // lowest body row, world, 1/32 px
    int8_t velocity_x;           // 1/32 px per frame
    int8_t velocity_y;           // 1/32 px per frame, positive is down
    int8_t input_x;              // d-pad: -1 left, 0, 1 right
    uint8_t jump_held;           // A is down this frame
    uint8_t jump_buffer_frames;  // counts down after a new press of A
    uint8_t coyote_frames;       // counts down after leaving the ground
    uint8_t on_ground;
    uint8_t touched_hazard;      // set when the body overlaps a hazard tile
    uint8_t touched_exit;        // set when the body overlaps the exit
    uint8_t direction;           // 0 left, 1 right
    uint8_t draw_frame;
    uint8_t animation_frame_count;
    uint8_t invincibility_frames;  // counts down after taking a hit; sprite flashes while > 0
} Player;

void player_init(Player* player, uint16_t start_x_px, uint16_t start_y_px);
void player_update(Player* player);
uint8_t player_draw(Player* player, int16_t camera_x);

#endif // PLAYER_H

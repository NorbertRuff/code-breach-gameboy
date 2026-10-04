#include "lift.h"

#define LIFT_FIRST_TILE 54   // sprite tiles after the pause label's (44-53)
#define LIFT_TILE_COUNT 2
#define LIFT_PAUSE_FRAMES 30   // 0.5 s at each end of its travel

// Placeholder art until M5: an 8 px platform with a dark top edge, in the top tile of an
// 8x16 sprite; the bottom tile is empty.
static const uint8_t lift_tiles[LIFT_TILE_COUNT * 16] = {
    0xff,0xff, 0xff,0x00, 0xff,0x00, 0x00,0xff, 0xff,0x00, 0xff,0x00, 0xff,0xff, 0x00,0x00,
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00
};

// Three 8x16 sprites side by side, their top on the pivot's row (the lift's top).
static const metasprite_t lift_metasprite[] = {
    METASPR_ITEM(0, -LIFT_HALF_WIDTH, 0, 0),
    METASPR_ITEM(0, 8, 0, 0),
    METASPR_ITEM(0, 8, 0, 0),
    METASPR_TERM
};

// Starts at the bottom of its travel, going up.
static void lift_spawn(Actor* lift, const LevelEntity* entity)
{
    lift->x = entity->x + LIFT_HALF_WIDTH;
    lift->y = lift->max_y = entity->y;
    lift->min_y = entity->y - entity->param;
    lift->velocity_x = 0;
    lift->velocity_y = -(int8_t)entity->speed;
    lift->wait_frames = 0;
}

// Takes one step up or down. At an end of its travel it waits, then turns around.
static void lift_update(Actor* lift)
{
    lift->step_y = 0;
    if (lift->wait_frames) {
        lift->wait_frames--;
        return;
    }
    lift->y += lift->velocity_y;
    lift->step_y = lift->velocity_y;
    if (lift->y == lift->min_y || lift->y == lift->max_y) {
        lift->wait_frames = LIFT_PAUSE_FRAMES;
        lift->velocity_y = -lift->velocity_y;
    }
}

// No hit box: a lift never hurts.
const ActorType lift_type = {
    lift_spawn, lift_update, lift_metasprite, lift_tiles, LIFT_FIRST_TILE, LIFT_TILE_COUNT,
    0, 0, 0, 0, LIFT_HALF_WIDTH, 1
};

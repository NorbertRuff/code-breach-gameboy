#include "packet.h"
#include "bot.h"

#define PACKET_HALF_WIDTH 8      // 16x16 px, x is its center
#define PACKET_FALL_SPEED 2      // px per frame
#define PACKET_FLOOR_Y 119       // feet row on the arena's floor: it vanishes there
#define PACKET_HIDDEN_X 0x4000   // far off screen, while it blinks off

// Waiting: counts down and blinks, 4 frames shown, 4 hidden. Then falls, and is gone at
// the floor.
static void packet_update(Actor* packet)
{
    if (packet->wait_frames) {
        packet->wait_frames--;
        packet->x = (packet->wait_frames & 4) ? packet->min_x : PACKET_HIDDEN_X;
        return;
    }
    packet->x = packet->min_x;
    packet->y += PACKET_FALL_SPEED;
    if (packet->y >= PACKET_FLOOR_Y) packet->x = ACTOR_GONE;
}

// Never spawned from a level entity. The bot's metasprite (16x16, the same tile layout)
// and hit box; kernel.c loads its tiles.
const ActorType packet_type = {
    0, packet_update, bot_metasprite, 0, PACKET_FIRST_TILE, PACKET_TILE_COUNT,
    -7, -15, 6, -1, PACKET_HALF_WIDTH, 0
};

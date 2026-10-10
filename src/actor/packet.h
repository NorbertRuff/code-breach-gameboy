#ifndef PACKET_H
#define PACKET_H

#include "actor.h"

#define PACKET_FIRST_TILE 84   // sprite tiles after the lift's (82-83)
#define PACKET_TILE_COUNT 4
#define PACKET_TOP_Y 23        // feet row while it waits: over the band, above any player

// KERNEL's data packet (src/game_state/kernel.c adds them): blinks at the top for its
// wait_frames, then falls to the floor and is gone. Its x while it falls is min_x.
extern const ActorType packet_type;

#endif // PACKET_H

#ifndef BOT_H
#define BOT_H

#include "actor.h"

#include "../graphics/programmer.h"

#define BOT_FIRST_TILE programmer_TILE_COUNT   // sprite tiles after the player's
#define BOT_TILE_COUNT 4

// The security bot: walks its patrol range and turns at each end.
extern const ActorType bot_type;

// Its art, also worn by the chaser until the M5 art.
extern const uint8_t bot_tiles[BOT_TILE_COUNT * 16];
extern const metasprite_t bot_metasprite[];

#endif // BOT_H

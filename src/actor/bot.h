#ifndef BOT_H
#define BOT_H

#include "actor.h"

#define FRAME_TILES_16X16 4   // a 16x16 frame's two 8x16 sprites: the next frame's start

// The security bot: walks its patrol range and turns at each end.
extern const ActorType bot_type;

// Its two 8x16 sprites, the layout of every 16x16 actor (the chaser, the packet).
extern const metasprite_t bot_metasprite[];

#endif // BOT_H

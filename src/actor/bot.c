#include "bot.h"
#include "../graphics/programmer.h"

#define BOT_FIRST_TILE programmer_TILE_COUNT   // sprite tiles after the player's
#define BOT_HALF_WIDTH 8                       // body: 16x16 px, x is its center

// Placeholder art: the mushroom from the early prototype, until the security bot (M5).
// Tiles in 8x16 sprite order: left column top and bottom, then the right column.
static const uint8_t bot_tiles[4 * 16] = {
    0x07,0xff, 0x19,0xf8, 0x21,0xe0, 0x43,0xc0, 0x46,0xc0, 0xfc,0x80, 0xcc,0x80, 0x86,0x80,
    0x87,0x80, 0xcf,0x80, 0xff,0x8f, 0x72,0xf2, 0x22,0xe2, 0x20,0xe0, 0x10,0xf0, 0x0f,0xff,
    0xe0,0xff, 0xf8,0x1f, 0xe4,0x07, 0xf2,0x03, 0x1a,0x03, 0x0f,0x01, 0x0f,0x01, 0x19,0x01,
    0xe1,0x01, 0xf3,0x01, 0xff,0xf1, 0x4e,0x4f, 0x44,0x47, 0x04,0x07, 0x08,0x0f, 0xf0,0xff
};

// Two 8x16 sprites around the pivot (body center, feet row). Each item's offset is from
// the item before it.
static const metasprite_t bot_metasprite[] = {
    METASPR_ITEM(-15, -8, 0, 0),    // left half
    METASPR_ITEM(0, 8, 2, 0),       // right half
    METASPR_TERM
};

// Starts at the left end of the patrol, walking right. The entity's x and width are the
// body's patrol range, so the center stays half a body inside it.
static void bot_spawn(Actor* bot, const LevelEntity* entity)
{
    bot->x = bot->min_x = entity->x + BOT_HALF_WIDTH;
    bot->max_x = entity->x + entity->param - BOT_HALF_WIDTH;
    bot->y = entity->y;
    bot->velocity_x = entity->speed;
}

// Walks at its speed and turns around at the ends of the patrol.
static void bot_update(Actor* bot)
{
    bot->x += bot->velocity_x;
    if (bot->velocity_x > 0 && bot->x >= bot->max_x) {
        bot->x = bot->max_x;
        bot->velocity_x = -bot->velocity_x;
    } else if (bot->velocity_x < 0 && bot->x <= bot->min_x) {
        bot->x = bot->min_x;
        bot->velocity_x = -bot->velocity_x;
    }
}

// The hit box is 1 px smaller than the body at the sides and the bottom.
const ActorType bot_type = {
    bot_spawn, bot_update, bot_metasprite, bot_tiles, BOT_FIRST_TILE, 4,
    -7, -15, 6, -1
};

#include "bot.h"

#define BOT_HALF_WIDTH 8                       // body: 16x16 px, x is its center

// Two 8x16 sprites around the pivot (body center, feet row): the left column's tiles,
// then the right's (png2asset's order). Each item's offset is from the item before it.
const metasprite_t bot_metasprite[] = {
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
    bot_spawn, bot_update, bot_metasprite, BOT_FIRST_TILE, FRAME_TILES_16X16,
    -7, -15, 6, -1, BOT_HALF_WIDTH, 0
};

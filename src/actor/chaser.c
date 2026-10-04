#include "chaser.h"
#include "bot.h"

#define CHASER_HALF_WIDTH 8   // body: 16x16 px, x is its center (the bot's)
#define SIGHT_X 48            // px: it sees the player up to 6 tiles away on either side
#define SIGHT_Y 16            // px: ... with the player's feet this close to its floor

// Starts at the right end of its range, facing left, standing still.
static void chaser_spawn(Actor* chaser, const LevelEntity* entity)
{
    uint16_t left = entity->x;
    chaser->min_x = left + CHASER_HALF_WIDTH;
    chaser->x = chaser->max_x = left + entity->param - CHASER_HALF_WIDTH;
    chaser->y = entity->y;
    chaser->velocity_x = -1;
}

// Walks 1 px per frame toward the player while it sees the player, staying inside its
// range; else it stands. velocity_x keeps the direction it faces.
static void chaser_update(Actor* chaser)
{
    uint16_t x = chaser->x;
    int16_t dx = actor_player_x - x;
    // One unsigned compare per axis: within SIGHT_X and SIGHT_Y on either side.
    if ((uint16_t)(dx + SIGHT_X) > 2 * SIGHT_X) return;
    if ((uint8_t)(actor_player_feet - chaser->y + SIGHT_Y) > 2 * SIGHT_Y) return;

    if (dx > 0) {
        chaser->velocity_x = 1;
        if (x < chaser->max_x) chaser->x = x + 1;
    } else if (dx < 0) {
        chaser->velocity_x = -1;
        if (x > chaser->min_x) chaser->x = x - 1;
    }
}

// Placeholder: the bot's art and hit box, until the chaser's own art (M5).
const ActorType chaser_type = {
    chaser_spawn, chaser_update, bot_metasprite, bot_tiles, BOT_FIRST_TILE, BOT_TILE_COUNT,
    -7, -15, 6, -1, CHASER_HALF_WIDTH, 0
};

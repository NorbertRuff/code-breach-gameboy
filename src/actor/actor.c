#include "actor.h"
#include "bot.h"
#include "lift.h"
#include "../level/level_manager.h"

// Read by the tests to notice Actor struct changes.
const uint8_t actor_struct_size = sizeof(Actor);

#define SPAWN_AHEAD 16                          // px past the screen's right edge
#define DESPAWN_BEHIND DEVICE_SCREEN_PX_WIDTH   // px past the screen's left edge
#define LIFT_REACH 19   // px: the player's 16 px body overlaps a 24 px lift up to this far from its center

Actor actors[MAX_ACTORS];
uint8_t actor_count;
uint8_t lifts_in_play;

// The actor type of each entity type. Fragments are drawn in the map, and gates and the
// exit are not actors.
static const ActorType* const actor_types[] = {
    &bot_type,   // ENTITY_ENEMY
    0,           // ENTITY_FRAGMENT
    0,           // ENTITY_GATE
    0,           // ENTITY_EXIT
    &lift_type,  // ENTITY_LIFT
};
#define ACTOR_TYPE_COUNT (sizeof(actor_types) / sizeof(actor_types[0]))

static uint8_t spawn_cursor;        // index of the next level entity to spawn
static LevelEntity next_entity;     // a copy of it, so most frames need no bank switch

// Copies the entity at the cursor out of ROM. Past the list's end, its x is out of reach.
static void read_next_entity(void)
{
    if (spawn_cursor < current_level.entity_count) {
        level_read_entity(spawn_cursor, &next_entity);
    } else {
        next_entity.x = 0xFFFF;
    }
}

// Loads every actor type's tiles into sprite VRAM.
void actors_load_tiles(void)
{
    for (uint8_t i = 0; i < ACTOR_TYPE_COUNT; i++) {
        const ActorType* type = actor_types[i];
        if (type) set_sprite_data(type->first_tile, type->tile_count, type->tiles);
    }
}

// Removes every actor and points the spawn cursor at a section's first entity: for a
// level start or a restart.
void actors_reset(uint8_t section)
{
    actor_count = 0;
    lifts_in_play = 0;
    spawn_cursor = current_level.section_start[section];
    read_next_entity();
}

// Spawns the level entities that came within SPAWN_AHEAD px of the screen's right edge,
// in the list's order (sorted by x). With every place taken, waits for one to free up.
void actors_spawn(int16_t camera_x)
{
    uint16_t reach = camera_x + DEVICE_SCREEN_PX_WIDTH + SPAWN_AHEAD;
    while (next_entity.x < reach) {
        const ActorType* type = actor_types[next_entity.type];
        if (type) {
            if (actor_count == MAX_ACTORS) return;
            Actor* actor = &actors[actor_count++];
            actor->type = type;
            type->spawn(actor, &next_entity);
            lifts_in_play += type->is_lift;
        }
        spawn_cursor++;
        read_next_entity();
    }
}

// Moves the lifts, before the player's update, so the player rides along with them.
void actors_move_lifts(void)
{
    if (!lifts_in_play) return;
    Actor* actor = actors;
    for (uint8_t i = 0; i < actor_count; i++, actor++) {
        if (actor->type->is_lift) actor->type->update(actor);
    }
}

// Moves every other actor. One that falls DESPAWN_BEHIND px behind the screen's left edge
// is removed, and comes back only when the section restarts: the last actor takes its
// place, and is updated next.
void actors_update(int16_t camera_x)
{
    int16_t despawn_x = camera_x - DESPAWN_BEHIND;
    Actor* actor = actors;
    for (uint8_t i = 0; i < actor_count; ) {
        const ActorType* type = actor->type;
        if (!(lifts_in_play && type->is_lift)) type->update(actor);
        if ((int16_t)actor->x < despawn_x) {
            lifts_in_play -= actor->type->is_lift;
            *actor = actors[--actor_count];
        } else {
            i++;
            actor++;
        }
    }
}

// The lift under the player's body (its center x in world pixels), or 0. Lifts never
// overlap in x (tools/tiled2c.py), so there is at most one.
Actor* actors_lift_at(uint16_t body_x)
{
    Actor* actor = actors;
    for (uint8_t i = 0; i < actor_count; i++, actor++) {
        // One unsigned compare: body_x within LIFT_REACH of the lift's center.
        if (actor->type->is_lift && (uint16_t)(body_x + LIFT_REACH - actor->x) <= 2 * LIFT_REACH) return actor;
    }
    return 0;
}

// Does a box (world pixels, inclusive) overlap the hit box of an actor other than a lift?
uint8_t actors_touch(int16_t left, int16_t top, int16_t right, int16_t bottom)
{
    Actor* actor = actors;
    for (uint8_t i = 0; i < actor_count; i++, actor++) {
        const ActorType* type = actor->type;
        if (lifts_in_play && type->is_lift) continue;
        int16_t x = actor->x, y = actor->y;
        if (right >= x + type->hit_left && left <= x + type->hit_right &&
            bottom >= y + type->hit_top && top <= y + type->hit_bottom) return 1;
    }
    return 0;
}

// Adds the actors on screen to the sprite list, from hardware sprite first_sprite on.
// Returns the next unused hardware sprite.
uint8_t actors_draw(uint8_t first_sprite, int16_t camera_x)
{
    Actor* actor = actors;
    for (uint8_t i = 0; i < actor_count; i++, actor++) {
        // Entirely off screen? Skip it: an 8-bit sprite x would wrap into view. On screen,
        // screen_x is within half_width of it: one unsigned compare checks both sides.
        const ActorType* type = actor->type;
        uint8_t half_width = type->half_width;
        int16_t screen_x = actor->x - camera_x;
        if ((uint16_t)(screen_x + half_width - 1) >= DEVICE_SCREEN_PX_WIDTH + 2 * half_width - 1) continue;

        uint8_t x = screen_x + DEVICE_SPRITE_PX_OFFSET_X;
        uint8_t y = actor->y + DEVICE_SPRITE_PX_OFFSET_Y;
        if (actor->velocity_x > 0) {
            first_sprite += move_metasprite_ex(type->metasprite, type->first_tile, 0, first_sprite, x, y);
        } else {
            first_sprite += move_metasprite_flipx(type->metasprite, type->first_tile, 0, first_sprite, x, y);
        }
    }
    return first_sprite;
}

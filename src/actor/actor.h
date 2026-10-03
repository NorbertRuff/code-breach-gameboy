#ifndef ACTOR_H
#define ACTOR_H

#include <gb/gb.h>
#include <gb/metasprites.h>
#include <stdint.h>
#include "../level/level_data.h"

// Actors are the level's moving objects (for now the patrol bots), spawned from its
// entity list as the camera reaches them. The player is not an actor.
#define MAX_ACTORS 8          // 2 hardware sprites each, after the player's 4

typedef struct ActorType ActorType;

typedef struct {
    const ActorType* type;
    uint16_t x;               // body center, world pixels
    uint8_t y;                // feet row, world pixels
    int8_t direction;         // 1 right, -1 left
    uint16_t min_x;           // patrol range of x
    uint16_t max_x;
} Actor;

// What an entity type does once spawned. Each type's tiles sit in sprite VRAM from
// first_tile on; its metasprite's pivot is the actor's (x, y): body center, feet row.
struct ActorType {
    void (*spawn)(Actor* actor, const LevelEntity* entity);
    void (*update)(Actor* actor);
    const metasprite_t* metasprite;
    const uint8_t* tiles;
    uint8_t first_tile;
    uint8_t tile_count;
    int8_t hit_left, hit_top, hit_right, hit_bottom;   // hit box around (x, y), inclusive
};

// The actors in play are actors[0] to actors[actor_count - 1].
extern Actor actors[MAX_ACTORS];
extern uint8_t actor_count;

void actors_load_tiles(void);
void actors_reset(uint8_t section);
void actors_spawn(int16_t camera_x);
void actors_update(int16_t camera_x);
uint8_t actors_touch(int16_t left, int16_t top, int16_t right, int16_t bottom);
uint8_t actors_draw(uint8_t first_sprite, int16_t camera_x);

#endif // ACTOR_H

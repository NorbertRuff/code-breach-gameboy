#ifndef ACTOR_H
#define ACTOR_H

#include <gb/gb.h>
#include <gb/metasprites.h>
#include <stdint.h>
#include "../level/level_data.h"
#include "../graphics/programmer.h"
#include "../graphics/security_bot.h"
#include "../graphics/chaser_drone.h"
#include "../graphics/lift_platform.h"
#include "../graphics/data_packet.h"

// Sprite VRAM tiles of the actors, after the player's; the pause label's follow.
#define BOT_FIRST_TILE    programmer_TILE_COUNT                          // 68-75
#define CHASER_FIRST_TILE (BOT_FIRST_TILE + security_bot_TILE_COUNT)     // 76-83
#define LIFT_FIRST_TILE   (CHASER_FIRST_TILE + chaser_drone_TILE_COUNT)  // 84-89
#define PACKET_FIRST_TILE (LIFT_FIRST_TILE + lift_platform_TILE_COUNT)   // 90-97
#define ACTOR_TILES_END   (PACKET_FIRST_TILE + data_packet_TILE_COUNT)   // 98

#define ACTOR_FRAME_BIT 8   // bit of the frame count: 2-frame actors switch every 8 frames

// Actors are the level's moving objects (patrol bots, chasers and lifts), spawned from
// its entity list as the camera reaches them, and KERNEL's packets and lift, which the
// boss adds itself (src/game_state/kernel.c). The player is not an actor.
#define MAX_ACTORS 8          // 2 or 3 hardware sprites each, after the player's 4
#define ACTOR_GONE 0x8000     // an x behind any camera: actors_update removes the actor

typedef struct ActorType ActorType;

typedef struct {
    const ActorType* type;
    uint16_t x;               // body center, world pixels
    uint8_t y;                // feet row, world pixels
    int8_t velocity_x;        // px per frame, negative is left (and faces left)
    uint16_t min_x;           // patrol range of x
    uint16_t max_x;
    int8_t velocity_y;        // a lift: px per frame, negative is up
    int8_t step_y;            // a lift: the step it took this frame (0 while it waits)
    uint8_t wait_frames;      // a lift: frames left to wait at an end of its travel
    uint8_t min_y;            // a lift: travel range of its top row
    uint8_t max_y;
} Actor;

// What an entity type does once spawned. Each type's tiles sit in sprite VRAM from
// first_tile on, its second animation frame's frame_tiles further on; its metasprite's
// pivot is the actor's (x, y): body center, feet row (a lift's top row). A lift moves
// before the player and carries it; it never hurts.
struct ActorType {
    void (*spawn)(Actor* actor, const LevelEntity* entity);
    void (*update)(Actor* actor);
    const metasprite_t* metasprite;
    uint8_t first_tile;
    uint8_t frame_tiles;      // 0: one frame only
    int8_t hit_left, hit_top, hit_right, hit_bottom;   // hit box around (x, y), inclusive
    uint8_t half_width;       // px from x to the art's edges, to skip actors off screen
    uint8_t is_lift;
};

// The actors in play are actors[0] to actors[actor_count - 1].
extern Actor actors[MAX_ACTORS];
extern uint8_t actor_count;
extern uint8_t lifts_in_play;   // most levels have none: the lift code then does nothing
extern uint16_t actor_player_x;     // the player's body center and feet row, world pixels,
extern uint8_t actor_player_feet;   // for actors that react to it (set by actors_update)

void actors_load_tiles(void);
void actors_reset(uint8_t section);
void actors_spawn(int16_t camera_x);
void actors_move_lifts(void);
void actors_update(int16_t camera_x, uint16_t player_x, uint8_t player_feet);
Actor* actors_lift_at(uint16_t body_x);
uint8_t actors_touch(int16_t left, int16_t top, int16_t right, int16_t bottom);
uint8_t actors_draw(uint8_t first_sprite, int16_t camera_x);

#endif // ACTOR_H

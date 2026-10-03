#ifndef LEVEL_MANAGER_H
#define LEVEL_MANAGER_H

#include <gb/gb.h>
#include <stdint.h>
#include "level_data.h"

#define TILE_SIZE 8

// Levels level_load() knows, by index.
#define LEVEL_GREYBOX_TEST 0
#define LEVEL_TEST_JUMPS   1    // fixtures for tests/test_jump_metrics.py

// Collision ring: the collision of the columns in the background map. 32 columns that
// wrap like the map, 16 screen rows each (the HUD rows are left out).
#define RING_COLUMNS 32
#define RING_ROWS 16
#define RING_INDEX(tile_x, tile_y) ((((tile_x) & (RING_COLUMNS - 1)) << 4) | (tile_y))

// The loaded level: a copy of its LevelData header, plus what changes while playing.
// map and entities point into ROM bank `bank`.
typedef struct {
    uint8_t bank;
    uint8_t width;              // tiles
    uint16_t width_px;
    uint16_t player_start_x;
    uint8_t player_start_y;
    uint8_t fragment_count;
    uint8_t collected_count;
    uint8_t entity_count;
    const uint8_t* map;
    const LevelEntity* entities;
} Level;

extern Level current_level;
extern uint8_t level_index;     // the level GAMEPLAY_START loads
extern uint8_t collision_ring[RING_COLUMNS * RING_ROWS];

void level_load(uint8_t index);
void level_read_entity(uint8_t index, LevelEntity* entity);

// Collision type of a level tile (tile_y in screen rows). Outside the level, or in a
// HUD row, is open. Only the drawn columns are in the ring, which always covers the screen.
// A macro, not a function: the player reads 20-30 tiles per frame. Pass plain variables.
#define LEVEL_COLLISION_AT(level, tile_x, tile_y)                          \
    (((tile_x) >= (level)->width || (tile_y) >= RING_ROWS) ? COLLISION_NONE \
     : collision_ring[RING_INDEX(tile_x, tile_y)])

// level_stream.c
void level_stream_start(void);
void level_stream_reset(int16_t camera_x);
void level_stream_update(int16_t camera_x);
void level_collect_fragment(uint8_t tile_x, uint8_t tile_y);

// HUD tiles: the window frame from the hud asset, then digits, '/' and a heart.
#define HUD_TILE_BASE       64
#define HUD_DIGIT_TILE_BASE 133   // tile 133 = '0', 134 = '1', ..., 142 = '9'
#define HUD_SLASH_TILE      143
#define HUD_HEART_TILE      144

void set_hud_window_tiles(void);
void set_programmer_metasprite(void);
void hud_update_count(uint8_t collected, uint8_t total);
void hud_update_lives(uint8_t lives);

#endif // LEVEL_MANAGER_H

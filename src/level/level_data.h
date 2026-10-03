#ifndef LEVEL_DATA_H
#define LEVEL_DATA_H

#include <stdint.h>

// Level data in ROM, written by tools/tiled2c.py from the Tiled maps in levels/.
// docs/LEVEL_PIPELINE.md describes the format.

// A level map is 13 rows tall: screen rows 3-15. Rows 0-2 are the parallax band.
#define LEVEL_MAP_ROWS 13
#define LEVEL_MAP_TOP_ROW 3

// Collision types, one per tileset tile (<tileset>_collision[] tables).
#define COLLISION_NONE 0
#define COLLISION_SOLID 1
#define COLLISION_ONE_WAY 2
#define COLLISION_HAZARD 3
#define COLLISION_COLLECTIBLE 4

// Entity types. x and y are world pixels (screen rows, so y includes the band).
#define ENTITY_ENEMY    0   // x: left end of the patrol, y: feet row, param: patrol width in px
#define ENTITY_FRAGMENT 1   // x, y: top-left of its tile
#define ENTITY_GATE     2   // x, y: top-left of its bottom tile, param: fragment quota
#define ENTITY_EXIT     3   // x, y: top-left of its tile

typedef struct {
    uint8_t type;
    uint16_t x;
    uint8_t y;
    uint8_t param;
} LevelEntity;

#define LEVEL_SECTIONS 3

typedef struct {
    uint8_t width;                          // tiles, at most 255
    uint16_t player_start_x;                // body center, world pixels
    uint8_t player_start_y;                 // feet row, world pixels
    uint8_t fragment_count;
    uint8_t entity_count;
    uint8_t section_start[LEVEL_SECTIONS];  // index of each section's first entity
    const uint8_t* map;                     // tile ids, LEVEL_MAP_ROWS rows of width tiles
    const LevelEntity* entities;            // sorted by x
} LevelData;

#endif // LEVEL_DATA_H

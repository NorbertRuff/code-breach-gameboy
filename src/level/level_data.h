#ifndef LEVEL_DATA_H
#define LEVEL_DATA_H

#include <gbdk/platform.h>
#include <stdint.h>

// Level data in ROM, written by tools/tiled2c.py from the Tiled maps in levels/.
// docs/LEVELS.md describes the Tiled side.

// A level map is 13 rows tall: screen rows 3-15. Rows 0-2 are the parallax band.
#define LEVEL_MAP_ROWS 13
#define LEVEL_MAP_TOP_ROW 3

// Collision types, one per tileset tile (<tileset>_collision[] tables). The types from
// FIRST_TOUCHED_TYPE on act when the body touches them.
#define COLLISION_NONE 0
#define COLLISION_SOLID 1
#define COLLISION_ONE_WAY 2
#define COLLISION_HAZARD 3
#define COLLISION_COLLECTIBLE 4
#define COLLISION_EXIT 5
#define FIRST_TOUCHED_TYPE COLLISION_HAZARD

// Entity types. x and y are world pixels (screen rows, so y includes the band).
#define ENTITY_ENEMY    0   // x: left end of the patrol, y: feet row, param: patrol width in px,
                            // speed: px per frame (1 or 2)
#define ENTITY_FRAGMENT 1   // x, y: top-left of its tile
#define ENTITY_GATE     2   // x, y: top-left of its bottom tile, param: fragment quota
#define ENTITY_EXIT     3   // x, y: top-left of its tile
#define ENTITY_LIFT     4   // x: left edge, y: its top row at the lowest point, param: travel
                            // up from there in px, speed: px per frame (1)
#define ENTITY_CHASER   5   // x: left end of its range, y: feet row, param: range width in px,
                            // speed: px per frame (1)

typedef struct {
    uint8_t type;
    uint16_t x;
    uint8_t y;
    uint8_t param;
    uint8_t speed;
} LevelEntity;

#define LEVEL_SECTIONS 3
#define EXIT_OPEN_TILES 8   // the opened elevator: 2 x 4 tiles, row by row

// A level theme: its tileset's art and band (png2asset, in their own banks; a bank is the
// address of its BANKREF symbol), and per tile, in this struct's bank, the collision type
// and the GBC palette. The marker tiles are drawn by the game where its objects are.
typedef struct {
    const void* tiles_bank;
    const uint8_t* tiles;
    uint8_t tile_count;
    const palette_color_t* palettes;
    uint8_t palette_count;
    const void* band_bank;
    const uint8_t* band_tiles;
    uint8_t band_tile_count;
    const uint8_t* band_map;                // 32 x 3 tiles
    const uint8_t* band_attributes;         // 32 x 3 tiles
    const palette_color_t* band_palettes;
    uint8_t band_palette_count;
    const uint8_t* collision;
    const uint8_t* palette;
    uint8_t fragment_tile, gate_tile, exit_tile;
    uint8_t exit_open[EXIT_OPEN_TILES];     // all 0: the tileset has no elevator
} TilesetData;

typedef struct {
    uint8_t width;                          // tiles, at most 255
    uint16_t player_start_x;                // body center, world pixels
    uint8_t player_start_y;                 // feet row, world pixels
    uint8_t fragment_count;
    uint8_t entity_count;
    uint8_t section_start[LEVEL_SECTIONS];  // index of each section's first entity
    const uint8_t* map;                     // tile ids, LEVEL_MAP_ROWS rows of width tiles
    const LevelEntity* entities;            // sorted by x
    const void* tileset_bank;               // the address of the tileset's BANKREF
    const TilesetData* tileset;
} LevelData;

#endif // LEVEL_DATA_H

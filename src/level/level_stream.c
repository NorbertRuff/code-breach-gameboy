#include <string.h>
#include "level_manager.h"
#include "../levels/greybox_tileset.h"
#include "../engine/banking.h"
#include "../sound/sound.h"

#define VISIBLE_COLUMNS 21      // 20 screen columns, plus one while scrolled between tiles
#define MAX_FRAGMENTS 32

uint8_t collision_ring[RING_COLUMNS * RING_ROWS];

static uint8_t tile_collision[256];                 // the tileset's table, copied to WRAM
static uint8_t fragment_count;
static uint8_t fragment_column[MAX_FRAGMENTS];      // tile column and screen row of each
static uint8_t fragment_row[MAX_FRAGMENTS];         // fragment, in entity order
static uint8_t fragments_collected[MAX_FRAGMENTS / 8];   // one bit per fragment
static uint8_t first_drawn_column;                  // leftmost column of the drawn window
static uint8_t column_tiles[LEVEL_MAP_ROWS];

// Lists the level's fragment tiles in WRAM, so drawing a column needs no entity search.
// Runs with the level's bank switched in.
static void list_fragments(void)
{
    const LevelEntity* entity = current_level.entities;
    fragment_count = 0;
    for (uint8_t i = 0; i < current_level.entity_count; i++, entity++) {
        if (entity->type != ENTITY_FRAGMENT) continue;
        fragment_column[fragment_count] = entity->x >> 3;
        fragment_row[fragment_count] = entity->y >> 3;
        fragment_count++;
    }
}

// Starts a level with nothing drawn and nothing collected: loads the tileset's
// collision table and lists the fragments.
void level_stream_start(void)
{
    memset(collision_ring, COLLISION_NONE, sizeof(collision_ring));
    memset(fragments_collected, 0, sizeof(fragments_collected));
    WITH_BANK(greybox_tileset, memcpy(tile_collision, greybox_tileset_collision, sizeof(greybox_tileset_collision)));
    WITH_ROM_BANK(current_level.bank, list_fragments());
}

static uint8_t is_collected(uint8_t fragment)
{
    return fragments_collected[fragment >> 3] & (1 << (fragment & 7));
}

// Reads one map column into column_tiles. Runs with the level's bank switched in.
static void read_column(uint8_t column)
{
    const uint8_t* tile = current_level.map + column;
    uint8_t width = current_level.width;
    for (uint8_t row = 0; row < LEVEL_MAP_ROWS; row++) {
        column_tiles[row] = *tile;
        tile += width;
    }
}

// Draws one level column into the background map and its collision into the ring,
// with the fragments that are still there.
static void draw_column(uint8_t column)
{
    if (column >= current_level.width) return;
    WITH_ROM_BANK(current_level.bank, read_column(column));
    for (uint8_t i = 0; i < fragment_count; i++) {
        if (fragment_column[i] == column && !is_collected(i)) {
            column_tiles[fragment_row[i] - LEVEL_MAP_TOP_ROW] = greybox_tileset_TILE_FRAGMENT;
        }
    }

    uint8_t* ring = &collision_ring[RING_INDEX(column, LEVEL_MAP_TOP_ROW)];
    for (uint8_t row = 0; row < LEVEL_MAP_ROWS; row++) {
        uint8_t tile = column_tiles[row];
        ring[row] = (tile == greybox_tileset_TILE_FRAGMENT) ? COLLISION_COLLECTIBLE : tile_collision[tile];
    }
    set_bkg_tiles(column & (RING_COLUMNS - 1), LEVEL_MAP_TOP_ROW, 1, LEVEL_MAP_ROWS, column_tiles);
}

// Draws every column the camera shows: for a level start, a respawn or any big jump.
void level_stream_reset(int16_t camera_x)
{
    first_drawn_column = camera_x >> 3;
    for (uint8_t i = 0; i < VISIBLE_COLUMNS; i++) {
        draw_column(first_drawn_column + i);
    }
}

// Draws the columns that came into view since the last frame, on either side.
void level_stream_update(int16_t camera_x)
{
    uint8_t first = camera_x >> 3;

    // Moved further than a screen? Redraw it all.
    if (first > first_drawn_column + VISIBLE_COLUMNS || first + VISIBLE_COLUMNS < first_drawn_column) {
        level_stream_reset(camera_x);
        return;
    }
    while (first_drawn_column < first) {
        first_drawn_column++;
        draw_column(first_drawn_column + VISIBLE_COLUMNS - 1);
    }
    while (first_drawn_column > first) {
        first_drawn_column--;
        draw_column(first_drawn_column);
    }
}

// Picks up the fragment at a tile: it stays collected even when its column is drawn
// again, its tile shows the map again, and the HUD count goes up.
void level_collect_fragment(uint8_t tile_x, uint8_t tile_y)
{
    for (uint8_t i = 0; i < fragment_count; i++) {
        if (fragment_column[i] == tile_x && fragment_row[i] == tile_y) {
            fragments_collected[i >> 3] |= 1 << (i & 7);
        }
    }
    uint8_t map_tile;
    const uint8_t* map_cell = current_level.map + (tile_y - LEVEL_MAP_TOP_ROW) * current_level.width + tile_x;
    WITH_ROM_BANK(current_level.bank, map_tile = *map_cell);
    collision_ring[RING_INDEX(tile_x, tile_y)] = tile_collision[map_tile];
    set_bkg_tiles(tile_x & (RING_COLUMNS - 1), tile_y, 1, 1, &map_tile);
    current_level.collected_count++;
    sound_play_collect();
}

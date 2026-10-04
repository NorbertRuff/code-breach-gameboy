#include <string.h>
#include "level_manager.h"
#include "../levels/greybox_tileset.h"
#include "../engine/banking.h"
#include "../sound/sound.h"

#define VISIBLE_COLUMNS 21      // 20 screen columns, plus one while scrolled between tiles
#define MAX_FRAGMENTS 32
#define NO_EXIT 0xFF            // exit_column of a test room

uint8_t collision_ring[RING_COLUMNS * RING_ROWS];
uint8_t ring_column_touched[RING_COLUMNS];

static uint8_t tile_collision[256];                 // the tileset's table, copied to WRAM
static uint8_t fragment_count;
static uint8_t fragment_column[MAX_FRAGMENTS];      // tile column, screen row and section
static uint8_t fragment_row[MAX_FRAGMENTS];         // of each fragment, in entity order
static uint8_t fragment_section[MAX_FRAGMENTS];
static uint8_t fragments_collected[MAX_FRAGMENTS / 8];   // one bit per fragment
static uint8_t fragment_columns[256 / 8];           // one bit per level column with a fragment
static uint8_t first_drawn_column;                  // leftmost column of the drawn window
static uint8_t column_tiles[LEVEL_MAP_ROWS];

// Lists the level's fragments, gates and exit in WRAM, so drawing a column needs no
// entity search, and counts each section's fragments. The entities are sorted by x, so
// a fragment's section is the number of gates listed before it. Runs with the level's
// bank switched in.
static void list_objects(void)
{
    const LevelEntity* entity = current_level.entities;
    for (uint8_t i = 0; i < current_level.entity_count; i++, entity++) {
        uint8_t column = entity->x >> 3, row = entity->y >> 3;
        if (entity->type == ENTITY_FRAGMENT) {
            fragment_columns[column >> 3] |= 1 << (column & 7);
            fragment_column[fragment_count] = column;
            fragment_row[fragment_count] = row;
            fragment_section[fragment_count] = current_level.gate_count;
            current_level.section_fragments[current_level.gate_count]++;
            fragment_count++;
        } else if (entity->type == ENTITY_GATE) {
            Gate* gate = &current_level.gates[current_level.gate_count++];
            gate->column = column;
            gate->bottom_row = row;
            gate->quota = entity->param;
            gate->open_rows = 0;
        } else if (entity->type == ENTITY_EXIT) {
            current_level.exit_column = column;
            current_level.exit_row = row;
        }
    }
}

// Starts a level with nothing drawn, nothing collected and every gate closed: loads the
// tileset's collision table and lists the level's objects.
void level_stream_start(void)
{
    memset(collision_ring, COLLISION_NONE, sizeof(collision_ring));
    memset(ring_column_touched, 0, sizeof(ring_column_touched));
    memset(fragments_collected, 0, sizeof(fragments_collected));
    memset(fragment_columns, 0, sizeof(fragment_columns));
    memset(current_level.section_fragments, 0, LEVEL_SECTIONS);
    memset(current_level.section_collected, 0, LEVEL_SECTIONS);
    fragment_count = 0;
    current_level.gate_count = 0;
    current_level.exit_column = NO_EXIT;
    current_level.section = 0;

    WITH_BANK(greybox_tileset, memcpy(tile_collision, greybox_tileset_collision, sizeof(greybox_tileset_collision)));
    // Marker tiles only appear where the game draws an object: give them its collision.
    tile_collision[greybox_tileset_TILE_FRAGMENT] = COLLISION_COLLECTIBLE;
    tile_collision[greybox_tileset_TILE_GATE] = COLLISION_SOLID;
    tile_collision[greybox_tileset_TILE_EXIT] = COLLISION_EXIT;
    WITH_ROM_BANK(current_level.bank, list_objects());
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

// Puts the objects in a column over its map tiles: the fragments still there, the rows
// of a gate that are still closed, and the exit.
static void add_objects(uint8_t column)
{
    // Most columns have no fragment: skip the search.
    if (fragment_columns[column >> 3] & (1 << (column & 7))) {
        for (uint8_t i = 0; i < fragment_count; i++) {
            if (fragment_column[i] == column && !is_collected(i)) {
                column_tiles[fragment_row[i] - LEVEL_MAP_TOP_ROW] = greybox_tileset_TILE_FRAGMENT;
            }
        }
    }
    Gate* gate = current_level.gates;
    for (uint8_t i = 0; i < current_level.gate_count; i++, gate++) {
        if (gate->column != column) continue;
        for (uint8_t row = LEVEL_MAP_TOP_ROW; row + gate->open_rows <= gate->bottom_row; row++) {
            column_tiles[row - LEVEL_MAP_TOP_ROW] = greybox_tileset_TILE_GATE;
        }
    }
    if (current_level.exit_column == column) {
        column_tiles[current_level.exit_row - LEVEL_MAP_TOP_ROW] = greybox_tileset_TILE_EXIT;
    }
}

// Draws one level column into the background map and its collision into the ring.
static void draw_column(uint8_t column)
{
    if (column >= current_level.width) return;
    WITH_ROM_BANK(current_level.bank, read_column(column));
    add_objects(column);

    uint8_t* ring = &collision_ring[RING_INDEX(column, LEVEL_MAP_TOP_ROW)];
    uint8_t touched = 0;
    for (uint8_t row = 0; row < LEVEL_MAP_ROWS; row++) {
        uint8_t type = tile_collision[column_tiles[row]];
        ring[row] = type;
        if (type >= FIRST_TOUCHED_TYPE) touched = 1;
    }
    // A pickup leaves the flag set: a column scanned for nothing costs a little, no more.
    ring_column_touched[column & (RING_COLUMNS - 1)] = touched;
    set_bkg_tiles(column & (RING_COLUMNS - 1), LEVEL_MAP_TOP_ROW, 1, LEVEL_MAP_ROWS, column_tiles);
}

// Draws every column the camera shows: for a level start, a restart or any big jump.
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

// Shows the map's own tile and collision at a cell on screen again: after a pickup, or
// as a gate opens.
static void restore_map_cell(uint8_t tile_x, uint8_t tile_y)
{
    uint8_t map_tile;
    const uint8_t* map_cell = current_level.map + (tile_y - LEVEL_MAP_TOP_ROW) * current_level.width + tile_x;
    WITH_ROM_BANK(current_level.bank, map_tile = *map_cell);
    collision_ring[RING_INDEX(tile_x, tile_y)] = tile_collision[map_tile];
    set_bkg_tiles(tile_x & (RING_COLUMNS - 1), tile_y, 1, 1, &map_tile);
}

// Picks up the fragment at a tile: it stays collected even when its column is drawn
// again, and counts for its section.
void level_collect_fragment(uint8_t tile_x, uint8_t tile_y)
{
    for (uint8_t i = 0; i < fragment_count; i++) {
        if (fragment_column[i] == tile_x && fragment_row[i] == tile_y) {
            fragments_collected[i >> 3] |= 1 << (i & 7);
            current_level.section_collected[fragment_section[i]]++;
        }
    }
    restore_map_cell(tile_x, tile_y);
    sound_play_collect();
}

// Puts a section's collected fragments back (game over). They show again when their
// columns are next drawn.
void level_reset_fragments(uint8_t section)
{
    for (uint8_t i = 0; i < fragment_count; i++) {
        if (fragment_section[i] == section) fragments_collected[i >> 3] &= ~(1 << (i & 7));
    }
    current_level.section_collected[section] = 0;
}

// Opens a gate's next row from the bottom; the gate must be on screen. Returns 1 once
// the gate is fully open.
uint8_t level_open_gate_row(Gate* gate)
{
    restore_map_cell(gate->column, gate->bottom_row - gate->open_rows);
    gate->open_rows++;
    return gate->open_rows > gate->bottom_row - LEVEL_MAP_TOP_ROW;
}

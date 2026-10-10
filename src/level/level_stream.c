#include <string.h>
#include "level_manager.h"
#include "../engine/banking.h"
#include "../engine/platform.h"
#include "../sound/sound.h"

#define VISIBLE_COLUMNS 21      // 20 screen columns, plus one while scrolled between tiles
#define MAX_FRAGMENTS 32
#define NO_EXIT 0xFF            // exit_column of a test room
#define NO_COLUMN 0xFF          // pending_column when no column waits
#define MAP_ROW_BYTES 32        // background map: 32 tiles per row
#define VBLANK_FIRST_LINE 144
#define LAST_DIRECT_WRITE_LINE 150      // a direct column write (2 lines) still ends before VBlank does (line 153)
#define LAST_DIRECT_WRITE_LINE_CGB 148  // the same with its palettes (4 lines)

uint8_t collision_ring[RING_COLUMNS * RING_ROWS];
uint8_t ring_column_touched[RING_COLUMNS];

static uint8_t tile_collision[256];                 // the tileset's tables, copied to WRAM
static uint8_t tile_palette[256];
static uint8_t fragment_count;
static uint8_t fragment_column[MAX_FRAGMENTS];      // tile column, screen row and section
static uint8_t fragment_row[MAX_FRAGMENTS];         // of each fragment, in entity order
static uint8_t fragment_section[MAX_FRAGMENTS];
static uint8_t fragments_collected[MAX_FRAGMENTS / 8];   // one bit per fragment
static uint8_t fragment_columns[256 / 8];           // one bit per level column with a fragment
static uint8_t first_drawn_column;                  // leftmost column of the drawn window
static uint8_t column_tiles[LEVEL_MAP_ROWS];
static uint8_t column_palettes[LEVEL_MAP_ROWS];     // their GBC attributes
static uint8_t pending_column = NO_COLUMN;          // map column whose column_tiles wait

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

// Copies the tileset's collision and palette tables to WRAM. Runs with the tileset's bank
// switched in.
static void copy_tile_tables(void)
{
    memcpy(tile_collision, current_tileset.collision, current_tileset.tile_count);
    memcpy(tile_palette, current_tileset.palette, current_tileset.tile_count);
}

// Starts a level with nothing drawn, nothing collected and every gate closed: loads the
// tileset's tables (current_tileset is set) and lists the level's objects.
void level_stream_start(uint8_t tileset_bank)
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
    pending_column = NO_COLUMN;

    WITH_ROM_BANK(tileset_bank, copy_tile_tables());
    // Marker tiles only appear where the game draws an object: give them its collision.
    tile_collision[current_tileset.fragment_tile] = COLLISION_COLLECTIBLE;
    tile_collision[current_tileset.gate_tile] = COLLISION_SOLID;
    tile_collision[current_tileset.exit_tile] = COLLISION_EXIT;
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
                column_tiles[fragment_row[i] - LEVEL_MAP_TOP_ROW] = current_tileset.fragment_tile;
            }
        }
    }
    Gate* gate = current_level.gates;
    for (uint8_t i = 0; i < current_level.gate_count; i++, gate++) {
        if (gate->column != column) continue;
        for (uint8_t row = LEVEL_MAP_TOP_ROW; row + gate->open_rows <= gate->bottom_row; row++) {
            column_tiles[row - LEVEL_MAP_TOP_ROW] = current_tileset.gate_tile;
        }
    }
    if (current_level.exit_column == column) {
        column_tiles[current_level.exit_row - LEVEL_MAP_TOP_ROW] = current_tileset.exit_tile;
    }
}

// Copies a column of 13 map bytes down the background map, into the VRAM bank switched
// in. Only while VRAM is free (VBlank): about 2 lines.
static void copy_column(uint8_t* map_cell, const uint8_t* value)
{
    uint8_t rows = LEVEL_MAP_ROWS;
    do {
        *map_cell = *value++;
        map_cell += MAP_ROW_BYTES;
    } while (--rows);
}

// Writes the column the last update drew into the background map, and on GBC its
// palettes. Called first in an update, so the write doesn't add to the update that
// found the column, and lands before the frame that scrolls it into view is drawn. In
// VBlank it writes VRAM directly; otherwise set_bkg_tiles waits for each tile (about
// 11 lines per column of bytes).
void level_stream_flush(void)
{
    if (pending_column == NO_COLUMN) return;
    uint8_t last_direct_line = is_cgb ? LAST_DIRECT_WRITE_LINE_CGB : LAST_DIRECT_WRITE_LINE;
    if (LY_REG >= VBLANK_FIRST_LINE && LY_REG <= last_direct_line) {
        uint8_t* map_cell = _SCRN0 + LEVEL_MAP_TOP_ROW * MAP_ROW_BYTES + pending_column;
        copy_column(map_cell, column_tiles);
        if (is_cgb) {
            VBK_REG = VBK_ATTRIBUTES;
            copy_column(map_cell, column_palettes);
            VBK_REG = VBK_TILES;
        }
    } else {
        set_bkg_tiles(pending_column, LEVEL_MAP_TOP_ROW, 1, LEVEL_MAP_ROWS, column_tiles);
        if (is_cgb) set_bkg_attributes(pending_column, LEVEL_MAP_TOP_ROW, 1, LEVEL_MAP_ROWS, column_palettes);
    }
    pending_column = NO_COLUMN;
}

// Draws one level column: its collision into the ring at once, its tiles into the
// background map at the next level_stream_flush().
static void draw_column(uint8_t column)
{
    if (column >= current_level.width) return;
    level_stream_flush();   // a second column in one update: write the first one now
    WITH_ROM_BANK(current_level.bank, read_column(column));
    add_objects(column);

    uint8_t* ring = &collision_ring[RING_INDEX(column, LEVEL_MAP_TOP_ROW)];
    uint8_t touched = 0;
    for (uint8_t row = 0; row < LEVEL_MAP_ROWS; row++) {
        uint8_t tile = column_tiles[row];
        uint8_t type = tile_collision[tile];
        column_palettes[row] = tile_palette[tile];
        ring[row] = type;
        if (type >= FIRST_TOUCHED_TYPE) touched = 1;
    }
    // A pickup leaves the flag set: a column scanned for nothing costs a little, no more.
    ring_column_touched[column & (RING_COLUMNS - 1)] = touched;
    pending_column = column & (RING_COLUMNS - 1);
}

// Draws every column the camera shows: for a level start, a restart or any big jump.
void level_stream_reset(int16_t camera_x)
{
    first_drawn_column = camera_x >> 3;
    for (uint8_t i = 0; i < VISIBLE_COLUMNS; i++) {
        draw_column(first_drawn_column + i);
    }
    level_stream_flush();
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

// Shows a tile at a level cell on screen, with its palette on GBC. In the column still
// waiting to be written? Changes it there too.
void level_show_tile(uint8_t tile_x, uint8_t tile_y, uint8_t tile)
{
    uint8_t column = tile_x & (RING_COLUMNS - 1);
    set_bkg_tiles(column, tile_y, 1, 1, &tile);
    if (is_cgb) {
        VBK_REG = VBK_ATTRIBUTES;
        set_bkg_tiles(column, tile_y, 1, 1, &tile_palette[tile]);
        VBK_REG = VBK_TILES;
    }
    if (pending_column == column) {
        column_tiles[tile_y - LEVEL_MAP_TOP_ROW] = tile;
        column_palettes[tile_y - LEVEL_MAP_TOP_ROW] = tile_palette[tile];
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
    level_show_tile(tile_x, tile_y, map_tile);
}

// Picks up the fragment at a tile: it stays collected even when its column is drawn
// again, and counts for its section.
void level_collect_fragment(uint8_t tile_x, uint8_t tile_y)
{
    for (uint8_t i = 0; i < fragment_count; i++) {
        if (fragment_column[i] == tile_x && fragment_row[i] == tile_y) {
            fragments_collected[i >> 3] |= 1 << (i & 7);
            current_level.section_collected[fragment_section[i]]++;
            break;
        }
    }
    restore_map_cell(tile_x, tile_y);
    sound_play(EFFECT_COLLECT);
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

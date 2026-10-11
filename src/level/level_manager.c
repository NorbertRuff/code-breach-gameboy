#include <string.h>
#include "level_manager.h"
#include "../graphics/hud.h"
#include "../graphics/programmer.h"
#include "../levels/greybox_test.h"
#include "../levels/level_1.h"
#include "../levels/level_2.h"
#include "../levels/level_3.h"
#include "../levels/level_4.h"
#include "../levels/level_5.h"
#include "../levels/test_jumps.h"
#include "../levels/test_movers.h"
#include "../engine/banking.h"
#include "../engine/palette.h"
#include "../engine/platform.h"
#include "../engine/text.h"
#include "../player/player.h"

Level current_level;
TilesetData current_tileset;
uint8_t level_index = LEVEL_1;
static uint8_t tileset_bank;
static const TilesetData* tileset_header;

// Read by the tests to notice Level struct changes.
const uint8_t level_struct_size = sizeof(Level);

// Shows the HUD window at the bottom of the screen and loads its frame tiles and, on
// GBC, its palettes: the chassis and the heart/floppy accents.
void set_hud_window_tiles(void)
{
    SHOW_WIN;
    move_win(7, 128);   // x 7 is the left screen edge
    WITH_BANK(hud, {
        set_win_data(HUD_TILE_BASE, hud_TILE_COUNT, hud_tiles);
        set_win_based_tiles(0, 0, hud_WIDTH >> 3, hud_HEIGHT >> 3, hud_map, HUD_TILE_BASE);
        palette_set_bkg(HUD_PALETTE, hud_PALETTE_COUNT, hud_palettes);
    });
    palette_use_win(0, 0, hud_WIDTH >> 3, hud_HEIGHT >> 3, HUD_PALETTE);
    palette_use_win(0, 0, 1, 1, HUD_ACCENT_PALETTE);
    palette_use_win(14, 0, 1, 1, HUD_ACCENT_PALETTE);
}

// Loads the player's sprite tiles and palette, its pale copy for invincibility, and the
// actors' palettes. The sheet carries the level set's 3 sprite palettes: the player's,
// the enemies' and the objects'.
void set_programmer_metasprite(void)
{
    WITH_BANK(programmer, {
        set_sprite_data(0, programmer_TILE_COUNT, programmer_tiles);
        palette_set_sprite(0, 1, programmer_palettes);
        palette_set_sprite(ENEMY_PALETTE, 1, programmer_palettes + PALETTE_COLORS);
        palette_set_sprite(OBJECT_PALETTE, 1, programmer_palettes + 2 * PALETTE_COLORS);
        palette_set_sprite(CHASER_PALETTE, 1, programmer_palettes + 3 * PALETTE_COLORS);
    });
    palette_set_pale_sprite(PLAYER_PALE_PALETTE, 0);
}

// Shows the number of lives next to the HUD frame's heart, in the top-left corner.
void hud_update_lives(uint8_t lives)
{
    uint8_t t_digit = FONT_DIGIT_TILE + (lives <= 9 ? lives : 9);
    set_win_tiles(1, 0, 1, 1, &t_digit);
}

// Shows "NN/TT" fragments in the HUD's top-right five tiles, each value capped at 99.
void hud_update_count(uint8_t collected, uint8_t total)
{
    uint8_t tiles[5];
    text_two_digit_tiles(collected, tiles);
    tiles[2] = FONT_SLASH_TILE;
    text_two_digit_tiles(total, tiles + 3);
    set_win_tiles(15, 0, 5, 1, tiles);
}

// Copies a level's header into current_level. Runs with the level's bank switched in.
static void copy_header(const LevelData* data)
{
    current_level.width = data->width;
    current_level.width_px = (uint16_t)data->width * TILE_SIZE;
    current_level.player_start_x = data->player_start_x;
    current_level.player_start_y = data->player_start_y;
    current_level.fragment_count = data->fragment_count;
    current_level.entity_count = data->entity_count;
    memcpy(current_level.section_start, data->section_start, LEVEL_SECTIONS);
    current_level.map = data->map;
    current_level.entities = data->entities;
    tileset_bank = (uint8_t)(uint16_t)data->tileset_bank;
    tileset_header = data->tileset;
}

// Loads the tileset's tiles from 0 and its palettes from LEVEL_PALETTE on.
static void load_tileset_art(void)
{
    WITH_ROM_BANK((uint8_t)(uint16_t)current_tileset.tiles_bank, {
        set_bkg_data(0, current_tileset.tile_count, current_tileset.tiles);
        palette_set_bkg(LEVEL_PALETTE, current_tileset.palette_count, current_tileset.palettes);
    });
}

// Draws the parallax band into rows 0-2 of the background map, once per level: its 32
// columns wrap with the map, and streaming never writes these rows. Its tiles follow the
// tileset's. Dual-palette on GBC when the band provides an accent palette.
static void draw_band(void)
{
    uint8_t base = current_tileset.tile_count;
    WITH_ROM_BANK((uint8_t)(uint16_t)current_tileset.band_bank, {
        set_bkg_data(base, current_tileset.band_tile_count, current_tileset.band_tiles);
        set_bkg_based_tiles(0, 0, RING_COLUMNS, LEVEL_MAP_TOP_ROW, current_tileset.band_map, base);
        palette_set_bkg(BAND_PALETTE, 1, current_tileset.band_palettes);
        if (current_tileset.band_palette_count > 1) {
            palette_set_bkg(BAND_ACCENT_PALETTE, 1, current_tileset.band_palettes + PALETTE_COLORS);
        }
    });
    palette_use_bkg(0, 0, RING_COLUMNS, LEVEL_MAP_TOP_ROW, BAND_PALETTE);
    if (is_cgb && current_tileset.band_palette_count > 1) {
        WITH_ROM_BANK((uint8_t)(uint16_t)current_tileset.band_bank, {
            VBK_REG = VBK_ATTRIBUTES;
            const uint8_t* attr = current_tileset.band_attributes;
            for (uint8_t y = 0; y < LEVEL_MAP_TOP_ROW; y++) {
                for (uint8_t x = 0; x < RING_COLUMNS; x++, attr++) {
                    if (*attr) set_bkg_tile_xy(x, y, BAND_ACCENT_PALETTE);
                }
            }
            VBK_REG = VBK_TILES;
        });
    }
}

// The levels by index (LEVEL_1, ...): each header's ROM bank and address. A bank is the
// address of its BANKREF symbol (what BANK() reads), kept as a pointer so the table is
// a constant.
static const void* const level_banks[LEVEL_COUNT] = {
    &__bank_level_1, &__bank_level_2, &__bank_level_3, &__bank_level_4, &__bank_level_5,
    &__bank_greybox_test, &__bank_test_jumps, &__bank_test_movers
};
static const LevelData* const level_headers[LEVEL_COUNT] = {
    &level_1, &level_2, &level_3, &level_4, &level_5, &greybox_test, &test_jumps, &test_movers
};

// Sets up a level behind a dark screen: header, tileset, band, HUD. The columns are
// drawn by level_stream_reset() once the camera is placed.
void level_load(uint8_t index)
{
    current_level.bank = (uint8_t)(uint16_t)level_banks[index];
    WITH_ROM_BANK(current_level.bank, copy_header(level_headers[index]));
    WITH_ROM_BANK(tileset_bank, current_tileset = *tileset_header);
    level_stream_start(tileset_bank);
    load_tileset_art();
    text_load_font();
    draw_band();
    set_hud_window_tiles();
    set_programmer_metasprite();
}

// Copies entity number index of the level out of ROM.
void level_read_entity(uint8_t index, LevelEntity* entity)
{
    WITH_ROM_BANK(current_level.bank, *entity = current_level.entities[index]);
}

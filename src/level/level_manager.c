#include <string.h>
#include "level_manager.h"
#include "../graphics/greybox.h"
#include "../graphics/greybox_band.h"
#include "../graphics/hud.h"
#include "../graphics/hud_digits.h"
#include "../graphics/programmer.h"
#include "../levels/greybox_test.h"
#include "../levels/level_1.h"
#include "../levels/level_2.h"
#include "../levels/level_3.h"
#include "../levels/level_4.h"
#include "../levels/test_jumps.h"
#include "../levels/test_movers.h"
#include "../engine/banking.h"
#include "../engine/palette.h"
#include "../player/player.h"

Level current_level;
uint8_t level_index = LEVEL_1;

// Read by the tests to notice Level struct changes.
const uint8_t level_struct_size = sizeof(Level);

#define BAND_TILE_BASE greybox_TILE_COUNT   // band tiles right after the level's tiles

// Shows the HUD window at the bottom of the screen and loads its frame tiles and, on
// GBC, its palette.
void set_hud_window_tiles(void)
{
    SHOW_WIN;
    move_win(7, 128);   // x 7 is the left screen edge
    WITH_BANK(hud, {
        set_win_data(HUD_TILE_BASE, hud_TILE_COUNT, hud_tiles);
        set_win_based_tiles(0, 0, hud_WIDTH >> 3, hud_HEIGHT >> 3, hud_map, HUD_TILE_BASE);
        palette_set_bkg(HUD_PALETTE, 1, hud_palettes);
    });
    palette_use_win(0, 0, hud_WIDTH >> 3, hud_HEIGHT >> 3, HUD_PALETTE);
}

// Loads the player's sprite tiles and palette, and its pale copy for invincibility.
void set_programmer_metasprite(void)
{
    WITH_BANK(programmer, {
        set_sprite_data(0, programmer_TILE_COUNT, programmer_tiles);
        palette_set_sprite(0, programmer_PALETTE_COUNT, programmer_palettes);
    });
    palette_set_pale_sprite(PLAYER_PALE_PALETTE, 0);
}

// Shows a heart and the number of lives in the HUD's top-left corner.
void hud_update_lives(uint8_t lives)
{
    uint8_t t_heart = HUD_HEART_TILE;
    uint8_t t_digit = HUD_DIGIT_TILE_BASE + (lives <= 9 ? lives : 9);
    set_win_tiles(0, 0, 1, 1, &t_heart);
    set_win_tiles(1, 0, 1, 1, &t_digit);
}

// Writes a value 0-99 as two digit tiles. Counts the tens by subtracting: the CPU has no
// division, and the library's takes about 6 screen lines.
static void write_two_digits(uint8_t value, uint8_t* tiles)
{
    uint8_t tens = 0;
    while (value >= 10) {
        value -= 10;
        tens++;
    }
    tiles[0] = HUD_DIGIT_TILE_BASE + tens;
    tiles[1] = HUD_DIGIT_TILE_BASE + value;
}

// Shows "NN/TT" fragments in the HUD's top-right five tiles, each value capped at 99.
void hud_update_count(uint8_t collected, uint8_t total)
{
    uint8_t tiles[5];
    if (collected > 99) collected = 99;
    if (total > 99) total = 99;
    write_two_digits(collected, tiles);
    tiles[2] = HUD_SLASH_TILE;
    write_two_digits(total, tiles + 3);
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
}

// Draws the parallax band into rows 0-2 of the background map, once per level: its 32
// columns wrap with the map, and streaming never writes these rows.
static void draw_band(void)
{
    WITH_BANK(greybox_band, {
        set_bkg_data(BAND_TILE_BASE, greybox_band_TILE_COUNT, greybox_band_tiles);
        set_bkg_based_tiles(0, 0, RING_COLUMNS, LEVEL_MAP_TOP_ROW, greybox_band_map, BAND_TILE_BASE);
        palette_set_bkg(BAND_PALETTE, 1, greybox_band_palettes);
    });
    palette_use_bkg(0, 0, RING_COLUMNS, LEVEL_MAP_TOP_ROW, BAND_PALETTE);
}

// The levels by index (LEVEL_1, ...): each header's ROM bank and address. A bank is the
// address of its BANKREF symbol (what BANK() reads), kept as a pointer so the table is
// a constant.
static const void* const level_banks[LEVEL_COUNT] = {
    &__bank_level_1, &__bank_level_2, &__bank_level_3, &__bank_level_4,
    &__bank_greybox_test, &__bank_test_jumps, &__bank_test_movers
};
static const LevelData* const level_headers[LEVEL_COUNT] = {
    &level_1, &level_2, &level_3, &level_4, &greybox_test, &test_jumps, &test_movers
};

// Sets up a level behind a dark screen: header, tiles, HUD. The columns are drawn by
// level_stream_reset() once the camera is placed.
void level_load(uint8_t index)
{
    current_level.bank = (uint8_t)(uint16_t)level_banks[index];
    WITH_ROM_BANK(current_level.bank, copy_header(level_headers[index]));
    level_stream_start();

    WITH_BANK(greybox, {
        set_bkg_data(0, greybox_TILE_COUNT, greybox_tiles);
        palette_set_bkg(LEVEL_PALETTE, 1, greybox_palettes);
    });
    WITH_BANK(hud_digits, set_bkg_data(HUD_DIGIT_TILE_BASE, hud_digits_TILE_COUNT, hud_digits_tiles));
    draw_band();
    set_hud_window_tiles();
    set_programmer_metasprite();
}

// Copies entity number index of the level out of ROM.
void level_read_entity(uint8_t index, LevelEntity* entity)
{
    WITH_ROM_BANK(current_level.bank, *entity = current_level.entities[index]);
}

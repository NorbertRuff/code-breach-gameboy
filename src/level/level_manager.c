#include <string.h>
#include "level_manager.h"
#include "../graphics/greybox.h"
#include "../graphics/hud.h"
#include "../graphics/programmer.h"
#include "../levels/greybox_test.h"
#include "../levels/test_jumps.h"
#include "../engine/banking.h"

Level current_level;
uint8_t level_index = LEVEL_GREYBOX_TEST;

// Read by the tests to notice Level struct changes.
const uint8_t level_struct_size = sizeof(Level);

// 8x8 2bpp digit tiles for HUD counter.
// Each row: two identical bytes (same bit pattern for both bit-planes → color 0 or 3 only).
// Tiles 0-9 map to digits '0'-'9', tile 10 maps to '/'.
static const uint8_t hud_digit_tiles[12 * 16] = {
    // '0'
    0x3C,0x3C, 0x66,0x66, 0x6E,0x6E, 0x76,0x76, 0x66,0x66, 0x66,0x66, 0x3C,0x3C, 0x00,0x00,
    // '1'
    0x18,0x18, 0x38,0x38, 0x18,0x18, 0x18,0x18, 0x18,0x18, 0x18,0x18, 0x7E,0x7E, 0x00,0x00,
    // '2'
    0x3C,0x3C, 0x66,0x66, 0x06,0x06, 0x0C,0x0C, 0x18,0x18, 0x30,0x30, 0x7E,0x7E, 0x00,0x00,
    // '3'
    0x3C,0x3C, 0x66,0x66, 0x06,0x06, 0x1C,0x1C, 0x06,0x06, 0x66,0x66, 0x3C,0x3C, 0x00,0x00,
    // '4'
    0x0C,0x0C, 0x1C,0x1C, 0x3C,0x3C, 0x6C,0x6C, 0x7E,0x7E, 0x0C,0x0C, 0x0C,0x0C, 0x00,0x00,
    // '5'
    0x7E,0x7E, 0x60,0x60, 0x7C,0x7C, 0x06,0x06, 0x06,0x06, 0x66,0x66, 0x3C,0x3C, 0x00,0x00,
    // '6'
    0x3C,0x3C, 0x66,0x66, 0x60,0x60, 0x7C,0x7C, 0x66,0x66, 0x66,0x66, 0x3C,0x3C, 0x00,0x00,
    // '7'
    0x7E,0x7E, 0x06,0x06, 0x06,0x06, 0x0C,0x0C, 0x18,0x18, 0x18,0x18, 0x18,0x18, 0x00,0x00,
    // '8'
    0x3C,0x3C, 0x66,0x66, 0x66,0x66, 0x3C,0x3C, 0x66,0x66, 0x66,0x66, 0x3C,0x3C, 0x00,0x00,
    // '9'
    0x3C,0x3C, 0x66,0x66, 0x66,0x66, 0x3E,0x3E, 0x06,0x06, 0x66,0x66, 0x3C,0x3C, 0x00,0x00,
    // '/'
    0x00,0x00, 0x06,0x06, 0x0C,0x0C, 0x18,0x18, 0x30,0x30, 0x60,0x60, 0x00,0x00, 0x00,0x00,
    // heart ♥
    0x00,0x00, 0x66,0x66, 0xFF,0xFF, 0xFF,0xFF, 0x7E,0x7E, 0x3C,0x3C, 0x18,0x18, 0x00,0x00,
};

// Shows the HUD window at the bottom of the screen and loads its frame tiles.
void set_hud_window_tiles(void)
{
    SHOW_WIN;
    move_win(7, 128);   // x 7 is the left screen edge
    WITH_BANK(hud, {
        set_win_data(HUD_TILE_BASE, hud_TILE_COUNT, hud_tiles);
        set_win_based_tiles(0, 0, hud_WIDTH >> 3, hud_HEIGHT >> 3, hud_map, HUD_TILE_BASE);
    });
}

// Loads the player's sprite tiles and palette.
void set_programmer_metasprite(void)
{
    WITH_BANK(programmer, {
        set_sprite_data(0, programmer_TILE_COUNT, programmer_tiles);
        set_sprite_palette(0, programmer_PALETTE_COUNT, programmer_palettes);
    });
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

// Sets up a level behind a dark screen: header, tiles, HUD. The columns are drawn by
// level_stream_reset() once the camera is placed.
void level_load(uint8_t index)
{
    if (index == LEVEL_TEST_JUMPS) {
        current_level.bank = BANK(test_jumps);
        WITH_BANK(test_jumps, copy_header(&test_jumps));
    } else {
        current_level.bank = BANK(greybox_test);
        WITH_BANK(greybox_test, copy_header(&greybox_test));
    }
    level_stream_start();

    WITH_BANK(greybox, set_bkg_data(0, greybox_TILE_COUNT, greybox_tiles));
    set_bkg_data(HUD_DIGIT_TILE_BASE, 12, hud_digit_tiles);
    fill_bkg_rect(0, 0, RING_COLUMNS, LEVEL_MAP_TOP_ROW, 0);   // the parallax band, empty until M3
    set_hud_window_tiles();
    set_programmer_metasprite();
}

// Copies entity number index of the level out of ROM.
void level_read_entity(uint8_t index, LevelEntity* entity)
{
    WITH_ROM_BANK(current_level.bank, *entity = current_level.entities[index]);
}

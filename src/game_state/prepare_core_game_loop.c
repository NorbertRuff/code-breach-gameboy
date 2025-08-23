#include <gb/gb.h>

#include "../graphics/background.h"
#include "../graphics/hud.h"
#include "../graphics/programmer.h"

#include "../utils.h"

void set_hud_window_tiles()
{
    SHOW_WIN;

    
    scroll_win(0, 128);

    
    // Set the tiles for our hud window in vram
    set_win_data(background_TILE_COUNT, hud_TILE_COUNT, hud_tiles);
    set_win_based_tiles(0, 0, hud_WIDTH >> 3, hud_HEIGHT >> 3, hud_map, background_TILE_COUNT);

    // Hearts counter
 //  set_win_based_tiles(1, 0, 2, 2, hud_tiles, 63);
  //  set_win_based_tiles(3, 0, 2, 2, hud_tiles, 63);
  //  set_win_based_tiles(5, 0, 2, 2, hud_tiles, 63);

    // Points counter 77 78 79 === 001
 //   set_win_based_tiles(16, 1, 1, 1, hud_tiles, 77);
 //   set_win_based_tiles(17, 1, 1, 1, hud_tiles, 77);
 //   set_win_based_tiles(18, 1, 1, 1, hud_tiles, 78);
}


void set_background_tiles()
{
    #if EMPTY_BACKGROUND
        uint8_t empty_tile[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        
        uint8_t empty_palette[4] = {RGB8(255, 255, 255), RGB8(0, 0, 0), RGB8(0, 0, 0), RGB8(0, 0, 0)};
        
        set_bkg_data(0, 1, empty_tile);
        set_bkg_palette(0, 1, empty_palette);
        
        // Fill entire background with empty tiles
        for (uint8_t y = 0; y < 18; y++) {
            for (uint8_t x = 0; x < 32; x++) {
                uint8_t tile_index = 0;
                set_bkg_tiles(x, y, 1, 1, &tile_index);
            }
        }
    #else
        // Set the tiles for our background in vram
        set_bkg_data(0, background_TILE_COUNT, background_tiles);
        // Set the colors used by our background also into vram
        set_bkg_palette(0, background_PALETTE_COUNT, background_palettes);
        set_bkg_tiles(0, 1, background_WIDTH >> 3, background_HEIGHT >> 3, background_map_attributes);
        set_bkg_tiles(0, 0, background_WIDTH >> 3, background_HEIGHT >> 3, background_map);
    #endif
}

void set_programmer_metasprite()
{
    // Set the programmer metasprite
    set_sprite_data(0, programmer_TILE_COUNT, programmer_tiles);
    set_sprite_palette(0, programmer_PALETTE_COUNT, programmer_palettes);
}

// Prepare the tile data and background
void prepare_core_game_loop()
{
    set_background_tiles();
    set_hud_window_tiles();
    set_programmer_metasprite();
}

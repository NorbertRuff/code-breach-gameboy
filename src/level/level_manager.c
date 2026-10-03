#include "level_manager.h"
#include "../graphics/background.h"
#include "../graphics/hud.h"
#include "../graphics/programmer.h"
#include "../sound/sound.h"
#include "../engine/banking.h"

// Background tiles for the level's debug view (shared VRAM block 128-255).
#define SOLID_TILE_INDEX       128
#define COLLECTIBLE_TILE_INDEX 132

// 8x8 2bpp block for solid ground: black border, white top-left bevel, light gray
// face, dark gray bottom-right shade. Readable in 4 shades. (Review G2; becomes the
// first greybox tile in roadmap M2.) Each row: low bit-plane byte, high bit-plane byte.
static const uint8_t solid_block_tile_data[16] = {
    0xFF,0xFF,   // 3 3 3 3 3 3 3 3
    0x83,0x81,   // 3 0 0 0 0 0 1 3
    0xBD,0x83,   // 3 0 1 1 1 1 2 3
    0xBD,0x83,
    0xBD,0x83,
    0xBD,0x83,
    0xC1,0xBF,   // 3 1 2 2 2 2 2 3
    0xFF,0xFF,
};

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

// 8x8 diamond / gem shape for collectible tiles
static const uint8_t collectible_tile_data[16] = {
    0x00, 0x00,
    0x18, 0x18,
    0x3C, 0x3C,
    0x7E, 0x7E,
    0x7E, 0x7E,
    0x3C, 0x3C,
    0x18, 0x18,
    0x00, 0x00
};

// HUD and scrolling setup functions
void set_hud_window_tiles()
{
    SHOW_WIN;
    
    // Position HUD window at bottom of screen. move_win() is absolute; scroll_win() is
    // relative and would move the window again on every level load. WX = 7 is the
    // left screen edge.
    move_win(7, 128);
    
    // Set the tiles for our HUD window in VRAM
    WITH_BANK(hud, {
        set_win_data(background_TILE_COUNT, hud_TILE_COUNT, hud_tiles);
        set_win_based_tiles(0, 0, hud_WIDTH >> 3, hud_HEIGHT >> 3, hud_map, background_TILE_COUNT);
    });
}

void set_programmer_metasprite()
{
    // Set the programmer metasprite
    WITH_BANK(programmer, {
        set_sprite_data(0, programmer_TILE_COUNT, programmer_tiles);
        set_sprite_palette(0, programmer_PALETTE_COUNT, programmer_palettes);
    });
}

void hud_update_lives(uint8_t lives)
{
    uint8_t t_heart = HUD_HEART_TILE;
    uint8_t t_digit = HUD_DIGIT_TILE_BASE + (lives <= 9 ? lives : 9);
    set_win_tiles(0, 0, 1, 1, &t_heart);
    set_win_tiles(1, 0, 1, 1, &t_digit);
}

void hud_update_count(uint8_t collected, uint8_t total)
{
    // Display "NN/TT" in the top-right five tiles of the HUD window (cols 15-19, row 0),
    // with leading zeros. Each value is capped at 99.
    uint8_t tiles[5];
    if (collected > 99) collected = 99;
    if (total > 99) total = 99;
    tiles[0] = HUD_DIGIT_TILE_BASE + collected / 10;
    tiles[1] = HUD_DIGIT_TILE_BASE + collected % 10;
    tiles[2] = HUD_SLASH_TILE;
    tiles[3] = HUD_DIGIT_TILE_BASE + total / 10;
    tiles[4] = HUD_DIGIT_TILE_BASE + total % 10;
    set_win_tiles(15, 0, 5, 1, tiles);
}

// Build the debug view of the level: solid tiles drawn as plain blocks, collectibles
// as diamonds, everything else from the background map. Reads background_map, so the
// caller must have the "background" bank mapped in.
static void build_level_view(Level* level, uint8_t view[LEVEL_HEIGHT][LEVEL_WIDTH])
{
    for (int y = 0; y < LEVEL_HEIGHT; y++) {
        for (int x = 0; x < LEVEL_WIDTH; x++) {
            if (level->collision_map[y][x] == COLLISION_SOLID) {
                view[y][x] = SOLID_TILE_INDEX;
            } else if (level->collision_map[y][x] == COLLISION_COLLECTIBLE) {
                view[y][x] = COLLECTIBLE_TILE_INDEX;
            } else {
                view[y][x] = level->background_map[y * LEVEL_WIDTH + x];
            }
        }
    }
}

void level_manager_init(LevelManager* manager)
{
    manager->current_level_id = 0;
    manager->total_levels = MAX_LEVELS;
    manager->level_state = LEVEL_STATE_LOADING;
    
    // Initialize current level
    manager->current_level.level_id = 0;
    manager->current_level.state = LEVEL_STATE_LOADING;
    manager->current_level.background_tiles = NULL;
    manager->current_level.background_map = NULL;
    manager->current_level.collectible_count = 0;
    manager->current_level.collected_count = 0;
    manager->current_level.required_keys = 0;
    manager->current_level.collected_keys = 0;
}

void level_load_level(LevelManager* manager, uint8_t level_id)
{
    // Unload current level first
    level_unload_level(manager);
    
    // Set loading state
    manager->level_state = LEVEL_STATE_LOADING;
    
    // Load specific level data
    switch (level_id) {
        case 1:
            level_1_server_room_init(&manager->current_level);
            break;
        default:
            // Default level (empty)
            break;
    }
    
    // Load background tiles into VRAM (Game Boy optimization)
    if (manager->current_level.background_tiles) {
        WITH_BANK(background, set_bkg_data(0, background_TILE_COUNT, manager->current_level.background_tiles));
    }
    
    // Load the solid block tile for the collision view
    set_bkg_data(SOLID_TILE_INDEX, 1, solid_block_tile_data);

    // Load collectible tile
    set_bkg_data(COLLECTIBLE_TILE_INDEX, 1, collectible_tile_data);

    // Load HUD digit/symbol tiles (0-9, '/', heart) at VRAM 133-144
    set_bkg_data(HUD_DIGIT_TILE_BASE, 12, hud_digit_tiles);

    // Create and load collision visualization map (static: 32×18=576 bytes avoids stack overflow)
    static uint8_t collision_vis_map[LEVEL_HEIGHT][LEVEL_WIDTH];
    WITH_BANK(background, build_level_view(&manager->current_level, collision_vis_map));
    
    // Load the collision visualization map
    set_bkg_tiles(0, 0, LEVEL_WIDTH, LEVEL_HEIGHT, (uint8_t*)collision_vis_map);
    
    // Set up HUD and sprites
    set_hud_window_tiles();
    set_programmer_metasprite();

    // Write initial count to HUD (0 / total)
    hud_update_count(0, manager->current_level.collectible_count);
    
    // Set level as active
    manager->current_level_id = level_id;
    manager->current_level.state = LEVEL_STATE_ACTIVE;
    manager->level_state = LEVEL_STATE_ACTIVE;
}

void level_unload_level(LevelManager* manager)
{
    // Clear background
    // Note: On Game Boy, we don't need to explicitly clear VRAM
    // The new tiles will overwrite the old ones
    
    // Reset level data
    manager->current_level.state = LEVEL_STATE_LOADING;
    manager->current_level.background_tiles = NULL;
    manager->current_level.background_map = NULL;
    
    // Clear collision map
    for (int y = 0; y < LEVEL_HEIGHT; y++) {
        for (int x = 0; x < LEVEL_WIDTH; x++) {
            manager->current_level.collision_map[y][x] = COLLISION_NONE;
        }
    }
}

void level_update(LevelManager* manager)
{
    if (manager->level_state != LEVEL_STATE_ACTIVE) {
        return;
    }
    
    // Update level-specific logic here
    // For now, just basic state management
}

void level_render(LevelManager* manager)
{
    if (manager->level_state != LEVEL_STATE_ACTIVE) {
        return;
    }
    
    // Background with collision visualization is already rendered via set_bkg_tiles
    // No additional rendering needed for now
}

// Level 1: Server Room implementation
void level_1_server_room_init(Level* level)
{
    level->level_id = 1;
    level->state = LEVEL_STATE_LOADING;
    
    // Set background data (using existing background for now)
    level->background_tiles = background_tiles;
    level->background_map = background_map;
    
    // Set player start position (bottom left of screen)
    level->player_start_x = 8;   // sprite spans x 0-15, clear of platform A (x 16-47)
    level->player_start_y = 119; // feet on the pixel row above the ground tiles (row 15 starts at 120),
                                 // where the collision code puts them after every landing
    
    // Set level exit (top right area)
    level->exit_x = 144;  // 18 tiles from left
    level->exit_y = 16;   // 2 tiles from top
    
    // Initialize collision map for server room
    for (int y = 0; y < LEVEL_HEIGHT; y++) {
        for (int x = 0; x < LEVEL_WIDTH; x++) {
            level->collision_map[y][x] = COLLISION_NONE;
        }
    }

    // The HUD window covers rows 16-17, so the level uses rows 0-15 and the ground is
    // row 15. (Review L1: everything was 2 rows lower and the floor was under the HUD.)
    // Rows 16-17 stay empty; falling into them leads to the fall death below the level.

    // Ground: full bottom row (all 32 cols)
    for (int x = 0; x < LEVEL_WIDTH; x++) {
        level->collision_map[15][x] = COLLISION_SOLID;
    }

    // Platform A: low-left  (row 12, cols 2-5)
    for (int x = 2; x <= 5; x++) {
        level->collision_map[12][x] = COLLISION_SOLID;
    }

    // Platform B: mid-center (row 9, cols 8-11)
    for (int x = 8; x <= 11; x++) {
        level->collision_map[9][x] = COLLISION_SOLID;
    }

    // Platform C: high-right (row 6, cols 14-17)
    for (int x = 14; x <= 17; x++) {
        level->collision_map[6][x] = COLLISION_SOLID;
    }

    // Platform D: mid-right (row 10, cols 21-24) — descends from C
    for (int x = 21; x <= 24; x++) {
        level->collision_map[10][x] = COLLISION_SOLID;
    }

    // Platform E: high far-right (row 7, cols 27-30)
    for (int x = 27; x <= 30; x++) {
        level->collision_map[7][x] = COLLISION_SOLID;
    }

    // Collectibles — placed at body height (player->y - TILE_SIZE) above each surface
    // Ground level (body row = 13)
    level->collision_map[13][2]  = COLLISION_COLLECTIBLE;
    level->collision_map[13][7]  = COLLISION_COLLECTIBLE;
    level->collision_map[13][12] = COLLISION_COLLECTIBLE;
    level->collision_map[13][19] = COLLISION_COLLECTIBLE;  // guides right
    // Platform A (body row = 10)
    level->collision_map[10][3]  = COLLISION_COLLECTIBLE;
    level->collision_map[10][4]  = COLLISION_COLLECTIBLE;
    // Platform B (body row = 7)
    level->collision_map[7][9]   = COLLISION_COLLECTIBLE;
    level->collision_map[7][10]  = COLLISION_COLLECTIBLE;
    // Platform C (body row = 4)
    level->collision_map[4][15]  = COLLISION_COLLECTIBLE;
    // Platform D (body row = 8)
    level->collision_map[8][22]  = COLLISION_COLLECTIBLE;
    level->collision_map[8][23]  = COLLISION_COLLECTIBLE;
    // Platform E (body row = 5)
    level->collision_map[5][28]  = COLLISION_COLLECTIBLE;

    // Set collectibles
    level->collectible_count = 12;
    level->collected_count = 0;
    
    // Set required keys
    level->required_keys = 2;
    level->collected_keys = 0;
    
    // Initialize object spawns (enemies, collectibles)
    // For now, just basic collectibles
    level->spawns[0].type = COLLISION_COLLECTIBLE;
    level->spawns[0].x = 4;
    level->spawns[0].y = 11;
    
    level->spawns[1].type = COLLISION_COLLECTIBLE;
    level->spawns[1].x = 14;
    level->spawns[1].y = 8;
    
    level->spawns[2].type = COLLISION_COLLECTIBLE;
    level->spawns[2].x = 6;
    level->spawns[2].y = 5;
}

// Collision detection function
uint8_t level_check_collision(Level* level, uint16_t x, uint16_t y, uint16_t scroll_x)
{
    // Convert world coordinates to tile coordinates
    uint8_t tile_x = (x + scroll_x) / TILE_SIZE;
    uint8_t tile_y = y / TILE_SIZE;

    // Bounds checking — treat out-of-bounds as no collision
    if (tile_x >= LEVEL_WIDTH || tile_y >= LEVEL_HEIGHT) {
        return COLLISION_NONE;
    }

    return level->collision_map[tile_y][tile_x];
}

// Handle collision response
void level_handle_collision(Level* level, uint8_t collision_type, uint8_t tile_x, uint8_t tile_y)
{
    switch (collision_type) {
        case COLLISION_HAZARD:
            // TODO: Implement damage system
            break;

        case COLLISION_COLLECTIBLE:
            level->collision_map[tile_y][tile_x] = COLLISION_NONE;
            level->collected_count++;
            sound_play_collect();
            {
                uint8_t bg_tile;
                WITH_BANK(background, bg_tile = level->background_map[tile_y * LEVEL_WIDTH + tile_x]);
                set_bkg_tiles(tile_x, tile_y, 1, 1, &bg_tile);
            }
            break;

        default:
            break;
    }
}

#ifndef LEVEL_MANAGER_H
#define LEVEL_MANAGER_H

#include <gb/gb.h>
#include <stdint.h>

// Level system constants
#define MAX_LEVELS 5
#define LEVEL_WIDTH 32
#define MAX_SCROLL_X ((LEVEL_WIDTH * TILE_SIZE) - 160)  // pixels the camera can scroll (96)
#define LEVEL_HEIGHT 18
#define TILE_SIZE 8

// Level states
#define LEVEL_STATE_LOADING 0
#define LEVEL_STATE_ACTIVE 1
#define LEVEL_STATE_COMPLETED 2
#define LEVEL_STATE_FAILED 3

// Collision types for tiles
#define COLLISION_NONE 0
#define COLLISION_SOLID 1
#define COLLISION_ONE_WAY 2
#define COLLISION_HAZARD 3
#define COLLISION_COLLECTIBLE 4

// Level data structure (memory-efficient for Game Boy)
typedef struct {
    uint8_t level_id;
    uint8_t state;
    
    // Background data (stored in VRAM)
    uint8_t* background_tiles;
    uint8_t* background_map;
    
    // Collision data (compact storage)
    uint8_t collision_map[LEVEL_HEIGHT][LEVEL_WIDTH];
    
    // Object spawns (efficient object-based storage)
    struct {
        uint8_t type;
        uint8_t x;
        uint8_t y;
    } spawns[16];  // Max 16 objects per level
    
    // Player start position
    uint16_t player_start_x;
    uint16_t player_start_y;
    
    // Level exit
    uint16_t exit_x;
    uint16_t exit_y;
    
    // Collectibles
    uint8_t collectible_count;
    uint8_t collected_count;
    
    // Level requirements
    uint8_t required_keys;
    uint8_t collected_keys;
} Level;

// Level manager structure
typedef struct {
    Level current_level;
    uint8_t current_level_id;
    uint8_t total_levels;
    uint8_t level_state;
} LevelManager;

// Function declarations
void level_manager_init(LevelManager* manager);
void level_load_level(LevelManager* manager, uint8_t level_id);
void level_unload_level(LevelManager* manager);
void level_update(LevelManager* manager);
void level_render(LevelManager* manager);

// HUD digit tiles — loaded at VRAM indices 133-143 (after collectible at 132)
#define HUD_DIGIT_TILE_BASE 133   // tile 133 = '0', 134 = '1', ..., 142 = '9'
#define HUD_SLASH_TILE      143   // tile 143 = '/'
#define HUD_HEART_TILE      144   // tile 144 = '♥'

// HUD and sprite setup functions
void set_hud_window_tiles();
void set_programmer_metasprite();
void hud_update_count(uint8_t collected, uint8_t total);
void hud_update_lives(uint8_t lives);

// Level-specific functions
void level_1_server_room_init(Level* level);
uint8_t level_check_collision(Level* level, uint16_t x, uint16_t y, uint16_t scroll_x);
void level_handle_collision(Level* level, uint8_t collision_type, uint8_t tile_x, uint8_t tile_y);

#endif // LEVEL_MANAGER_H

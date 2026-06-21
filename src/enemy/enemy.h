#ifndef ENEMY_H
#define ENEMY_H

#include <gb/gb.h>
#include <stdint.h>

#define MAX_ENEMIES 4

// Sprite VRAM slots 40-43: TL, BL, TR, BR (reordered for 8x16 mode)
#define ENEMY_SPRITE_BASE_TILE 40
// OAM slots 0-3 used by player; enemies start at 4 (2 OAM entries per enemy)
#define ENEMY_OAM_BASE 4

#define ENEMY_HALF_W  8   // half sprite width (16px total)
#define ENEMY_HEIGHT 16   // sprite height

typedef struct {
    uint16_t x;           // world X (center)
    uint8_t  y;           // screen/world Y (feet)
    int8_t   dir;         // 1 = right, -1 = left
    uint16_t patrol_min;  // world X min for center
    uint16_t patrol_max;  // world X max for center
    uint8_t  active;
} Enemy;

void enemy_init_sprite_data(void);
void enemy_init(Enemy* e, uint16_t x, uint8_t y, uint16_t patrol_min, uint16_t patrol_max);
void enemy_update(Enemy* e);
void enemy_draw(Enemy* e, uint8_t oam_base, int16_t camera_x);
void enemy_hide(uint8_t oam_base);
uint8_t enemy_check_player_collision(Enemy* e, uint16_t player_x, uint8_t player_y);

#endif // ENEMY_H

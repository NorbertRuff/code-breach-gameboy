#include "enemy.h"
#include "../player/player.h"

// Read by the tests to notice Enemy struct changes.
const uint8_t enemy_struct_size = sizeof(Enemy);

// Mushroom tiles reordered for 8x16 sprite mode.
// Original mushroom_tiles layout (png2asset row-major): [TL][TR][BL][BR]
// 8x16 sprite mode needs pairs stacked vertically: left col=[TL,BL], right col=[TR,BR]
// So sprite VRAM order: slot 40=TL, slot 41=BL, slot 42=TR, slot 43=BR
static const uint8_t mushroom_sprite_tiles[64] = {
    // Slot 40: top-left  (mushroom_tiles bytes 0-15)
    0x07,0xff, 0x19,0xf8, 0x21,0xe0, 0x43,0xc0, 0x46,0xc0, 0xfc,0x80, 0xcc,0x80, 0x86,0x80,
    // Slot 41: bot-left  (mushroom_tiles bytes 32-47)
    0x87,0x80, 0xcf,0x80, 0xff,0x8f, 0x72,0xf2, 0x22,0xe2, 0x20,0xe0, 0x10,0xf0, 0x0f,0xff,
    // Slot 42: top-right (mushroom_tiles bytes 16-31)
    0xe0,0xff, 0xf8,0x1f, 0xe4,0x07, 0xf2,0x03, 0x1a,0x03, 0x0f,0x01, 0x0f,0x01, 0x19,0x01,
    // Slot 43: bot-right (mushroom_tiles bytes 48-63)
    0xe1,0x01, 0xf3,0x01, 0xff,0xf1, 0x4e,0x4f, 0x44,0x47, 0x04,0x07, 0x08,0x0f, 0xf0,0xff
};

void enemy_init_sprite_data(void)
{
    set_sprite_data(ENEMY_SPRITE_BASE_TILE, 4, mushroom_sprite_tiles);
}

void enemy_init(Enemy* e, uint16_t x, uint8_t y, uint16_t patrol_min, uint16_t patrol_max)
{
    e->x          = x;
    e->y          = y;
    e->dir        = 1;
    e->patrol_min = patrol_min;
    e->patrol_max = patrol_max;
    e->active     = 1;
}

void enemy_update(Enemy* e)
{
    if (!e->active) return;
    int16_t nx = (int16_t)e->x + e->dir;
    if (nx <= (int16_t)e->patrol_min) { nx = (int16_t)e->patrol_min; e->dir =  1; }
    if (nx >= (int16_t)e->patrol_max) { nx = (int16_t)e->patrol_max; e->dir = -1; }
    e->x = (uint16_t)nx;
}

void enemy_hide(uint8_t oam_base)
{
    move_sprite(oam_base,   0, 0);
    move_sprite(oam_base+1, 0, 0);
}

void enemy_draw(Enemy* e, uint8_t oam_base, int16_t camera_x)
{
    if (!e->active) {
        enemy_hide(oam_base);
        return;
    }

    int16_t sx = (int16_t)e->x - camera_x;
    if (sx < -8 || sx > 168) {
        enemy_hide(oam_base);
        return;
    }

    // GB hardware OAM offset: visible x starts at oam_x=8, visible y starts at oam_y=16.
    // Enemy center is at sx; sprite is 16px wide so left edge = sx-8.
    // oam_x_left  = (sx - 8) + 8 = sx
    // oam_x_right = sx + 8
    // Enemy feet at e->y; sprite is 16px tall so top = e->y - 16.
    // oam_y = (e->y - 16) + 16 = e->y
    uint8_t oam_x = (uint8_t)sx;
    uint8_t oam_y = e->y;

    if (e->dir == -1) {
        // Moving left: swap columns and flip each tile horizontally
        set_sprite_tile(oam_base,   ENEMY_SPRITE_BASE_TILE + 2);
        set_sprite_tile(oam_base+1, ENEMY_SPRITE_BASE_TILE);
        set_sprite_prop(oam_base,   S_FLIPX);
        set_sprite_prop(oam_base+1, S_FLIPX);
    } else {
        set_sprite_tile(oam_base,   ENEMY_SPRITE_BASE_TILE);
        set_sprite_tile(oam_base+1, ENEMY_SPRITE_BASE_TILE + 2);
        set_sprite_prop(oam_base,   0);
        set_sprite_prop(oam_base+1, 0);
    }
    move_sprite(oam_base,   oam_x,   oam_y);
    move_sprite(oam_base+1, oam_x+8, oam_y);
}

uint8_t enemy_check_player_collision(Enemy* e, uint16_t player_x, uint8_t player_y)
{
    if (!e->active) return 0;

    // Enemy AABB (world coords)
    uint16_t e_left  = e->x - ENEMY_HALF_W;
    uint16_t e_right = e->x + ENEMY_HALF_W - 1;
    uint8_t  e_top   = e->y - ENEMY_HEIGHT;
    uint8_t  e_bot   = e->y;

    // Player AABB (world coords)
    uint16_t p_left  = (player_x > SPRITE_LEFT_W)    ? player_x - SPRITE_LEFT_W  : 0;
    uint16_t p_right = player_x + SPRITE_RIGHT_W;
    uint8_t  p_top   = (player_y > SPRITE_HEAD_OFFSET) ? player_y - SPRITE_HEAD_OFFSET : 0;
    uint8_t  p_bot   = player_y;

    return (p_right > e_left && p_left < e_right && p_bot > e_top && p_top < e_bot);
}

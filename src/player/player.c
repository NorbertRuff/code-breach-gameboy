#include <string.h>
#include "player.h"
#include "../graphics/programmer.h"
#include "../sound/sound.h"
#include "../engine/banking.h"
#include "../engine/camera.h"
#include "../engine/platform.h"

// Read by the tests to notice Player struct changes.
const uint8_t player_struct_size = sizeof(Player);

// Tile types that stop the feet (landing) and the body (walls and ceilings).
#define BLOCKS_FEET ((1 << COLLISION_SOLID) | (1 << COLLISION_ONE_WAY))
#define BLOCKS_BODY (1 << COLLISION_SOLID)

// A position in 1/32 px: a tile is 256 units, so the tile is the high byte, and bits 5-7
// are the pixel (0-7) within the tile.
#define TILE_OF(pos) ((uint8_t)((pos) >> 8))
#define PIXEL_IN_TILE(pos) ((uint8_t)(pos) >> 5)

// Tile columns and rows of the body's edges, from x (center) and y (lowest row).
#define LEFT_COLUMN(x)  TILE_OF((x) - SPRITE_LEFT_W * SUBPIXELS)
#define RIGHT_COLUMN(x) TILE_OF((x) + SPRITE_RIGHT_W * SUBPIXELS)
#define FEET_ROW(y)     TILE_OF(y)

#define RUN_LAST_FRAME 4   // run cycle: frames 1-4; frame 0 is standing

// Starts the player standing still, facing right, at a world position in pixels.
void player_init(Player* player, uint16_t start_x_px, uint16_t start_y_px)
{
    memset(player, 0, sizeof(Player));
    player->x = start_x_px * SUBPIXELS;
    player->y = start_y_px * SUBPIXELS;
    player->direction = 1;
}

// Pixel to tile. Left of the level gives column 0.
static uint8_t tile_of(int16_t px)
{
    return (px < 0) ? 0 : px >> 3;
}

// Tile row of the body's top. Above the map is row 0: rows 0-2 (the parallax band) are
// always open. A function, not a macro: SDCC miscompiles this ?: as a call argument.
static uint8_t head_row(uint16_t y)
{
    return (y < SPRITE_HEAD_OFFSET * SUBPIXELS) ? 0 : TILE_OF(y - SPRITE_HEAD_OFFSET * SUBPIXELS);
}

// Does any tile of one column, from row top to row bottom, have one of the collision
// types? The ring keeps a column's rows next to each other, so a pointer walks them.
// Columns past the level's end and rows below the ring are open.
static uint8_t column_has(uint8_t column, uint8_t top, uint8_t bottom, uint8_t types)
{
    if (column >= current_level.width) return 0;
    if (bottom >= RING_ROWS) bottom = RING_ROWS - 1;
    const uint8_t* cell = &collision_ring[RING_INDEX(column, top)];
    for (; top <= bottom; top++, cell++) {
        if (types & (1 << *cell)) return 1;
    }
    return 0;
}

// Does any tile from column first to last, row top to bottom, have one of the types?
static uint8_t area_has(uint8_t first, uint8_t top, uint8_t last, uint8_t bottom, uint8_t types)
{
    for (; first <= last; first++) {
        if (column_has(first, top, bottom, types)) return 1;
    }
    return 0;
}

// Moves a speed toward a target by at most step.
static int8_t approach(int8_t speed, int8_t target, uint8_t step)
{
    if (speed < target) return (target - speed > step) ? speed + step : target;
    if (speed > target) return (speed - target > step) ? speed - step : target;
    return speed;
}

// Speeds up toward the d-pad direction, or slows down when it is released.
// Control is weaker in the air.
static void update_run_speed(Player* p)
{
    if (p->input_x) {
        p->direction = p->input_x > 0;
        p->velocity_x = approach(p->velocity_x, p->input_x * RUN_SPEED_MAX, p->on_ground ? RUN_ACCEL : AIR_ACCEL);
    } else {
        p->velocity_x = approach(p->velocity_x, 0, p->on_ground ? RUN_DECEL : AIR_DECEL);
    }
}

// Starts a jump if A was pressed recently (jump buffer) and the player is on the ground
// or has just left it (coyote time). Otherwise counts both timers down.
static void try_jump(Player* p)
{
    if (p->jump_buffer_frames && (p->on_ground || p->coyote_frames)) {
        p->velocity_y = -JUMP_SPEED;
        p->jump_buffer_frames = 0;
        p->coyote_frames = 0;
        p->on_ground = 0;
        sound_play_jump();
        return;
    }
    if (p->jump_buffer_frames) p->jump_buffer_frames--;
    if (p->coyote_frames && !p->on_ground) p->coyote_frames--;
}

// Gravity for this frame: light while rising with A held, strong when falling or after
// A is released (jump cut), and halved around the top of a held jump (apex hang).
static uint8_t choose_gravity(const Player* p)
{
    uint8_t gravity = (p->jump_held && p->velocity_y < 0) ? GRAVITY_RISE : GRAVITY_FALL;
    if (p->jump_held && p->velocity_y > -APEX_SPEED && p->velocity_y < APEX_SPEED) {
        gravity >>= 1;
    }
    return gravity;
}

// Pulls the player down while in the air, up to the fall speed limit.
static void apply_gravity(Player* p)
{
    if (p->on_ground) return;
    int16_t speed = p->velocity_y + choose_gravity(p);
    p->velocity_y = (speed > FALL_SPEED_MAX) ? FALL_SPEED_MAX : speed;
}

// Moves sideways. Stops flush against walls, at the level's right edge, and at the
// camera's left limit (the level's left edge, or just past an opened gate).
static void move_x(Player* p)
{
    if (!p->velocity_x) return;
    uint8_t moving_right = p->velocity_x > 0;
    uint16_t min_x = (uint16_t)(camera_left_limit + SPRITE_LEFT_W) * SUBPIXELS;
    uint16_t max_x = (current_level.width_px - 1 - SPRITE_RIGHT_W) * SUBPIXELS;
    uint16_t x = p->x + p->velocity_x;
    if (x < min_x || x > max_x) {
        x = (x < min_x) ? min_x : max_x;
        p->velocity_x = 0;
    }

    // Is the leading edge's column a wall anywhere from head to feet? Then the edge
    // stops on the last pixel before the wall tile.
    uint8_t column = moving_right ? RIGHT_COLUMN(x) : LEFT_COLUMN(x);
    if (column_has(column, head_row(p->y), FEET_ROW(p->y), BLOCKS_BODY)) {
        x = moving_right ? ((uint16_t)column << 8) - SUBPIXELS - SPRITE_RIGHT_W * SUBPIXELS
                         : ((uint16_t)(column + 1) << 8) + SPRITE_LEFT_W * SUBPIXELS;
        p->velocity_x = 0;
    }
    p->x = x;
}

// Head against a ceiling corner? If only the tile at one end of the head row is solid,
// and the body reaches into it by at most CORNER_CORRECTION px, slides out of it, as long
// as the body (rows top to bottom) fits there. Returns 1 if it slid.
static uint8_t slide_past_corner(Player* p, uint8_t top, uint8_t bottom)
{
    int16_t left = TO_PIXELS(p->x) - SPRITE_LEFT_W;
    int16_t right = TO_PIXELS(p->x) + SPRITE_RIGHT_W;
    uint8_t first_col = left >> 3, last_col = right >> 3;
    uint8_t first_solid = LEVEL_COLLISION_AT(&current_level, first_col, top) == COLLISION_SOLID;
    uint8_t last_solid = LEVEL_COLLISION_AT(&current_level, last_col, top) == COLLISION_SOLID;

    int16_t shift;
    if (first_solid && !last_solid) shift = ((first_col + 1) << 3) - left;      // out to the right
    else if (last_solid && !first_solid) shift = (last_col << 3) - 1 - right;   // out to the left
    else return 0;
    if (shift > CORNER_CORRECTION || shift < -CORNER_CORRECTION) return 0;
    if (area_has(tile_of(left + shift), top, tile_of(right + shift), bottom, BLOCKS_BODY)) return 0;
    p->x += shift * SUBPIXELS;
    return 1;
}

// Moves up or down. Lands on floors and one-way tops, stops the head at ceilings.
static void move_y(Player* p)
{
    if (!p->velocity_y) return;
    uint16_t y = p->y + p->velocity_y;
    if (p->velocity_y < 0 && y > p->y) y = 0;   // never above the top of the map
    uint8_t left = LEFT_COLUMN(p->x), right = RIGHT_COLUMN(p->x);

    if (p->velocity_y > 0) {
        // Did the feet just enter a tile row with a floor or a one-way top? Then they stop
        // on the last pixel row above it.
        uint8_t feet = FEET_ROW(y);
        if (feet != FEET_ROW(p->y) && area_has(left, feet, right, feet, BLOCKS_FEET)) {
            y = ((uint16_t)feet << 8) - SUBPIXELS;
            p->velocity_y = 0;
        }
    } else {
        // Head in a ceiling? Then it stops on the first pixel row below it.
        uint8_t head = head_row(y);
        if (area_has(left, head, right, head, BLOCKS_BODY) && !slide_past_corner(p, head, FEET_ROW(y))) {
            y = ((uint16_t)(head + 1) << 8) + SPRITE_HEAD_OFFSET * SUBPIXELS;
            p->velocity_y = 0;
        }
    }
    p->y = y;
}

// Standing on a floor or a one-way top? On the ground, coyote time is full.
static void update_ground(Player* p)
{
    // Only feet on a tile's lowest pixel row can stand on the row below.
    uint8_t below = FEET_ROW(p->y) + 1;
    p->on_ground = p->velocity_y >= 0 && PIXEL_IN_TILE(p->y) == 7 &&
                   area_has(LEFT_COLUMN(p->x), below, RIGHT_COLUMN(p->x), below, BLOCKS_FEET);
    if (p->on_ground) {
        p->velocity_y = 0;
        p->coyote_frames = COYOTE_FRAMES;
    }
}

// Collects the fragments the body overlaps, and notes whether it touches a hazard or
// the exit.
static void check_touched_tiles(Player* p)
{
    uint8_t top = head_row(p->y), bottom = FEET_ROW(p->y), last = RIGHT_COLUMN(p->x);
    if (bottom >= RING_ROWS) bottom = RING_ROWS - 1;

    p->touched_hazard = 0;
    p->touched_exit = 0;
    for (uint8_t column = LEFT_COLUMN(p->x); column <= last && column < current_level.width; column++) {
        if (!ring_column_touched[column & (RING_COLUMNS - 1)]) continue;   // most columns
        const uint8_t* cell = &collision_ring[RING_INDEX(column, top)];
        for (uint8_t row = top; row <= bottom; row++, cell++) {
            uint8_t type = *cell;
            if (type < FIRST_TOUCHED_TYPE) continue;
            if (type == COLLISION_COLLECTIBLE) level_collect_fragment(column, row);
            else if (type == COLLISION_HAZARD) p->touched_hazard = 1;
            else p->touched_exit = 1;
        }
    }
}

// Picks the sprite frame: jump pose in the air, run cycle while moving, standing otherwise.
static void update_animation(Player* p)
{
    if (!p->on_ground) {
        p->draw_frame = JUMP_POSE_FRAME;
    } else if (!p->velocity_x) {
        p->draw_frame = 0;
        p->animation_frame_count = 0;
    } else if (++p->animation_frame_count == ANIMATION_FRAME_LENGTH) {
        p->animation_frame_count = 0;
        p->draw_frame = (p->draw_frame >= RUN_LAST_FRAME) ? 1 : p->draw_frame + 1;
    }
}

// One frame of player movement: run, jump, gravity, collision, pickups, animation.
// Reads the input fields that input_read_player() set this frame.
void player_update(Player* player)
{
    update_run_speed(player);
    try_jump(player);
    apply_gravity(player);
    move_x(player);
    move_y(player);
    update_ground(player);
    check_touched_tiles(player);
    update_animation(player);
}

// Puts the player first in the sprite list, so the sprite covers its collision box.
// While invincible it blinks on DMG and is pale on GBC. Returns the number of hardware
// sprites used.
uint8_t player_draw(Player* player, int16_t camera_x)
{
    uint8_t palette = 0;
    if (player->invincibility_frames) {
        if (is_cgb) palette = PLAYER_PALE_PALETTE;
        else if (player->invincibility_frames & 2) return 0;
    }

    // The metasprite pivot lies programmer_PIVOT_X/Y in from the box's top-left corner,
    // and hardware sprite coordinates start 8 px left of and 16 px above the screen.
    uint8_t left = TO_PIXELS(player->x) - SPRITE_LEFT_W - camera_x;
    uint8_t top = TO_PIXELS(player->y) - SPRITE_HEAD_OFFSET;
    uint8_t x = left + programmer_PIVOT_X + DEVICE_SPRITE_PX_OFFSET_X;
    uint8_t y = top + programmer_PIVOT_Y + DEVICE_SPRITE_PX_OFFSET_Y;
    uint8_t used;
    WITH_BANK(programmer, {
        if (player->direction) {
            used = move_metasprite_ex(programmer_metasprites[player->draw_frame], 0, palette, 0, x, y);
        } else {
            used = move_metasprite_flipx(programmer_metasprites[player->draw_frame], 0, palette, 0, x, y);
        }
    });
    return used;
}

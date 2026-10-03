#include <string.h>
#include "player.h"
#include "../graphics/programmer.h"
#include "../sound/sound.h"
#include "../engine/banking.h"

// Read by the tests to notice Player struct changes.
const uint8_t player_struct_size = sizeof(Player);

// Tile types that stop the feet (landing) and the body (walls and ceilings).
#define BLOCKS_FEET ((1 << COLLISION_SOLID) | (1 << COLLISION_ONE_WAY))
#define BLOCKS_BODY (1 << COLLISION_SOLID)

// How far the body center may go: the box stays inside the level.
#define PLAYER_MIN_X ((uint16_t)SPRITE_LEFT_W * SUBPIXELS)
#define PLAYER_MAX_X ((uint16_t)(LEVEL_WIDTH * TILE_SIZE - 1 - SPRITE_RIGHT_W) * SUBPIXELS)

#define RUN_LAST_FRAME 4   // run cycle: frames 1-4; frame 0 is standing

// Starts the player standing still, facing right, at a world position in pixels.
void player_init(Player* player, uint16_t start_x_px, uint16_t start_y_px)
{
    memset(player, 0, sizeof(Player));
    player->x = start_x_px * SUBPIXELS;
    player->y = start_y_px * SUBPIXELS;
    player->direction = 1;
}

// Does any tile in the pixel row y, between the pixel columns left and right, have one
// of the collision types? Above the level counts as open.
static uint8_t row_has(Level* level, int16_t y, int16_t left, int16_t right, uint8_t types)
{
    if (y < 0) return 0;
    if (left < 0) left = 0;
    uint8_t tile_y = y >> 3;
    uint8_t last_x = right >> 3;
    for (uint8_t tile_x = left >> 3; tile_x <= last_x; tile_x++) {
        if (types & (1 << level_collision_at(level, tile_x, tile_y))) return 1;
    }
    return 0;
}

// Does any tile in the pixel rectangle have one of the collision types?
static uint8_t area_has(Level* level, int16_t left, int16_t top, int16_t right, int16_t bottom, uint8_t types)
{
    if (top < 0) top = 0;
    for (int16_t y = top & ~7; y <= bottom; y += TILE_SIZE) {
        if (row_has(level, y, left, right, types)) return 1;
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

// Moves sideways. Stops flush against walls and at the level's edges.
static void move_x(Player* p, Level* level)
{
    if (!p->velocity_x) return;
    uint8_t moving_right = p->velocity_x > 0;
    uint16_t x = p->x + p->velocity_x;
    if (x < PLAYER_MIN_X || x > PLAYER_MAX_X) {
        x = (x < PLAYER_MIN_X) ? PLAYER_MIN_X : PLAYER_MAX_X;
        p->velocity_x = 0;
    }

    // Is the leading edge inside a wall, anywhere from head to feet?
    int16_t feet = TO_PIXELS(p->y);
    int16_t head = feet - SPRITE_HEAD_OFFSET;
    int16_t edge = moving_right ? TO_PIXELS(x) + SPRITE_RIGHT_W : TO_PIXELS(x) - SPRITE_LEFT_W;
    if (area_has(level, edge, head, edge, feet, BLOCKS_BODY)) {
        int16_t center = moving_right ? (edge & ~7) - 1 - SPRITE_RIGHT_W
                                      : (edge | 7) + 1 + SPRITE_LEFT_W;
        x = (uint16_t)center * SUBPIXELS;
        p->velocity_x = 0;
    }
    p->x = x;
}

// Head against a ceiling corner? Slides up to CORNER_CORRECTION px sideways to get past it.
// Returns 1 if it found a free spot.
static uint8_t slide_past_corner(Player* p, Level* level, int16_t head)
{
    int16_t center = TO_PIXELS(p->x);
    int16_t feet = head + SPRITE_HEAD_OFFSET;
    for (int8_t shift = 1; shift <= CORNER_CORRECTION; shift++) {
        for (int8_t side = -shift; side <= shift; side += 2 * shift) {
            int16_t x = center + side;
            if (!area_has(level, x - SPRITE_LEFT_W, head, x + SPRITE_RIGHT_W, feet, BLOCKS_BODY)) {
                p->x += side * SUBPIXELS;
                return 1;
            }
        }
    }
    return 0;
}

// Moves up or down. Lands on floors and one-way tops, stops the head at ceilings.
static void move_y(Player* p, Level* level)
{
    if (!p->velocity_y) return;
    uint16_t y = p->y + p->velocity_y;
    if (p->velocity_y < 0 && y > p->y) y = 0;   // never above the top of the map
    int16_t feet = TO_PIXELS(y);
    int16_t left = TO_PIXELS(p->x) - SPRITE_LEFT_W;
    int16_t right = TO_PIXELS(p->x) + SPRITE_RIGHT_W;

    if (p->velocity_y > 0) {
        // Did the feet just enter a tile row with a floor or a one-way top?
        if ((feet >> 3) != (TO_PIXELS(p->y) >> 3) && row_has(level, feet, left, right, BLOCKS_FEET)) {
            y = (uint16_t)((feet & ~7) - 1) * SUBPIXELS;
            p->velocity_y = 0;
        }
    } else {
        int16_t head = feet - SPRITE_HEAD_OFFSET;
        if (row_has(level, head, left, right, BLOCKS_BODY) && !slide_past_corner(p, level, head)) {
            y = (uint16_t)((head | 7) + 1 + SPRITE_HEAD_OFFSET) * SUBPIXELS;
            p->velocity_y = 0;
        }
    }
    p->y = y;
}

// Standing on a floor or a one-way top? On the ground, coyote time is full.
static void update_ground(Player* p, Level* level)
{
    int16_t below = TO_PIXELS(p->y) + 1;
    int16_t center = TO_PIXELS(p->x);
    p->on_ground = p->velocity_y >= 0 && (below & 7) == 0 &&
                   row_has(level, below, center - SPRITE_LEFT_W, center + SPRITE_RIGHT_W, BLOCKS_FEET);
    if (p->on_ground) {
        p->velocity_y = 0;
        p->coyote_frames = COYOTE_FRAMES;
    }
}

// Collects the fragments the body overlaps, and notes whether it touches a hazard.
static void check_touched_tiles(Player* p, Level* level)
{
    int16_t center = TO_PIXELS(p->x);
    int16_t feet = TO_PIXELS(p->y);
    int16_t head = feet - SPRITE_HEAD_OFFSET;
    uint8_t first_x = (center - SPRITE_LEFT_W) >> 3, last_x = (center + SPRITE_RIGHT_W) >> 3;
    uint8_t first_y = (head < 0) ? 0 : head >> 3, last_y = feet >> 3;

    p->touched_hazard = 0;
    for (uint8_t tile_y = first_y; tile_y <= last_y; tile_y++) {
        for (uint8_t tile_x = first_x; tile_x <= last_x; tile_x++) {
            uint8_t type = level_collision_at(level, tile_x, tile_y);
            if (type == COLLISION_COLLECTIBLE) level_handle_collision(level, type, tile_x, tile_y);
            if (type == COLLISION_HAZARD) p->touched_hazard = 1;
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
void player_update(Player* player, Level* level)
{
    update_run_speed(player);
    try_jump(player);
    apply_gravity(player);
    move_x(player, level);
    move_y(player, level);
    update_ground(player, level);
    check_touched_tiles(player, level);
    update_animation(player);
}

// Draws the player so the sprite covers its collision box. Flashes while invincible.
void player_draw(Player* player, int16_t camera_x)
{
    if (player->invincibility_frames & 2) {
        hide_sprites_range(0, 4);
        return;
    }

    // The metasprite pivot lies programmer_PIVOT_X/Y in from the box's top-left corner,
    // and hardware sprite coordinates start 8 px left of and 16 px above the screen.
    uint8_t left = TO_PIXELS(player->x) - SPRITE_LEFT_W - camera_x;
    uint8_t top = TO_PIXELS(player->y) - SPRITE_HEAD_OFFSET;
    uint8_t x = left + programmer_PIVOT_X + DEVICE_SPRITE_PX_OFFSET_X;
    uint8_t y = top + programmer_PIVOT_Y + DEVICE_SPRITE_PX_OFFSET_Y;
    WITH_BANK(programmer, {
        if (player->direction) {
            move_metasprite_ex(programmer_metasprites[player->draw_frame], 0, 0, 0, x, y);
        } else {
            move_metasprite_flipx(programmer_metasprites[player->draw_frame], 0, 0, 0, x, y);
        }
    });
}

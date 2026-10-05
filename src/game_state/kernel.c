// KERNEL, the boss of level 5 (docs/LEVEL_DESIGN.md section 7, "Level 5 design"). Not
// per-frame code but kernel_update: in a ROM bank, to keep bank 0 free.
#pragma bank 255

#include "kernel.h"
#include "game_flow.h"
#include "story.h"
#include "../actor/actor.h"
#include "../actor/lift.h"
#include "../actor/packet.h"
#include "../engine/palette.h"
#include "../engine/screen.h"
#include "../engine/text.h"
#include "../graphics/hud.h"
#include "../level/level_manager.h"
#include "../sound/sound.h"
#include "../utils.h"

#define UPLOADS 3                   // one per phase; the HUD counts them as the arena's fragments
#define UPLOAD_FRAMES 32            // Up held on the lit terminal: 4 steps of 8
#define UPLOAD_STEP_FRAMES 8
#define UPLOAD_STEP_SHIFT 3         // frames >> 3: the step
#define TERMINAL_LIGHT_FRAMES 150   // a phase's terminal lights once its first volley has fallen
#define BLINK_FRAMES 16             // the lit terminal: shown 16 frames, off 16
#define FIRST_VOLLEY_FRAMES 60      // 1 s from a phase's start to its first volley
#define VOLLEY_FRAMES 150           // 2.5 s from one volley to the next
#define PACKET_WARNING_FRAMES 30    // a packet blinks this long before it falls
#define PACKET_STAGGER_FRAMES 8     // between a volley's packets: they fall 16 px apart
#define EYES_WIDE_FRAMES 30
#define HIT_STOP_FRAMES 8
#define SHAKE_FRAMES 16
#define SHAKE_PX 2
#define INTRO_READ_FRAMES 120       // log 5 stays 2 s

// Packets fall in 10 lanes of 16 px. Each volley drops one in the player's lane and the
// lanes in this table, a table per phase, repeating. Phase 2's stay left of the lift
// (lanes 6-9), so only the player's own packet falls on the lift and the shelf.
#define LANE_SHIFT 4
#define LAST_LANE 9
#define VOLLEYS 4
#define NO_LANE 0xFF
static const uint8_t volley_lanes[UPLOADS][VOLLEYS][2] = {
    { {3, NO_LANE}, {8, NO_LANE}, {1, NO_LANE}, {6, NO_LANE} },
    { {0, 3}, {1, 4}, {2, 5}, {0, 4} },
    { {6, NO_LANE}, {2, NO_LANE}, {8, NO_LANE}, {1, NO_LANE} },
};

// The arena (levels/level_5.tmj) in screen tiles; its tiles are the greybox tileset's.
#define ARENA_WIDTH 20
#define ARENA_EMPTY_TILE 0
#define ARENA_FLOOR_TILE 1
#define ARENA_HAZARD_TILE 3
#define FLOOR_ROW 15
#define TEXT_ROW 9                  // log 5, under the face

// The terminals, in the order they light: the tile above the floor or the shelf.
static const uint8_t terminal_x[UPLOADS] = { 1, 18, 9 };
static const uint8_t terminal_y[UPLOADS] = { 14, 7, 14 };

// Phase 2's lift: in the columns left of the shelf, from 2 tiles above the floor up to
// the shelf's top (ENTITY_LIFT's fields: x, y, travel, speed).
static const LevelEntity lift_entity = { ENTITY_LIFT, 104, 96, 32, 1 };

// Phase 3's corrupted floor: 5 segments of 4 tiles, in this order, round and round.
// Each warns (flickers, still solid), then is a hazard, then floor again for a while.
#define SEGMENT_TILES 4
#define FLOOR_TURNS 5
#define FLOOR_SAFE_FRAMES 30
#define FLOOR_WARNING_FRAMES 30
#define FLOOR_HAZARD_FRAMES 90
static const uint8_t floor_order[FLOOR_TURNS] = { 2, 0, 4, 1, 3 };
enum { FLOOR_SAFE, FLOOR_WARNING, FLOOR_HAZARD };

// KERNEL's background tiles, after the HUD frame's. Placeholder art until M5.
#define KERNEL_TILE_BASE (HUD_TILE_BASE + hud_TILE_COUNT)
enum {
    FRAME_TL, FRAME_T, FRAME_TR, FRAME_L, SCREEN, FRAME_R, FRAME_BL, FRAME_B, FRAME_BR,
    EYE, EYE_WIDE, MOUTH, GLITCH, TERMINAL_OFF, TERMINAL_LIT, TERMINAL_FILL, KERNEL_TILE_COUNT = TERMINAL_FILL + 3
};
static const uint8_t kernel_tiles[KERNEL_TILE_COUNT * 16] = {
    0xFF,0xFF, 0xFF,0xFF, 0xC0,0xFF, 0xC0,0xFF, 0xC0,0xFF, 0xC0,0xFF, 0xC0,0xFF, 0xC0,0xFF,   // FRAME_TL
    0xFF,0xFF, 0xFF,0xFF, 0x00,0xFF, 0x00,0xFF, 0x00,0xFF, 0x00,0xFF, 0x00,0xFF, 0x00,0xFF,   // FRAME_T
    0xFF,0xFF, 0xFF,0xFF, 0x03,0xFF, 0x03,0xFF, 0x03,0xFF, 0x03,0xFF, 0x03,0xFF, 0x03,0xFF,   // FRAME_TR
    0xC0,0xFF, 0xC0,0xFF, 0xC0,0xFF, 0xC0,0xFF, 0xC0,0xFF, 0xC0,0xFF, 0xC0,0xFF, 0xC0,0xFF,   // FRAME_L
    0x00,0xFF, 0x00,0xFF, 0x00,0xFF, 0x00,0xFF, 0x00,0xFF, 0x00,0xFF, 0x00,0xFF, 0x00,0xFF,   // SCREEN
    0x03,0xFF, 0x03,0xFF, 0x03,0xFF, 0x03,0xFF, 0x03,0xFF, 0x03,0xFF, 0x03,0xFF, 0x03,0xFF,   // FRAME_R
    0xC0,0xFF, 0xC0,0xFF, 0xC0,0xFF, 0xC0,0xFF, 0xC0,0xFF, 0xC0,0xFF, 0xFF,0xFF, 0xFF,0xFF,   // FRAME_BL
    0x00,0xFF, 0x00,0xFF, 0x00,0xFF, 0x00,0xFF, 0x00,0xFF, 0x00,0xFF, 0xFF,0xFF, 0xFF,0xFF,   // FRAME_B
    0x03,0xFF, 0x03,0xFF, 0x03,0xFF, 0x03,0xFF, 0x03,0xFF, 0x03,0xFF, 0xFF,0xFF, 0xFF,0xFF,   // FRAME_BR
    0x00,0xFF, 0x00,0xFF, 0x00,0xC3, 0x00,0xC3, 0x00,0xC3, 0x00,0xC3, 0x00,0xFF, 0x00,0xFF,   // EYE
    0x00,0xFF, 0x00,0x81, 0x00,0x81, 0x00,0x81, 0x00,0x81, 0x00,0x81, 0x00,0x81, 0x00,0xFF,   // EYE_WIDE
    0x00,0xFF, 0x00,0xFF, 0x00,0xFF, 0x00,0x00, 0x00,0x00, 0x00,0xFF, 0x00,0xFF, 0x00,0xFF,   // MOUTH
    0xAA,0xFF, 0x55,0xFF, 0xAA,0xFF, 0x55,0xFF, 0xAA,0xFF, 0x55,0xFF, 0xAA,0xFF, 0x55,0xFF,   // GLITCH
    0x7E,0x7E, 0x42,0x7E, 0x42,0x7E, 0x42,0x7E, 0x42,0x7E, 0x7E,0x7E, 0x18,0x18, 0x3C,0x3C,   // TERMINAL_OFF
    0x7E,0x7E, 0x42,0x42, 0x42,0x42, 0x42,0x42, 0x42,0x42, 0x7E,0x7E, 0x18,0x18, 0x3C,0x3C,   // TERMINAL_LIT
    0x7E,0x7E, 0x42,0x42, 0x42,0x42, 0x42,0x42, 0x7E,0x7E, 0x7E,0x7E, 0x18,0x18, 0x3C,0x3C,   // upload 1/4
    0x7E,0x7E, 0x42,0x42, 0x42,0x42, 0x7E,0x7E, 0x7E,0x7E, 0x7E,0x7E, 0x18,0x18, 0x3C,0x3C,   // upload 2/4
    0x7E,0x7E, 0x42,0x42, 0x7E,0x7E, 0x7E,0x7E, 0x7E,0x7E, 0x7E,0x7E, 0x18,0x18, 0x3C,0x3C,   // upload 3/4
};

// The packet's sprite tiles: a 16x16 box with two dark stripes, in the bot's tile order.
static const uint8_t packet_tiles[PACKET_TILE_COUNT * 16] = {
    0xFF,0xFF, 0xFF,0x80, 0xFF,0x80, 0xFF,0x80, 0xFF,0x80, 0xFF,0xBF, 0xFF,0x80, 0xFF,0x80,
    0xFF,0x80, 0xFF,0x80, 0xFF,0xBF, 0xFF,0x80, 0xFF,0x80, 0xFF,0x80, 0xFF,0x80, 0xFF,0xFF,
    0xFF,0xFF, 0xFF,0x01, 0xFF,0x01, 0xFF,0x01, 0xFF,0x01, 0xFF,0xFD, 0xFF,0x01, 0xFF,0x01,
    0xFF,0x01, 0xFF,0x01, 0xFF,0xFD, 0xFF,0x01, 0xFF,0x01, 0xFF,0x01, 0xFF,0x01, 0xFF,0xFF,
};

// The face: a monitor of 7x6 tiles at the top middle, in the background.
#define FACE_X 6
#define FACE_Y 3
#define FACE_WIDTH 7
#define FACE_HEIGHT 6
#define LEFT_EYE_X (FACE_X + 2)
#define RIGHT_EYE_X (FACE_X + 4)
#define EYES_Y (FACE_Y + 1)
static const uint8_t face_map[FACE_WIDTH * FACE_HEIGHT] = {
    FRAME_TL, FRAME_T, FRAME_T, FRAME_T, FRAME_T, FRAME_T, FRAME_TR,
    FRAME_L,  SCREEN,  EYE,     SCREEN,  EYE,     SCREEN,  FRAME_R,
    FRAME_L,  SCREEN,  SCREEN,  SCREEN,  SCREEN,  SCREEN,  FRAME_R,
    FRAME_L,  SCREEN,  MOUTH,   MOUTH,   MOUTH,   SCREEN,  FRAME_R,
    FRAME_L,  SCREEN,  SCREEN,  SCREEN,  SCREEN,  SCREEN,  FRAME_R,
    FRAME_BL, FRAME_B, FRAME_B, FRAME_B, FRAME_B, FRAME_B, FRAME_BR,
};

// Screen cells that glitch as KERNEL loses: 3 more after each upload.
#define GLITCHES_PER_UPLOAD 3
static const uint8_t glitch_x[GLITCHES_PER_UPLOAD * 2] = { 1, 5, 3, 2, 5, 1 };
static const uint8_t glitch_y[GLITCHES_PER_UPLOAD * 2] = { 2, 4, 1, 4, 1, 3 };

static uint8_t phase_frames;     // since the phase started, up to TERMINAL_LIGHT_FRAMES
static uint8_t upload_frames;    // Up held on the lit terminal
static uint8_t blink_frames;
static uint8_t volley_frames;    // to the next volley
static uint8_t volley_turn;      // the next volley in the phase's table
static uint8_t eyes_frames;      // left with the eyes wide
static uint8_t floor_turn, floor_state, floor_frames;

// Uploads done: the phase, 0 to 2. Kept as the arena's collected fragments, so the HUD
// shows them, a lost life keeps them and a game over clears them (game_flow.c).
#define uploads current_level.section_collected[0]

// Writes one of KERNEL's tiles at a screen tile.
static void draw_tile(uint8_t x, uint8_t y, uint8_t tile)
{
    set_bkg_tile_xy(x, y, KERNEL_TILE_BASE + tile);
}

// Draws the face, with a glitch for every upload done.
static void draw_face(void)
{
    set_bkg_based_tiles(FACE_X, FACE_Y, FACE_WIDTH, FACE_HEIGHT, face_map, KERNEL_TILE_BASE);
    for (uint8_t i = 0; i < uploads * GLITCHES_PER_UPLOAD; i++) {
        draw_tile(FACE_X + glitch_x[i], FACE_Y + glitch_y[i], GLITCH);
    }
}

// Draws both eyes with one tile: EYE or EYE_WIDE.
static void draw_eyes(uint8_t tile)
{
    draw_tile(LEFT_EYE_X, EYES_Y, tile);
    draw_tile(RIGHT_EYE_X, EYES_Y, tile);
}

// Draws the current phase's terminal with a tile.
static void draw_terminal(uint8_t tile)
{
    draw_tile(terminal_x[uploads], terminal_y[uploads], tile);
}

// Adds phase 2's lift, waiting at the bottom of its travel.
static void add_lift(void)
{
    Actor* lift = &actors[actor_count++];
    lift->type = &lift_type;
    lift_type.spawn(lift, &lift_entity);
    lifts_in_play++;
}

// Adds a packet in a lane: it waits PACKET_WARNING_FRAMES, plus PACKET_STAGGER_FRAMES for
// each packet before it in the volley.
static void drop_packet(uint8_t lane, uint8_t order)
{
    if (actor_count == MAX_ACTORS) return;
    Actor* packet = &actors[actor_count++];
    packet->type = &packet_type;
    packet->x = packet->min_x = (lane << LANE_SHIFT) + (1 << LANE_SHIFT) / 2;
    packet->y = PACKET_TOP_Y;
    packet->velocity_x = 1;
    packet->wait_frames = PACKET_WARNING_FRAMES + order * PACKET_STAGGER_FRAMES;
}

// The lane of the player's body center.
static uint8_t player_lane(void)
{
    uint8_t lane = TO_PIXELS(player.x) >> LANE_SHIFT;
    return lane > LAST_LANE ? LAST_LANE : lane;
}

// A volley: KERNEL's eyes go wide, and packets drop in the player's lane and the table's.
static void start_volley(void)
{
    const uint8_t* lanes = volley_lanes[uploads][volley_turn];
    drop_packet(player_lane(), 0);
    for (uint8_t i = 0; i < 2 && lanes[i] != NO_LANE; i++) drop_packet(lanes[i], i + 1);
    volley_turn = (volley_turn + 1) & (VOLLEYS - 1);
    draw_eyes(EYE_WIDE);
    eyes_frames = EYES_WIDE_FRAMES;
}

// Removes every packet; the lift stays.
static void remove_packets(void)
{
    uint8_t kept = 0;
    for (uint8_t i = 0; i < actor_count; i++) {
        if (actors[i].type != &packet_type) actors[kept++] = actors[i];
    }
    actor_count = kept;
}

// Sets a floor segment's 4 tiles to a tile and, unless only_look, a collision type.
static void set_segment(uint8_t segment, uint8_t tile, uint8_t collision, uint8_t only_look)
{
    uint8_t first = segment * SEGMENT_TILES;
    fill_bkg_rect(first, FLOOR_ROW, SEGMENT_TILES, 1, tile);
    if (only_look) return;
    for (uint8_t column = first; column < first + SEGMENT_TILES; column++) {
        collision_ring[RING_INDEX(column, FLOOR_ROW)] = collision;
        ring_column_touched[column] = 1;   // the player looks for hazards there
    }
}

// Phase 3, once per frame: the segment whose turn it is warns, is a hazard, then rests.
static void update_floor(void)
{
    uint8_t segment = floor_order[floor_turn];
    if (--floor_frames) {
        if (floor_state == FLOOR_WARNING && !(floor_frames & 3)) {
            uint8_t tile = (floor_frames & 4) ? ARENA_HAZARD_TILE : ARENA_FLOOR_TILE;
            set_segment(segment, tile, 0, 1);
        }
        return;
    }
    if (floor_state == FLOOR_SAFE) {
        floor_state = FLOOR_WARNING;
        floor_frames = FLOOR_WARNING_FRAMES;
    } else if (floor_state == FLOOR_WARNING) {
        set_segment(segment, ARENA_HAZARD_TILE, COLLISION_HAZARD, 0);
        floor_state = FLOOR_HAZARD;
        floor_frames = FLOOR_HAZARD_FRAMES;
    } else {
        set_segment(segment, ARENA_FLOOR_TILE, COLLISION_SOLID, 0);
        floor_state = FLOOR_SAFE;
        floor_frames = FLOOR_SAFE_FRAMES;
        floor_turn = floor_turn == FLOOR_TURNS - 1 ? 0 : floor_turn + 1;
    }
}

// Is the player standing on the current phase's terminal?
static uint8_t on_terminal(void)
{
    uint16_t x = TO_PIXELS(player.x);
    uint8_t column = terminal_x[uploads];
    return player.on_ground && (TO_PIXELS(player.y) >> 3) == terminal_y[uploads]
        && ((x - SPRITE_LEFT_W) >> 3) <= column && ((x + SPRITE_RIGHT_W) >> 3) >= column;
}

// The current terminal, once per frame: off, then lit and blinking once the phase's first
// volley has fallen. Up held on it fills it in 4 steps; letting go starts it over.
// Returns 1 when the upload is done.
static uint8_t update_terminal(void)
{
    if (phase_frames < TERMINAL_LIGHT_FRAMES) {
        if (++phase_frames == TERMINAL_LIGHT_FRAMES) draw_terminal(TERMINAL_LIT);
        return 0;
    }
    if (on_terminal() && (joypad_current & J_UP)) {
        if (++upload_frames == UPLOAD_FRAMES) return 1;
        if (!(upload_frames & (UPLOAD_STEP_FRAMES - 1))) {
            draw_terminal(TERMINAL_FILL + (upload_frames >> UPLOAD_STEP_SHIFT) - 1);
            sound_play(EFFECT_COLLECT);
        }
        return 0;
    }
    upload_frames = 0;
    if (!(++blink_frames & (BLINK_FRAMES - 1))) {
        draw_terminal((blink_frames & BLINK_FRAMES) ? TERMINAL_OFF : TERMINAL_LIT);
    }
    return 0;
}

// Starts the current phase: its terminal off, the first volley in a second, the floor
// whole, and from phase 2 on the lift (unless it is already there).
static void start_phase(void)
{
    draw_face();
    draw_terminal(TERMINAL_OFF);
    phase_frames = upload_frames = blink_frames = volley_turn = eyes_frames = floor_turn = 0;
    volley_frames = FIRST_VOLLEY_FRAMES;
    floor_state = FLOOR_SAFE;
    floor_frames = FLOOR_SAFE_FRAMES;
    if (uploads && !lifts_in_play) add_lift();
}

// Shakes the whole screen sideways for SHAKE_FRAMES.
static void shake_screen(void)
{
    for (uint8_t frame = 0; frame < SHAKE_FRAMES; frame++) {
        int16_t offset = (frame & 2) ? SHAKE_PX : -SHAKE_PX;
        screen_scroll(offset);
        wait_vbl_done();
    }
    screen_scroll(0);
}

// After level_load: KERNEL's tiles and the packet's, and the uploads as the arena's
// fragment quota, so the HUD shows "00/03".
void kernel_start(void) BANKED
{
    set_bkg_data(KERNEL_TILE_BASE, KERNEL_TILE_COUNT, kernel_tiles);
    set_sprite_data(PACKET_FIRST_TILE, PACKET_TILE_COUNT, packet_tiles);
    current_level.section_fragments[0] = UPLOADS;
}

// After the arena is drawn (a start, or a restart after a lost life): the map columns a
// shake shows past its right edge, the terminals, and the current phase from its start.
void kernel_restart(void) BANKED
{
    fill_bkg_rect(ARENA_WIDTH, LEVEL_MAP_TOP_ROW, RING_COLUMNS - ARENA_WIDTH, FLOOR_ROW - LEVEL_MAP_TOP_ROW, ARENA_EMPTY_TILE);
    fill_bkg_rect(ARENA_WIDTH, FLOOR_ROW, RING_COLUMNS - ARENA_WIDTH, 1, ARENA_FLOOR_TILE);
    for (uint8_t i = 0; i < UPLOADS; i++) {
        set_bkg_tile_xy(terminal_x[i], terminal_y[i], KERNEL_TILE_BASE + TERMINAL_OFF);
    }
    start_phase();
}

// Once the arena has faded in: KERNEL's log 5 under its face, then the fight.
void kernel_intro(void) BANKED
{
    text_load_font();
    story_type_log(LEVEL_BOSS, TEXT_ROW);
    wait_frames(INTRO_READ_FRAMES);
    fill_bkg_rect(0, TEXT_ROW, ARENA_WIDTH, 2, ARENA_EMPTY_TILE);
}

// Once per gameplay frame, early (VBlank time for its tile writes): eyes, volleys, the
// floor in phase 3, the terminal. Returns 1 when an upload is done.
uint8_t kernel_update(void) BANKED
{
    if (eyes_frames && !--eyes_frames) draw_eyes(EYE);
    if (!--volley_frames) {
        start_volley();
        volley_frames = VOLLEY_FRAMES;
    }
    if (uploads == UPLOADS - 1) update_floor();
    return update_terminal();
}

// An upload done: hit-stop, a shake and slow flashes; then the next phase, or after the
// last upload the face breaks and the ending follows. Returns the next game state.
uint8_t kernel_hit(void) BANKED
{
    sound_play(EFFECT_GATE);
    draw_terminal(TERMINAL_OFF);
    uploads++;
    wait_frames(HIT_STOP_FRAMES);
    shake_screen();
    remove_packets();
    draw_sprites(0);
    if (uploads == UPLOADS) {
        fill_bkg_rect(FACE_X + 1, FACE_Y + 1, FACE_WIDTH - 2, FACE_HEIGHT - 2, KERNEL_TILE_BASE + GLITCH);
        palette_flash(3, 10);
        palette_fade_out(SCREEN_FADE_FRAMES);
        return ENDING;
    }
    palette_flash(2, 10);
    start_phase();
    return COREGAMELOOP;
}

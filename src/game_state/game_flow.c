// Not per-frame code: in a ROM bank, to keep bank 0 free.
#pragma bank 255

#include <gb/gb.h>
#include "game_flow.h"
#include "game_over.h"
#include "kernel.h"
#include "pause.h"
#include "story.h"
#include "../engine/camera.h"
#include "../engine/palette.h"
#include "../engine/screen.h"
#include "../level/level_manager.h"
#include "../level/section.h"
#include "../actor/actor.h"
#include "../sound/music.h"
#include "../utils.h"

#define RESPAWN_INVINCIBILITY_FRAMES 90   // 1.5 s at 60 fps

// Starts the current section again: the player at its start, the gates and actors as
// they were when it began, the camera on the player with everything it shows drawn, and
// the HUD count. For a level start and after losing a life.
static void restart_section(void)
{
    uint16_t x;
    uint8_t y;
    section_start_position(&x, &y);
    player_init(&player, x, y);
    sections_restart();
    actors_reset(current_level.section);
    camera_reset(TO_PIXELS(player.x), current_level.width_px);
    level_stream_reset(camera_x);
    screen_scroll(camera_x);
    actors_spawn(camera_x);
    if (level_index == LEVEL_BOSS) kernel_restart();
    redraw_hud_count();
}

// Restarts the section while the screen is dark, then fades back in. After a lost life
// (not a game over) the player is invincible for a moment.
static void restart_behind_fade(uint8_t invincible)
{
    palette_fade_out(RESTART_FADE_FRAMES);
    screen_show_sprites(0);
    restart_section();
    if (invincible) player.invincibility_frames = RESPAWN_INVINCIBILITY_FRAMES;
    hud_update_lives(lives);
    draw_sprites(0);
    palette_fade_in(RESTART_FADE_FRAMES);
    screen_show_sprites(1);
}

// Builds the level behind a black screen, then fades in.
void start_level(void) BANKED
{
    palette_fade_to(PALETTE_BLACK);
    screen_show_sprites(0);
    level_load(level_index);
    if (level_index == LEVEL_BOSS) kernel_start();
    music_play_level(level_index);
    actors_load_tiles();
    pause_load_tiles();
    hud_update_lives(lives);
    restart_section();
    draw_sprites(0);
    palette_fade_in(SCREEN_FADE_FRAMES);
    screen_show_sprites(1);
    if (level_index == LEVEL_BOSS) kernel_intro();
}

// Takes a life and restarts the section. On game over the player continues (the lives
// refill and the section's fragments come back) or quits to the title. Returns the next
// game state.
uint8_t lose_life(void) BANKED
{
    lives--;
    hud_update_lives(lives);
    if (lives) {
        restart_behind_fade(1);
        return COREGAMELOOP;
    }
    if (!game_over()) {
        palette_fade_out(SCREEN_FADE_FRAMES);
        return TITLE_SCREEN_STATE;
    }
    lives = START_LIVES;
    level_reset_fragments(current_level.section);
    music_play_level(level_index);
    restart_behind_fade(0);
    return COREGAMELOOP;
}

// After the exit: the elevator ride with its log line, then the next game level, with
// full lives; a test room starts over. The boss level has no exit: the ending follows it.
void go_to_next_level(void) BANKED
{
    story_elevator_log(level_index);
    if (level_index < GAME_LEVEL_COUNT) level_index++;
    lives = START_LIVES;
}

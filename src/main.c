#include <gb/gb.h>
#include <stdint.h>

#include "./utils.h"
#include "./engine/camera.h"
#include "./engine/palette.h"
#include "./engine/screen.h"
#include "./level/level_manager.h"
#include "./level/section.h"
#include "./player/player.h"
#include "./input/input.h"
#include "./actor/actor.h"
#include "./sound/music.h"
#include "./sound/sound.h"

#include "./game_state/game_over.h"
#include "./game_state/intro_credits.h"
#include "./game_state/pause.h"
#include "./game_state/prepare_gameboy.h"
#include "./game_state/start_screen.h"

// Buttons held this frame and the last one (J_* bits).
uint8_t joypad_current = 0;
uint8_t joypad_previous = 0;

uint8_t current_game_state = INTRO_CREDITS_STATE;   // the states are in utils.h

Player player;

#define START_LIVES 3
uint8_t lives = START_LIVES;   // refilled on game over

// Frames the screen showed without a new update, because an update ran past VBlank.
// Read by the tests; in Emulicious, watch _lag_frames.
uint8_t lag_frames = 0;

#define RESPAWN_INVINCIBILITY_FRAMES 90   // 1.5 s at 60 fps
#define RESTART_FADE_FRAMES 4             // per fade step: about half a second out and in

// Feet below this y: the player fell out of the level.
#define FALL_DEATH_Y (DEVICE_SCREEN_PX_HEIGHT + SPRITE_HEAD_OFFSET)

// What the HUD's fragment count shows, so it's redrawn only when it changes.
static uint8_t shown_collected, shown_quota;

// Shows the current section's collected fragments against its quota, if either changed.
static void update_hud_count(void)
{
    uint8_t collected = current_level.section_collected[current_level.section];
    uint8_t quota = section_quota();
    if (collected == shown_collected && quota == shown_quota) return;
    hud_update_count(collected, quota);
    shown_collected = collected;
    shown_quota = quota;
}

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
    shown_collected = 0xFF;    // nothing shown yet
    update_hud_count();
}

// Follows the player, draws the columns that scroll into view, and spawns the actors the
// camera reaches.
static void update_view(void)
{
    camera_update(TO_PIXELS(player.x), current_level.width_px);
    level_stream_update(camera_x);
    screen_scroll(camera_x);
    actors_spawn(camera_x);
}

// Does the player's body touch an actor?
static uint8_t player_touches_actor(void)
{
    int16_t x = TO_PIXELS(player.x), y = TO_PIXELS(player.y);
    return actors_touch(x - SPRITE_LEFT_W, y - SPRITE_HEAD_OFFSET, x + SPRITE_RIGHT_W, y);
}

// Hardware sprites the last sprite list used; at boot, all of them may hold something.
static uint8_t sprites_shown = MAX_HARDWARE_SPRITES;

// Builds this frame's sprite list: the player first, then the actors, then the pause
// label if asked, then hides the hardware sprites the last list used and this one doesn't.
static void draw_sprites(uint8_t with_pause_label)
{
    uint8_t used = player_draw(&player, camera_x);
    used = actors_draw(used, camera_x);
    if (with_pause_label) used = pause_draw(used);
    if (used < sprites_shown) hide_sprites_range(used, sprites_shown);
    sprites_shown = used;
}

// Was the button pressed this frame, not held from the last?
static uint8_t new_press(uint8_t button)
{
    return (joypad_current & button) && !(joypad_previous & button);
}

// While paused: START goes back to the game. With the level-select cheat, SELECT loads
// the next level.
static void update_pause(void)
{
    if (new_press(J_START)) {
        sound_play(EFFECT_PAUSE);
        current_game_state = COREGAMELOOP;
    }
#if LEVEL_SELECT_CHEAT
    else if (new_press(J_SELECT)) {
        level_index++;
        if (level_index == LEVEL_COUNT) level_index = 0;
        palette_fade_out(RESTART_FADE_FRAMES);
        current_game_state = GAMEPLAY_START;
    }
#endif
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

// One frame of play: input, physics, the level, the actors and the sprites. Ends the
// frame early to pause, or with a state change when the player is hit or done.
void main_game_loop(void)
{
    level_stream_flush();   // while VBlank lasts

    // START pauses: everything stops where it is, with "PAUSE" over it.
    if (new_press(J_START)) {
        sound_play(EFFECT_PAUSE);
        draw_sprites(1);
        current_game_state = PAUSED;
        return;
    }

    input_read_player(&player, joypad_current, joypad_previous);
    player_update(&player);

    // Fell out of the level? Costs a life, even while invincible.
    if (TO_PIXELS(player.y) > FALL_DEATH_Y) {
        sound_play(EFFECT_HIT);
        current_game_state = GAMEPLAY_END;
        return;
    }

    // Reached the exit elevator?
    if (player.touched_exit) current_game_state = LEVEL_COMPLETE;

    sections_update(&player);
    update_view();
    actors_update(camera_x);
    update_hud_count();

    // Invincibility countdown
    if (player.invincibility_frames > 0) player.invincibility_frames--;

    // Hit by an actor or a hazard tile? Not while invincible.
    if (!player.invincibility_frames && (player.touched_hazard || player_touches_actor())) {
        sound_play(EFFECT_HIT);
        current_game_state = GAMEPLAY_END;
    }

    draw_sprites(0);
}

// Builds the level behind a black screen, then fades in.
static void start_level(void)
{
    palette_fade_to(PALETTE_BLACK);
    screen_show_sprites(0);
    level_load(level_index);
    music_play_level(level_index);
    actors_load_tiles();
    pause_load_tiles();
    hud_update_lives(lives);
    restart_section();
    draw_sprites(0);
    palette_fade_in(SCREEN_FADE_FRAMES);
    screen_show_sprites(1);
}

// Takes a life and restarts the section. On game over the lives refill and the
// section's fragments come back.
static void lose_life(void)
{
    lives--;
    hud_update_lives(lives);
    if (lives) {
        restart_behind_fade(1);
        return;
    }
    game_over();
    lives = START_LIVES;
    level_reset_fragments(current_level.section);
    restart_behind_fade(0);
}

// After the exit: the next game level, with full lives. After the last level comes
// level 1 again, until the ending exists; a test room starts over.
static void go_to_next_level(void)
{
    if (level_index < GAME_LEVEL_COUNT) {
        level_index++;
        if (level_index == GAME_LEVEL_COUNT) level_index = LEVEL_1;
    }
    lives = START_LIVES;
}

// Boots, then runs one game state per frame.
void main(void)
{
    prepare_gameboy();
#if SKIP_INTRO_AND_START
    current_game_state = GAMEPLAY_START;
#endif

    while (1) {
        input_update_joypad_state(&joypad_current, &joypad_previous);

        switch (current_game_state) {
        case INTRO_CREDITS_STATE:
            play_intro_credits();
            current_game_state = TITLE_SCREEN_STATE;
            break;
        case TITLE_SCREEN_STATE:
            run_title_screen();
            current_game_state = GAMEPLAY_START;
            break;
        case GAMEPLAY_START:
            start_level();
            current_game_state = COREGAMELOOP;
            break;
        case COREGAMELOOP:
        {
            // Every VBlank that passes during the update is a lag frame.
            uint8_t start_time = (uint8_t)sys_time;
            main_game_loop();
            lag_frames += (uint8_t)sys_time - start_time;
            break;
        }
        case PAUSED:
            update_pause();
            break;
        case LEVEL_COMPLETE:
            level_complete();
            palette_fade_out(SCREEN_FADE_FRAMES);
            go_to_next_level();
            current_game_state = GAMEPLAY_START;
            break;
        case GAMEPLAY_END:
            lose_life();
            current_game_state = COREGAMELOOP;
            break;
        }

        wait_vbl_done();
    }
}

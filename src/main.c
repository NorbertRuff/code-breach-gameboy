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

#include "./game_state/game_flow.h"
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

uint8_t lives = START_LIVES;   // refilled on game over

// Frames the screen showed without a new update, because an update ran past VBlank.
// Read by the tests; in Emulicious, watch _lag_frames.
uint8_t lag_frames = 0;

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

// Follows the player (body center x in world pixels), draws the columns that scroll into
// view, and spawns the actors the camera reaches.
static void update_view(uint16_t player_x)
{
    camera_update(player_x, current_level.width_px);
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

// Shows the HUD count again, whatever it showed before: after a (re)start.
void redraw_hud_count(void)
{
    shown_collected = 0xFF;    // nothing shown yet
    update_hud_count();
}

// Hardware sprites the last sprite list used; at boot, all of them may hold something.
static uint8_t sprites_shown = MAX_HARDWARE_SPRITES;

// Builds this frame's sprite list: the player first, then the actors, then the pause
// label if asked, then hides the hardware sprites the last list used and this one doesn't.
void draw_sprites(uint8_t with_pause_label)
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
    actors_move_lifts();
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
    uint16_t player_x = TO_PIXELS(player.x);
    update_view(player_x);
    actors_update(camera_x, player_x, TO_PIXELS(player.y));
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
            current_game_state = lose_life();
            break;
        }

        wait_vbl_done();
    }
}

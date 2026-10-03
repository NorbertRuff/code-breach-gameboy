#include <gb/gb.h>
#include <stdint.h>
#include <stdio.h>

#include "./utils.h"
#include "./engine/camera.h"
#include "./level/level_manager.h"
#include "./level/section.h"
#include "./player/player.h"
#include "./input/input.h"
#include "./actor/actor.h"
#include "./sound/sound.h"

#include "./game_state/game_over.h"
#include "./game_state/intro_credits.h"
#include "./game_state/prepare_gameboy.h"
#include "./game_state/start_screen.h"

// Game state variables
uint8_t joypad_current = 0;
uint8_t joypad_previous = 0;
uint16_t text_animation_state = 0;  // frames into the "PRESS START" blink cycle
uint8_t current_game_state = INTRO_CREDITS_STATE;

// Player instance
Player player;

// Lives left; refilled on game over.
uint8_t lives = 3;

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
    move_bkg((uint8_t)camera_x, 0);
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
    move_bkg((uint8_t)camera_x, 0);
    actors_spawn(camera_x);
}

// Does the player's body touch an actor?
static uint8_t player_touches_actor(void)
{
    int16_t x = TO_PIXELS(player.x), y = TO_PIXELS(player.y);
    return actors_touch(x - SPRITE_LEFT_W, y - SPRITE_HEAD_OFFSET, x + SPRITE_RIGHT_W, y);
}

// Builds this frame's sprite list: the player first, then the actors, then hides the
// hardware sprites left over.
static void draw_sprites(void)
{
    uint8_t used = player_draw(&player, camera_x);
    used = actors_draw(used, camera_x);
    hide_sprites_range(used, MAX_HARDWARE_SPRITES);
}

// Restarts the section while the screen is dark, then fades back in. After a lost life
// (not a game over) the player is invincible for a moment.
static void restart_behind_fade(uint8_t invincible)
{
    fade_out_screen(RESTART_FADE_FRAMES);
    HIDE_SPRITES;
    restart_section();
    if (invincible) player.invincibility_frames = RESPAWN_INVINCIBILITY_FRAMES;
    hud_update_lives(lives);
    draw_sprites();
    fade_in_screen(RESTART_FADE_FRAMES);
    SHOW_SPRITES;
}

// Main game loop
void main_game_loop()
{
    input_read_player(&player, joypad_current, joypad_previous);
    player_update(&player);

    // Fell out of the level? Costs a life, even while invincible.
    if (TO_PIXELS(player.y) > FALL_DEATH_Y) {
        sound_play_hit();
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
        sound_play_hit();
        current_game_state = GAMEPLAY_END;
    }

    draw_sprites();
}

void main(void)
{
    prepare_gameboy();

    // Feature flag: Skip intro and start screen for development/testing
    #if SKIP_INTRO_AND_START
        // Skip directly to gameplay
        current_game_state = GAMEPLAY_START;
    #endif

    while (1)
    {
        // Update input state
        input_update_joypad_state(&joypad_current, &joypad_previous);
        
        switch (current_game_state)
        {
        case INTRO_CREDITS_STATE:
            handle_intro_credits();
            current_game_state = TITLE_SCREEN_STATE;
            break;
        case TITLE_SCREEN_STATE:
            init_splash_screen();
            // Wait for START, one frame per loop.
            while (!(joypad() & J_START))
            {
                update_splash_screen(&text_animation_state);
                wait_vbl_done();
            }
            fade_out_screen(10);
            current_game_state = GAMEPLAY_START;
            break;
        case GAMEPLAY_START:
            // Build the level behind a black screen, then fade in.
            BGP_REG = 0xFF;
            HIDE_SPRITES;
            level_load(level_index);
            actors_load_tiles();
            hud_update_lives(lives);
            restart_section();
            draw_sprites();
            fade_in_screen(10);
            SHOW_SPRITES;
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
        case LEVEL_COMPLETE:
            level_complete();
            fade_out_screen(10);
            current_game_state = GAMEPLAY_START;
            break;
        case GAMEPLAY_END:
            lives--;
            if (lives == 0) {
                // Game over: the lives refill and the section's fragments come back.
                game_over();
                lives = 3;
                level_reset_fragments(current_level.section);
                restart_behind_fade(0);
            } else {
                restart_behind_fade(1);
            }
            current_game_state = COREGAMELOOP;
            break;
        default:
            break;
        }

        // Done processing, yield CPU and wait for start of next frame
        wait_vbl_done();
    }
}

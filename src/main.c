#include <gb/gb.h>
#include <stdint.h>
#include <stdio.h>

#include "./utils.h"
#include "./engine/camera.h"
#include "./level/level_manager.h"
#include "./player/player.h"
#include "./input/input.h"
#include "./enemy/enemy.h"
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

// Enemy instances
Enemy enemies[MAX_ENEMIES];

// Lives left; refilled on game over.
uint8_t lives = 3;

// Frames the screen showed without a new update, because an update ran past VBlank.
// Read by the tests; in Emulicious, watch _lag_frames.
uint8_t lag_frames = 0;

#define RESPAWN_INVINCIBILITY_FRAMES 90   // 1.5 s at 60 fps

// Feet below this y: the player fell out of the level.
#define FALL_DEATH_Y (DEVICE_SCREEN_PX_HEIGHT + SPRITE_HEAD_OFFSET)

// Places the level's enemies at the left end of their patrols, up to MAX_ENEMIES.
static void spawn_enemies(void)
{
    LevelEntity entity;
    uint8_t count = 0;
    for (uint8_t i = 0; i < current_level.entity_count && count < MAX_ENEMIES; i++) {
        level_read_entity(i, &entity);
        if (entity.type != ENTITY_ENEMY) continue;
        // The patrol range is for the 16 px body: its center stays 8 px inside.
        enemy_init(&enemies[count++], entity.x + 8, entity.y, entity.x + 8, entity.x + entity.param - 8);
    }
    while (count < MAX_ENEMIES) enemies[count++].active = 0;
}

// Puts the camera on the player at once and draws what it shows: level start, respawn.
static void reset_view(void)
{
    camera_reset(TO_PIXELS(player.x), current_level.width_px);
    level_stream_reset(camera_x);
    move_bkg((uint8_t)camera_x, 0);
}

// Follows the player and draws the columns that scroll into view.
static void update_view(void)
{
    camera_update(TO_PIXELS(player.x), current_level.width_px);
    level_stream_update(camera_x);
    move_bkg((uint8_t)camera_x, 0);
}

// Draws enemies and the player at their screen positions.
static void draw_actors(void)
{
    uint8_t i;
    for (i = 0; i < MAX_ENEMIES; i++) {
        enemy_draw(&enemies[i], ENEMY_OAM_BASE + i * 2, camera_x);
    }
    player_draw(&player, camera_x);
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

    update_view();

    // Win condition: all fragments collected
    if (current_level.fragment_count > 0 &&
        current_level.collected_count >= current_level.fragment_count) {
        current_game_state = LEVEL_COMPLETE;
    }

    // Update HUD counter when count changes
    {
        static uint8_t prev_collected = 0xFF;
        uint8_t cur = current_level.collected_count;
        if (cur != prev_collected) {
            hud_update_count(cur, current_level.fragment_count);
            prev_collected = cur;
        }
    }

    // Invincibility countdown
    if (player.invincibility_frames > 0) player.invincibility_frames--;

    // Hit by an enemy or a hazard tile? Not while invincible.
    {
        uint8_t i;
        uint8_t hit = player.touched_hazard;
        for (i = 0; i < MAX_ENEMIES; i++) {
            enemy_update(&enemies[i]);
            hit |= enemy_check_player_collision(&enemies[i], TO_PIXELS(player.x), TO_PIXELS(player.y));
        }
        if (hit && !player.invincibility_frames) {
            sound_play_hit();
            current_game_state = GAMEPLAY_END;
        }
    }

    draw_actors();
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
            player_init(&player, current_level.player_start_x, current_level.player_start_y);
            enemy_init_sprite_data();
            spawn_enemies();
            hud_update_lives(lives);
            reset_view();
            draw_actors();
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
                // All lives spent — show game over and do a full restart
                lives = 3;
                game_over();
                fade_out_screen(10);
                current_game_state = GAMEPLAY_START;
            } else {
                // Respawn: reset player and enemies, grant invincibility frames
                player_init(&player, current_level.player_start_x, current_level.player_start_y);
                player.invincibility_frames = RESPAWN_INVINCIBILITY_FRAMES;
                spawn_enemies();
                hud_update_lives(lives);
                reset_view();
                current_game_state = COREGAMELOOP;
            }
            break;
        default:
            break;
        }

        // Done processing, yield CPU and wait for start of next frame
        wait_vbl_done();
    }
}

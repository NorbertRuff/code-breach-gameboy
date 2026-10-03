#include <gb/gb.h>
#include <stdint.h>
#include <stdio.h>

#include "./graphics/background.h"
#include "./graphics/programmer.h"
#include "./utils.h"
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
uint16_t scroll_value = 0;
int16_t total_scroll_x = 0;  // Accumulated scroll offset for collision detection
uint16_t text_animation_state = 0;  // frames into the "PRESS START" blink cycle
uint8_t current_game_state = INTRO_CREDITS_STATE;

// Player instance
Player player;

// Level manager instance
LevelManager level_manager;

// Enemy instances
Enemy enemies[MAX_ENEMIES];

// Lives left; refilled on game over.
uint8_t lives = 3;

#define RESPAWN_INVINCIBILITY_FRAMES 90   // 1.5 s at 60 fps

// Feet below this y: the player fell out of the level.
#define FALL_DEATH_Y (LEVEL_HEIGHT * TILE_SIZE + SPRITE_HEAD_OFFSET)

// Level 1 enemies: start x, feet y, patrol range.
typedef struct {
    uint16_t x;
    uint8_t  y;
    uint16_t patrol_min;
    uint16_t patrol_max;
} EnemySpawn;

static const EnemySpawn level1_enemies[] = {
    {  88, 119,  64, 112 },   // ground (row 15)
    {  80,  71,  72,  88 },   // platform B (row 9)
    { 184,  79, 176, 192 },   // platform D (row 10)
};
#define LEVEL1_ENEMY_COUNT (sizeof(level1_enemies) / sizeof(level1_enemies[0]))

// Places the level's enemies at their start positions.
static void spawn_enemies(void)
{
    uint8_t i;
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (i < LEVEL1_ENEMY_COUNT) {
            const EnemySpawn* s = &level1_enemies[i];
            enemy_init(&enemies[i], s->x, s->y, s->patrol_min, s->patrol_max);
        } else {
            enemies[i].active = 0;
        }
    }
}

// Keeps the player centered, within the level bounds.
static void update_camera(void)
{
    int16_t cam = (int16_t)player.x - 80;
    if (cam < 0) cam = 0;
    if (cam > (int16_t)MAX_SCROLL_X) cam = (int16_t)MAX_SCROLL_X;
    total_scroll_x = cam;
    move_bkg((uint8_t)total_scroll_x, 0);
}

// Draws enemies and the player at their screen positions.
static void draw_actors(void)
{
    uint8_t i;
    for (i = 0; i < MAX_ENEMIES; i++) {
        enemy_draw(&enemies[i], ENEMY_OAM_BASE + i * 2, total_scroll_x);
    }
    player.draw_x = (uint16_t)((int16_t)player.x - total_scroll_x);
    player_draw(&player);
}

// Main game loop
void main_game_loop()
{
    // Handle input (movement_force only; camera handles scrolling)
    input_handle_joystick_input(&player, joypad_current, joypad_previous, &scroll_value);

    // Update player physics in world coordinates (scroll_x=0: player->x is world X)
    player_handle_movement_physics(&player, &level_manager.current_level, 0);
    player_handle_jumping_physics(&player, &level_manager.current_level, 0);

    // Fell out of the level? Costs a life, even while invincible.
    if ((int16_t)player.y > (int16_t)FALL_DEATH_Y) {
        sound_play_hit();
        current_game_state = GAMEPLAY_END;
        return;
    }

    update_camera();

    // Update and render level
    level_update(&level_manager);
    level_render(&level_manager);

    // Check collectibles (world coords, no scroll offset needed)
    player_check_collectibles(&player, &level_manager.current_level, 0);

    // Win condition: all collectibles gathered
    if (level_manager.current_level.collectible_count > 0 &&
        level_manager.current_level.collected_count >= level_manager.current_level.collectible_count) {
        current_game_state = LEVEL_COMPLETE;
    }

    // Update HUD counter when count changes
    {
        static uint8_t prev_collected = 0xFF;
        uint8_t cur = level_manager.current_level.collected_count;
        if (cur != prev_collected) {
            hud_update_count(cur, level_manager.current_level.collectible_count);
            prev_collected = cur;
        }
    }

    // Invincibility countdown
    if (player.invincibility_frames > 0) player.invincibility_frames--;

    // Update enemies and check player collision (skip if invincible)
    {
        uint8_t i;
        for (i = 0; i < MAX_ENEMIES; i++) {
            enemy_update(&enemies[i]);
            if (!player.invincibility_frames &&
                enemy_check_player_collision(&enemies[i], player.x, player.y)) {
                sound_play_hit();
                current_game_state = GAMEPLAY_END;
            }
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
            // Initialize level system (includes HUD and sprite setup)
            level_manager_init(&level_manager);
            level_load_level(&level_manager, 1);  // Load Level 1: Server Room
            // Initialize player at level start position
            player_init(&player, level_manager.current_level.player_start_x, level_manager.current_level.player_start_y);
            // Initialize enemies and load their sprite tiles into sprite VRAM
            enemy_init_sprite_data();
            spawn_enemies();
            hud_update_lives(lives);
            update_camera();
            draw_actors();
            fade_in_screen(10);
            SHOW_SPRITES;
            current_game_state = COREGAMELOOP;
            break;
        case COREGAMELOOP:
            main_game_loop();
            // current_game_state = GAMEPLAY_END;
            // fade_out_screen(10);
            break;
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
                player_init(&player,
                    level_manager.current_level.player_start_x,
                    level_manager.current_level.player_start_y);
                player.invincibility_frames = RESPAWN_INVINCIBILITY_FRAMES;
                spawn_enemies();
                hud_update_lives(lives);
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

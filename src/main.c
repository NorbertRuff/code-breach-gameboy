#include <gb/gb.h>
#include <stdint.h>
#include <stdio.h>

#include "../res/background.h"
#include "../res/mario.h"
#include "./utils.h"

#include "./game_state/core_game_loop.h"
#include "./game_state/intro_credits.h"
#include "./game_state/prepare_core_game_loop.h"
#include "./game_state/start_screen.h"

#define FLOOR_Y 130
#define MARIO_RUN_FRAMECOUNT 4
#define MARIO_WALKING_SPEED 20

uint16_t mario_x;

uint16_t mario_draw_x;
uint16_t mario_draw_frame;
uint8_t mario_direction = 0;

uint8_t mario_frame = 0;

uint8_t joypad_current = 0;
uint8_t joypad_previous = 0;

uint16_t scroll_value = 0;
uint16_t text_animation_state = ANIMATION_COUNTER_TIME;

uint8_t current_game_state = INTRO_CREDITS_STATE;

void handle_background_scrolling()
{
    scroll_bkg(scroll_value, 0);
    scroll_value = 0;
}

int8_t handle_joystick_input()
{

    int8_t movement = 0;
    if (joypad_current & J_LEFT)
    {
        movement = -1;
        mario_direction = 0;
        scroll_value -= 2;
    }
    else if (joypad_current & J_RIGHT)
    {
        movement = 1;
        mario_direction = 1;
        scroll_value += 2;
    }

    // When a is pressed
    if ((joypad_current & J_A) && !(joypad_previous & J_A))
    {
        // Unshift the value if we WERE  in SPR
        mario_x = mario_x >> 4;
        // SHIFT the value if we are now in SPR
        mario_x = mario_x << 4;
    }
    return movement;
}

void handle_mario_animation(int8_t *movement)
{

    mario_draw_x = mario_x;
    mario_draw_frame = mario_frame;

    // Increase/Decrease the "face" value
    mario_x += *movement * MARIO_WALKING_SPEED;

    // Draw mario at the true value
    mario_draw_x = mario_x >> 4;

    // If the player is moving
    if (*movement != 0)
    {

        // Increase mario's frame by 2
        // This is a sub-pixel resolution value
        mario_frame += 2;

        // Increase mario's frame
        // Loop background after 3
        // Shift right 4 bits to get it's true value
        if ((mario_frame >> 4) >= MARIO_RUN_FRAMECOUNT)
            mario_frame = 0;

        // Frame 0 is or standing
        // Increase by one for running frames
        // Shift right 4 bits to get it's true value
        mario_draw_frame = 1 + mario_frame >> 4;
    }
    else
    {

        // Draw with our standing frame 0
        mario_frame = 0;
        mario_draw_frame = 0;
    }

    // Draw the mario metasprite
    // Flip the metasprite horizontally (using vflip) if mario isn' facing right
    if (mario_direction == 1)
    {
        move_metasprite(mario_metasprites[mario_draw_frame], 0, 0, mario_draw_x, FLOOR_Y);
    }
    else
    {
        move_metasprite_vflip(mario_metasprites[mario_draw_frame], 0, 0, mario_draw_x + 4, FLOOR_Y);
    }
}

// Main game loop
void main_game_loop()
{
    handle_background_scrolling();
    int16_t movement = handle_joystick_input();

    handle_mario_animation(&movement);
}

void main(void)
{
    prepare_gameboy();

    // Loop forever
    while (1)
    {
        joypad_previous = joypad_current;
        joypad_current = joypad();
        switch (current_game_state)
        {
        case INTRO_CREDITS_STATE:
            handle_intro_credits();
            current_game_state = TITLE_SCREEN_STATE;
            break;
        case TITLE_SCREEN_STATE:
            init_splash_screen();
            // Wait for the player to press start
            while (joypad_current != J_START)
            {
                update_splash_screen(&text_animation_state);
            }
            fade_out_screen(10);
            current_game_state = GAMEPLAY_START;
            break;
        case GAMEPLAY_START:
            prepare_core_game_loop();
            // Put mario in the middle
            mario_x = 10;
            current_game_state = COREGAMELOOP;
            break;
        case COREGAMELOOP:
            main_game_loop();
            current_game_state = GAMEPLAY_END;
            fade_out_screen(10);
            break;
        case GAMEPLAY_END:
            game_over();
            fade_out_screen(10);
            current_game_state = GAMEPLAY_START;
            break;
        default:
            break;
        }

        // Done processing, yield CPU and wait for start of next frame
        vsync();
    }
}

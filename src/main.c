#include <gb/gb.h>
#include <stdint.h>
#include <stdio.h>
#include "../res/splashscreen.h"
#include "../res/background.h"
#include "../res/intro.h"
#include "../res/utils.h"

uint16_t scroll_value = 0;
uint8_t game_state;
uint16_t text_animation_timer;
uint8_t button_pressed = 0;

enum game_states
{
    TITLE,
    SPLASH_SCREEN,
    PLAYING,
    GAME_OVER,
    WIN,
    PAUSE
} state = TITLE;

void handle_background_scrolling()
{
    // What scanline are we currently at?
    switch (LYC_REG)
    {
    case 0:
        // The interrupt should next trigger here
        LYC_REG = 65;
        // No need to move the background here
        // This will all be one color
        // Any motion wont be noticable
        SCX_REG = 0;
        break;
    case 65:
        // The interrupt should next trigger here
        LYC_REG = 79;
        // Move everything below on the background (until our next interrupt)
        SCX_REG = scrollValue >> 3;
        break;
    case 79:
        // The interrupt should next trigger here
        LYC_REG = 95;
        // Move everything below on the background (until our next interrupt)
        SCX_REG = scrollValue >> 5;
        break;
    case 95:
        // The interrupt should next trigger here
        LYC_REG = 116;
        // Move everything below on the background (until our next interrupt)
        SCX_REG = scrollValue >> 7;
        break;
    case 116:
        // The interrupt should next trigger here
        LYC_REG = 0;
        // Move everything below on the background (until our next interrupt)
        SCX_REG = scrollValue;
        break;
    }
}


void main(void)
{
    DISPLAY_ON;
    // Set the intro screen
    intro_credits(100);

    // Initialize the splash screen
    init_splash_screen(&game_state, &text_animation_timer);

    // Wait for start button to be pressed
    waitpad(J_START);


    // Fade out screen
    fade_out(10);

    // Set the tiles for our background in vram
    set_bkg_data(0, background_TILE_COUNT, background_tiles);

    // Set the colors used by our background also into vram
    set_bkg_palette(0, background_PALETTE_COUNT, background_palettes);

    // Set the attributes for each background tile
    VBK_REG = 1;
    set_bkg_tiles(0, 0, background_WIDTH >> 3, background_HEIGHT >> 3, background_map_attributes);

    // Set the actual background tile
    VBK_REG = 0;
    set_bkg_tiles(0, 0, background_WIDTH >> 3, background_HEIGHT >> 3, background_map);

    // Use interrupts to achieve parallax scrolling
    // Set the LYC register at 0, where we will start the scrolling logic
    // From there move diferent chunks of the background different amounts
    // This will create the illusion of parallax scrolling

    STAT_REG |= STATF_LYC; // prepare LYC=LY interrupt when LCD/STAT interrupts are enabled
    LYC_REG = 0u;          // set the scanline where interrupt first fires
    CRITICAL
    {
        // disable LCD interrupt while we change the handler
        add_LCD(handle_background_scrolling);
    }
    set_interrupts(LCD_IFLAG | VBL_IFLAG); // additionally enable LCD interrupt

    // Fade in screen
    fade_in(10);
    state = PLAYING;

    // Loop forever
    while (1)
    {
        joypad_current = joypad();
        switch (state)
        {
        case PLAYING:
            performant_delay(5);

            if (joypad_current & J_A)
            {
                scroll_value += 10;
            }

            if (joypad_current & J_LEFT)
            {
                scroll_value -= 10;
            }

            if (joypad_current & J_RIGHT)
            {
                scroll_value += 10;
            }

            // Done processing, yield CPU and wait for start of next frame
            wait_vbl_done();
        }
    }
}
#include <gb/gb.h>
#include <stdint.h>
#include <stdio.h>
#include "../res/splashscreen.h"
#include "../res/background.h"
#include "../res/mario.h"
#include "../res/intro.h"
#include "../res/utils.h"

#define FLOOR_Y 130

#define DEFAULT 0 
#define FRAME_RATE 1 
#define COUNTER 2
#define SUB_PIXEL_RESOLUTION 3 

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define ABS(x) ((x < 0) ? -x : x)

#define MARIO_RUN_FRAMECOUNT 4

uint8_t runCounter=0;
uint8_t counter=0;
uint16_t marioX;

uint16_t marioDrawX;
uint16_t marioDrawFrame;
uint8_t marioDirection=0;

uint8_t marioFrame=0;

uint8_t joypadCurrent=0;
uint8_t joypadPrevious=0;


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
        SCX_REG = scroll_value >> 3;
        break;
    case 79:
        // The interrupt should next trigger here
        LYC_REG = 95;
        // Move everything below on the background (until our next interrupt)
        SCX_REG = scroll_value >> 5;
        break;
    case 95:
        // The interrupt should next trigger here
        LYC_REG = 116;
        // Move everything below on the background (until our next interrupt)
        SCX_REG = scroll_value >> 7;
        break;
    case 116:
        // The interrupt should next trigger here
        LYC_REG = 0;
        // Move everything below on the background (until our next interrupt)
        SCX_REG = scroll_value;
        break;
    }
}


int8_t HandleInput()
{
    int8_t movement = 0;
    if(joypadCurrent & J_LEFT){
        movement = -1;
        marioDirection=0;
        scroll_value -= 10;

    }
    else if(joypadCurrent & J_RIGHT){
        movement=1;
        marioDirection=1;
        scroll_value += 10;
    }

    // When a is pressed
    if((joypadCurrent & J_A) && !(joypadPrevious & J_A)){

        // Unshift the value if we WERE  in SPR
        marioX=marioX>>4;


        // SHIFT the value if we are now in SPR
        marioX=marioX<<4;
    }
    return movement;
}


void setup() 
{
    SHOW_SPRITES;
    SPRITES_8x16;
    DISPLAY_ON;
}

void main(void)
{
    setup();
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

    // <<<<< MARIO >>>>>

    set_sprite_data(0,mario_TILE_COUNT,mario_tiles);
    set_sprite_palette(0,mario_PALETTE_COUNT,mario_palettes);


    // Fade in screen
    fade_in(10);
    state = PLAYING;
    
    // Put mario in the middle
    marioX=10;
    // <<<<< MARIO >>>>>


    // Loop forever
    while (1)
    {
        joypadPrevious=joypadCurrent;
        joypadCurrent=joypad();

        int16_t movement = HandleInput();

        marioDrawX=marioX;
        marioDrawFrame=marioFrame;


        // Increase/Decrease the "face" value 
        marioX+=movement*25;

        // Draw mario at the true value
        marioDrawX=marioX>>4;

        // If the player is moving
        if(movement!=0){

            // Increase mario's frame by 2
            // This is a sub-pixel resolution value
            marioFrame+=2;
            
            // Increase mario's frame
            // Loop background after 3
            // Shift right 4 bits to get it's true value
            if((marioFrame>>4)>=MARIO_RUN_FRAMECOUNT)marioFrame=0;
        
            // Frame 0 is or standing
            // Increase by one for running frames
            // Shift right 4 bits to get it's true value
            marioDrawFrame=1+marioFrame>>4;
        }else{

            // Draw with our standing frame 0
            marioFrame=0;
            marioDrawFrame=0;
        }

        // Draw the mario metasprite
        // Flip the metasprite horizontally (using vflip) if mario isn' facing right
        if(marioDirection==1)move_metasprite(mario_metasprites[marioDrawFrame],0,0,marioDrawX,FLOOR_Y);
        else move_metasprite_vflip(mario_metasprites[marioDrawFrame],0,0,marioDrawX+4,FLOOR_Y);
        

        // Done processing, yield CPU and wait for start of next frame
        wait_vbl_done();
    
        switch (state)
        {
        case PLAYING:
            performant_delay(5);

            if (joypadCurrent & J_A)
            {
                scroll_value += 10;
            }

            if (joypadCurrent & J_LEFT)
            {
                scroll_value -= 10;
            }

            if (joypadCurrent & J_RIGHT)
            {
                scroll_value += 10;
            }

            // Done processing, yield CPU and wait for start of next frame
            wait_vbl_done();
        }
    }
}
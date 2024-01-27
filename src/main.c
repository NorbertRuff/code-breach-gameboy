#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include "../res/Background.h"

uint16_t scrollValue=0;

void HandleBackgroundScrolling(){
    // What scanline are we currently at?
    switch(LYC_REG){
        case 0:
            // The interrupt should next trigger here
            LYC_REG=65;
            // No need to move the background here
            // This will all be one color
            // Any motion wont be noticable
            SCX_REG = 0;
            break;
        case 65:
            // The interrupt should next trigger here
            LYC_REG=79;
            // Move everything below on the background (until our next interrupt)
            SCX_REG = scrollValue>>3;
            break;
        case 79:
            // The interrupt should next trigger here
            LYC_REG=95;
            // Move everything below on the background (until our next interrupt)
            SCX_REG = scrollValue>>5;
            break;
        case 95:
            // The interrupt should next trigger here
            LYC_REG=116;
            // Move everything below on the background (until our next interrupt)
            SCX_REG = scrollValue>>7;
            break;
        case 116:
            // The interrupt should next trigger here
            LYC_REG=0;
            // Move everything below on the background (until our next interrupt)
            SCX_REG = scrollValue;
            break;
    }

}


void main(void)
{
    // Set the tiles for our background in vram
    set_bkg_data(0,Background_TILE_COUNT,Background_tiles);

    // Set the colors used by our background also into vram
    set_bkg_palette(0,Background_PALETTE_COUNT,Background_palettes);

    SHOW_BKG;
    DISPLAY_ON;

    // Set the attributes for each background tile
    VBK_REG =1;
    set_bkg_tiles(0,0,Background_WIDTH>>3,Background_HEIGHT>>3,Background_map_attributes);

    // Set the actual background tile
    VBK_REG =0;
    set_bkg_tiles(0,0,Background_WIDTH>>3,Background_HEIGHT>>3,Background_map);


    // We're gonna use interrupts to achieve parallax scrolling
    // Set the LYC register at 0, where we will start the scrolling logic
    // From there we will move diferent chunks of the background different amounts
    STAT_REG|=STATF_LYC; // prepare LYC=LY interrupt when LCD/STAT interrupts are enabled
    LYC_REG=0u; // set the scanline where interrupt first fires
    CRITICAL{
        add_LCD(HandleBackgroundScrolling);
    }
    set_interrupts(LCD_IFLAG|VBL_IFLAG); // additionally enable LCD interrupt


    // Loop forever
    while(1) {


        delay(100);
        if ((joypad() & J_A)) {
            scrollValue += 10;
        }
        if (joypad() & J_LEFT) {
            scrollValue -= 10;
        }
        if (joypad() & J_RIGHT) {
            scrollValue += 10;
        }

        // Done processing, yield CPU and wait for start of next frame
        wait_vbl_done();
    }
}
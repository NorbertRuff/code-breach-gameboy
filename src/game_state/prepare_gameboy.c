#include <gb/cgb.h>
#include <gb/gb.h>

void setup_display(void);
void setup_sound(void);

void prepare_gameboy()
{
    setup_display();
    setup_sound();
}

void setup_display()
{
    DISPLAY_ON;
    SHOW_BKG;
    SHOW_SPRITES;
    SPRITES_8x16;
}

void setup_sound()
{
    // This enables Sound
    // these registers must be in this specific order!
    NR52_REG = 0x80; // is 1000 0000 in binary and turns on sound
    NR50_REG = 0x77; // sets the volume for both left and right channel just set to max 0x77
    NR51_REG = 0xFF; // is 1111 1111 in binary, select which chanels we want to use in this case all of them. One bit for the L one bit for the R of all four channels
}

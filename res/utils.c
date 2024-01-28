#include <gb/gb.h>
#include "utils.h"


void performant_delay(uint16_t time) {
    uint8_t i;
    for (i = 0; i < time; i++) {
        wait_vbl_done();
    }
}



void fade_out(uint16_t time) {
    for (uint8_t i = 0; i < 4; i++) {
        switch (i) {
            case 0:
                BGP_REG = 0xE4;
                break;
            case 1:
                BGP_REG = 0xF9;
                break;
            case 2:
                BGP_REG = 0xFE;
                break;
            case 3:
                BGP_REG = 0xFF;
                break;
        }
        performant_delay(time);
    }
}

void fade_in(uint16_t time) {
    for (uint8_t i = 0; i < 3; i++) {
        switch (i) {
            case 0:
                BGP_REG = 0xFE;
                break;
            case 1:
                BGP_REG = 0xF9;
                break;
            case 2:
                BGP_REG = 0xE4;
                break;
            case 3:
                BGP_REG = 0xE0;
                break;
        }
        performant_delay(time);
    }
}

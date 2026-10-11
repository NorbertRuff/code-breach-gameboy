#ifndef DEMO_H
#define DEMO_H

#include <gb/gb.h>
#include <stdint.h>

// The demo: after a minute on the title screen without a press, a clip of the next level
// plays itself from recorded buttons (demo.c; the clips come from "make demo").

#define DEMO_IDLE_FRAMES 3600   // 1 min at 60 fps

extern uint8_t demo_playing;

void demo_start(void) BANKED;
uint8_t demo_play_input(uint8_t* buttons) BANKED;

#endif // DEMO_H

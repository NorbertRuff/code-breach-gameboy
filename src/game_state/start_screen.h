#ifndef START_SCREEN_H_
#define START_SCREEN_H_

// "PRESS START" blink: shown 40 of every 60 frames.
#define PRESS_START_CYCLE_FRAMES 60
#define PRESS_START_SHOWN_FRAMES 40

void init_splash_screen();

// Call once per frame while the title screen waits for START.
void update_splash_screen(uint16_t *frame_counter);

#endif // START_SCREEN_H_

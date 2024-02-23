#ifndef INTRO_H_
#define INTRO_H_

#define ANIMATION_COUNTER_TIME 3000

#define START_TEXT_X 15
#define START_TEXT_Y 15

void intro_credits(uint16_t time);

void init_splash_screen();

void update_splash_screen(uint16_t *text_animation_state);

#endif

#ifndef INTRO_H_
#define INTRO_H_

#define ANIMATION_CYCLE_TIME 100
#define HALF_ANIMATION_CYCLE_TIME ANIMATION_CYCLE_TIME / 2

#define PRESS_TEXT_X 2
#define PRESS_TEXT_Y 15

#define START_TEXT_X 15
#define START_TEXT_Y 15

void intro_credits(uint16_t time);

void init_splash_screen(uint8_t *game_state, uint16_t *text_animation_timer);

void update_splash_screen(uint16_t *title_screen_animation_timer);

#endif

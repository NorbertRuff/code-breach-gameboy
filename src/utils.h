#ifndef UTILS_H_
#define UTILS_H_

#define INTRO_CREDITS_STATE 0
#define TITLE_SCREEN_STATE 1
#define GAMEPLAY_START 2
#define GAMEPLAY_END 3
#define COREGAMELOOP 4

void performant_delay(uint16_t time);

void fade_out_screen(uint16_t time);

void fade_in_screen(uint16_t time);

#endif /* UTILS_H_ */

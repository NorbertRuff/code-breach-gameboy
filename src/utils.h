#ifndef UTILS_H_
#define UTILS_H_

// 1: skip the intro and title screen (make EXTRA_CFLAGS=-DSKIP_INTRO_AND_START=0 to see them)
#ifndef SKIP_INTRO_AND_START
#define SKIP_INTRO_AND_START 1
#endif

#define INTRO_CREDITS_STATE 0
#define TITLE_SCREEN_STATE 1
#define GAMEPLAY_START 2
#define COREGAMELOOP 3
#define GAMEPLAY_END 4
#define LEVEL_COMPLETE 5

void performant_delay(uint16_t time);

void fade_out_screen(uint16_t time);

void fade_in_screen(uint16_t time);

#endif /* UTILS_H_ */

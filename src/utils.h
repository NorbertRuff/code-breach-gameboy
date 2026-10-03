#ifndef UTILS_H_
#define UTILS_H_

// Feature flags for development and testing
// Set to 1 to skip intro credits and press start screen. Can be overridden from the
// command line: make EXTRA_CFLAGS=-DSKIP_INTRO_AND_START=0
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

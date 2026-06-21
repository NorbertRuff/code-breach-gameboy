#ifndef UTILS_H_
#define UTILS_H_

// Feature flags for development and testing
#define SKIP_INTRO_AND_START 1  // Set to 1 to skip intro credits and press start screen
#define EMPTY_BACKGROUND 1       // Set to 1 to draw empty background (useful for debugging)
#define DISABLE_BACKGROUND_SCROLLING 0  // Set to 1 to disable background scrolling

#define INTRO_CREDITS_STATE 0
#define TITLE_SCREEN_STATE 1
#define GAMEPLAY_START 2
#define COREGAMELOOP 3
#define GAMEPLAY_END 4

void performant_delay(uint16_t time);

void fade_out_screen(uint16_t time);

void fade_in_screen(uint16_t time);

#endif /* UTILS_H_ */

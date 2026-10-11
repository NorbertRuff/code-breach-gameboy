#ifndef UTILS_H_
#define UTILS_H_

#include <stdint.h>

// 1: skip the intro and title screen (make EXTRA_CFLAGS=-DSKIP_INTRO_AND_START=0 to see them)
#ifndef SKIP_INTRO_AND_START
#define SKIP_INTRO_AND_START 1
#endif

// 1: SELECT in the pause menu loads the next level, test rooms included (for testing)
#ifndef LEVEL_SELECT_CHEAT
#define LEVEL_SELECT_CHEAT 1
#endif

// 1: B in the pause menu turns the no-death cheat on or off (for testing)
#ifndef NO_DEATH_CHEAT
#define NO_DEATH_CHEAT 1
#endif

// Game states (current_game_state in main.c)
#define INTRO_CREDITS_STATE 0
#define TITLE_SCREEN_STATE 1
#define GAMEPLAY_START 2
#define COREGAMELOOP 3
#define GAMEPLAY_END 4
#define LEVEL_COMPLETE 5
#define PAUSED 6
#define BOSS_HIT 7          // an upload done at KERNEL's terminal (src/game_state/kernel.c)
#define ENDING 8

void wait_frames(uint16_t frames);

#endif /* UTILS_H_ */

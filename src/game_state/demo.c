// Not per-frame code: in a ROM bank, to keep bank 0 free. The clips are in this file's
// bank too (demo_clips.h), so it reads them without switching banks.
#pragma bank 255

#include "demo.h"
#include "demo_clips.h"
#include "game_flow.h"
#include "../level/level_manager.h"

uint8_t demo_playing;                  // 1 while a clip plays in place of the joypad
static uint8_t next_demo_level;        // the levels take turns, LEVEL_1 (0) first
static const uint8_t* clip_read;       // the next (buttons, updates) pair
static const uint8_t* clip_end;
static uint8_t clip_buttons;
static uint8_t clip_updates_left;      // of clip_buttons

// Sets up the demo of the next level in turn, with full lives; GAMEPLAY_START loads it.
void demo_start(void) BANKED
{
    level_index = next_demo_level;
    if (++next_demo_level == GAME_LEVEL_COUNT) next_demo_level = LEVEL_1;
    lives = START_LIVES;
    clip_read = demo_clips[level_index];
    clip_end = demo_clip_ends[level_index];
    clip_updates_left = 0;
    demo_playing = 1;
}

// Once per gameplay update in the demo: puts the clip's buttons in place of the joypad's.
// Returns 1, and ends the demo, when a real button is pressed or the clip has run out.
uint8_t demo_play_input(uint8_t* buttons) BANKED
{
    if (*buttons || (!clip_updates_left && clip_read == clip_end)) {
        demo_playing = 0;
        return 1;
    }
    if (!clip_updates_left) {
        clip_buttons = *clip_read++;
        clip_updates_left = *clip_read++;
    }
    clip_updates_left--;
    *buttons = clip_buttons;
    return 0;
}

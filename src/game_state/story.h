#ifndef STORY_H
#define STORY_H

#include <gb/gb.h>
#include <stdint.h>

// The story's few lines and the ending (docs/GAME_DESIGN.md, "Story"; story.c, a ROM bank).
void story_boot_screen(void) BANKED;
void story_type_log(uint8_t level, uint8_t row) BANKED;
void story_elevator_log(uint8_t level) BANKED;
void story_ending(void) BANKED;

#endif // STORY_H

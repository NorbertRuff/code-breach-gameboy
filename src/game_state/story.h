#ifndef STORY_H
#define STORY_H

#include <gb/gb.h>
#include <stdint.h>

// The story's few lines (docs/STORY.md; story.c, a ROM bank).
void story_boot_screen(void) BANKED;
void story_elevator_log(uint8_t level) BANKED;

#endif // STORY_H

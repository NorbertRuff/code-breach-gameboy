#ifndef KERNEL_H
#define KERNEL_H

#include <gb/gb.h>
#include <stdint.h>

// KERNEL, the boss of level 5 (kernel.c, a ROM bank; docs/LEVEL_DESIGN.md section 7).
#define LEVEL_BOSS LEVEL_5

void kernel_start(void) BANKED;
void kernel_restart(void) BANKED;
void kernel_intro(void) BANKED;
uint8_t kernel_update(void) BANKED;
uint8_t kernel_hit(void) BANKED;

#endif // KERNEL_H

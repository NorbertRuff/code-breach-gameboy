#ifndef PROGRESS_H
#define PROGRESS_H

#include <gb/gb.h>
#include <stdint.h>

// What the player has done, kept in a 4-letter password (docs/ARCHITECTURE.md, "Saves: passwords"): the
// furthest level they may play and a 100% mark per finished level.
// progress.c, a ROM bank.

#define PASSWORD_LENGTH 4
#define PASSWORD_LETTERS "BCDFGHJKLMNPRTWX"   // easy to read; letter n has the value n
#define PASSWORD_LETTER_COUNT 16

extern uint8_t furthest_level;   // the highest level index the player may pick
extern uint8_t perfect_levels;   // bit n: level n finished with all of its fragments

void progress_reset(void) BANKED;
void progress_finish_level(uint8_t level, uint8_t perfect) BANKED;
void password_make(char* text) BANKED;
uint8_t password_check(const uint8_t* typed) BANKED;

#endif // PROGRESS_H

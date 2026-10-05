// Not per-frame code: in a ROM bank, to keep bank 0 free.
#pragma bank 255

#include "progress.h"
#include "../level/level_manager.h"

// The progress byte a password holds.
#define LEVEL_BITS    0x07   // bits 0-2: the furthest level
#define PERFECT_SHIFT 3      // bits 3-6: the 100% marks of levels 1-4
#define PERFECT_BITS  0x0F
#define HARD_MODE_BIT 0x80
#define MARKED_LEVELS 4      // the boss level (5) has no mark

uint8_t furthest_level;
uint8_t perfect_levels;
uint8_t hard_mode;

static const char letters[] = PASSWORD_LETTERS;

// A new game: only level 1, no marks, normal mode.
void progress_reset(void) BANKED
{
    furthest_level = 0;
    perfect_levels = 0;
    hard_mode = 0;
}

// Notes a finished game level (test rooms don't count): the next level becomes playable,
// up to the last one this build has, and all of its fragments earn its 100% mark.
void progress_finish_level(uint8_t level, uint8_t perfect) BANKED
{
    if (level >= GAME_LEVEL_COUNT) return;
    if (perfect && level < MARKED_LEVELS) perfect_levels |= 1 << level;
    uint8_t next = level + 1;
    if (next == GAME_LEVEL_COUNT) next = level;
    if (next > furthest_level) furthest_level = next;
}

// The check byte of a progress byte. Each progress byte gets a different one, so a
// single mistyped letter always gives a password that is refused.
static uint8_t check_byte(uint8_t progress)
{
    uint8_t mixed = progress * 0x9D + 0x4B;
    return (mixed << 3) | (mixed >> 5);
}

// Writes the password of the progress so far: 4 letters and a 0. The letters hold the
// check byte's high half, the progress's low half, the check's low half, the progress's
// high half (tools/password.py does the same).
void password_make(char* text) BANKED
{
    uint8_t progress = furthest_level | (perfect_levels << PERFECT_SHIFT);
    if (hard_mode) progress |= HARD_MODE_BIT;
    uint8_t check = check_byte(progress);
    text[0] = letters[check >> 4];
    text[1] = letters[progress & 0x0F];
    text[2] = letters[check & 0x0F];
    text[3] = letters[progress >> 4];
    text[4] = 0;
}

// Takes a typed password (4 letter values, 0-15) as the progress if it is one: its check
// byte matches, its level is in this build, and only finished levels have marks.
// Returns 1 if taken.
uint8_t password_check(const uint8_t* typed) BANKED
{
    uint8_t check = (typed[0] << 4) | typed[2];
    uint8_t progress = (typed[3] << 4) | typed[1];
    uint8_t level = progress & LEVEL_BITS;
    uint8_t perfect = (progress >> PERFECT_SHIFT) & PERFECT_BITS;
    if (check != check_byte(progress) || level >= GAME_LEVEL_COUNT || (perfect >> (level + 1))) return 0;
    furthest_level = level;
    perfect_levels = perfect;
    hard_mode = (progress & HARD_MODE_BIT) ? 1 : 0;
    return 1;
}

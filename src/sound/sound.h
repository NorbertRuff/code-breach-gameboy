#ifndef SOUND_H
#define SOUND_H

#include <gb/gb.h>
#include <stdint.h>
#include "effects.h"   // EFFECT_*, from sounds/effects.txt

// Sound effects from FX Hammer files (sounds/effects.txt), on CH2 (square) and CH4
// (noise). One plays at a time; it takes its channels from the music until it ends.

#define SOUND_CH2 0x02   // hUGE_mute_mask bits
#define SOUND_CH4 0x08

// One effect: steps of 8 bytes (frames, channels, NR21, NR22, NR23, NR24, NR42, NR43),
// ended by a 0. Made by tools/fxhammer2c.py.
typedef struct {
    const uint8_t* steps;
    uint8_t channels;    // SOUND_CH2 | SOUND_CH4
    uint8_t priority;    // cut only by an effect with the same or a higher one
} sound_effect_t;

BANKREF_EXTERN(sound_effects)
extern const sound_effect_t sound_effects[];   // effects.c, made with effects.h

void sound_play(uint8_t effect);
void sound_step(void);

#endif // SOUND_H

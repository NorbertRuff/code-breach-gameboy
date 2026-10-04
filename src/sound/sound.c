#include <gb/gb.h>
#include "hUGEDriver.h"
#include "sound.h"
#include "../engine/banking.h"

// A step's bytes (tools/fxhammer2c.py).
enum { STEP_FRAMES, STEP_CHANNELS, STEP_NR21, STEP_NR22, STEP_NR23, STEP_NR24, STEP_NR42, STEP_NR43, STEP_SIZE };

#define NOISE_TRIGGER 0x80   // NR44: restart CH4, no length limit

static const uint8_t* next_step;   // in the effects' bank; NULL while no effect plays
static uint8_t wait_frames;        // frames before the next step
static uint8_t taken_channels;     // the playing effect's channels (SOUND_CH2, SOUND_CH4)
static uint8_t playing_priority;

// Silences the effect's channels and gives them back to the music, which sounds there
// again from its next note.
static void end_effect(void)
{
    if (taken_channels & SOUND_CH2) NR22_REG = 0;
    if (taken_channels & SOUND_CH4) NR42_REG = 0;
    hUGE_mute_mask &= ~taken_channels;
    taken_channels = 0;
    next_step = NULL;
}

// Takes the effect's channels from the music; its first step plays at the next music step.
static void start_effect(const sound_effect_t* effect)
{
    if (next_step && effect->priority < playing_priority) return;
    end_effect();
    next_step = effect->steps;
    wait_frames = 0;
    taken_channels = effect->channels;
    playing_priority = effect->priority;
    hUGE_mute_mask |= taken_channels;
}

// Plays an effect (EFFECT_* in effects.h), unless one with a higher priority is playing.
void sound_play(uint8_t effect)
{
    CRITICAL {
        WITH_BANK(sound_effects, start_effect(&sound_effects[effect]));
    }
}

// Writes the next step's sound registers, or ends the effect after its last step.
static void play_step(void)
{
    const uint8_t* step = next_step;
    if (!step[STEP_FRAMES]) {
        end_effect();
        return;
    }
    if (step[STEP_CHANNELS] & SOUND_CH2) {
        NR21_REG = step[STEP_NR21];
        NR22_REG = step[STEP_NR22];
        NR23_REG = step[STEP_NR23];
        NR24_REG = step[STEP_NR24];
    }
    if (step[STEP_CHANNELS] & SOUND_CH4) {
        NR42_REG = step[STEP_NR42];
        NR43_REG = step[STEP_NR43];
        NR44_REG = NOISE_TRIGGER;
    }
    wait_frames = step[STEP_FRAMES] - 1;
    next_step = step + STEP_SIZE;
}

// Once per frame, from music_step: the playing effect's next step when its time has come.
void sound_step(void)
{
    if (!next_step) return;
    if (wait_frames) {
        wait_frames--;
        return;
    }
    WITH_BANK(sound_effects, play_step());
}

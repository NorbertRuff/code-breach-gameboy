#include <gb/gb.h>
#include "hUGEDriver.h"
#include "sound.h"

static uint8_t muted_frames[4];   // per channel: frames it stays taken from the music

// A channel's frequency register: value = 2048 - 131072 / Hz.
// 0x200 = 85 Hz, 0x600 = 256 Hz, 0x700 = 512 Hz, 0x780 = 1024 Hz, 0x7A0 = 1365 Hz.

// Takes a channel from the music for an effect's length: the music stops writing it.
static void take_channel(uint8_t channel, uint8_t frames)
{
    CRITICAL {
        muted_frames[channel] = frames;
        hUGE_mute_channel(channel, HT_CH_MUTE);
    }
}

// Called once per frame: gives the music back each channel whose effect is over. The
// music sounds there again from its next note.
void sound_release_channels(void)
{
    for (uint8_t channel = 0; channel < 4; channel++) {
        if (muted_frames[channel] && !--muted_frames[channel]) hUGE_mute_channel(channel, HT_CH_PLAY);
    }
}

// Jump: a rising "boing" on CH1, from 512 Hz up to about 5 kHz in about 40 ms.
void sound_play_jump(void)
{
    take_channel(HT_CH1, 4);
    NR10_REG = 0x15;  // sweep period=1 (7.8 ms/step), direction=up, shifts=5
    NR11_REG = 0x80;  // 50% duty, no length limit
    NR12_REG = 0xF2;  // initial vol=15, decay, period=2
    NR13_REG = 0x00;  // freq low  (combined: 0x700)
    NR14_REG = 0x87;  // trigger, freq high=7
}

// Fragment pickup: a short bright blip on CH2 at 1365 Hz.
void sound_play_collect(void)
{
    take_channel(HT_CH2, 14);
    NR21_REG = 0x40;  // 25% duty, no length limit
    NR22_REG = 0xF1;  // initial vol=15, fast decay, period=1
    NR23_REG = 0xA0;  // freq low  (combined: 0x7A0)
    NR24_REG = 0x87;  // trigger, freq high=7
}

// Gate opening: a slow rising sweep on CH1 from 256 Hz.
void sound_play_gate(void)
{
    take_channel(HT_CH1, 22);
    NR10_REG = 0x36;  // sweep period=3 (23 ms/step), direction=up, shifts=6
    NR11_REG = 0x80;  // 50% duty
    NR12_REG = 0xF3;  // vol=15, decay, period=3
    NR13_REG = 0x00;  // freq low  (combined: 0x600)
    NR14_REG = 0x86;  // trigger, freq high=6
}

// Hit: a low thud on CH1 at 85 Hz with a noise crunch on CH4.
void sound_play_hit(void)
{
    take_channel(HT_CH1, 11);
    take_channel(HT_CH4, 28);
    NR10_REG = 0x00;  // no sweep
    NR11_REG = 0x80;  // 50% duty
    NR12_REG = 0xC1;  // vol=12, decay, fast period=1
    NR13_REG = 0x00;  // freq low  (combined: 0x200)
    NR14_REG = 0x82;  // trigger, freq high=2

    NR41_REG = 0x00;  // no length limit
    NR42_REG = 0xF2;  // vol=15, decay, period=2
    NR43_REG = 0x47;  // shift=4, 15-bit LFSR, divider=7 -> medium crunch
    NR44_REG = 0x80;  // trigger
}

// Pause and resume: a short blip on CH2 at 1024 Hz, lower than the pickup.
void sound_play_pause(void)
{
    take_channel(HT_CH2, 9);
    NR21_REG = 0x80;  // 50% duty, no length limit
    NR22_REG = 0xA1;  // vol=10, fast decay, period=1
    NR23_REG = 0x80;  // freq low  (combined: 0x780)
    NR24_REG = 0x87;  // trigger, freq high=7
}

// Denied (a closed gate without its quota): a low falling buzz on CH1 from 171 Hz.
void sound_play_denied(void)
{
    take_channel(HT_CH1, 19);
    NR10_REG = 0x1B;  // sweep period=1 (7.8 ms/step), direction=down, shifts=3
    NR11_REG = 0x80;  // 50% duty
    NR12_REG = 0xA2;  // vol=10, decay, period=2
    NR13_REG = 0x00;  // freq low  (combined: 0x500)
    NR14_REG = 0x85;  // trigger, freq high=5
}

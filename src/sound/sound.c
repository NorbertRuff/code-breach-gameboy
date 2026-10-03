#include <gb/gb.h>
#include "sound.h"

// Frequency register values for common pitches:
// freq_reg = 2048 - (131072 / Hz)
// 0x700 = 1792  ->  512 Hz  (C5)
// 0x740 = 1856  ->  668 Hz  (E5)
// 0x780 = 1920  -> 1024 Hz  (C6)
// 0x7A0 = 1952  -> 1365 Hz  (D#6)
// 0x7C0 = 1984  -> 2048 Hz
// 0x200 = 512   ->   85 Hz  (low thud)

void sound_play_jump(void)
{
    // CH1 square wave with upward frequency sweep — rising "boing"
    // Starting at ~512 Hz (0x700), sweep shifts=5 pushes it to ~5 kHz over ~40 ms
    NR10_REG = 0x15;  // sweep period=1 (7.8 ms/step), direction=up, shifts=5
    NR11_REG = 0x80;  // 50% duty, no length limit
    NR12_REG = 0xF2;  // initial vol=15, decay, period=2
    NR13_REG = 0x00;  // freq low  (combined: 0x700)
    NR14_REG = 0x87;  // trigger, freq high=7
}

void sound_play_collect(void)
{
    // CH2 short bright blip at ~1365 Hz (0x7A0)
    NR21_REG = 0x40;  // 50% duty, no length limit
    NR22_REG = 0xF1;  // initial vol=15, fast decay, period=1
    NR23_REG = 0xA0;  // freq low  (combined: 0x7A0)
    NR24_REG = 0x87;  // trigger, freq high=7
}

void sound_play_gate(void)
{
    // CH1 slow upward sweep from ~256 Hz (0x600): a gate opening
    NR10_REG = 0x36;  // sweep period=3 (23 ms/step), direction=up, shifts=6
    NR11_REG = 0x80;  // 50% duty
    NR12_REG = 0xF3;  // vol=15, decay, period=3
    NR13_REG = 0x00;  // freq low  (combined: 0x600)
    NR14_REG = 0x86;  // trigger, freq high=6
}

void sound_play_hit(void)
{
    // CH1 low thud at ~85 Hz (0x200)
    NR10_REG = 0x00;  // no sweep
    NR11_REG = 0x80;  // 50% duty
    NR12_REG = 0xC1;  // vol=12, decay, fast period=1
    NR13_REG = 0x00;  // freq low  (combined: 0x200)
    NR14_REG = 0x82;  // trigger, freq high=2

    // CH4 noise crunch layered on top
    NR41_REG = 0x00;  // no length limit
    NR42_REG = 0xF2;  // vol=15, decay, period=2
    NR43_REG = 0x47;  // shift=4, 15-bit LFSR, divider=7 -> medium crunch
    NR44_REG = 0x80;  // trigger
}

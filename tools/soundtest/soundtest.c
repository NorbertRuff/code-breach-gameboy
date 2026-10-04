// Sound test: every effect of the GBFX and GBFX2 packs, played by the game's effect
// player (src/sound/sound.c), and every song of Tronimal's music packs, to pick them by
// ear (sounds/effects.txt, music/). Built by "make soundtest" into obj/soundtest.gb.
// SELECT: effects or songs. LEFT/RIGHT: previous/next, UP/DOWN: 10 back/on, A: play,
// B: stop the song. tools/render_sounds.py and tests/test_soundtest.py set "showing_songs"
// and "selected", then press A.

#include <gb/gb.h>
#include <stdint.h>
#include <stdio.h>
#include <gbdk/console.h>
#include "sound.h"
#include "song_table.h"
#include "../../src/engine/banking.h"

extern const uint8_t sound_effect_count;          // obj/soundtest/soundtest_effects.c
extern const char* const sound_effect_names[];

uint8_t showing_songs;   // 1: the song list, 0: the effect list
uint8_t selected;        // the effect or song A plays
static uint8_t song_bank;   // the playing song's bank; 0 while none plays

// Each frame: the effect's next step and the song's.
static void play_frame(void)
{
    sound_step();
    if (song_bank) WITH_ROM_BANK(song_bank, hUGE_dosound());
}

// Effects or songs in the shown list.
static uint8_t list_length(void)
{
    uint8_t count;
    if (showing_songs) return song_count;
    WITH_BANK(sound_effects, count = sound_effect_count);
    return count;
}

// Shows the list, the selected number and its name.
static void show_selected(void)
{
    const char* list_name = showing_songs ? "SONG" : "EFFECT";   // a ?: argument can miscompile
    gotoxy(0, 7);
    printf("%s %u/%u    \n\n", list_name, (uint16_t)(selected + 1), (uint16_t)list_length());
    printf("                    \n                    ");
    gotoxy(0, 9);
    if (showing_songs) printf("%s", song_table[selected].title);
    else WITH_BANK(sound_effects, printf("%s", sound_effect_names[selected]));
}

// Moves the selection by "step", wrapping around at both ends.
static void move_selection(int8_t step)
{
    int16_t next = (int16_t)selected + step;
    if (next < 0) next += list_length();
    if (next >= list_length()) next -= list_length();
    selected = (uint8_t)next;
    show_selected();
}

// Starts the selected song from its beginning.
static void play_song(void)
{
    const song_entry_t* entry = &song_table[selected];
    CRITICAL {
        WITH_ROM_BANK(entry->bank, hUGE_init(entry->song));
        song_bank = entry->bank;
    }
}

// Stops the song and silences the channels it used.
static void stop_song(void)
{
    song_bank = 0;
    NR12_REG = NR22_REG = NR32_REG = NR42_REG = 0;
}

void main(void)
{
    NR52_REG = 0x80;   // sound on
    NR50_REG = 0x77;   // full volume, both sides
    NR51_REG = 0xFF;   // every channel on both sides
    printf("SOUND TEST\n\nSELECT: LIST\nLEFT/RIGHT: 1\nUP/DOWN: 10\nA: PLAY  B: STOP");
    show_selected();
    CRITICAL { add_VBL(play_frame); }

    uint8_t held = 0;
    while (1) {
        vsync();
        uint8_t pressed = joypad() & ~held;
        held = joypad();
        if (pressed & J_SELECT) {
            showing_songs = !showing_songs;
            selected = 0;
            show_selected();
        }
        if (pressed & J_RIGHT) move_selection(1);
        if (pressed & J_LEFT) move_selection(-1);
        if (pressed & J_DOWN) move_selection(10);
        if (pressed & J_UP) move_selection(-10);
        if (pressed & J_B) stop_song();
        if (pressed & J_A) {
            if (showing_songs) play_song();
            else sound_play(selected);
        }
    }
}

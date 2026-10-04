#include <gb/gb.h>
#include "music.h"
#include "sound.h"
#include "../engine/banking.h"
#include "../level/level_manager.h"
#include "../music/song_neurotic_robonaut.h"
#include "../music/song_tech.h"

static uint8_t song_bank;   // the playing song's ROM bank; 0 while none plays

// Once per frame, from the line interrupt below the parallax band (screen.c): one step of the song
// (hUGEDriver reads it from the song's bank), after the sound effects that are done
// hand their channels back.
void music_step(void)
{
    sound_release_channels();
    if (song_bank) WITH_ROM_BANK(song_bank, hUGE_dosound());
}

// Starts a song from its beginning. hUGEDriver keeps a pointer to it, so the VBlank
// handler switches to its bank for every step.
static void play_song(const hUGESong_t* song, uint8_t bank)
{
    CRITICAL {
        WITH_ROM_BANK(bank, hUGE_init(song));
        song_bank = bank;
    }
}

// Plays a level's song. The test rooms play level 1's.
void music_play_level(uint8_t level)
{
    if (level == LEVEL_2) play_song(&song_neurotic_robonaut, BANK(song_neurotic_robonaut));
    else play_song(&song_tech, BANK(song_tech));
}

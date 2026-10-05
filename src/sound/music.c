#include <gb/gb.h>
#include "music.h"
#include "sound.h"
#include "../engine/banking.h"
#include "../level/level_manager.h"
#include "../music/song_decampment.h"
#include "../music/song_delight.h"
#include "../music/song_gone_missing.h"
#include "../music/song_neurotic_robonaut.h"
#include "../music/song_observing_jupiter.h"
#include "../music/song_tech.h"
#include "../music/song_whirlwind.h"

static uint8_t song_bank;   // the playing song's ROM bank; 0 while none plays

// Once per frame, from the line interrupt below the parallax band (screen.c): one step of the song
// (hUGEDriver reads it from the song's bank), after the sound effect's next step.
void music_step(void)
{
    sound_step();
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
    else if (level == LEVEL_3) play_song(&song_observing_jupiter, BANK(song_observing_jupiter));
    else if (level == LEVEL_4) play_song(&song_whirlwind, BANK(song_whirlwind));
    else play_song(&song_tech, BANK(song_tech));
}

// The title screen's song.
void music_play_title(void)
{
    play_song(&song_decampment, BANK(song_decampment));
}

// The game over screen's song.
void music_play_game_over(void)
{
    play_song(&song_gone_missing, BANK(song_gone_missing));
}

// The level complete screen's song.
void music_play_level_complete(void)
{
    play_song(&song_delight, BANK(song_delight));
}

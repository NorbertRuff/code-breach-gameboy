// The sound test's songs: every song of the music packs (tools/uge2c.py --table).
#ifndef SONG_TABLE_H
#define SONG_TABLE_H

#include <gbdk/platform.h>
#include <stdint.h>
#include "hUGEDriver.h"

typedef struct {
    const hUGESong_t* song;
    uint8_t bank;
    const char* title;
} song_entry_t;

extern const uint8_t song_count;
extern const song_entry_t song_table[];

#endif

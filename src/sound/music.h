#ifndef MUSIC_H
#define MUSIC_H

#include <stdint.h>

void music_step(void);
void music_play_level(uint8_t level);
void music_play_title(void);
void music_play_game_over(void);
void music_play_level_complete(void);

#endif // MUSIC_H

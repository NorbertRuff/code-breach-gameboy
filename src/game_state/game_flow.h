#ifndef GAME_FLOW_H
#define GAME_FLOW_H

#include <gb/gb.h>
#include <stdint.h>
#include "../player/player.h"

// Starting a level, restarting a section and losing a life (game_flow.c, a ROM bank).

#define START_LIVES 3
#define RESTART_FADE_FRAMES 4   // per fade step: about half a second out and in

void start_level(void) BANKED;
void lose_life(void) BANKED;
void go_to_next_level(void) BANKED;

// From main.c.
extern Player player;
extern uint8_t lives;
void draw_sprites(uint8_t with_pause_label);
void redraw_hud_count(void);

#endif // GAME_FLOW_H

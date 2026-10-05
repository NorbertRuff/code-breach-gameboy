#ifndef TITLE_MENU_H
#define TITLE_MENU_H

#include <gb/gb.h>
#include <stdint.h>

// The title screen's menu (title_menu.c, a ROM bank): a new game, or a password and then
// a level to play.
uint8_t title_menu(void) BANKED;

#endif // TITLE_MENU_H

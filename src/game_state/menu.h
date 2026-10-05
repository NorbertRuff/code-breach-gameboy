#ifndef MENU_H
#define MENU_H

#include <gb/gb.h>
#include <stdint.h>

// What the menu screens share (menu.c, a ROM bank): a blank screen, button presses,
// and picking a line with a cursor.

void menu_open_screen(void) BANKED;
void menu_start_input(void) BANKED;
uint8_t menu_wait_press(void) BANKED;
uint8_t menu_choose(uint8_t x, uint8_t top, uint8_t count, uint8_t choice) BANKED;
void menu_print_password(uint8_t x, uint8_t y) BANKED;

#endif // MENU_H

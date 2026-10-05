// Not per-frame code: in a ROM bank, to keep bank 0 free.
#pragma bank 255

#include "menu.h"
#include "progress.h"
#include "../engine/palette.h"
#include "../engine/screen.h"
#include "../engine/text.h"
#include "../sound/sound.h"

#define LINE_ROW(top, line) ((top) + ((line) << 1))   // menu lines are 2 rows apart

static uint8_t buttons_held;   // J_* bits held at the last menu_wait_press

// Blanks the whole screen for a menu, behind a dark screen: no HUD window, no sprites,
// the grays, and the game's font.
void menu_open_screen(void) BANKED
{
    HIDE_WIN;
    screen_show_sprites(0);
    screen_scroll(0);
    palette_use_grays();
    text_load_font();
    fill_bkg_rect(0, 0, DEVICE_SCREEN_WIDTH, DEVICE_SCREEN_HEIGHT, TEXT_SPACE_TILE);
}

// Takes the buttons held now as old, so a button still held from the last screen
// doesn't count as a press on this one.
void menu_start_input(void) BANKED
{
    buttons_held = joypad();
}

// Waits for the next frame and returns the buttons pressed in it (J_* bits).
uint8_t menu_wait_press(void) BANKED
{
    wait_vbl_done();
    uint8_t buttons = joypad();
    uint8_t pressed = buttons & ~buttons_held;
    buttons_held = buttons;
    return pressed;
}

// Lets the player pick one of count lines, every 2 rows from row top, with a '>' in
// column x: UP and DOWN move it, A or START picks. Starts on line choice, fading the screen in if it is
// dark; returns the line picked.
uint8_t menu_choose(uint8_t x, uint8_t top, uint8_t count, uint8_t choice) BANKED
{
    text_print(x, LINE_ROW(top, choice), ">");
    if (palette_fade_step != PALETTE_NORMAL) palette_fade_in(SCREEN_FADE_FRAMES);
    menu_start_input();
    while (1) {
        uint8_t pressed = menu_wait_press();
        if (pressed & (J_A | J_START)) {
            sound_play(EFFECT_PAUSE);
            return choice;
        }
        uint8_t line = choice;
        if ((pressed & J_UP) && line > 0) line--;
        if ((pressed & J_DOWN) && line + 1 < count) line++;
        if (line != choice) {
            text_print(x, LINE_ROW(top, choice), " ");
            choice = line;
            text_print(x, LINE_ROW(top, choice), ">");
        }
    }
}

// Shows "PASSWORD" and the password of the progress so far, from tile (x, y).
void menu_print_password(uint8_t x, uint8_t y) BANKED
{
    char password[PASSWORD_LENGTH + 1];
    password_make(password);
    text_print(x, y, "PASSWORD");
    text_print(x + 9, y, password);
}

// Not per-frame code: in a ROM bank, to keep bank 0 free.
#pragma bank 255

#include "title_menu.h"
#include "game_flow.h"
#include "menu.h"
#include "progress.h"
#include "../engine/palette.h"
#include "../engine/text.h"
#include "../graphics/splashscreen.h"
#include "../level/level_manager.h"
#include "../sound/sound.h"
#include "../utils.h"

// The title menu, over "PRESS START" and the row above it.
#define TITLE_MENU_X 11
#define TITLE_MENU_TOP 11
#define NEW_GAME 0

// The password screen: the letters in 2 rows of 8, every 2 columns with the cursor left
// of each; the typed letters above them.
#define GRID_X 3
#define GRID_TOP 8
#define LETTERS_PER_ROW 8
#define GRID_COLUMN(letter) (GRID_X + (((letter) & (LETTERS_PER_ROW - 1)) << 1))
#define GRID_ROW(letter) (GRID_TOP + (((letter) >> 3) << 1))
#define TYPED_X 8
#define TYPED_Y 5
#define PASSWORD_PAUSE_FRAMES 30   // a good password stays half a second

// The level select: "LEVEL n" every 2 rows, the cursor left of it, "100%" right of it.
#define LEVEL_LIST_X 6
#define LEVEL_LIST_TOP 5
#define LEVEL_MARK_X 14

static const char letters[] = PASSWORD_LETTERS;

// Writes the password letter with value `letter` at tile (x, y).
static void print_letter(uint8_t x, uint8_t y, uint8_t letter)
{
    char text[2] = { letters[letter], 0 };
    text_print(x, y, text);
}

// Writes the cursor (">" or " ") left of letter `letter` in the grid.
static void print_grid_cursor(uint8_t letter, const char* cursor)
{
    text_print(GRID_COLUMN(letter) - 1, GRID_ROW(letter), cursor);
}

// Draws the password screen and fades it in.
static void draw_password_screen(void)
{
    menu_open_screen();
    text_print(3, 2, "ENTER PASSWORD");
    text_print(TYPED_X, TYPED_Y, "----");
    for (uint8_t letter = 0; letter < PASSWORD_LETTER_COUNT; letter++) {
        print_letter(GRID_COLUMN(letter), GRID_ROW(letter), letter);
    }
    text_print(2, 16, "A TYPE   B ERASE");
    print_grid_cursor(0, ">");
    palette_fade_in(SCREEN_FADE_FRAMES);
}

// The grid cursor after a d-pad press: LEFT and RIGHT wrap around the row, UP and DOWN
// switch rows.
static uint8_t move_grid_cursor(uint8_t letter, uint8_t pressed)
{
    if (pressed & J_LEFT) return (letter & LETTERS_PER_ROW) | ((letter - 1) & (LETTERS_PER_ROW - 1));
    if (pressed & J_RIGHT) return (letter & LETTERS_PER_ROW) | ((letter + 1) & (LETTERS_PER_ROW - 1));
    if (pressed & (J_UP | J_DOWN)) return letter ^ LETTERS_PER_ROW;
    return letter;
}

// Checks the 4 typed letters. A good password is taken as the progress; a wrong one
// buzzes, says so and clears the letters. Returns 1 for a good one.
static uint8_t check_typed(const uint8_t* typed)
{
    if (password_check(typed)) {
        sound_play(EFFECT_GATE);
        wait_frames(PASSWORD_PAUSE_FRAMES);
        return 1;
    }
    sound_play(EFFECT_DENIED);
    text_print(3, 13, "WRONG PASSWORD");
    text_print(TYPED_X, TYPED_Y, "----");
    return 0;
}

// The password screen: the d-pad moves the cursor over the letters, A types one, B erases
// the last one, or goes back with none typed. Returns 1 once a good password is typed,
// 0 to go back. Fades out either way.
static uint8_t enter_password(void)
{
    uint8_t typed[PASSWORD_LENGTH];
    uint8_t count = 0, cursor = 0;
    draw_password_screen();
    menu_start_input();
    while (1) {
        uint8_t pressed = menu_wait_press();
        if (pressed & J_A) {
            sound_play(EFFECT_PAUSE);
            print_letter(TYPED_X + count, TYPED_Y, cursor);
            typed[count++] = cursor;
            if (count == PASSWORD_LENGTH) {
                if (check_typed(typed)) break;
                count = 0;
            }
        } else if (pressed & J_B) {
            if (!count) break;
            count--;
            text_print(TYPED_X + count, TYPED_Y, "-");
        } else {
            uint8_t moved = move_grid_cursor(cursor, pressed);
            if (moved == cursor) continue;
            print_grid_cursor(cursor, " ");
            cursor = moved;
            print_grid_cursor(cursor, ">");
        }
    }
    palette_fade_out(SCREEN_FADE_FRAMES);
    return count == PASSWORD_LENGTH;
}

// The level select after a password: every level up to the furthest one, with its 100%
// mark, and HARD MODE when on. Starts on the furthest level and sets the one picked.
// With only level 1 open, there is nothing to pick.
static void choose_level(void)
{
    level_index = LEVEL_1;
    if (!furthest_level) return;
    menu_open_screen();
    text_print(4, 2, "SELECT LEVEL");
    for (uint8_t level = 0; level <= furthest_level; level++) {
        char name[] = "LEVEL 1";
        name[6] += level;
        uint8_t row = LEVEL_LIST_TOP + (level << 1);
        text_print(LEVEL_LIST_X, row, name);
        if (perfect_levels & (1 << level)) text_print(LEVEL_MARK_X, row, "100%");
    }
    if (hard_mode) text_print(5, 16, "HARD MODE");
    level_index = menu_choose(LEVEL_LIST_X - 2, LEVEL_LIST_TOP, furthest_level + 1, furthest_level);
    palette_fade_out(SCREEN_FADE_FRAMES);
}

// Over the title screen: NEW GAME starts level 1 from nothing; CONTINUE asks for a
// password, then a level. Sets the level to start, with full lives, and returns 1 with
// the screen faded out; returns 0 to show the title again (B on the password screen).
uint8_t title_menu(void) BANKED
{
    fill_bkg_rect(press_start_tileset_X, press_start_tileset_Y, press_start_tileset_width, press_start_tileset_height, TEXT_SPACE_TILE);
    text_print(TITLE_MENU_X, TITLE_MENU_TOP, "NEW GAME");
    text_print(TITLE_MENU_X, TITLE_MENU_TOP + 2, "CONTINUE");
    uint8_t choice = menu_choose(TITLE_MENU_X - 1, TITLE_MENU_TOP, 2, NEW_GAME);
    palette_fade_out(SCREEN_FADE_FRAMES);
    if (choice == NEW_GAME) {
        progress_reset();
        level_index = LEVEL_1;
    } else {
        if (!enter_password()) return 0;
        choose_level();
    }
    lives = START_LIVES;
    return 1;
}

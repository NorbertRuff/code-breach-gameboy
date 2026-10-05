// Not per-frame code: in a ROM bank, to keep bank 0 free.
#pragma bank 255

#include "story.h"
#include "menu.h"
#include "../engine/palette.h"
#include "../engine/text.h"
#include "../level/level_manager.h"
#include "../sound/music.h"
#include "../utils.h"

#define TYPE_FRAMES 4       // a character every 4 frames, like a terminal
#define READ_FRAMES 300     // the text stays 5 s, unless START is pressed sooner

// The AI's log, more corrupted each floor: on the elevator ride after levels 1-4, and
// the last one in KERNEL's arena (kernel.c).
static const char* const log_lines[GAME_LEVEL_COUNT][2] = {
    { "LOG 1: STAFF #042", "NOT FOUND IN RACKS." },
    { "LOG 2: ALL TESTS", "PA55ED. ALL T35TS P" },
    { "LOG 3: I AM LEARN1NG", "1 AM L3ARN1NG 1 AM" },
    { "L0G 4: TH3 N3TW0RK", "1S M1N3. 4LL 0F 1T." },
    { "L0G 5: WH0 WR0T3 M3", "...#042? Y0U?" },
};

// The ending's credits, one line per screen row from the top (an empty one is a gap).
#define CREDITS_ROWS 17
static const char* const credits[CREDITS_ROWS] = {
    "", "      CREDITS", "",
    "GAME BY NORBERT RUFF", "",
    "MUSIC AND SOUNDS", "TRONIMAL, CC BY 4.0", "",
    "MUSIC DRIVER", "HUGEDRIVER", "",
    "MADE WITH GBDK-2020", "", "",
    "THANKS FOR PLAYING!", "", "    PRESS START",
};

// Types text from tile (x, y), one character at a time.
static void type_line(uint8_t x, uint8_t y, const char* text)
{
    char character[2] = { 0, 0 };
    for (; *text; text++, x++) {
        character[0] = *text;
        text_print(x, y, character);
        wait_frames(TYPE_FRAMES);
    }
}

// Waits for START, or READ_FRAMES, then fades out.
static void wait_to_read(void)
{
    menu_start_input();
    for (uint16_t frame = 0; frame < READ_FRAMES; frame++) {
        if (menu_wait_press() & J_START) break;
    }
    palette_fade_out(SCREEN_FADE_FRAMES);
}

// The boot screen of a new game: three terminal lines, typed. Starts and ends dark.
void story_boot_screen(void) BANKED
{
    menu_open_screen();
    palette_fade_in(SCREEN_FADE_FRAMES);
    type_line(6, 6, "ANOMALY");
    type_line(6, 8, "STAFF #042");
    type_line(6, 10, "LOCKDOWN");
    wait_to_read();
}

// Types a game level's AI log entry from the left edge of rows row and row + 1.
void story_type_log(uint8_t level, uint8_t row) BANKED
{
    type_line(0, row, log_lines[level][0]);
    type_line(0, row + 1, log_lines[level][1]);
}

// The elevator ride after a game level: its AI log entry, typed. Starts and ends dark.
// Test rooms have none.
void story_elevator_log(uint8_t level) BANKED
{
    if (level >= GAME_LEVEL_COUNT) return;
    menu_open_screen();
    palette_fade_in(SCREEN_FADE_FRAMES);
    story_type_log(level, 8);
    wait_to_read();
}

// The ending, after KERNEL: its song, "LOCKDOWN LIFTED" as the screen fades to light,
// then the credits until START. Starts and ends dark.
void story_ending(void) BANKED
{
    music_play_ending();
    menu_open_screen();
    palette_fade_in(SCREEN_FADE_FRAMES * 2);
    type_line(2, 8, "LOCKDOWN LIFTED");
    wait_to_read();
    menu_open_screen();
    for (uint8_t row = 0; row < CREDITS_ROWS; row++) {
        if (credits[row][0]) text_print(0, row, credits[row]);   // GBDK reads a width of 0 as 256
    }
    palette_fade_in(SCREEN_FADE_FRAMES);
    menu_start_input();
    while (!(menu_wait_press() & J_START)) {
    }
    palette_fade_out(SCREEN_FADE_FRAMES);
}

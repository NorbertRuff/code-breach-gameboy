"""Passwords and the game flow around them (roadmap M7, DECISIONS.md "Saves"): the format
in tools/password.py, the same as src/game_state/progress.c; the title menu, the password
screen and the level select (title_menu.c); QUIT on the game over menu.
The password on the level complete screen is checked in test_level_2.py."""

import sys
from pathlib import Path

from gamelib import COREGAMELOOP, LEVEL_1, LEVEL_2, TITLE_SCREEN_STATE

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import password  # noqa: E402

GAME_LEVELS = 3     # src/level/level_manager.h: GAME_LEVEL_COUNT
LCDC_WINDOW_ON = 0x20


def open_title_menu(game) -> None:
    """From play: the title screen, then START for its menu."""
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.set_state(TITLE_SCREEN_STATE)
    game.frames(150)
    game.tap("start")
    game.wait_for_text(11, 11, "NEW GAME")


def type_password(game, text: str, cursor: int = 0) -> int:
    """Type a password on the password screen, from the cursor on letter `cursor` (0 = B,
    where the screen starts it): DOWN switches rows of 8 letters, RIGHT steps along a row
    and wraps. Returns where the cursor ends."""
    for letter in text:
        target = password.LETTERS.index(letter)
        if (target ^ cursor) & 8:
            game.tap("down")
            cursor ^= 8
        while cursor != target:
            game.tap("right")
            cursor = (cursor & 8) | ((cursor + 1) & 7)
        game.tap("a")
    return cursor


def test_password_holds_the_progress_and_refuses_typos():
    # Every progress a password can hold comes back from it, and a single wrong letter
    # never gives another valid password. A build refuses levels it doesn't have.
    for level in range(password.LEVELS):
        for perfect in range(1 << min(level + 1, password.MARKED_LEVELS)):
            for hard in (False, True):
                text = password.make(level, perfect, hard)
                assert password.read(text) == (level, perfect, hard)
                assert (password.read(text, GAME_LEVELS) is None) == (level >= GAME_LEVELS)
                for i in range(4):
                    for letter in password.LETTERS.replace(text[i], ""):
                        assert password.read(text[:i] + letter + text[i + 1:]) is None


def test_continue_takes_a_password_then_a_level(game):
    # B with nothing typed goes back to the title. A mistyped password is refused; a good
    # one (level 3 open, 100% on levels 1 and 2, hard mode) opens the level select, which
    # starts on level 3. Level 2 picked: it starts with full lives, each gate asking for
    # all of its section's fragments.
    open_title_menu(game)
    game.tap("down start")
    game.wait_for_text(3, 2, "ENTER PASSWORD")
    game.tap("b")
    game.wait_for_state(TITLE_SCREEN_STATE)
    game.frames(150)
    game.tap("start")
    game.wait_for_text(11, 11, "NEW GAME")
    game.tap("down start")
    game.wait_for_text(3, 2, "ENTER PASSWORD")
    good = password.make(2, 0b11, hard=True)
    cursor = type_password(game, good[:3] + password.LETTERS[password.LETTERS.index(good[3]) ^ 1])
    game.frames(5)
    assert game.text(3, 13, 14) == "WRONG PASSWORD" and game.text(8, 5, 4) == "----"
    type_password(game, good, cursor)
    game.wait_for_text(4, 2, "SELECT LEVEL")
    assert [game.text(4, row, 14) for row in (5, 7, 9)] == ["  LEVEL 1 100%", "  LEVEL 2 100%", "> LEVEL 3     "]
    assert game.text(5, 16, 9) == "HARD MODE"
    game.tap("up start")
    game.wait_for_state(COREGAMELOOP, timeout=300)
    level = game.level()
    assert game.u8("_level_index") == LEVEL_2 and game.lives() == 3
    assert [gate[2] for gate in level.gates[:level.gate_count]] == level.section_fragments[:level.gate_count]


def test_quit_after_game_over_goes_to_the_title_and_a_new_game(game):
    # QUIT: the title screen, without the HUD window. NEW GAME starts level 1 with full
    # lives and no progress.
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.load_level(LEVEL_2)
    game.set_lives(1)
    game.lose_life()
    game.answer_game_over(quit=True)
    game.wait_for_state(TITLE_SCREEN_STATE, timeout=300)
    game.frames(100)
    assert not game.reg("LCDC") & LCDC_WINDOW_ON
    game.tap("start")
    game.wait_for_text(11, 11, "NEW GAME")
    game.tap("start")
    game.wait_for_state(COREGAMELOOP, timeout=300)
    assert game.u8("_level_index") == LEVEL_1 and game.lives() == 3
    assert game.u8("_furthest_level") == 0

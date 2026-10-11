"""Screen text in the game's font (src/engine/text.c): the intro credits are written with
it (the test build skips the intro, so the test sends the game back to it from play).
The level complete summary is checked in test_level_2.py."""

from gamelib import COREGAMELOOP, FONT_CHARS, FONT_FIRST_TILE, INTRO_CREDITS_STATE


def font_tiles(text: str) -> list:
    return [FONT_FIRST_TILE + FONT_CHARS.index(c) for c in text.upper()]


def test_intro_credits_are_printed_in_the_game_font(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.set_state(INTRO_CREDITS_STATE)
    game.frames(40)                                         # drawn; the fade-in runs
    lines = {2: (4, "Developed By"), 6: (4, "Norbert Ruff"), 10: (4, "Version 1.0"), 15: (8, "2026")}
    for row, (column, text) in lines.items():
        assert [game.bkg_tile(column + i, row) for i in range(len(text))] == font_tiles(text), f"row {row}"
    assert game.bkg_tile(0, 0) == FONT_FIRST_TILE           # the rest of the screen is blank

"""The demo (src/game_state/demo.c): a minute on the title screen without a press plays a
clip of the next level from recorded buttons ("make demo"); the clip's end, or any button,
goes back to the title."""

import pytest

from gamelib import COREGAMELOOP, LEVEL_1, LEVEL_5, TITLE_SCREEN_STATE

DEMO_IDLE_FRAMES = 3600   # demo.h
FADE_FRAMES = 200         # generous: a fade out, or the title's fade out and the level's load


def go_to_title(game) -> None:
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.set_state(TITLE_SCREEN_STATE)


def wait_for_demo(game, level: int) -> None:
    """From the title screen, wait without a press and see the demo of level start."""
    game.frames(DEMO_IDLE_FRAMES)
    game.wait_for_state(COREGAMELOOP, timeout=600)     # level 5 first shows KERNEL's log
    assert game.u8("_demo_playing") == 1
    assert game.u8("_level_index") == level


def watch_demo(game) -> None:
    """Let the clip play out: it must keep every life, get past a gate (KERNEL: two
    uploads, which count as the arena's fragments), then go back to the title."""
    level = game.u8("_level_index")
    lives = game.lives()
    while game.u8("_demo_playing"):
        game.frames(1)
        assert game.lives() == lives, f"the demo of level {level + 1} lost a life"
    progress = game.level()
    if level == LEVEL_5:
        assert sum(progress.section_collected) >= 2, "the demo of KERNEL lost its way"
    else:
        assert progress.section >= 1, f"the demo of level {level + 1} didn't open a gate"
    game.wait_for_state(TITLE_SCREEN_STATE, timeout=FADE_FRAMES)


def test_the_title_plays_the_level_1_demo_after_a_minute_and_a_press_ends_it(game):
    go_to_title(game)
    wait_for_demo(game, LEVEL_1)
    game.frames(120)
    game.tap("a")
    game.wait_for_state(TITLE_SCREEN_STATE, timeout=FADE_FRAMES)
    assert game.u8("_demo_playing") == 0


@pytest.mark.full
def test_every_level_s_demo_plays_out_in_turn(game):
    go_to_title(game)
    for level in range(LEVEL_1, LEVEL_5 + 1):
        wait_for_demo(game, level)
        watch_demo(game)

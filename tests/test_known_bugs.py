"""Known bugs from docs/PROJECT_STATE.md, written as tests that fail today.

Each test is marked xfail with strict=True: while the bug exists the run stays green,
and once the bug is fixed the test passes, which fails the run (see pytest.ini).
The fix then removes the marker, and the test stays as a regression test.
"""

import pytest

from gamelib import COREGAMELOOP


def lose_life(game, timeout=400):
    """Wait until invincibility is over, then walk right into the first patrol bot."""
    lives = game.lives()
    for _ in range(200):
        if game.player().invincibility_frames == 0:
            break
        game.frames(1)
    game.pyboy.button_press("right")
    try:
        for _ in range(timeout):
            game.frames(1)
            if game.lives() != lives:
                return
    finally:
        game.pyboy.button_release("right")
    raise AssertionError("walking right never hit an enemy")


@pytest.mark.xfail(reason="bug 1: scroll_win() is relative, so every second level load moves the HUD window to the top")
def test_hud_window_stays_at_bottom_after_game_over(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    for _ in range(2):
        lose_life(game)
        game.wait_for_state(COREGAMELOOP)
    lose_life(game)                       # third hit: game over, then the level reloads
    for _ in range(1200):                 # game over flash, fade, reload
        game.frames(1)
        if game.lives() == 3 and game.state() == COREGAMELOOP:
            break
    else:
        raise AssertionError("the game never restarted after game over")
    game.frames(5)
    game.screenshot("after_restart")
    assert game.reg("WY") == 128


@pytest.mark.xfail(reason="bug 13: slowing_x and last_movement_x are uint8_t, so -1 becomes 255 and the slide goes right")
def test_no_slide_to_the_right_after_releasing_left(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.frames(10)
    game.hold("right", 12)                # make room on the left (the first bot is further right)
    game.frames(10)
    game.hold("left", 8)
    xs = [game.player().x]
    for _ in range(6):
        game.frames(1)
        xs.append(game.player().x)
    assert all(b <= a for a, b in zip(xs, xs[1:])), f"x moved right after releasing left: {xs}"

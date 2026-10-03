"""Known bugs from docs/PROJECT_STATE.md, written as tests that fail today.

Each test is marked xfail with strict=True: while the bug exists the run stays green,
and once the bug is fixed the test passes, which fails the run (see pytest.ini).
The fix then moves the test to test_regressions.py without the marker.

Both bugs below are planned for the physics and collision rewrite (roadmap M2).
"""

import pytest

from gamelib import (COLLISION_SOLID, COREGAMELOOP, SPRITE_HEAD_OFFSET, SPRITE_LEFT_W,
                     SPRITE_RIGHT_W, TILE_SIZE)


def solid_tiles_overlapped(game) -> list:
    """Solid level tiles that the player's 16x32 box overlaps right now."""
    p = game.player()
    left, right = max(p.x - SPRITE_LEFT_W, 0), p.x + SPRITE_RIGHT_W
    top, bottom = max(p.y - SPRITE_HEAD_OFFSET + 1, 0), p.y   # y is the feet pixel row
    hits = []
    for ty in range(top // TILE_SIZE, bottom // TILE_SIZE + 1):
        for tx in range(left // TILE_SIZE, right // TILE_SIZE + 1):
            if ty < 18 and tx < 32 and game.collision(tx, ty) == COLLISION_SOLID:
                hits.append((tx, ty))
    return hits


@pytest.mark.xfail(reason="bug 5/6: falling speed comes from a frame-count table that wraps at 60 and starts with upward speed")
def test_falling_never_moves_up(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.frames(5)
    game.set_player_y(40)                 # in the air, above open floor
    ys = [game.player().y]
    for _ in range(90):
        game.frames(1)
        ys.append(game.player().y)
    rises = [(i, a, b) for i, (a, b) in enumerate(zip(ys, ys[1:])) if b < a]
    assert not rises, f"the player moved up while falling: {rises[:5]}"


@pytest.mark.xfail(reason="bug 7: horizontal collision checks one point near the feet, so the body walks into platform A")
def test_player_never_overlaps_solid_tiles(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.frames(5)
    game.pyboy.button_press("right")
    try:
        for _ in range(40):
            game.frames(1)
            hits = solid_tiles_overlapped(game)
            assert not hits, f"player at x={game.player().x} y={game.player().y} overlaps {hits}"
    finally:
        game.pyboy.button_release("right")

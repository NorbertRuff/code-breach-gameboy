"""Map streaming, the collision ring and the camera (roadmap M2), in levels/greybox_test.tmj.

The visible columns of the background map and of the collision ring must always match
the Tiled map, while the camera scrolls past the 256 px of one background map and back.
"""

import sys
from pathlib import Path

from gamelib import COREGAMELOOP, GAMEPLAY_END

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import tiled2c  # noqa: E402

LEVEL = tiled2c.load_level(ROOT / "levels" / "greybox_test.tmj")
COLLISION = {"": 0, "solid": 1, "one_way": 2, "hazard": 3}
COLLECTIBLE = 4
FRAGMENT_TILE = next(i for i, c in LEVEL.tileset.classes.items() if c == "fragment")
FRAGMENTS = {(e.x // 8, e.y // 8) for e in LEVEL.entities if e.kind == "fragment"}
MAP_TOP_ROW = 3

CAMERA_CENTER, CAMERA_DEADZONE, LOOK_AHEAD = 80, 8, 24    # src/engine/camera.h


def screen_mismatches(game) -> list:
    """Visible cells whose tile or collision differs from the map. A fragment cell may
    show the fragment (not collected yet) or the map tile under it (collected)."""
    wrong = []
    first = game.camera_x() // 8
    for col in range(first, min(first + 21, LEVEL.width)):
        for row in range(MAP_TOP_ROW, 16):
            tile = LEVEL.tiles[(row - MAP_TOP_ROW) * LEVEL.width + col]
            expected = {(tile, COLLISION[LEVEL.tileset.collision[tile]])}
            if (col, row) in FRAGMENTS:
                expected.add((FRAGMENT_TILE, COLLECTIBLE))
            seen = (game.bkg_tile(col, row), game.collision(col, row))
            if seen not in expected:
                wrong.append((col, row, seen))
    return wrong


def run_right_and_back(game, each_frame) -> None:
    """From x = 156 (start_on_open_ground): run right, jump the pit at x 280-311, stop
    under the one-way shelf, then run back left. Calls each_frame(game) after every frame."""
    game.pyboy.button_press("right")
    while game.player().px < 345:
        if 262 <= game.player().px < 270:
            game.pyboy.button_press("a")
        if game.player().px > 300:
            game.pyboy.button_release("a")
        game.frames(1)
        each_frame(game)
    game.pyboy.button_release("right")
    game.pyboy.button_release("a")
    game.pyboy.button_press("left")
    for _ in range(18):
        game.frames(1)
        each_frame(game)
    game.pyboy.button_release("left")
    for _ in range(10):
        game.frames(1)
        each_frame(game)


def test_screen_and_collision_match_the_map_while_scrolling(game):
    cameras = []

    def check(g):
        cameras.append(g.camera_x())
        wrong = screen_mismatches(g)
        assert not wrong, f"camera {g.camera_x()}: cells differ from the map: {wrong[:5]}"

    game.start_on_open_ground()
    run_right_and_back(game, check)
    assert max(cameras) > 256 - 160, "the camera never scrolled past the first background map"
    assert cameras[-1] < max(cameras), "the camera never scrolled back left"
    assert game.state() == COREGAMELOOP


def test_no_lag_frames_while_streaming(game):
    # The game loop must run once per frame, also on frames that draw a column.
    game.start_on_open_ground()
    calls = []
    game.pyboy.hook_register(None, "_main_game_loop", lambda c: c.append(1), calls)
    per_frame = []

    def count(g):
        per_frame.append(len(calls))
        calls.clear()

    run_right_and_back(game, count)
    lag_frames = [i for i, n in enumerate(per_frame) if n != 1]
    assert not lag_frames, f"frames without exactly one game loop: {lag_frames}"


def test_collected_fragment_stays_gone(game):
    # x = 156 stands on the fragment at column 19, row 13. Go to the far end (its column's
    # place in the background map is drawn over), come back, and look again.
    game.start_on_open_ground()
    assert game.collision(19, 13) != COLLECTIBLE
    game.place_player(490, 103)                 # on the step at the far end
    game.frames(10)
    assert game.camera_x() == LEVEL.width * 8 - 160
    game.place_player(156, 119)
    game.frames(10)
    assert game.bkg_tile(19, 13) != FRAGMENT_TILE
    assert game.collision(19, 13) != COLLECTIBLE
    assert not screen_mismatches(game)


def test_respawn_redraws_the_start(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.lose_life()
    game.wait_for_state(COREGAMELOOP)
    game.frames(2)
    assert game.camera_x() == 0
    assert not screen_mismatches(game)


def test_camera_leads_where_the_player_runs(game):
    # Running right, the focus sits LOOK_AHEAD px ahead at the deadzone's right edge, so
    # the player is drawn left of center. Running left, the lead flips.
    game.start_on_open_ground()
    game.hold("right", 70)
    player = game.player()
    assert player.px - game.camera_x() == CAMERA_CENTER + CAMERA_DEADZONE - LOOK_AHEAD
    game.hold("left", 90)
    assert game.player().px - game.camera_x() == CAMERA_CENTER - CAMERA_DEADZONE + LOOK_AHEAD


def test_camera_moves_at_most_1_px_faster_than_the_player(game):
    game.start_on_open_ground()
    last = (game.player().px, game.camera_x())
    for button, frames in (("right", 70), ("left", 90)):
        game.pyboy.button_press(button)
        for _ in range(frames):
            game.frames(1)
            now = (game.player().px, game.camera_x())
            assert abs(now[1] - last[1]) <= abs(now[0] - last[0]) + 1, f"camera jumped: {last} -> {now}"
            last = now
        game.pyboy.button_release(button)

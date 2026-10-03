"""tools/physics.py and tools/levelpreview.py: the physics copy matches the ROM frame by
frame, the reachability check agrees with the measured jump metrics, and it reports what
can't be reached."""

import copy
import sys
from pathlib import Path

import pytest

from test_jump_metrics import FLOOR_Y, MAX_CLIMB_HELD, MAX_GAP_RUNNING, PITS, WALLS

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import levelpreview  # noqa: E402
import physics  # noqa: E402
import tiled2c  # noqa: E402

GREYBOX = tiled2c.load_level(ROOT / "levels" / "greybox_test.tmj")
TEST_JUMPS = tiled2c.load_level(ROOT / "levels" / "test_jumps.tmj")
FIELDS = ("x", "y", "velocity_x", "velocity_y", "jump_buffer_frames", "coyote_frames", "on_ground")

# Start position, then (frames, d-pad, A down) steps. Places in greybox_test.tmj.
SCRIPTS = {
    "run, jump and land": (156, 119, [(10, 1, False), (25, 1, True), (30, 1, False), (20, 0, False)]),
    "slide past platform D's corner": (164, 119, [(40, 0, True), (30, 0, False)]),
    "up through the one-way shelf": (336, 119, [(60, 0, True), (20, -1, False), (10, 1, True)]),
}


@pytest.mark.parametrize("name", SCRIPTS)
def test_physics_copy_matches_the_rom(game, name):
    # player() shows the frame that finished during the last step, so the ROM's state
    # after step k + 1 is the copy's state after step k.
    x, y, script = SCRIPTS[name]
    game.start_on_open_ground()
    game.place_player(x, y)
    start = game.player()
    body = physics.Body(start.x, start.y, start.velocity_x, start.velocity_y,
                        jump_buffer_frames=start.jump_buffer_frames,
                        coyote_frames=start.coyote_frames, on_ground=bool(start.on_ground))
    room = levelpreview.make_room(GREYBOX)
    rom, copied, a_was_down = [], [], False
    for frames, dpad, a_down in script:
        for button, down in (("left", dpad < 0), ("right", dpad > 0), ("a", a_down)):
            (game.pyboy.button_press if down else game.pyboy.button_release)(button)
        for _ in range(frames):
            game.frames(1)
            rom.append(game.player())
            physics.step(body, room, dpad, a_down, a_was_down)
            a_was_down = a_down
            copied.append({f: int(getattr(body, f)) for f in FIELDS})
    for frame, (state, expected) in enumerate(zip(copied, rom[1:])):
        seen = {f: int(getattr(expected, f)) for f in FIELDS}
        assert state == seen, f"frame {frame}: copy {state} != ROM {seen}"


def reaches(level, start_x: int, target_x: int, target_feet: int) -> bool:
    result = levelpreview.explore(level, start_at=(start_x, FLOOR_Y))
    return any(s.feet == target_feet and s.x0 <= target_x <= s.x1 for s in result.paths)


def test_check_agrees_with_the_jump_metrics():
    # The same limits the ROM shows in test_jump_metrics.py, found by the search.
    for height, expected in ((MAX_CLIMB_HELD, True), (MAX_CLIMB_HELD + 1, False)):
        wall = WALLS[height] * 8
        assert reaches(TEST_JUMPS, wall - 16, wall + 32, (15 - height) * 8 - 1) == expected, f"wall {height}"
    for gap, expected in ((MAX_GAP_RUNNING, True), (MAX_GAP_RUNNING + 1, False)):
        ledge_end = PITS[gap] * 8 + 7
        assert reaches(TEST_JUMPS, ledge_end - 24, ledge_end + gap * 8 + 24, FLOOR_Y) == expected, f"pit {gap}"


def test_greybox_level_can_be_finished():
    result = levelpreview.explore(GREYBOX)
    assert not result.problems
    assert len(result.paths) == len(result.surfaces)


def test_check_reports_what_cant_be_reached():
    # Widen the pit after gate A to 14 tiles and remove the one-way shelf above it (a
    # running jump reaches the shelf, and the far side from there): nothing past it can
    # be reached.
    level = copy.deepcopy(GREYBOX)
    for col in range(35, 49):
        level.tiles[12 * level.width + col] = 0
    for col in range(40, 44):
        level.tiles[8 * level.width + col] = 0
    problems = "\n".join(levelpreview.explore(level).problems)
    assert "exit at column 62" in problems
    assert "fragment at column 41" in problems
    assert "gate at column 50 asks for 1 fragments, only 0" in problems


def test_preview_image(tmp_path):
    image = levelpreview.draw(GREYBOX, levelpreview.explore(GREYBOX))
    assert image.size == (GREYBOX.width * 8 * 2, 16 * 8 * 2 + 16)

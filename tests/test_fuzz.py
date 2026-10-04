"""Random play: random buttons for a while (pause and the level-select cheat included),
checking after every gameplay frame that nothing breaks. Fixed seeds, so a failure
replays the same way: run the seed again with the printed frame. A scan of a few hundred
seeds found bugs 21 and 22 (docs/PROJECT_STATE.md)."""

import random
import sys
from pathlib import Path

import pytest

from gamelib import (COREGAMELOOP, GAMEPLAY_END, GAMEPLAY_START, HUD_DIGIT_TILE_BASE, LEVEL_1,
                     LEVEL_COMPLETE, LEVEL_GREYBOX_TEST, LEVEL_TEST_JUMPS, MAX_ACTORS, PAUSED,
                     read_symbol)

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import tiled2c  # noqa: E402

LEVEL_MAPS = {LEVEL_1: "level_1", LEVEL_GREYBOX_TEST: "greybox_test", LEVEL_TEST_JUMPS: "test_jumps"}
COLLISION = {"": 0, "solid": 1, "one_way": 2, "hazard": 3}
MARKER_COLLISION = {"fragment": 4, "gate": 1, "exit": 5, "player_start": 0}   # level_stream.c

FRAMES = 1500
STATES = {GAMEPLAY_START, COREGAMELOOP, GAMEPLAY_END, LEVEL_COMPLETE, PAUSED}
# Button sets and how often each is picked: mostly running and jumping.
CHOICES = [("right", 6), ("right a", 5), ("a", 2), ("left", 3), ("left a", 2), ("", 2),
           ("start", 1), ("select", 1)]


def random_inputs(seed: int):
    """(buttons, frames) steps from a seed."""
    rng = random.Random(seed)
    buttons = [b for b, weight in CHOICES for _ in range(weight)]
    while True:
        yield rng.choice(buttons), rng.randint(1, 40)


def play(game, seed: int, frames: int, check) -> None:
    """Random buttons from a seed; check(frame) after every frame."""
    inputs = random_inputs(seed)
    held, frames_left = set(), 0
    for frame in range(frames):
        if frames_left == 0:
            wanted, frames_left = next(inputs)
            for button in held - set(wanted.split()):
                game.pyboy.button_release(button)
            for button in set(wanted.split()) - held:
                game.pyboy.button_press(button)
            held = set(wanted.split())
        frames_left -= 1
        game.frames(1)
        check(frame)


def problems(game) -> list:
    """What is wrong in a frame of play (an empty list: nothing)."""
    level, camera = game.level(), game.camera_x()
    left_limit = int.from_bytes(read_symbol(game.pyboy, "_camera_left_limit", 2), "little", signed=True)
    last_camera_x = level.width_px - 160
    found = []
    if game.solid_tiles_overlapped():
        found.append(f"the body is inside {game.solid_tiles_overlapped()}")
    if game.sprites_on_busiest_line() > 10:
        found.append("more than 10 sprites on a line")
    if not min(left_limit, last_camera_x) <= camera <= last_camera_x:
        found.append(f"camera at {camera}, left limit {left_limit}, level end {last_camera_x}")
    if len(game.actors()) > MAX_ACTORS or not 1 <= game.lives() <= 3:
        found.append(f"{len(game.actors())} actors, {game.lives()} lives")
    if any(got > has for got, has in zip(level.section_collected, level.section_fragments)):
        found.append(f"collected {level.section_collected} of {level.section_fragments}")
    if game.state() == COREGAMELOOP and level.gate_count:
        digits = [game.window_tile(col, 0) - HUD_DIGIT_TILE_BASE for col in (15, 16, 18, 19)]
        section = level.section
        quota = level.gates[section][2] if section < level.gate_count else level.section_fragments[section]
        if (digits[0] * 10 + digits[1], digits[2] * 10 + digits[3]) != (level.section_collected[section], quota):
            found.append(f"the HUD shows {digits}, the section has {level.section_collected[section]}/{quota}")
    return found


@pytest.mark.level(LEVEL_1)
@pytest.mark.parametrize("seed", [1, 2])
def test_random_play_breaks_nothing(game, seed):
    game.wait_for_state(COREGAMELOOP, timeout=300)

    def check(frame):
        where = f"seed {seed}, frame {frame}"
        assert game.state() in STATES, f"{where}: state {game.state()}"
        assert game.u8("_lag_frames") == 0, f"{where}: a lag frame"
        if game.state() in (COREGAMELOOP, PAUSED):
            assert not problems(game), f"{where}: {problems(game)}"

    play(game, seed, FRAMES, check)


def marker_places(level) -> dict:
    """Where each marker may show, as {(column, map row): marker class}: the fragments, each
    gate's column from the top down to its bottom tile, and the exit."""
    places = {}
    for e in level.entities:
        column, row = e.x // 8, e.y // 8 - tiled2c.MAP_TOP_ROW
        if e.kind == "gate":
            places.update({(column, r): "gate" for r in range(row + 1)})
        elif e.kind in ("fragment", "exit"):
            places[(column, row)] = e.kind
    return places


def screen_mismatch(game, levels: dict):
    """The first screen cell whose tile isn't the map's (or a marker where the level has
    that object) or whose collision isn't its tile's, or None."""
    level, places = levels[game.u8("_level_index")]
    marker_class = level.tileset.classes
    first = game.camera_x() >> 3
    for column in range(first, min(first + 21, level.width)):
        for row in range(tiled2c.MAP_ROWS):
            tile = game.bkg_tile(column, row + tiled2c.MAP_TOP_ROW)
            where = f"column {column}, row {row}"
            if tile in marker_class:
                if places.get((column, row)) != marker_class[tile]:
                    return f"{where}: a {marker_class[tile]} marker where the level has none"
                wanted = MARKER_COLLISION[marker_class[tile]]
            elif tile != level.tiles[row * level.width + column]:
                return f"{where}: tile {tile}, the map has {level.tiles[row * level.width + column]}"
            else:
                wanted = COLLISION[level.tileset.collision[tile]]
            if game.collision(column, row + tiled2c.MAP_TOP_ROW) != wanted:
                return f"{where}: collision {game.collision(column, row + tiled2c.MAP_TOP_ROW)}, tile {tile} has {wanted}"
    return None


@pytest.mark.level(LEVEL_1)
def test_random_play_keeps_the_screen_and_collision_right(game):
    # Checked when each update ends, so a column being drawn is never half done.
    maps = {index: tiled2c.load_level(ROOT / "levels" / f"{name}.tmj") for index, name in LEVEL_MAPS.items()}
    levels = {index: (level, marker_places(level)) for index, level in maps.items()}
    game.wait_for_state(COREGAMELOOP, timeout=300)
    found = []

    def at_update_end(_):
        if not found and game.state() == COREGAMELOOP:
            mismatch = screen_mismatch(game, levels)
            if mismatch:
                found.append(mismatch)

    game.at_update_end(at_update_end)
    play(game, 3, FRAMES, lambda frame: None)
    assert not found, found[0]

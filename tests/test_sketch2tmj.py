"""tools/sketch2tmj.py: level text sketches to Tiled maps and back."""

import json
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import sketch2tmj  # noqa: E402
import tiled2c  # noqa: E402

TILESET = tiled2c.load_tileset(sketch2tmj.TILESET)


def convert(tmj: dict) -> tiled2c.Level:
    return tiled2c.convert("sketch", tmj, TILESET, firstgid=1)


def same_level(a: tiled2c.Level, b: tiled2c.Level) -> bool:
    return (a.tiles, a.entities, a.player_start, a.section_start) == \
           (b.tiles, b.entities, b.player_start, b.section_start)


@pytest.mark.parametrize("name", ["greybox_test"])   # sketches are greybox maps
def test_a_map_survives_the_trip_to_a_sketch_and_back(name):
    tmj = json.loads((ROOT / "levels" / f"{name}.tmj").read_text(encoding="utf-8"))
    rows, quotas = sketch2tmj.read_sketch(sketch2tmj.tmj_to_sketch(tmj))
    assert same_level(convert(sketch2tmj.sketch_to_tmj(rows, quotas)), convert(tmj))


SKETCH = """# A small level with every character.
# quota: 1 2
0         1         2
....................
....................
LLL.................
LLL.................
LLL.................
LLL.................
.........===........
....................
....................
....................
.....*.......*......
.P...EEE..GCC*.GFFD*
###^^###############
"""


def test_a_sketch_with_every_character_converts():
    level = convert(sketch2tmj.sketch_to_tmj(*sketch2tmj.read_sketch(SKETCH)))
    kinds = [e.kind for e in level.entities]
    assert kinds.count("fragment") == 4 and kinds.count("enemy") == 2 and kinds.count("exit") == 1
    assert [e.param for e in level.entities if e.kind == "gate"] == [1, 2]
    bot, fast_bot = (e for e in level.entities if e.kind == "enemy")
    assert (bot.x, bot.param, bot.speed) == (5 * 8, 3 * 8, 1)  # the run of E's is the patrol
    assert (fast_bot.x, fast_bot.param, fast_bot.speed) == (16 * 8, 2 * 8, 2)
    assert level.tileset.collision[level.tiles[6 * 20 + 9]] == "one_way"
    assert level.tileset.collision[level.tiles[12 * 20 + 3]] == "hazard"
    chaser = next(e for e in level.entities if e.kind == "chaser")
    assert (chaser.x, chaser.param, chaser.speed) == (11 * 8, 2 * 8, 1)
    lift = next(e for e in level.entities if e.kind == "lift")    # rows 2-5: tops at world y 40-64
    assert (lift.x, lift.y, lift.param) == (0, 64, 24)


def test_a_lift_survives_the_trip_to_a_map_and_back():
    rows, quotas = sketch2tmj.read_sketch(SKETCH)
    back, _ = sketch2tmj.read_sketch(sketch2tmj.tmj_to_sketch(sketch2tmj.sketch_to_tmj(rows, quotas)))
    assert [row[:3] for row in back[2:6]] == ["LLL"] * 4 and back[1][:3] == back[6][:3] == "..."


def test_the_written_text_is_the_map_with_one_line_per_row():
    text = sketch2tmj.tmj_text(sketch2tmj.sketch_to_tmj(*sketch2tmj.read_sketch(SKETCH)))
    tmj = json.loads(text)
    assert tmj["width"] == 20 and len(tmj["layers"][0]["data"]) == 20 * 13
    assert sum(line.startswith("  2, 2, 2, 4") for line in text.splitlines()) == 1   # the floor row


def test_unknown_characters_and_wrong_heights_stop_it():
    with pytest.raises(SystemExit, match="unknown character 'x'"):
        sketch2tmj.sketch_to_tmj(["x" * 20] + ["." * 20] * 12, [])
    with pytest.raises(SystemExit, match="12 map rows"):
        sketch2tmj.read_sketch("\n".join(["." * 20] * 12))

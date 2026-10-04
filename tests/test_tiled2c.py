"""tools/tiled2c.py: the committed level files, the level data in the ROM, and the rules
a level must follow (docs/LEVEL_PIPELINE.md)."""

import struct
import sys
from pathlib import Path

import pytest

from gamelib import read_symbol

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import tiled2c  # noqa: E402

GREYBOX_TSX = ROOT / "graphics" / "tilesets" / "greybox.tsx"
EMPTY, SOLID, GATE, EXIT, FRAGMENT, START = 1, 2, 5, 6, 7, 8   # gids, firstgid 1


def test_committed_level_files_are_up_to_date(tmp_path):
    # Fails when a .tmj or .tsx changed without "make levels".
    for tmj in sorted((ROOT / "levels").glob("*.tmj")):
        level = tiled2c.load_level(tmj)
        tiled2c.write_outputs(level, tmj.relative_to(ROOT), tmp_path)
    for generated in tmp_path.iterdir():
        committed = ROOT / "src" / "levels" / generated.name
        assert committed.read_text(encoding="utf-8") == generated.read_text(encoding="utf-8"), \
            f"{committed.name} is out of date: run make levels"


def test_greybox_test_level():
    level = tiled2c.load_level(ROOT / "levels" / "greybox_test.tmj")
    assert level.width * 8 > 256, "the test level must be wider than one background map"
    assert level.player_start == (8, 119)
    assert level.fragment_count == 15
    assert [e.x for e in level.entities] == sorted(e.x for e in level.entities)
    assert level.section_start == [0, 15, 18]
    assert not level.warnings


def test_level_data_is_in_the_rom(game):
    # LevelData from src/level/level_data.h: width, player_start_x (2 bytes),
    # player_start_y, fragment_count, entity_count.
    raw = read_symbol(game.pyboy, "_greybox_test", 6)
    assert struct.unpack("<BHBBB", raw) == (64, 8, 119, 15, 21)


# --- rules -------------------------------------------------------------------------

def tile_object(gid: int, col: int, row: int, **props) -> dict:
    obj = {"gid": gid, "x": col * 8, "y": (row + 1) * 8, "width": 8, "height": 8, "type": ""}
    if props:
        obj["properties"] = [{"name": k, "value": v} for k, v in props.items()]
    return obj


def small_map() -> dict:
    """A valid 20x13 map: floor, start, two sections of one fragment and a gate, exit."""
    data = [EMPTY] * (20 * 12) + [SOLID] * 20
    objects = [tile_object(START, 0, 11), tile_object(FRAGMENT, 3, 11),
               tile_object(GATE, 5, 11, quota=1), tile_object(FRAGMENT, 8, 11),
               tile_object(GATE, 10, 11, quota=1), tile_object(EXIT, 18, 11),
               {"type": "enemy", "x": 96, "y": 80, "width": 32, "height": 16}]
    for i, obj in enumerate(objects, 1):
        obj["id"] = i
    return {"width": 20, "height": 13,
            "layers": [{"name": "tiles", "data": data},
                       {"name": "entities", "objects": objects}]}


def convert(tmj: dict) -> tiled2c.Level:
    return tiled2c.convert("small", tmj, tiled2c.load_tileset(GREYBOX_TSX), firstgid=1)


def objects(tmj: dict) -> list:
    return tmj["layers"][1]["objects"]


def test_small_map_is_valid():
    assert convert(small_map()).section_start == [0, 2, 4]


def test_bot_speed_is_a_property_with_default_1():
    tmj = small_map()
    bot = lambda: next(e for e in convert(tmj).entities if e.kind == "enemy")
    assert bot().speed == 1
    objects(tmj)[6]["properties"] = [{"name": "speed", "value": 2}]
    assert bot().speed == 2


def flip_first_tile(tmj):
    tmj["layers"][0]["data"][0] |= 0x80000000


def remove_gid(tmj, gid):
    objects(tmj)[:] = [o for o in objects(tmj) if o.get("gid") != gid]


def dig_pit_right_of_first_gate(tmj):
    for column in (6, 7):                               # the gate is at column 5
        tmj["layers"][0]["data"][12 * 20 + column] = EMPTY


RULE_BREAKS = {
    "12 rows tall": (lambda t: t.update(height=12), "must be 13"),
    "19 tiles wide": (lambda t: t.update(width=19), "20 to 255"),
    "flipped tile": (flip_first_tile, "flipped"),
    "marker painted as a tile": (lambda t: t["layers"][0]["data"].__setitem__(0, FRAGMENT), "fragment marker"),
    "unknown class": (lambda t: objects(t).append(
        {"id": 99, "type": "boss_door", "x": 0, "y": 0, "width": 8, "height": 8}), "class must be"),
    "no player_start": (lambda t: remove_gid(t, START), "exactly one player_start"),
    "two exits": (lambda t: objects(t).append(dict(tile_object(EXIT, 15, 11), id=99)), "exactly one exit"),
    "one gate": (lambda t: objects(t).pop(4), "exactly 2 gates"),
    "quota too high": (lambda t: objects(t)[2]["properties"][0].update(value=2), "asks for 2 fragments"),
    "wall right of a gate": (lambda t: t["layers"][0]["data"].__setitem__(11 * 20 + 6, SOLID), "room to restart"),
    "pit right of a gate": (lambda t: dig_pit_right_of_first_gate(t), "room to restart"),
    "off the grid": (lambda t: objects(t)[1].update(x=27), "8 px grid"),
    "patrol too short": (lambda t: objects(t)[6].update(width=8), "patrol width"),
    "bot too fast": (lambda t: objects(t)[6].update(properties=[{"name": "speed", "value": 3}]), "speed must be 1 or 2"),
}


@pytest.mark.parametrize("break_it, message", RULE_BREAKS.values(), ids=RULE_BREAKS.keys())
def test_rule_breaks_stop_the_conversion(break_it, message):
    tmj = small_map()
    break_it(tmj)
    with pytest.raises(tiled2c.LevelError, match=message):
        convert(tmj)


def test_boss_level_needs_no_gates():
    tmj = small_map()
    remove_gid(tmj, GATE)
    tmj["properties"] = [{"name": "boss", "value": True}]
    convert(tmj)


def test_enemy_over_a_pit_is_a_warning():
    tmj = small_map()
    tmj["layers"][0]["data"][12 * 20 + 13] = EMPTY      # a hole under the patrol
    assert any("no ground at column 13" in w for w in convert(tmj).warnings)


def test_narrow_pit_is_a_warning():
    tmj = small_map()
    for column in (15, 16):
        tmj["layers"][0]["data"][12 * 20 + column] = EMPTY
    assert any("pit at columns 15-16 is 2 wide" in w for w in convert(tmj).warnings)
    tmj["layers"][0]["data"][12 * 20 + 17] = EMPTY
    assert not any("pit at" in w for w in convert(tmj).warnings)

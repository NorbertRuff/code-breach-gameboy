"""KERNEL's tiles (src/game_state/kernel.c) are the mainframe tileset's: the ids kernel.c
swaps in name the right tiles of graphics/tilesets/mainframe.tsx, sit where level 5's map
has them, and share the palette of the tiles they replace, because KERNEL writes tiles only
and the GBC attributes stay as the map set them."""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import tiled2c  # noqa: E402

FACE_X, FACE_Y, MAP_TOP_ROW = 6, 3, 3                   # kernel.c, screen tiles
TERMINALS = ((1, 14), (18, 7), (9, 14))
EXPECTED_NAMES = {
    "ARENA_EMPTY_TILE": "empty", "ARENA_FLOOR_TILE": "top", "ARENA_HAZARD_TILE": "floor_corrupt",
    "EYE_LEFT": "kernel_2_2", "EYE_RIGHT": "kernel_4_2", "EYE_WIDE_LEFT": "kernel_eye_wide_left",
    "EYE_WIDE_RIGHT": "kernel_eye_wide_right", "GLITCH": "kernel_glitch", "TERMINAL_OFF": "terminal_off",
    "TERMINAL_LIT": "terminal_lit", "TERMINAL_FILL": "terminal_fill_1",
}


def kernel_tile_ids() -> dict:
    source = (ROOT / "src" / "game_state" / "kernel.c").read_text(encoding="utf-8")
    return {name: int(value) for name, value in re.findall(r"#define (\w+) (\d+)", source) if name in EXPECTED_NAMES}


def test_kernel_swaps_tiles_of_the_same_palette_as_the_map_cells():
    level = tiled2c.load_level(ROOT / "levels" / "level_5.tmj")
    names, palette = level.tileset.names, level.tileset.palette
    ids = kernel_tile_ids()
    assert {k: names[v] for k, v in ids.items()} == EXPECTED_NAMES
    assert [names[ids["TERMINAL_FILL"] + i] for i in (1, 2)] == ["terminal_fill_2", "terminal_fill_3"]

    def map_tile(x: int, y: int) -> int:                # screen tiles
        return level.tiles[(y - MAP_TOP_ROW) * level.width + x]

    assert map_tile(FACE_X + 2, FACE_Y + 2) == ids["EYE_LEFT"] and map_tile(FACE_X + 4, FACE_Y + 2) == ids["EYE_RIGHT"]
    assert {map_tile(x, y) for x, y in TERMINALS} == {ids["TERMINAL_OFF"]}
    assert {map_tile(x, 15) for x in range(level.width)} == {ids["ARENA_FLOOR_TILE"]}

    swaps = [(ids["EYE_LEFT"], ids["EYE_WIDE_LEFT"]), (ids["EYE_RIGHT"], ids["EYE_WIDE_RIGHT"]),
             (ids["ARENA_FLOOR_TILE"], ids["ARENA_HAZARD_TILE"])]
    swaps += [(ids["TERMINAL_OFF"], ids["TERMINAL_LIT"] + i) for i in range(4)]
    face_inside = [map_tile(FACE_X + x, FACE_Y + y) for x in range(1, 6) for y in range(1, 5)]
    swaps += [(tile, ids["GLITCH"]) for tile in face_inside]          # glitches; the last upload fills it
    wrong = [(names[a], names[b]) for a, b in swaps if palette[a] != palette[b]]
    assert not wrong, f"swapped tiles with another palette than the map's: {wrong}"

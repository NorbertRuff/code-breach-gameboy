"""Convert Tiled levels (levels/*.tmj) into banked C data for the game.

    python tools/tiled2c.py levels/greybox_test.tmj [more .tmj files] -o src/levels

For each level it writes <name>.c and <name>.h (a LevelData from src/level/level_data.h),
and for the level's tileset <tileset>_tileset.c and .h (one collision type per tile).
"make levels" runs it on every level. The output is committed, like png2asset output.

The format and the checks are described in docs/LEVEL_PIPELINE.md. A failed check stops
the conversion with a message that names the level and the problem.
"""

from __future__ import annotations

import argparse
import itertools
import json
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from pathlib import Path

TILE = 8
MAP_ROWS = 13              # screen rows 3-15
MAP_TOP_ROW = 3            # screen row of map row 0
MIN_WIDTH, MAX_WIDTH = 20, 255
MAX_FRAGMENTS = 32
TILESET_BUDGET = 190       # unique tiles per level tileset, docs/ART_PIPELINE.md
SECTIONS = 3
MIN_PIT_WIDTH = 3          # tiles: the 16 px body walks across a narrower pit
FLIP_FLAGS = 0xE0000000    # Tiled stores flips and rotation in a gid's top bits

COLLISION_TYPES = {"": "COLLISION_NONE", "solid": "COLLISION_SOLID",
                   "one_way": "COLLISION_ONE_WAY", "hazard": "COLLISION_HAZARD"}
ENTITY_TYPES = {"enemy": "ENTITY_ENEMY", "fragment": "ENTITY_FRAGMENT",
                "gate": "ENTITY_GATE", "exit": "ENTITY_EXIT"}
OBJECT_CLASSES = set(ENTITY_TYPES) | {"player_start"}


class LevelError(Exception):
    """A level breaks a rule; the message says which."""


@dataclass
class Tileset:
    name: str
    collision: list[str]               # per tile: "", "solid", "one_way" or "hazard"
    names: list[str]                   # per tile, from its "name" property
    classes: dict[int, str]            # tile id -> class, for marker tiles


@dataclass
class Entity:
    kind: str                          # key of ENTITY_TYPES
    x: int                             # world pixels, see level_data.h
    y: int
    param: int = 0


@dataclass
class Level:
    name: str
    width: int
    tiles: list[int]                   # MAP_ROWS rows of width tile ids
    player_start: tuple[int, int]
    entities: list[Entity]
    section_start: list[int]
    tileset: Tileset
    test_room: bool = False            # map property test = true: no exit or gates needed
    warnings: list[str] = field(default_factory=list)

    @property
    def fragment_count(self) -> int:
        return sum(e.kind == "fragment" for e in self.entities)


def load_tileset(path: Path) -> Tileset:
    """Read a Tiled .tsx: each tile's collision property and class."""
    root = ET.parse(path).getroot()
    count = int(root.get("tilecount"))
    collision = [""] * count
    names = [""] * count
    classes = {}
    for tile in root.findall("tile"):
        tile_id = int(tile.get("id"))
        tile_class = tile.get("type") or tile.get("class") or ""
        if tile_class:
            classes[tile_id] = tile_class
        for prop in tile.iter("property"):
            if prop.get("name") == "name":
                names[tile_id] = prop.get("value")
            if prop.get("name") == "collision":
                if prop.get("value") not in COLLISION_TYPES:
                    raise LevelError(f"{path.name}: tile {tile_id} has unknown collision {prop.get('value')!r}")
                collision[tile_id] = prop.get("value")
    if count > TILESET_BUDGET:
        raise LevelError(f"{path.name}: {count} tiles, the budget is {TILESET_BUDGET}")
    return Tileset(path.stem, collision, names, classes)


def properties(obj: dict) -> dict:
    return {p["name"]: p["value"] for p in obj.get("properties", [])}


def convert(name: str, tmj: dict, tileset: Tileset, firstgid: int) -> Level:
    """Check a parsed .tmj map and turn it into a Level."""
    def fail(message: str) -> None:
        raise LevelError(f"{name}: {message}")

    width = tmj["width"]
    if tmj["height"] != MAP_ROWS:
        fail(f"the map is {tmj['height']} tiles tall, it must be {MAP_ROWS} (screen rows 3-15)")
    if not MIN_WIDTH <= width <= MAX_WIDTH:
        fail(f"the map is {width} tiles wide, it must be {MIN_WIDTH} to {MAX_WIDTH}")

    layers = {layer["name"]: layer for layer in tmj["layers"]}
    if "tiles" not in layers or "entities" not in layers:
        fail("needs a tile layer named 'tiles' and an object layer named 'entities'")

    tiles = []
    for index, gid in enumerate(layers["tiles"]["data"]):
        if gid & FLIP_FLAGS:
            fail(f"tile at column {index % width}, row {index // width} is flipped or rotated (DMG can't flip background tiles)")
        tile_id = gid - firstgid if gid else 0
        if not 0 <= tile_id < len(tileset.collision):
            fail(f"tile at column {index % width}, row {index // width} is not in tileset {tileset.name}")
        if tile_id in tileset.classes:
            fail(f"tile at column {index % width}, row {index // width} is a {tileset.classes[tile_id]} "
                 f"marker: place it as a tile object in the entities layer")
        tiles.append(tile_id)

    starts, entities = [], []
    for obj in layers["entities"]["objects"]:
        gid = obj.get("gid", 0) & ~FLIP_FLAGS
        obj_class = obj.get("type") or obj.get("class") or tileset.classes.get(gid - firstgid, "")
        where = f"object {obj['id']} ({obj_class or 'no class'}) at x {obj['x']:g}, y {obj['y']:g}"
        if obj_class not in OBJECT_CLASSES:
            fail(f"{where}: the class must be one of {', '.join(sorted(OBJECT_CLASSES))}")
        left, world_y = int(obj["x"]), int(obj["y"]) + MAP_TOP_ROW * TILE

        if obj_class == "enemy":
            # Rectangle: its width is the patrol range of the body, its bottom the feet.
            patrol = int(obj["width"])
            if not 2 * TILE <= patrol <= 255:
                fail(f"{where}: an enemy's patrol width must be 16 to 255 px, not {patrol}")
            entities.append(Entity("enemy", left, world_y + int(obj["height"]) - 1, patrol))
            continue

        # Tile objects: x is the left edge, y the bottom edge, snapped to the tile grid.
        if not gid:
            fail(f"{where}: place it as a tile object (insert its marker tile)")
        if left % TILE or int(obj["y"]) % TILE:
            fail(f"{where}: not on the 8 px grid (turn on Snap to Grid)")
        top = world_y - TILE
        if obj_class == "player_start":
            starts.append((left + TILE, world_y - 1))    # body's left edge on the marker
        elif obj_class == "gate":
            if "quota" not in properties(obj):
                fail(f"{where}: a gate needs an int property 'quota'")
            entities.append(Entity("gate", left, top, properties(obj)["quota"]))
        else:
            entities.append(Entity(obj_class, left, top))

    if len(starts) != 1:
        fail(f"needs exactly one player_start, found {len(starts)}")
    # Test rooms for the PyBoy tests (map property test = true) need no exit or gates.
    test_room = properties(tmj).get("test", False)
    exits = sum(e.kind == "exit" for e in entities)
    if exits != 1 and not test_room:
        fail(f"needs exactly one exit, found {exits}")
    gates = sorted(e.x for e in entities if e.kind == "gate")
    if len(gates) != SECTIONS - 1 and not (test_room or properties(tmj).get("boss", False)):
        fail(f"needs exactly {SECTIONS - 1} gates (or the map property boss = true), found {len(gates)}")

    entities.sort(key=lambda e: (e.x, e.y))
    level = Level(name, width, tiles, starts[0], entities,
                  section_starts(entities, gates), tileset, test_room)
    if level.fragment_count > MAX_FRAGMENTS:
        fail(f"{level.fragment_count} fragments, at most {MAX_FRAGMENTS}")
    check_quotas(level, gates, fail)
    check_checkpoints(level, fail)
    warn_about_patrols(level)
    if not test_room:
        warn_about_narrow_pits(level)
    return level


def section_of(x: int, gates: list[int]) -> int:
    """Sections end at their gate: a gate belongs to the section it closes."""
    return sum(gate_x < x for gate_x in gates)


def section_starts(entities: list[Entity], gates: list[int]) -> list[int]:
    """Index of each section's first entity in the sorted list."""
    starts = []
    for section in range(SECTIONS):
        first = next((i for i, e in enumerate(entities) if section_of(e.x, gates) >= section),
                     len(entities))
        starts.append(first)
    return starts


def check_quotas(level: Level, gates: list[int], fail) -> None:
    """A gate can't ask for more fragments than its section has (hard mode needs all)."""
    for section, gate in enumerate(e for e in level.entities if e.kind == "gate"):
        found = sum(e.kind == "fragment" and section_of(e.x, gates) == section for e in level.entities)
        if not 1 <= gate.param <= found:
            fail(f"gate at x {gate.x} asks for {gate.param} fragments, section {'ABC'[section]} has {found}")


def check_checkpoints(level: Level, fail) -> None:
    """The player restarts just right of an opened gate, feet on the gate's bottom row
    (src/level/section.c): the two columns there need the body's 4 rows free of solid and
    hazard tiles, and ground under at least one of them."""
    def collision(column: int, row: int) -> str:
        if not (0 <= row < MAP_ROWS and column < level.width):
            return "outside"
        return level.tileset.collision[level.tiles[row * level.width + column]]

    for gate in (e for e in level.entities if e.kind == "gate"):
        bottom = gate.y // TILE - MAP_TOP_ROW
        columns = (gate.x // TILE + 1, gate.x // TILE + 2)
        body = {collision(c, r) for c in columns for r in range(bottom - 3, bottom + 1)}
        ground = {collision(c, bottom + 1) for c in columns}
        if body & {"solid", "hazard", "outside"} or not ground & {"solid", "one_way"}:
            fail(f"gate at x {gate.x}: no room to restart right of it (2 free columns, 4 rows up "
                 f"from its bottom tile, ground below)")


def warn_about_patrols(level: Level) -> None:
    """Warn when an enemy patrols over a tile without ground under its feet."""
    for e in (e for e in level.entities if e.kind == "enemy"):
        row = (e.y + 1) // TILE - MAP_TOP_ROW
        for column in range(e.x // TILE, (e.x + e.param - 1) // TILE + 1):
            ground = level.tiles[row * level.width + column] if 0 <= row < MAP_ROWS else 0
            if level.tileset.collision[ground] not in ("solid", "one_way"):
                level.warnings.append(f"{level.name}: enemy at x {e.x} has no ground at column {column}")
                break


def warn_about_narrow_pits(level: Level) -> None:
    """Warn about pits in the bottom row narrower than MIN_PIT_WIDTH: the body is 2 tiles
    wide, so it walks across them without falling in."""
    column = 0
    bottom_row = level.tiles[(MAP_ROWS - 1) * level.width:]
    has_floor = lambda tile: level.tileset.collision[tile] in ("solid", "one_way")
    for floor, run in itertools.groupby(bottom_row, key=has_floor):
        width = len(list(run))
        if not floor and 0 < column and column + width < level.width and width < MIN_PIT_WIDTH:
            level.warnings.append(f"{level.name}: the pit at columns {column}-{column + width - 1} is "
                                  f"{width} wide; the body walks across pits under {MIN_PIT_WIDTH}")
        column += width


def c_level(level: Level, source: str) -> tuple[str, str]:
    """The .c and .h text for a level."""
    n = level.name
    rows = []
    for row in range(MAP_ROWS):
        cells = level.tiles[row * level.width:(row + 1) * level.width]
        rows.append("    " + ",".join(str(t) for t in cells) + ",")
    entities = [f"    {{ {ENTITY_TYPES[e.kind]}, {e.x}, {e.y}, {e.param} }},"
                for e in level.entities]
    # C has no empty arrays: a level without entities gets a null pointer.
    entity_array = f"""
static const LevelEntity {n}_entities[{len(entities)}] = {{
{chr(10).join(entities)}
}};
""" if entities else ""
    x, y = level.player_start
    c = f"""// Generated by tools/tiled2c.py from {source}. Don't edit: change the map in Tiled
// and run "make levels".
#pragma bank 255

#include "{n}.h"

BANKREF({n})

static const uint8_t {n}_map[{MAP_ROWS * level.width}] = {{
{chr(10).join(rows)}
}};
{entity_array}
const LevelData {n} = {{
    {level.width}, {x}, {y}, {level.fragment_count}, {len(level.entities)},
    {{ {', '.join(str(s) for s in level.section_start)} }},
    {n}_map, {f"{n}_entities" if entities else "0"}
}};
"""
    h = f"""// Generated by tools/tiled2c.py from {source}. Don't edit.
#ifndef LEVEL_{n.upper()}_H
#define LEVEL_{n.upper()}_H

#include <gbdk/platform.h>
#include "../level/level_data.h"

BANKREF_EXTERN({n})
extern const LevelData {n};

#endif
"""
    return c, h


def c_tileset(tileset: Tileset, source: str) -> tuple[str, str]:
    """The .c and .h text for a tileset's collision table."""
    n = f"{tileset.name}_tileset"
    values = [f"    {COLLISION_TYPES[c] + ',':22}// {i} {name}"
              for i, (c, name) in enumerate(zip(tileset.collision, tileset.names))]
    markers = [f"#define {n}_TILE_{cls.upper()} {i}" for i, cls in sorted(tileset.classes.items())]
    c = f"""// Generated by tools/tiled2c.py from {source}. Don't edit: change the
// tile properties in Tiled and run "make levels".
#pragma bank 255

#include "{n}.h"

BANKREF({n})

const uint8_t {n}_collision[{len(tileset.collision)}] = {{
{chr(10).join(values)}
}};
"""
    h = f"""// Generated by tools/tiled2c.py from {source}. Don't edit.
#ifndef TILESET_{n.upper()}_H
#define TILESET_{n.upper()}_H

#include <gbdk/platform.h>
#include "../level/level_data.h"

// Marker tiles, by class
{chr(10).join(markers)}

BANKREF_EXTERN({n})
extern const uint8_t {n}_collision[{len(tileset.collision)}];

#endif
"""
    return c, h


def load_level(path: Path) -> Level:
    """Read a .tmj and its one external tileset, and convert them."""
    tmj = json.loads(path.read_text(encoding="utf-8"))
    if len(tmj["tilesets"]) != 1 or "source" not in tmj["tilesets"][0]:
        raise LevelError(f"{path.stem}: needs exactly one tileset, saved as an external .tsx")
    ref = tmj["tilesets"][0]
    tileset = load_tileset((path.parent / ref["source"]).resolve())
    return convert(path.stem, tmj, tileset, ref["firstgid"])


def write_outputs(level: Level, source: Path, out_dir: Path) -> None:
    """Write the level's and its tileset's .c and .h files."""
    tsx = f"graphics/tilesets/{level.tileset.name}.tsx"
    for name, (c, h) in ((level.name, c_level(level, source.as_posix())),
                         (f"{level.tileset.name}_tileset", c_tileset(level.tileset, tsx))):
        (out_dir / f"{name}.c").write_text(c, encoding="utf-8", newline="\n")
        (out_dir / f"{name}.h").write_text(h, encoding="utf-8", newline="\n")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("levels", nargs="+", type=Path)
    parser.add_argument("-o", "--out", type=Path, required=True)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    try:
        for path in args.levels:
            level = load_level(path)
            for warning in level.warnings:
                print(f"warning: {warning}")
            write_outputs(level, path, args.out)
            print(f"{path} -> {args.out / level.name}.c: {level.width} tiles wide, "
                  f"{level.fragment_count} fragments, {len(level.entities)} entities")
    except LevelError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

"""Turn a level text sketch into a Tiled map, or a Tiled map back into a sketch.

    python tools/sketch2tmj.py draft.txt -o levels/level_2.tmj [--quota 3,3]
    python tools/sketch2tmj.py --from-tmj levels/level_1.tmj

A sketch is 13 rows of characters, one per tile (screen rows 3-15), in the legend of
docs/LEVEL_DESIGN.md section 1:
    .  air          #  solid        =  one-way      ^  hazard
    *  fragment     P  player start G  gate         D  exit
    E  enemy: a run of E's is one patrol bot, standing on the row below it; the run is
       its patrol range. Separate two bots' runs by another character.
Lines starting with '#' and a space are comments; "# quota: 3 3" gives the gates' quotas
from left to right (default 3 each). A line of only digits and spaces (a column ruler) is
skipped. The map uses the greybox tileset; make it look right in Tiled afterwards.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import tiled2c

ROOT = Path(__file__).resolve().parent.parent
TILESET = ROOT / "graphics" / "tilesets" / "greybox.tsx"
TILESET_REF = "../graphics/tilesets/greybox.tsx"
ROWS = tiled2c.MAP_ROWS
TILE = tiled2c.TILE
BOT_HEIGHT = 16
DEFAULT_QUOTA = 3
TILE_CHARS = {".": "empty", "#": "solid", "=": "one_way", "^": "hazard"}
OBJECT_CHARS = {"*": "fragment", "P": "player_start", "G": "gate", "D": "exit"}


def tile_ids() -> dict[str, int]:
    """Greybox tile id by name (from the .tsx), for tiles and marker objects."""
    tileset = tiled2c.load_tileset(TILESET)
    return {name: i for i, name in enumerate(tileset.names)}


def read_sketch(text: str) -> tuple[list[str], list[int]]:
    """The sketch's map rows (padded to one width) and the quotas from its header."""
    rows, quotas = [], []
    for line in text.splitlines():
        if line.startswith("# quota:"):
            quotas = [int(q) for q in line.split(":", 1)[1].split()]
        elif line.startswith("# ") or not line.strip() or line.replace(" ", "").isdigit():
            continue
        else:
            rows.append(line.rstrip())
    if len(rows) != ROWS:
        raise SystemExit(f"error: the sketch has {len(rows)} map rows, it needs {ROWS}")
    width = max(len(row) for row in rows)
    return [row.ljust(width, ".") for row in rows], quotas


def sketch_to_tmj(rows: list[str], quotas: list[int]) -> dict:
    """A Tiled map (as the JSON dict) from sketch rows."""
    ids = tile_ids()
    width = len(rows[0])
    data, objects = [], []

    def add_object(obj: dict) -> None:
        objects.append({"id": len(objects) + 1, "name": "", "rotation": 0, "visible": True, **obj})

    gates = 0
    for r, row in enumerate(rows):
        c = 0
        while c < width:
            char = row[c]
            if char == "E":
                run = len(row[c:]) - len(row[c:].lstrip("E"))
                add_object({"type": "enemy", "x": c * TILE, "y": (r + 1) * TILE - BOT_HEIGHT,
                            "width": run * TILE, "height": BOT_HEIGHT,
                            "properties": [{"name": "kind", "type": "string", "value": "patrol"}]})
                data += [ids["empty"] + 1] * run
                c += run
                continue
            if char in OBJECT_CHARS:
                obj = {"gid": ids[OBJECT_CHARS[char]] + 1, "type": "", "x": c * TILE, "y": (r + 1) * TILE,
                       "width": TILE, "height": TILE}
                if char == "G":
                    quota = quotas[gates] if gates < len(quotas) else DEFAULT_QUOTA
                    obj["properties"] = [{"name": "quota", "type": "int", "value": quota}]
                    gates += 1
                add_object(obj)
                char = "."
            if char not in TILE_CHARS:
                raise SystemExit(f"error: unknown character {char!r} at column {c}, row {r}")
            data.append(ids[TILE_CHARS[char]] + 1)
            c += 1
    return {
        "compressionlevel": -1, "height": ROWS, "infinite": False,
        "layers": [
            {"data": data, "height": ROWS, "id": 1, "name": "tiles", "opacity": 1,
             "type": "tilelayer", "visible": True, "width": width, "x": 0, "y": 0},
            {"draworder": "topdown", "id": 2, "name": "entities", "objects": objects,
             "opacity": 1, "type": "objectgroup", "visible": True, "x": 0, "y": 0},
        ],
        "nextlayerid": 3, "nextobjectid": len(objects) + 1, "orientation": "orthogonal",
        "renderorder": "right-down", "tiledversion": "1.12.2", "tileheight": TILE,
        "tilesets": [{"firstgid": 1, "source": TILESET_REF}],
        "tilewidth": TILE, "type": "map", "version": "1.10", "width": width,
    }


def tmj_text(tmj: dict) -> str:
    """The map as JSON, with the tile layer written one map row per line, as Tiled does."""
    width = tmj["width"]
    data = tmj["layers"][0]["data"]
    text = json.dumps({**tmj, "layers": [{**tmj["layers"][0], "data": "@DATA@"}] + tmj["layers"][1:]}, indent=1)
    rows = ",\n".join("  " + ", ".join(str(g) for g in data[r * width:(r + 1) * width]) for r in range(ROWS))
    return text.replace('"@DATA@"', "[\n" + rows + "]") + "\n"


def tmj_to_sketch(tmj: dict) -> str:
    """A sketch from a Tiled map: a column ruler, the quota line and the 13 rows."""
    names = tiled2c.load_tileset(TILESET).names
    chars = {name: char for char, name in TILE_CHARS.items()}
    width = tmj["width"]
    grid = [[chars.get(names[gid - 1], "?") for gid in tmj["layers"][0]["data"][r * width:(r + 1) * width]]
            for r in range(ROWS)]
    quotas = []
    for obj in sorted(tmj["layers"][1]["objects"], key=lambda o: o["x"]):
        col = int(obj["x"]) // TILE
        if obj.get("type") == "enemy":
            row = (int(obj["y"]) + int(obj["height"])) // TILE - 1
            for c in range(col, col + int(obj["width"]) // TILE):
                grid[row][c] = "E"
            continue
        kind = names[obj["gid"] - 1]
        grid[int(obj["y"]) // TILE - 1][col] = {v: k for k, v in OBJECT_CHARS.items()}[kind]
        if kind == "gate":
            quotas += [p["value"] for p in obj.get("properties", []) if p["name"] == "quota"]
    ruler = "".join(str(c // 10 % 10) if c % 10 == 0 else " " for c in range(width)).rstrip()
    lines = [f"# quota: {' '.join(map(str, quotas))}", ruler] + ["".join(row) for row in grid]
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("sketch", nargs="?", type=Path)
    parser.add_argument("-o", "--out", type=Path, help="the .tmj to write")
    parser.add_argument("--quota", help="gate quotas left to right, like 3,3 (else from the sketch)")
    parser.add_argument("--from-tmj", type=Path, help="print the sketch of a Tiled map instead")
    args = parser.parse_args()
    if args.from_tmj:
        print(tmj_to_sketch(json.loads(args.from_tmj.read_text(encoding="utf-8"))), end="")
        return 0
    if not args.sketch or not args.out:
        parser.error("give a sketch and -o, or --from-tmj")
    rows, quotas = read_sketch(args.sketch.read_text(encoding="utf-8"))
    if args.quota:
        quotas = [int(q) for q in args.quota.split(",")]
    args.out.write_text(tmj_text(sketch_to_tmj(rows, quotas)), encoding="utf-8", newline="\n")
    print(f"{args.sketch} -> {args.out}: {len(rows[0])} tiles wide. Check it with 'make levels' and 'make preview'.")
    return 0


if __name__ == "__main__":
    sys.exit(main())

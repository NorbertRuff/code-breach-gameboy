"""Draw a whole level and check that everything in it can be reached.

    python tools/levelpreview.py levels/greybox_test.tmj [...] [-o tests/out/levels]

For each level it writes <name>.png: the tiles with column numbers, the entities, the
surfaces the player can stand on (green: reached, red: not), and for each reached surface
the jump that first got there. Then it reports what can't be reached and exits with 1.
"make preview" runs it on every level.

How the check works (docs/LEVELS.md, section 5):
- A surface is a run of x positions where the body can stand on one tile row.
- From the start, it tries jumps from every reached surface with the game's own physics
  (tools/physics.py): tapped, medium and full jumps, left, right or straight up, from
  standing or at full speed, steering at once or late, walking off the edges, and
  dropping through one-way tops (Down + A).
  Where a jump lands is reached too. A running start is assumed possible anywhere.
- Gates count as open, and the player can't go back left past one (the camera lock).
  Enemies are ignored. Spikes and pits end a jump, and its pickups don't count.
- A lift counts as a one-way top at every tile row it passes, as if it waited there.
"""

from __future__ import annotations

import argparse
import sys
from dataclasses import dataclass, field
from pathlib import Path

from PIL import Image, ImageDraw

import tiled2c
from physics import (BLOCKS_BODY, BLOCKS_FEET, COYOTE_FRAMES, HAZARD, NONE, ONE_WAY,
                     RUN_SPEED_MAX, SOLID, SPRITE_HEAD_OFFSET, SPRITE_LEFT_W,
                     SPRITE_RIGHT_W, SUBPIXELS, Body, Room, area_has, body_tiles, step,
                     tile_of)

ROOT = Path(__file__).resolve().parent.parent
TILE = 8
MAP_TOP_ROW = 3
SCREEN_ROWS = 16
COLLISION = {"": NONE, "solid": SOLID, "one_way": ONE_WAY, "hazard": HAZARD}
MAX_JUMP_FRAMES = 240
SAMPLE_STEP = 16           # px between jump start points on a surface


@dataclass(frozen=True)
class Surface:
    row: int               # tile row the feet stand on
    x0: int                # leftmost and rightmost body center, px
    x1: int
    section: int

    @property
    def feet(self) -> int:
        return self.row * TILE - 1


@dataclass(frozen=True)
class Jump:
    hold: int              # frames A is held; 0 walks off an edge without jumping
    direction: int         # d-pad: -1, 0 or 1
    running: bool          # starts at full speed
    steer_after: int = 0   # frames before the d-pad is pressed
    drop: bool = False     # Down + A on the first frame: drops through a one-way top


JUMPS = [Jump(hold, d, run, steer)
         for hold in (1, 8, 60) for d in (-1, 0, 1) for run in (False, True)
         for steer in ((0, 12) if hold == 60 and d else (0,))
         if d or not run] + [Jump(0, d, True) for d in (-1, 1)] +         [Jump(1, d, False, steer, drop=True) for d in (-1, 0, 1) for steer in ((0, 12) if d else (0,))]


@dataclass
class Exploration:
    surfaces: list[Surface]
    paths: dict[Surface, list[tuple[int, int]]]     # reached surface -> jump that got there
    touched: set[tuple[int, int]]                   # tiles the body reached (col, screen row)
    problems: list[str] = field(default_factory=list)


def lift_tiles(level: tiled2c.Level) -> set[tuple[int, int]]:
    """Every tile a lift's top passes on the 8 px grid (column, screen row): the check
    treats them all as one-way tops, as if the lift waited at each."""
    return {(column, (lift.y - lift.param) // TILE + step)
            for lift in level.entities if lift.kind == "lift"
            for column in range(lift.x // TILE, (lift.x + tiled2c.LIFT_WIDTH) // TILE)
            for step in range(lift.param // TILE + 1)}


def make_room(level: tiled2c.Level, min_x_px: int = SPRITE_LEFT_W) -> Room:
    """The level's collision for the physics. Gates are open (they are entities); lifts
    are one-way tops wherever they pass (lift_tiles)."""
    lifts = lift_tiles(level)

    def collision(tile_x: int, tile_y: int) -> int:
        if (tile_x, tile_y) in lifts:
            return ONE_WAY
        if not MAP_TOP_ROW <= tile_y < MAP_TOP_ROW + tiled2c.MAP_ROWS:
            return NONE
        tile = level.tiles[(tile_y - MAP_TOP_ROW) * level.width + tile_x]
        return COLLISION[level.tileset.collision[tile]]
    return Room(level.width * TILE, collision, min_x_px * SUBPIXELS)


def gate_xs(level: tiled2c.Level) -> list[int]:
    return sorted(e.x for e in level.entities if e.kind == "gate")


def section_at(x: int, gates: list[int]) -> int:
    """Sections passed: the body's left edge is right of the gate tile."""
    return sum(x - SPRITE_LEFT_W >= gate + TILE for gate in gates)


def can_stand(room: Room, x: int, feet: int) -> bool:
    left, right = tile_of(x - SPRITE_LEFT_W), tile_of(x + SPRITE_RIGHT_W)
    below, top = tile_of(feet + 1), tile_of(feet - SPRITE_HEAD_OFFSET)
    return ((feet + 1) % TILE == 0 and area_has(room, left, below, right, below, BLOCKS_FEET)
            and not area_has(room, left, top, right, tile_of(feet), BLOCKS_BODY | 1 << HAZARD))


def find_surfaces(level: tiled2c.Level, room: Room, gates: list[int]) -> list[Surface]:
    """Every run of standable x positions on every row, split at gates."""
    surfaces = []
    for row in range(1, SCREEN_ROWS):
        run: list[int] = []
        for x in range(SPRITE_LEFT_W, level.width * TILE - SPRITE_RIGHT_W):
            if can_stand(room, x, row * TILE - 1) and (not run or section_at(x, gates) == section_at(run[-1], gates)):
                run.append(x)
                continue
            if run:
                surfaces.append(Surface(row, run[0], run[-1], section_at(run[0], gates)))
            run = [x] if can_stand(room, x, row * TILE - 1) else []
        if run:
            surfaces.append(Surface(row, run[0], run[-1], section_at(run[0], gates)))
    return surfaces


def surface_at(surfaces: list[Surface], x: int, feet: int) -> Surface | None:
    return next((s for s in surfaces if s.feet == feet and s.x0 <= x <= s.x1), None)


def walk_tiles(surface: Surface) -> set[tuple[int, int]]:
    """Tiles the body reaches walking along a surface."""
    rows = range((surface.feet - SPRITE_HEAD_OFFSET) // TILE, surface.row)
    cols = range((surface.x0 - SPRITE_LEFT_W) // TILE, (surface.x1 + SPRITE_RIGHT_W) // TILE + 1)
    return {(col, row) for col in cols for row in rows}


def simulate(room: Room, x: int, feet: int, jump: Jump):
    """Run one jump from standing at (x, feet). Returns the path of body positions, the
    body when it lands (None if it died or never landed), and the tiles it touched."""
    body = Body(x * SUBPIXELS, feet * SUBPIXELS, on_ground=True, coyote_frames=COYOTE_FRAMES,
                velocity_x=jump.direction * RUN_SPEED_MAX if jump.running else 0)
    path, touched = [(x, feet)], set()
    a_was_down, airborne = False, False
    for frame in range(MAX_JUMP_FRAMES):
        a_down = frame < jump.hold
        step(body, room, jump.direction if frame >= jump.steer_after else 0, a_down, a_was_down,
             jump.drop and frame == 0)
        a_was_down = a_down
        path.append((body.px, body.py))
        touched.update(body_tiles(body))
        if body.dead:
            return path, None, touched
        if airborne and body.on_ground:
            return path, body, touched
        airborne |= not body.on_ground
    return path, None, touched


def can_drop(room: Room, x: int, feet: int) -> bool:
    """Is nothing under the feet solid, so Down + A drops through (on_one_way_only)?"""
    below = tile_of(feet + 1)
    return not area_has(room, tile_of(x - SPRITE_LEFT_W), below, tile_of(x + SPRITE_RIGHT_W), below, BLOCKS_BODY)


def start_points(room: Room, surface: Surface, jump: Jump) -> list[int]:
    if jump.hold == 0:                       # walking off an edge
        return [surface.x1 if jump.direction > 0 else surface.x0]
    points = sorted({surface.x0, surface.x1, *range(surface.x0, surface.x1, SAMPLE_STEP)})
    return [x for x in points if can_drop(room, x, surface.feet)] if jump.drop else points


def explore(level: tiled2c.Level, start_at: tuple[int, int] | None = None) -> Exploration:
    """Find every surface the player can reach from the start (or from start_at, a body
    center and feet row in px), and what can't be reached."""
    gates = gate_xs(level)
    rooms = [make_room(level, SPRITE_LEFT_W)] + [make_room(level, gate + TILE + SPRITE_LEFT_W) for gate in gates]
    surfaces = find_surfaces(level, rooms[0], gates)
    start = surface_at(surfaces, *(start_at or level.player_start))
    result = Exploration(surfaces, {start: []}, walk_tiles(start))
    queue = [start]
    while queue:
        surface = queue.pop(0)
        for jump in JUMPS:
            room = rooms[surface.section]
            for x in start_points(room, surface, jump):
                path, landed, touched = simulate(room, x, surface.feet, jump)
                if landed is None:
                    continue
                result.touched |= touched
                target = surface_at(surfaces, landed.px, landed.py)
                if target and target not in result.paths:
                    result.paths[target] = path
                    result.touched |= walk_tiles(target)
                    queue.append(target)
    result.problems = find_problems(level, result, gates)
    return result


def find_problems(level: tiled2c.Level, result: Exploration, gates: list[int]) -> list[str]:
    """The exit, every fragment, and every gate's quota must be reachable."""
    problems = []
    tile = lambda e: (e.x // TILE, e.y // TILE)
    for e in level.entities:
        if e.kind in ("fragment", "exit") and tile(e) not in result.touched:
            if e.kind == "exit" and level.test_room:
                continue
            problems.append(f"{level.name}: {e.kind} at column {e.x // TILE}, row {e.y // TILE} can't be reached")
    for section, gate in enumerate(e for e in level.entities if e.kind == "gate"):
        reached = sum(e.kind == "fragment" and tile(e) in result.touched and
                      tiled2c.section_of(e.x, gates) == section for e in level.entities)
        if reached < gate.param:
            problems.append(f"{level.name}: gate at column {gate.x // TILE} asks for {gate.param} fragments, "
                            f"only {reached} can be reached in section {'ABC'[section]}")
    return problems


def draw(level: tiled2c.Level, result: Exploration) -> Image.Image:
    """The level at 2x: tiles, column numbers, entities, surfaces and jump arcs."""
    scale, header = 2, 16
    tiles = Image.open(ROOT / "graphics" / "tilesets" / f"{level.tileset.name}.png").convert("RGB")
    columns = tiles.width // TILE
    tile_image = lambda i: tiles.crop((i % columns * TILE, i // columns * TILE,
                                       i % columns * TILE + TILE, i // columns * TILE + TILE))
    marker = {c: i for i, c in level.tileset.classes.items()}

    base = Image.new("RGB", (level.width * TILE, SCREEN_ROWS * TILE), (200, 215, 235))   # the band
    for row in range(tiled2c.MAP_ROWS):
        for col in range(level.width):
            base.paste(tile_image(level.tiles[row * level.width + col]), (col * TILE, (row + MAP_TOP_ROW) * TILE))
    for e in level.entities:
        if e.kind in marker:
            base.paste(tile_image(marker[e.kind]), (e.x, e.y))

    image = Image.new("RGB", (base.width * scale, base.height * scale + header), (40, 40, 40))
    image.paste(base.resize((base.width * scale, base.height * scale), Image.NEAREST), (0, header))
    pen = ImageDraw.Draw(image)
    at = lambda x, y: (x * scale, y * scale + header)

    for col in range(0, level.width, 5):
        pen.text((col * TILE * scale + 2, 2), str(col), fill=(255, 255, 255))
        pen.line([at(col * TILE, 0), at(col * TILE, 3)], fill=(255, 255, 255))
    arc_colors = [(30, 90, 220), (20, 150, 60), (150, 40, 170)]
    for surface, path in result.paths.items():
        if len(path) > 1:
            pen.line([at(x, y) for x, y in path], fill=arc_colors[surface.section % 3], width=1)
    for surface in result.surfaces:
        color = (0, 170, 0) if surface in result.paths else (220, 0, 0)
        y = surface.row * TILE
        pen.line([at(surface.x0 - SPRITE_LEFT_W, y), at(surface.x1 + SPRITE_RIGHT_W + 1, y)], fill=color, width=2)
    for e in level.entities:
        if e.kind == "enemy":   # fast bots in red
            pen.rectangle([at(e.x, e.y - 15), at(e.x + e.param, e.y + 1)],
                          outline=(240, 140, 0) if e.speed == 1 else (230, 30, 30), width=e.speed)
        elif e.kind == "chaser":   # its range, in purple
            pen.rectangle([at(e.x, e.y - 15), at(e.x + e.param, e.y + 1)], outline=(170, 40, 200), width=2)
        elif e.kind == "lift":    # the area its platform sweeps
            pen.rectangle([at(e.x, e.y - e.param), at(e.x + tiled2c.LIFT_WIDTH, e.y + TILE)],
                          outline=(0, 160, 200), width=2)
        elif e.kind == "fragment":
            color = (0, 170, 0) if (e.x // TILE, e.y // TILE) in result.touched else (220, 0, 0)
            pen.rectangle([at(e.x - 1, e.y - 1), at(e.x + TILE + 1, e.y + TILE + 1)], outline=color, width=2)
        elif e.kind == "gate":
            pen.text(at(e.x - 2, e.y - 12), f"Q{e.param}", fill=(200, 0, 120))
        elif e.kind == "exit":
            pen.text(at(e.x - 4, e.y - 12), "EXIT", fill=(200, 0, 120))
    x, y = level.player_start
    pen.rectangle([at(x - SPRITE_LEFT_W, y - SPRITE_HEAD_OFFSET), at(x + SPRITE_RIGHT_W + 1, y + 1)], outline=(0, 120, 255), width=2)
    return image


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("levels", nargs="+", type=Path)
    parser.add_argument("-o", "--out", type=Path, default=ROOT / "tests" / "out" / "levels")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    failed = False
    for path in args.levels:
        level = tiled2c.load_level(path)
        result = explore(level)
        draw(level, result).save(args.out / f"{level.name}.png")
        fragments = [e for e in level.entities if e.kind == "fragment"]
        reached = sum((e.x // TILE, e.y // TILE) in result.touched for e in fragments)
        print(f"{level.name}: {len(result.paths)} of {len(result.surfaces)} surfaces reached, "
              f"{reached} of {len(fragments)} fragments -> {args.out / level.name}.png")
        for problem in result.problems:
            print(f"problem: {problem}")
        failed |= bool(result.problems)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())

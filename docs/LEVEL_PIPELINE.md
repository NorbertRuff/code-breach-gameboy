# Level Pipeline

**Tiled 1.12 with our own Python converter.** Status: confirmed 2026-10-02, converter in
use since M2.

Level 1 was `for` loops in `level_1_server_room_init`. Five levels need a visual
editor, a data format in ROM, and maps wider than one 256 px background.

**Files:** maps in `levels/*.tmj`, tilesets in `graphics/tilesets/*.tsx` next to their
PNG, generated C in `src/levels/`, the data format in `src/level/level_data.h`. Until the
Server Room art (M5), levels use the greybox tileset drawn by
`tools/greybox_tileset.py`.

## 1. Options compared

| Tool | Visual editing | Enemies and pickups | Auto edge tiles | Maintained | Claude can edit files | Verdict |
|---|---|---|---|---|---|---|
| **Tiled** 1.12.2 (2026-05) | Yes | Object layers with custom properties | Terrain sets, Automapping rules | Active | Yes. `.tmj` is JSON, layer data can be CSV | **Recommended** |
| **LDtk** 1.5.3 (2024-01) | Yes | Entity layers | Best of the group: paint collision values, rules pick the art | No release since January 2024 | Yes, but verbose JSON | Runner-up |
| **Tilemap Studio** 4.0.1 | Yes, built for GB/GBC | No | No | Yes | Binary formats | Can't place enemies or collision |
| **GBMB** (Game Boy Map Builder) | Yes | No | No | Abandoned, no GBC | No | No |
| **PNG level images** (GBDK platformer template) | Any image editor | No, spawns go in C | No | n/a | No | Fallback |
| **Text grid files** | No | Yes, one character per object | Script | n/a | Best | Useful for test levels only |

Existing converter: Larold's [tiled-gameboy-tool](https://github.com/LaroldsJubilantJunkyard/tiled-gameboy-tool)
exports Tiled maps for GBDK. Its last commit was in July 2023, it needs Node.js, and its
GBC attribute and collision support is unclear. Our own converter is about 200 lines of
Python and fits our collision and palette needs exactly.

## 2. Tiled setup

- **One tileset per level theme**, made from the converted tileset PNG
  (`graphics/tilesets/server_room.png`, the output of `gbconvert.py`).
- **Per-tile custom property `collision`**: `solid`, `one_way` or `hazard`. Empty means
  no collision.
- **Palettes are not set by hand.** `gbconvert.py` already knows each tile's palette.
- **Terrain sets** for ground and wall edges, so painting picks the right edge tiles.
- **Map size:** 13 tiles tall: screen rows 3–15. Rows 0–2 are the parallax band and
  rows 16–17 the HUD, neither part of the Tiled map. At least 20 tiles wide.
  Horizontal scrolling only.
- **Parallax band:** a separate 32×3-tile strip per level theme (it wraps around the
  32-tile background map), drawn once at level load and scrolled at half the camera's
  speed by a screen-line interrupt (`src/engine/screen.c`). Map streaming never writes
  rows 0–2. Bands are PNGs in `graphics/bands/` (the greybox one is drawn by a script).
- **Layers:**
  - `tiles`: tile layer.
  - `entities`: object layer. Object classes: `player_start`, `enemy` (property `kind`;
    int property `speed`, 1 or 2 px per frame, default 1; the object's width is the patrol range), `lift`, `fragment`, `gate` (property `quota`; also
    the checkpoint), `exit` (the elevator). Each fragment belongs to the section it sits in,
    by X position between gates; a gate belongs to the section it closes. The game draws
    a gate as a column from its marker's tile up to the map's top row, solid until it
    opens; after it opens, the player restarts just right of it, feet on its bottom row. Level structure
    rules are in [LEVEL_DESIGN.md](LEVEL_DESIGN.md).
  - **Placing them:** `enemy` is a rectangle: its left and right edges are the body's
    patrol range, its bottom edge the floor it walks on. `lift` is a rectangle 24 px wide
    on the 8 px grid, at least 16 px tall: the area its 8 px platform sweeps, from its
    highest place (top edge) to its lowest (bottom edge); it starts at the bottom, going
    up. Everything else is a tile object
    (Insert Tile) with the matching marker tile from the tileset, which already carries
    the class, on the 8 px grid. The player's body starts with its left edge on the
    `player_start` marker's left edge and its feet on the marker's bottom.
  - **Boss level:** map property `boss` = true; it needs no gates.
  - **Test room:** map property `test` = true; it needs no gates and no exit. Used for
    fixtures the PyBoy tests load (`levels/test_jumps.tmj`).
  - **Marker tiles** (the tile classes) are tile objects only: painting one on the tile
    layer is an error. The tileset header gets a `#define` for each, like
    `greybox_tileset_TILE_FRAGMENT`.
- **Saved as `.tmj`** (JSON) with CSV layer encoding, so diffs are readable in PRs.

## 3. Converter: `tools/tiled2c.py`

Runs from `make levels` and writes `src/levels/<map name>.c` and `.h`, plus
`<tileset>_tileset.c` and `.h` for the map's tileset. The output is committed;
`tests/test_tiled2c.py` fails when it is older than the maps.

| Output | Content |
|---|---|
| `<name>_map[]` | Tile index per map cell, row-major, 13 rows (screen rows 3–15) |
| `<name>_entities[]` | `{type, x, y, param, speed}` sorted by X, so entities spawn as the camera reaches them. A cursor walks the list. Coordinates per type are in `level_data.h`. |
| `<name>` (`LevelData`) | Width, player start, fragment count, entity count, `section_start[]` (index of each section's first entity, so a checkpoint restart only resets the cursor), pointers to the map and entities |
| `<tileset>_tileset_collision[]` | One collision type per tileset tile, from the `collision` property |
| GBC attribute map | Planned for M4, with the palette module |

All of it is banked ROM data (`BANKREF`, `#pragma bank 255`): the level in one bank, its
tileset table in another, both picked by autobank.

**Collision lives in a 32×16 RAM ring, filled from the tileset** (decided 2026-10-02,
in use since M2; `src/level/level_stream.c`). Each tileset gets a ROM table
`<tileset>_tileset_collision[tile]`, one byte per tile, copied to WRAM when the level
loads. When a column streams into VRAM, its collision values are looked up once and
written into a 512-byte WRAM ring indexed by `((x & 31) << 4) + y` (`y` in screen rows),
the same wrap as the VRAM map. A collision check is a RAM read: no bank switch, no
multiplication. Opening a gate or collecting a fragment is a write to the ring plus one
tile in VRAM. The ring costs 512 bytes at any level width (the old 32-tile level map cost
1.1 KB). Collected fragments are a bitset in WRAM, 1 bit each, so a fragment stays gone
when its column streams in again; their columns and rows are listed in WRAM at load.

**Streaming** (`level_stream_update`): when the camera's tile column changes, the column
entering the view is drawn, on whichever side it enters. Within a section the player
can walk back left, so columns are redrawn on the left too; the gate lock (M3) only stops
the camera from going back past an opened gate. A column is read from the banked map
into a 13-byte buffer, fragments are put in, and it is written with `set_bkg_tiles`
(not `set_bkg_submap`, which takes the same coordinates for the map and the screen,
while our map starts at screen row 3). A jump of more than a screen (respawn, level
start) redraws all 21 visible columns.

Levels can be at most 255 tiles wide: map widths are `uint8_t`, as in GBDK's
`large_map` example.

The converter fails the build when:

- the map isn't 13 tiles tall, or is narrower than 20;
- there isn't exactly one `player_start`, or (but in the boss arena, map property `boss`)
  exactly one `exit`;
- a level other than the boss arena doesn't have exactly two `gate`s;
- there are more than 32 fragments;
- a gate asks for more fragments than its section has, or has no room to restart right
  of it (2 free columns, the body's 4 rows free of solid and hazard tiles, ground below);
- a lift isn't 24 px wide on the grid, has a solid tile in its columns from 4 rows above
  its highest place down to its lowest (the platform and the body riding it), or is less
  than 16 px from another lift;
- the tileset has more unique tiles than the budget in [ART_PIPELINE.md](ART_PIPELINE.md).

It warns when an enemy's patrol range has no ground under it, and when a pit in the
bottom row is narrower than 3 tiles (the 2-tile-wide body walks across it).

## 4. Levels wider than 256 px

The background map in VRAM is 32×32 tiles and wraps around. As the camera moves, the
column about to come into view is read into a 13-byte buffer and written with
`set_bkg_tiles()` (section 3; `set_bkg_submap()` was the first plan, see DECISIONS.md).
A column draw takes about 30-40 screen lines, so one column per frame fits the frame
budget. On GBC the band and HUD attributes are written once per level; the streamed
columns keep palette 0 until the M5 tilesets bring a palette per tile, and then the
attribute column is written with the tile column. GBDK ships a large-map scrolling
example that shows the pattern. The "Drawing Large Maps" notes in GAME_DESIGN.md describe
the same technique.

## 5. Preview and reachability check: `tools/levelpreview.py`

`make preview` draws every level to `tests/out/levels/<name>.png` at 2×: the tiles with
column numbers every 5, the parallax band, fragments (green outline: reachable, red: not),
gate quotas, the exit, enemy patrol ranges, the player start, the surfaces the player can
stand on (green or red line), and for each reached surface the jump that first got there
(blue, green, purple for sections A, B, C). It fails when something can't be reached.

The check uses `tools/physics.py`, a frame-exact copy of `player.c`.
`tests/test_levelpreview.py` runs the same inputs in the ROM and in the copy and compares
every frame, so a physics change that isn't copied fails the tests.

How it searches:
- A **surface** is a run of x positions where the body can stand on one tile row (not in
  a wall, not on spikes).
- From the start it tries jumps from every reached surface: tapped, medium and full; left,
  right or straight up; from standing or at full speed; steering at once or 12 frames
  late; and walking off each edge. Start points are both ends and every 16 px. Where a
  jump lands is reached too.
- **Gates count as open**, and the player can't go back left past one (the camera lock,
  M3). **Enemies are ignored.** Spikes and pits end a jump, and fragments touched on such
  a jump don't count. A running start is assumed possible anywhere.
- **Problems:** the exit can't be reached, a fragment can't be reached, or a gate asks for
  more fragments than can be reached in its section. Test rooms skip the exit.

## 6. Level list

From GAME_DESIGN.md. Widths are targets to tune in playtesting.

| # | Level | New mechanic | Enemies | Collectible |
|---|---|---|---|---|
| 1 | Server Room | Basics: run, jump, fragments, gates, pits | Patrol security bot | Code fragments |
| 2 | Code Testing Lab | One-way platforms | Faster bots | Bug reports, debug tools |
| 3 | AI Research Wing | Vertical movers, chasers | Malfunctioning AI | AI cores |
| 4 | Network Hub | Hazard tiles | Network bots, data corruption | Security protocols |
| 5 | Mainframe | Boss fight | Final AI | n/a |

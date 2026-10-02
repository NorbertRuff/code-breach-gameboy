# Level Pipeline

**Recommendation: Tiled 1.12 with our own Python converter.** Status: proposed. Confirm
before milestone M2.

Levels today are `for` loops in `level_1_server_room_init`. Five levels need a visual
editor, a data format in ROM, and maps wider than one 256 px background.

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
- **Map size:** 16 tiles tall (the 2 bottom rows of the screen are the HUD), at least
  20 tiles wide. Horizontal scrolling only, at first.
- **Layers:**
  - `tiles`: tile layer.
  - `entities`: object layer. Object classes: `player_start`, `enemy` (property `kind`;
    the object's width is the patrol range), `fragment`, `gate` (property `quota`; also
    the checkpoint), `exit` (the elevator). Each fragment belongs to the section it sits in,
    by X position between gates. Level structure rules are in [LEVEL_DESIGN.md](LEVEL_DESIGN.md).
- **Saved as `.tmj`** (JSON) with CSV layer encoding, so diffs are readable in PRs.

## 3. Converter: `tools/tiled2c.py`

Runs from `make levels` and writes `src/levels/level_N.c` and `.h`.

| Output | Content |
|---|---|
| `level_N_map[]` | Tile index per map cell, row-major |
| `level_N_attr[]` | GBC palette index per map cell |
| `level_N_entities[]` | `{type, x, y, param}` sorted by X, so entities spawn as the camera reaches them. A cursor walks the list. |
| `level_N_section_start[]` | Index of each section's first entity, so a checkpoint restart only resets the cursor |
| `level_N` header struct | Width, tileset reference, player start, fragment count |

All of it is banked ROM data (`BANKREF`, `-b 255`).

**Collision lives in a 32×16 RAM ring, filled from the tileset** (decided 2026-10-02).
Each tileset gets a 256-byte ROM table `tileset_collision[tile]`. When a column streams
into VRAM, its 16 collision values are looked up once and written into a 512-byte WRAM
ring indexed by `((x & 31) << 4) + y`, the same wrap as the VRAM map. A collision check
is a RAM read: no bank switch, no multiplication. Opening a gate or collecting a fragment
is a write to the ring plus one tile in VRAM. Today one 32-tile level costs 1.1 KB of
WRAM. The ring costs 512 bytes at any level width. Collected fragments are also a bitset
in WRAM, 1 bit each, so a fragment stays gone when its column streams in again.

The camera locks behind opened gates, so streaming only runs to the right, and the ring
never has to be refilled backward.

Levels can be at most 255 tiles wide: map widths are `uint8_t`, as in GBDK's
`large_map` example.

The converter fails the build when:

- the map isn't 16 tiles tall, or is narrower than 20;
- there isn't exactly one `player_start` and one `exit`;
- a level other than the boss level doesn't have exactly two `gate`s;
- there are more than 32 fragments;
- the tileset has more unique tiles than the budget in [ART_PIPELINE.md](ART_PIPELINE.md).

It warns when an enemy's patrol range has no ground under it.

## 4. Levels wider than 256 px

The background map in VRAM is 32×32 tiles and wraps around. As the camera moves, the
column about to come into view is copied in with `set_bkg_submap()`. On GBC the attribute
column is copied too, with `set_bkg_submap_attributes()`. One column per frame is within
the DMG's VRAM write budget. GBDK ships a large-map scrolling example that shows the
pattern. The "Drawing Large Maps" notes in GAME_DESIGN.md describe the same technique.

## 5. Level list

From GAME_DESIGN.md. Widths are targets to tune in playtesting.

| # | Level | New mechanic | Enemies | Collectible |
|---|---|---|---|---|
| 1 | Server Room | Basics: run, jump, fragments, gates, pits | Patrol security bot | Code fragments |
| 2 | Code Testing Lab | One-way platforms | Faster bots | Bug reports, debug tools |
| 3 | AI Research Wing | Vertical movers, chasers | Malfunctioning AI | AI cores |
| 4 | Network Hub | Hazard tiles | Network bots, data corruption | Security protocols |
| 5 | Mainframe | Boss fight | Final AI | n/a |

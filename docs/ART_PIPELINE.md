# Art Pipeline

Art is designed in Claude Design. A conversion script makes it fit the hardware, then
`png2asset` turns it into C.

```
Claude Design  →  exported PNG  →  tools/gbconvert.py  →  graphics/<asset>.png  →  make assets  →  src/graphics/<asset>.c
                                   (snap, check, preview)  (indexed, GB-ready)      (png2asset)
```

## 1. Hardware rules

Every asset must follow these. The script checks all of them.

| Rule | Value |
|---|---|
| Tile size | 8×8 px. Everything on screen is built from tiles. |
| Colors per background tile | 4, all from one palette |
| Colors per sprite tile | 3 + transparent (index 0) |
| GBC palettes | 8 background + 8 sprite palettes |
| DMG palettes | 1 background palette for all tiles, 2 sprite palettes |
| Palette order | Index 0 lightest → index 3 darkest, in every palette. For sprites: 0 transparent, 1 lightest → 3 darkest. This makes the DMG show correct grays. |
| Background flips | Not allowed. DMG cannot flip background tiles. Draw mirrored tiles as separate tiles. |
| Sprite mode | 8×16. The 16×32 player is 4 hardware sprites (2 wide × 2 tall). |
| Sprites per scanline | 10 at most. The player uses 2 per line and a 16 px enemy uses 2. Avoid more than 4 enemies at the same height. |
| Sprites total | 40 |
| Color depth | RGB555 (32 levels per channel). Pure hex colors, no anti-aliasing, no gradients. |
| GBC screen | Per channel, values `$10–$1F` all look very bright on a real GBC, so keep mid and dark colors in `$00–$0F`. Colors look washed out compared with a PC monitor. Preview with SameBoy's color correction or `rgbgfx -C` (Pan Docs, Palettes). |
| GBA | A second, brighter palette table per asset, loaded when `_is_GBA` is set ([CGB_PLAN.md](CGB_PLAN.md) section 8). |
| Readability | Shades 0–1 behind the play lane; shade 3 for outlines, solid edges and hazards. On GBC, tile types get colors (red hazards, blue one-way, gold fragments), but each type also has a unique shape so it reads on DMG. |

## 2. VRAM tile budget

The ROM must work on DMG, so all tiles live in VRAM bank 0: 384 tiles in three blocks.

| Block | Tiles | Used for |
|---|---|---|
| Sprite-only (`0x8000`) | 128 | Player (68, all 10 frames), current level's enemies and objects (about 40), pause label (10), spare |
| Shared (`0x8800`) | 128 | HUD frame and font (about 60), the rest goes to level tiles |
| Background-only (`0x9000`) | 128 | Level tileset |

So a level tileset gets about **190 unique tiles**. Screens without a level, like the
title, can use all 256 background tiles.

## 3. Claude Design workflow

1. **Style guide** (one Claude Design page):
   - Per level: 4 background palettes of 4 colors each, ordered light → dark, each shown
     next to its DMG gray version.
   - Sprite palettes for the player, enemies and pickups.
   - Character proportions on a pixel grid.
   - The palettes are copied into `graphics/palettes.json`. That file is the color
     source of truth for the script.
2. **Mockups** at 160×144 for each level, to check composition and readability before
   any tile work.
3. **Sprite sheets** at native size on a grid: player frames 16×32, enemies 16×16. Only
   style-guide colors.
4. **Tilesets** on an 8×8 grid. Each tile uses one palette. For a level: ground top,
   edges and fill, one-way platforms, walls, background props, hazards.

Prompt wording that helps: "pixel art on an exact 8×8 grid, no anti-aliasing, no
gradients, only these hex colors: …". Exports can be at any whole-number scale.

**Base material and touch-ups.** The openly licensed tilesets, fonts and palettes in
[RESEARCH.md](RESEARCH.md) section 4 can seed the designs (CC0 preferred; CC-BY items go
into `CREDITS.md`). For hand fixes, Aseprite with the CC0 *Palette Guard* plugin flags
tiles that break the 4-colors-from-one-palette rule while drawing. LibreSprite and
Pixelorama are free alternatives. The Spriters Resource sheets are for study only, never
for shipping.

## 4. `tools/gbconvert.py`

Built on 2026-10-04 (M5), tested on generated pictures (`tests/test_gbconvert.py`) and
checked end to end through png2asset. Run it as
`.venv/Scripts/python.exe tools/gbconvert.py <asset> <exported.png>`.

Input: an exported PNG plus its entry in `graphics/assets.json`:

```json
{ "programmer": { "type": "sprite", "palettes": "server_room", "budget": 48,
                  "size": [80, 32], "out": "graphics/player/programmer.png" } }
```

`graphics/palettes.json` holds the palette sets, each with `bg` palettes (4 colors) and
`sprite` palettes (3 colors; index 0 is transparent), every palette light to dark, as hex
colors. It has the `greybox` set (the DMG grays) and one set per level from the style
guide: `server_room`, `code_testing_lab`, `ai_research_wing`, `network_hub`, `mainframe`.

1. Shrink the whole-number upscale (for example 4×) with nearest-neighbor: to the
   entry's `size` when given, else at the largest scale that fits. Fail if the pixel grid
   doesn't line up. Give the size: art drawn only with 2 px features also fits at half
   its size.
2. Snap each pixel to the nearest style-guide color. Report the share of changed pixels.
   Fail above a threshold (start at 2%).
3. For each 8×8 tile, check the color limit and that all colors come from one palette.
   Record that palette.
4. Count unique tiles (sprites may use flips, backgrounds may not). A pattern used in
   two palettes is one tile: the palette lives in the attribute map or the sprite's
   attributes, as png2asset stores it. Check the budget.
5. Write an indexed PNG with palettes in groups of 4, each ordered light → dark. This
   goes to `png2asset -keep_palette_order`. Sprite palettes get a filler color at index 0,
   saved as transparent.
6. Write previews to `tests/out/previews/`: `<asset>_cgb.png` and `<asset>_dmg.png` at 4×
   with a tile grid. Print a short report.

Errors name the tile position and the colors at fault, so they can be fixed in Claude
Design.

## 5. png2asset settings

| Asset | Flags |
|---|---|
| Sprites | `-spr8x16 -keep_palette_order -sw W -sh H -px X -py Y -b 255` |
| Actor sheets (enemies, the lift) | `-spr8x16 -keep_palette_order -keep_duplicate_tiles -noflip -tiles_only -b 255`: tiles only, in sheet order (each 8 px column's top, then bottom); the metasprites are written by hand in `src/actor/` |
| Full-screen images (title, endings) | `-map -noflip -keep_palette_order -use_map_attributes -b 255` |
| Level tilesets | `-map -noflip -keep_palette_order -tiles_only -b 255`. Maps come from the level converter ([LEVEL_PIPELINE.md](LEVEL_PIPELINE.md)). |
| Parallax bands (256×24 px) | `-map -noflip -keep_palette_order -b 255`: tiles and a 32×3 map, drawn into rows 0–2 with a tile offset |

## 6. Asset list for the full game

| Asset | Size | Content | Status |
|---|---|---|---|
| Player (programmer) | 16×32 | Idle (2), run (4), jump up, fall, hurt, death | Done (2026-10-10): 10 frames, owner's design |
| Security bot (levels 1–5) | 16×16 | Walk (2) | Done (2026-10-10) |
| Chaser (L3) | 16×16 | Move (2) | Done (2026-10-10) |
| Lift (L3, L5) | 24×16 | Left cap, middle, right cap | Done (2026-10-10) |
| KERNEL's data packet (L5) | 16×16 | Flicker (2) | Done (2026-10-10) |
| Enemies for L2–L4 | 16×16 or 16×32 | One or two types per level | Not started |
| Boss (L5) | Large | Built from background tiles or a metasprite within 10 sprites per line | Not designed |
| Collectibles | 8×8 or 16×16 | Code fragment, bug report, debug tool, AI core, security protocol | Hand-coded diamond tile |
| Level objects | 16×32 | Security gate (closed, opening, open; shows the quota), exit elevator (doors closed and open) | Not started |
| Hazards | 8×8 tiles | Spikes or exposed wiring, sparks | Not started |
| Parallax bands | 32×3 tiles each | One per level theme, wraps around; decoration only | Greybox placeholder (`graphics/bands/greybox_band.png`, drawn by `tools/greybox_band.py`, 8 tiles) |
| Tilesets | ≤ 190 tiles each | Server Room, Code Testing Lab, AI Research Wing, Network Hub, Mainframe | Placeholder city background |
| HUD and font | 59 tiles | Frame (heart, floppy icon, riveted strip), A–Z, 0–9, punctuation | Done (2026-10-10): `graphics/hud/hud.png`, `graphics/font/font.png` |
| Screens | ≤ 256 tiles each | Title logo, intro credits, game over, level complete, ending, credits | Old `splashscreen.c`, unused `apexLogo.png` |

## 7. Existing art

| File | Use |
|---|---|
| `graphics/player/programmer.png` | The final player sheet (SOURCES.md). Palette light → dark since 2026-10-03 (review G1). |
| `graphics/background/background.png` | Old city skyline, not drawn anywhere (its C file removed 2026-10-10). Replace. |
| `graphics/hud/Font.png` | Not used anywhere; the game font is `graphics/font/font.png`. |
| `graphics/start_screen/apexLogo.png` | Title art, generated but never drawn. The title screen uses `src/graphics/splashscreen.c` from an older pipeline. |

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
| Sprite-only (`0x8000`) | 128 | Player (≤ 48), current level's enemies (≤ 40), pickups and effects (≤ 24), spare |
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

Built in milestone M5. Input: an exported PNG plus its entry in `graphics/assets.json`
(type `sprite` or `bg`, palette set, frame size, tile budget).

1. Detect the whole-number upscale (for example 4×) and shrink with nearest-neighbor.
   Fail if the pixel grid doesn't line up.
2. Snap each pixel to the nearest style-guide color. Report the share of changed pixels.
   Fail above a threshold (start at 2%).
3. For each 8×8 tile, check the color limit and that all colors come from one palette.
   Record that palette.
4. Count unique tiles (sprites may use flips, backgrounds may not). Check the budget.
5. Write an indexed PNG with palettes in groups of 4, each ordered light → dark. This
   goes to `png2asset -keep_palette_order`.
6. Write previews to `build/previews/`: `<asset>_cgb.png` and `<asset>_dmg.png` at 4×
   with a tile grid. Print a short report.

Errors name the tile position and the colors at fault, so they can be fixed in Claude
Design.

## 5. png2asset settings

| Asset | Flags |
|---|---|
| Sprites | `-spr8x16 -keep_palette_order -sw W -sh H -px X -py Y -b 255` |
| Full-screen images (title, endings) | `-map -noflip -keep_palette_order -use_map_attributes -b 255` |
| Level tilesets | `-map -noflip -keep_palette_order -tiles_only -b 255`. Maps come from the level converter ([LEVEL_PIPELINE.md](LEVEL_PIPELINE.md)). |

## 6. Asset list for the full game

| Asset | Size | Content | Status |
|---|---|---|---|
| Player (programmer) | 16×32 | Idle (2), run (6), jump up, fall, land, hurt, death | Placeholder: 5 frames, dark → light palette |
| Security bot (L1) | 16×16 | Walk (2–4), turn | Mushroom placeholder |
| Enemies for L2–L4 | 16×16 or 16×32 | One or two types per level | Not started |
| Boss (L5) | Large | Built from background tiles or a metasprite within 10 sprites per line | Not designed |
| Collectibles | 8×8 or 16×16 | Code fragment, bug report, debug tool, AI core, security protocol | Hand-coded diamond tile |
| Level objects | 16×32 | Security gate (closed, opening, open; shows the quota), exit elevator (doors closed and open) | Not started |
| Hazards | 8×8 tiles | Spikes or exposed wiring, sparks | Not started |
| Tilesets | ≤ 190 tiles each | Server Room, Code Testing Lab, AI Research Wing, Network Hub, Mainframe | Placeholder city background |
| HUD and font | about 60 tiles | Frame, A–Z, 0–9, punctuation, icons | Hand-coded digits, `hud_a.png` |
| Screens | ≤ 256 tiles each | Title logo, intro credits, game over, level complete, ending, credits | Old `splashscreen.c`, unused `apexLogo.png` |

## 7. Existing art

| File | Use |
|---|---|
| `graphics/player/programmer.png` | Keep as a reference for proportions. Redraw with light → dark palette order. |
| `graphics/background/background.png` | Placeholder city skyline. Replace. |
| `graphics/enemies/Mushroom.png` | Placeholder. Replace with the security bot. |
| `graphics/hud/hud_a.png` | The layout idea (heart left, counter right) can stay. |
| `graphics/hud/Font.png` | Not used anywhere. Candidate base for the font. |
| `graphics/start_screen/apexLogo.png` | Title art, generated but never drawn. The title screen uses `src/graphics/splashscreen.c` from an older pipeline. |
| `graphics/player/programmerMario.png`, `graphics/start_screen/apexLogo3.*` | Unused. Remove during cleanup. |

# Art Pipeline

How a picture becomes Game Boy data, and the rules it must follow so the same ROM looks
right on the Game Boy Color (GBC) and the original Game Boy (DMG). The `add-asset` skill
(`.claude/skills/add-asset/`) walks through it step by step.

```
drawing (PNG)  →  tools/gbconvert.py  →  graphics/<asset>.png  →  make assets  →  src/graphics/<asset>.c
                  (snap, check, preview)  (indexed, committed)      (png2asset)      (committed)
```

## 1. Hardware rules

`tools/gbconvert.py` checks the art rules; tests check the code rules.

| Rule | Why |
|---|---|
| **Tiles are 8×8.** A background tile has 4 colors from one palette; a sprite tile 3 colors plus transparent (index 0). | The hardware draws everything from tiles. |
| **Every palette runs light to dark:** index 0 lightest, 3 darkest (sprites: 0 transparent, 1 lightest, 3 darkest). | The DMG has one background palette and two sprite palettes of 4 grays; with this order it shows the same picture in the right shades. `tests/test_screen.py` and `test_regressions.py` check it. |
| **No background tile flips** (`png2asset -noflip`); draw mirrored tiles as separate tiles. Sprite flips are fine. | Background flip bits live in the GBC attribute map, which the DMG doesn't have. Sprite flips are in OAM on both. |
| **GBC: 8 background and 8 sprite palettes,** colors in RGB555. No anti-aliasing or gradients. | The palette table size. |
| **Keep mid and dark colors in `$00-$0F` per channel;** judge colors on a real GBC. | Values `$10-$1F` all look very bright on the GBC screen, which also looks less saturated than a PC monitor (Pan Docs, Palettes). |
| **Give tile types a shape, not only a color:** red hazards, blue one-way platforms, gold fragments on GBC, each with its own outline. Shades 0-1 behind the play lane, shade 3 for outlines, solid edges and hazards. | It must read in 4 grays on the DMG. |
| **Sprites in 8×16 mode,** at most 10 per screen line and 40 in total. The 16×32 player is 4 sprites (2 per line), a 16×16 enemy 2, the lift 3. | Hardware limits. |
| **All tiles in VRAM bank 0.** GBC VRAM bank 1 holds only attributes (a palette per map cell), written behind `if (is_cgb)`. | The DMG has no bank 1; an attribute write there would land on the tile map. |
| **Palettes are written only by `src/engine/palette.c`** (and `palette_effects.c`), which has a DMG and a GBC path for loads, fades and flashes. | `BGP_REG`/`OBP*_REG` do nothing in GBC mode, GBC palette registers do nothing on DMG. `tests/test_palette.py` fails on a palette write anywhere else. |
| **No GBC-only features:** 8 KB of WRAM, no double speed, no HDMA, no gameplay that needs the GBC's background-over-sprite priority bit. | The DMG has none of them; the DMG sets the performance budget. |

## 2. Tile budget

All tiles share VRAM bank 0: 384 tiles. Where each one goes is in
[ARCHITECTURE.md](ARCHITECTURE.md), "Memory map". What that leaves for art:

| Asset | Budget |
|---|---|
| Level tileset | 128 tiles (every cell of its sheet; `tools/tiled2c.py` checks) |
| Parallax band | 32 tiles (256×24 px, wraps around) |
| Sprite tiles | 128 in all: the player's 68, the actors' about 40, the pause label 10 |
| A full screen without a level (title, endings) | Up to 256 background tiles; 207 when it prints text, around the font |

## 3. Drawing

- **Colors come from `graphics/palettes.json`:** palette sets (`greybox`, `hud`, `title`,
  one per level theme and one per band), each with `bg` palettes of 4 colors and `sprite`
  palettes of 3, light to dark, as hex. It is the color source of truth for the script.
- **Draw at 1× on an 8×8 grid** (exports at any whole-number scale are fine), only in the
  set's colors, one palette per tile. Image generators don't keep to a pixel grid: use
  their pictures as references and redraw the tiles.
- **Touch-ups in Aseprite** on the indexed PNG in `graphics/`. Pick colors from the
  palette row of the tile's palette: a black or white that looks the same in another row
  breaks the one-palette-per-tile rule. The Palette Guard plugin (CC0) flags such tiles
  while drawing.
- `graphics/start_screen/apexLogo.png` is the owner's first title picture, kept but not
  in the build.

## 4. `tools/gbconvert.py`

```
.venv/Scripts/python.exe tools/gbconvert.py <asset> <exported.png>
```

The asset's entry in `graphics/assets.json` gives its type (`bg` or `sprite`), palette
set, tile budget, size in Game Boy pixels and output path. The script shrinks the export
to 1×, snaps each pixel to the nearest palette color (fails above 2% changed), checks
every tile's colors and palette, counts unique tiles against the budget, and writes the
indexed PNG (palettes in groups of 4, light to dark) plus GBC and DMG previews to
`tests/out/previews/`. Errors name the tile and the colors at fault. Tests:
`tests/test_gbconvert.py`.

## 5. png2asset settings

`make assets` runs png2asset on every PNG in `graphics/` (the flags are in the
`Makefile`). Read its output: an unknown flag only prints "not recognized" and goes on.

| Asset | Flags |
|---|---|
| Player | `-sw 16 -sh 32 -spr8x16 -keep_palette_order -b 255` |
| Actor sheets (enemies, lift, packet) | `-spr8x16 -keep_palette_order -keep_duplicate_tiles -noflip -tiles_only -b 255`: tiles in sheet order; the metasprites are written by hand in `src/actor/` |
| Level tilesets | `-map -noflip -keep_duplicate_tiles -keep_palette_order -tiles_only -b 255`: one tile per sheet cell, so Tiled's tile ids are the VRAM tiles; maps and per-tile palettes come from `tools/tiled2c.py` ([LEVELS.md](LEVELS.md)) |
| Parallax bands | `-map -noflip -keep_palette_order -use_map_attributes -b 255`: the base and accent palettes per cell |
| HUD, title warning | `-map -noflip -keep_palette_order -b 255` |
| Full screens (title) | `-map -noflip -keep_palette_order -use_map_attributes -b 255`; with one palette, no `-use_map_attributes` |
| Font | `-map -bpp 1 -noflip -keep_duplicate_tiles -tiles_only -b 255` |

`-b 255` lets autobank pick each asset's ROM bank; code reads it with `WITH_BANK`.

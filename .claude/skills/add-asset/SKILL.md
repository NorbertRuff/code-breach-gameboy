---
name: add-asset
description: Add or replace pixel art in the ApexLab Game Boy game (player and enemy sprites, tilesets, HUD, fonts, title screens, parallax bands), from Claude Design or the owner's drawings through png2asset into banked C data, following the GBC + DMG palette rules. Use for any change under graphics/ or src/graphics/, or when the owner asks for new art.
---

# Add or replace an asset

The rules come from `docs/ART_PIPELINE.md`; reasons are in `docs/CGB_PLAN.md`.

1. **Check the record.** `docs/SOURCES.md`, Assets section: what this replaces and where
   it came from. Nothing copied from Mario or other commercial games may ship.
2. **Design** in Claude Design (Artifact tool, `quickstart` with intent `design`) or take
   the owner's drawing. Prompt with: pixel art on an exact 8×8 grid, no anti-aliasing,
   no gradients, only the listed hex colors. The hardware rules:
   - 8×8 tiles. A background tile has at most 4 colors, all from one palette. A sprite
     tile has 3 colors plus transparent.
   - **Every palette is ordered light → dark**: index 0 lightest (transparent for
     sprites), index 3 darkest. DMG shows index n as gray n.
   - No flipped background tiles (DMG can't flip them). Sprites may flip.
   - Tile budget: `docs/ART_PIPELINE.md` section 2. The player is 16×32 in 8×16 sprite mode.
   - Real GBC screens wash colors out: keep mid and dark channel values in `$00–$0F`.
3. **Save the PNG** in `graphics/<area>/<name>.png`, indexed, palette in groups of 4,
   light → dark. Until `tools/gbconvert.py` exists (M5), check it with Pillow: palette
   entries, used indices, colors per 8×8 tile. `Image.remap_palette` reorders a palette
   without changing the picture.
4. **Convert.** Add a line to the `assets` target in the `Makefile`, then run
   `make assets` and read the output for `not recognized` (png2asset only warns):
   - Sprite: `-spr8x16 -keep_palette_order -sw W -sh H -px X -py Y -b 255`
   - Background or tileset: `-map -noflip -keep_palette_order -b 255`
     (add `-use_map_attributes` for GBC palettes per tile)
   - 1-bit font: `-map -bpp 1 -noflip -keep_duplicate_tiles -tiles_only -b 255`
5. **Load it in code** inside `WITH_BANK(<name>, ...)` from `src/engine/banking.h`: one
   asset per `WITH_BANK`, and metasprite draws count too. Tile regions: sprite-only
   0–127, shared 128–255, background-only 0–127.
6. **Test the palette order**, like `test_player_palette_is_light_to_dark` in
   `tests/test_regressions.py`, reading `_<name>_palettes`.
7. **Compare before and after:**
   - before the change: `.venv/Scripts/python.exe tools/shots.py --out tests/out/shots_before`
   - after: `make shots`
   - `.venv/Scripts/python.exe tools/compare_shots.py tests/out/shots_before tests/out/shots`
   Look at the `diff_*.png` pictures and both contact sheets (DMG and GBC).
8. **Update the records:** the Assets row in `docs/SOURCES.md` (origin, license, credit),
   the asset list in `docs/ART_PIPELINE.md`, and a `CREDITS.md` entry for CC-BY material.
9. **Report** with the before/after pictures and a suggested commit message.

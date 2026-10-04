# Game Boy Color Plan

Goal: one ROM file. On a Game Boy Color (GBC, also called CGB) it runs in color. On an
original Game Boy (DMG) it runs in 4 shades with the same gameplay. Test devices: a DMG
and a GBC with an EverDrive-style flashcart.

## 1. Header and cartridge flags

Flags go to `makebin` through `lcc` with the `-Wm` prefix.

| Flag | Effect | Why |
|---|---|---|
| `-Wm-yc` | Header byte `0x143 = 0x80`: GBC-enhanced, DMG-compatible | Not `-Wm-yC` (`0xC0`), which is GBC-only and refuses to run on DMG. |
| `-Wm-yt0x1B` | Cartridge type MBC5 + RAM + battery | Battery save for progress. MBC5 works on DMG too, and flashcarts support it well. |
| `-Wm-ya1` | 1 RAM bank (8 KB) | Saves need well under 1 KB. |
| `-autobank` and `-Wm-yoA` | Assign banks for data marked `-b 255`; size the ROM automatically | Stops us from hand-placing assets in banks. |
| `-Wm-yn"APEXLAB"` | Cartridge title | 11 characters or fewer when the GBC flag is set. |
| `-Wm-yS` | Write `obj/ApexLab.sym` from the `.noi` file | PyBoy loads it for tests (see [TESTING.md](TESTING.md)). |

## 2. Detect the hardware once

```c
// src/engine/platform.c
uint8_t is_cgb;
void platform_init(void) { is_cgb = (_cpu == CGB_TYPE); }
```

A GBA running the game in GBC mode also reports `CGB_TYPE`. That is what we want.

## 3. Rules for code that runs on both

1. **Guard every write to VRAM bank 1.** On DMG, writes to `VBK_REG` are ignored. An
   "attribute" write would then land in bank 0 and overwrite the tile map. Every
   attribute write goes behind `if (is_cgb)`.
2. **Every palette change needs two paths.** `BGP_REG`, `OBP0_REG` and `OBP1_REG` do
   nothing in GBC color mode. GBC palette registers do nothing on DMG. One module,
   `src/engine/palette.c`, owns all palette state. No other file writes palette registers.
3. **Tiles live in VRAM bank 0 only.** GBC bank 1 holds 384 more tiles, but a DMG cannot
   see them. Bank 1 holds attribute maps only.
4. **8 KB of WRAM.** The GBC's extra WRAM banks don't exist on DMG.
5. **No double-speed mode.** `cpu_fast()` only exists on GBC. Game logic must fit in one
   DMG frame (about 70,000 CPU cycles). The DMG sets the performance budget.
6. **No HDMA.** GBC DMA for VRAM copies doesn't exist on DMG. Map streaming must work with
   normal copies, about one 16-tile column per frame.
7. **No background tile flips.** Tile flip bits live in the GBC attribute map, so a DMG
   shows flipped background tiles unflipped. Backgrounds are exported with `-noflip`.
   Sprite flips are fine: those bits are in OAM on both systems.
8. **Load palettes before showing a screen.** On GBC an unloaded palette can show as
   all white. Verify this on day one in PyBoy's GBC mode.

## 4. How one tile works on both systems

A tile pixel is a 2-bit color index (0–3).

- **GBC** looks the index up in one of 8 background palettes. The attribute map picks
  the palette for each map cell.
- **DMG** looks the index up in `BGP_REG`, one 4-shade palette for every background tile.

So **in every palette, index 0 is the lightest color and index 3 the darkest**. Then the
DMG shows the same picture in 4 grays, with the shades in the right order.

Sprites work the same way, except index 0 is transparent:

| | GBC | DMG |
|---|---|---|
| Background palettes | 8 × 4 colors | 1 (`BGP_REG`) |
| Sprite palettes | 8 × 3 colors + transparent | 2 (`OBP0_REG`, `OBP1_REG`) |
| Sprite palette select | OAM attribute bits 0–2 | OAM attribute bit 4 |

`png2asset` writes the GBC palette bits into metasprite properties. Our code sets bit 4
for sprites that should use `OBP1` on DMG, for example to make enemies contrast with
the player.

Default DMG palettes: `BGP_REG = 0xE4` and `OBP0_REG = 0xE4` (index 0 white, 3 black).
`OBP1_REG` is picked with the final art.

## 5. Fades and flashes

Until 2026-10-04, `fade_in_screen`, `fade_out_screen` and the game-over flash wrote DMG
registers only. The palette module (`src/engine/palette.c`, built that day) has one API for
both systems; the plan was:

```c
void palette_fade_out(uint8_t frames_per_step);   // to black, 4 steps
void palette_fade_in(uint8_t frames_per_step);
void palette_flash(uint8_t count, uint8_t hold);
```

- **DMG:** step through `BGP_REG` values `0xE4 → 0xF9 → 0xFE → 0xFF`, as today.
- **GBC:** for each step, scale every color toward black in RGB555 and write it with
  `set_bkg_palette` / `set_sprite_palette`. Source palettes stay in ROM. The working copy
  is 128 bytes of WRAM (8 background + 8 sprite palettes × 4 colors × 2 bytes).

Both paths take 4 steps, so fades last the same time on both systems.

## 6. Attribute maps

- `png2asset -use_map_attributes` exports `<name>_map_attributes`: one byte per map cell
  holding the palette index.
- Write them with `set_bkg_attributes()` / `set_bkg_submap_attributes()` behind `is_cgb`.
- Level streaming copies the attribute column together with the tile column.
- Attribute bit 7 (background over sprites) is GBC-only. Gameplay must not depend on it.

## 7. Migration steps

Each step lands with a PyBoy test that runs in both modes.

Status 2026-10-04: steps 4 and 5 are done for what is drawn once per level: GBC
background palette 0 is the level's, 1 the band's, 2 the HUD's, each from its asset, and
the band rows and the HUD window get theirs in the attribute map. Streamed columns wait
for per-tile palettes (M5). Step 3 is done too: `src/engine/palette.c` owns every palette write, and
fades and flashes work on GBC too. A fade step on GBC scales the colors to about 5/8,
1/4 and 0 with masked shifts of the whole RGB555 word, and writes all 8 palettes in one
call (scaling each channel by multiplying took 2 frames per step). GBDK's
`set_bkg_palette`/`set_sprite_palette` wait for STAT modes 0-1 before each byte (read
from `gbdk/lib/gb/gb.lib`), so palette writes outside VBlank are safe on hardware,
where palette memory is locked while a line is drawn.

Status 2026-10-03: steps 1 and 2 are done (M0). `platform_init()` also loads gray GBC
palettes that match the DMG shades and clears the attribute maps, so the game looks the
same on both until step 4. Fades and flashes still only work on DMG.

1. Makefile flags from section 1. Test: header byte `0x143` is `0x80`, byte `0x147` is `0x1B`.
2. `platform.c` with `is_cgb`. Test: `_is_cgb` is 0 in DMG mode and 1 in GBC mode.
3. `palette.c`. Replace every direct `BGP_REG` / `OBP0_REG` write. A test greps `src/`
   and fails if any other file writes those registers.
4. Load the palettes `png2asset` already exports for the background, HUD and sprites.
5. Attribute maps for the title screen, HUD and level.
6. Re-export the art with light-to-dark palette order ([ART_PIPELINE.md](ART_PIPELINE.md)).
7. Check on the DMG and GBC.

## 8. Things PyBoy cannot show

- A real GBC screen shows colors less saturated than a PC monitor. Keep strong contrast,
  and judge colors on the GBC itself.
- The DMG screen blurs fast movement. Flashing effects look different than in an emulator.
- A GBA in GBC mode shows the game darker. Decided on 2026-10-02: when `_is_GBA` is set,
  load a brighter copy of every palette (Shantae does the same). A rough conversion from
  the research: GBA ≈ GBC × 3/4 + `$08` per channel, so the bright set compensates for
  that. It is not a test device, but many people play that way.

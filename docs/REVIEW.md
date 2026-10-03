# Review Pass: Level 1

Done on 2026-10-03, after M1, at the owner's request ("a lot of mistakes in graphics,
logic, visuals"). Method: a scripted PyBoy playthrough in DMG and GBC mode (`make shots`,
`tools/shots.py`), a picture of the whole level map from VRAM, and a read of the code.

Pictures from this pass:

- [DMG contact sheet](img/review-2026-10-03-dmg.png): 17 steps, from level start to after game over
- [GBC contact sheet](img/review-2026-10-03-cgb.png): the same steps in GBC mode
- [Level map](img/review-2026-10-03-level-map.png): all 32×18 tiles, column numbers every 5

`make shots` makes new ones in `tests/out/shots/`.

Most of what looks wrong comes from three things: the level is 18 rows tall but the HUD
covers the bottom 2, the art is placeholder, and the physics is the old prototype. Each
finding names the milestone that fixes it. **Quick win** marks the ones that are cheap
to fix now and still worth it, even though a later milestone replaces that code.

## Layout

| # | What you see | Cause | Fix | When |
|---|---|---|---|---|
| L1 | The floor is invisible. The player and the first enemy stand inside the HUD bar. | The HUD window covers rows 16–17 (y ≥ 128). Level 1's ground is row 17 and the player's feet are at y = 135. | Move level 1 up 2 rows: ground on row 15, everything else 16 px higher. The planned level format already uses a 16-row playfield. | **Quick win**, then M3 |
| L2 | Sprites draw over the HUD: player, enemies, and anything that falls into a pit. | Sprites always draw above the window layer. | With L1 fixed, sprites only reach the HUD when falling. Then hide sprites below line 127 with a screen-line interrupt, or despawn fallen actors. | M3 |
| L3 | At the start, the player sits at the screen's left edge, half covering the HUD heart. | Start x = 8 (M1 fix for bug 16) with the camera at 0. | Comes out right once L1 lifts the player above the HUD. | With L1 |

## Graphics

| # | What you see | Cause | Fix | When |
|---|---|---|---|---|
| G1 | On DMG the player is pale and hard to see: its outline is light gray. | `programmer.png`'s palette is ordered transparent, black, gray, white (dark to light). DMG maps index 1 to light gray. | Reorder the PNG's palette to transparent, white, gray, black and run `make assets`. DMG then shows a black outline; GBC keeps the same colors. | **Quick win** |
| G2 | Platforms look like rows of fins, not solid ground. | Solid tiles are drawn with tile 128, the top-left quarter of the mushroom picture (a debug view). | A plain 8×8 block tile for solid ground: the start of the greybox tileset. | **Quick win**, then M2 greybox |
| G3 | Fragments are black diamonds inside white boxes. | The fragment tile uses colors 0 and 3 only. Color 0 is white, and the sky behind it is dark gray. | Proper fragment art. Until then, it stays as it is. | M5 |
| G4 | The sky is dark gray, so the scene reads as night. | The placeholder background's colors map to dark shades through the DMG palette. | New Server Room tileset. | M5 |
| G5 | On GBC, the enemy mushrooms look washed out. | Enemies use sprite palette 0, which holds the player's colors on GBC. | Separate sprite palettes per actor type. | M4 |
| G6 | The HUD has a coin icon (column 13) that means nothing. The heart and lives digit are drawn over the HUD art's own heart. | Placeholder HUD art from before the counter existed. | New HUD design with lives and fragments against the quota. | M3 (layout), M5 (art) |
| G7 | Enemies are mushrooms. | Placeholder. | Security bot sprite. | M5 |

## Gameplay and logic

| # | What happens | Cause | Fix | When |
|---|---|---|---|---|
| P1 | Holding A makes the player jump again on every landing. | `input.c` starts a jump whenever A is down while on the ground, not only on a new press. | Start a jump only on a new press (`!(joypad_previous & J_A)`). M2 adds jump buffering on top. | **Quick win**, then M2 |
| P2 | The player walks through platform A at body height. | Bug 7: horizontal collision checks one point near the feet. | Collision rewrite. Expected-failure test exists. | M2 |
| P3 | Falling can turn upward; releasing A doesn't shorten a jump. | Bugs 5 and 6: gravity from a frame-count table. | Physics rewrite. Expected-failure test exists for 5. | M2 |
| P4 | Level complete and game over are only flashes. No text, no score; level complete reloads the same level. | Not built yet. | Level complete and game over screens. | M7 |
| P5 | During the game-over wait, the HUD still shows the old lives count for about 1.5 s. | The HUD updates only after the level reloads. | Comes with the game-over screen. | M7 |
| P6 | On GBC, the screen never fades or flashes, and the level reload after game over or level complete is visible. | Fades write `BGP_REG`, which GBC ignores. | Palette module. | M4 |

## Text

| # | What you see | Fix | When |
|---|---|---|---|
| T1 | The intro credits say "v1.0.0" and "2024". | Update to the current year and a development version. | **Quick win** |
| T2 | Intro text uses GBDK's built-in font, not the game's own. | Game font. | M5 |

## Proposed quick wins

Small changes that make the game look and feel right on hardware now. Each gets a
regression test or a screenshot check.

| # | Change | Size | Thrown away later? |
|---|---|---|---|
| L1 | Lift level 1 by 2 rows above the HUD | About 15 lines in `level_manager.c` and the enemy spawn table | Yes, in M3 (Tiled level 1). Until then it fixes every screenshot and hardware test. |
| G1 | Reorder the player PNG's palette light → dark | A small script plus `make assets` | No: it's the art pipeline rule applied to existing art |
| G2 | Plain block tile for solid ground | One 16-byte tile | Mostly: it becomes the first greybox tile in M2 |
| P1 | Jump only on a new A press | One condition in `input.c` | No: M2 keeps it |
| T1 | Intro credits year and version | Two strings | No |

The rest stays with its milestone.

**Status 2026-10-03: all five quick wins are done** (the owner picked all of them), each
with a regression test in `tests/test_regressions.py` that fails on the old build:
`test_level_and_player_are_above_the_hud` (L1), `test_player_palette_is_light_to_dark`
(G1), `test_holding_a_jumps_only_once` (P1). G2 and T1 were checked on screenshots.
[After the quick wins](img/review-2026-10-03-dmg-after-quick-wins.png): the floor and
platforms are visible blocks above the HUD, and the player has a black outline on DMG.

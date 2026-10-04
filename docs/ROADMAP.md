# Roadmap

From the current prototype to the full 5-level game, playable in color on GBC and in
4 shades on DMG. Claude writes the code; the owner runs git, reviews, and plays it in
Emulicious and on hardware. Items marked **(owner)** need the owner.

## Status (2026-10-04)

- **Playable:** level 1 (Server Room, art pending) and the level 2 greybox (Code Testing
  Lab), with music. Next: the owner's level 2 pass, then M5 art or levels 3-5.
- **Tests:** `make test` 188 tests in about 12 s; `make test-full` 280 in about 15 s.
- **Memory:** bank 0 has 2,235 bytes free; WRAM uses 1,447 of 4,096 bytes.
- **Frame budget** (worst update, line of the 140 allowed; average): level 1 126 (77),
  level 2 105 (65), greybox route 118 (78). The music step costs about 9 lines, up to 18.

## Bugs

Open bugs, numbered on from the audit's 22 ([archive/PROJECT_STATE.md](archive/PROJECT_STATE.md)).
A fixed bug leaves this list; its regression test keeps the number.

- None open.

## Waiting for the owner

- Play level 2 on DMG and GBC; listen to the music and effects on hardware.
- Try `savetest_mbc1.gb` and `savetest_mbc3.gb` on the EverDrive (M7).
- Pick songs for title, game over and level complete (M6).
- Hardware checks of M2 and M4 (streaming, one-way shelf, spikes, pit, respawn, head
  bumps; colors on GBC).
- Claude Design style guide (M5); edit the story text in [STORY.md](STORY.md) (M7).

The physics and level loading (M2) came before color (M4) and art (M5) on purpose: level
layouts depend on the jump, and levels are designed with plain blocks first.

---

## M0–M3 — Done (2026-10-02 to 2026-10-04)

- **M0, toolchain and test loop:** Makefile with MBC5 + autobank and banked assets, ROM
  layout checks, PyBoy harness, CI, save-persistence test ROM.
- **M1, bug fixes:** the audit's bugs fixed with regression tests; review pass and its
  quick wins ([archive/REVIEW.md](archive/REVIEW.md)).
- **M2, physics and level loading:** sub-pixel physics with coyote time, jump buffer and
  corner correction; camera; tile collision; Tiled levels with `tools/tiled2c.py`; the
  collision ring and column streaming; `tools/levelpreview.py` with the reachability check.
  Jump metrics in [LEVEL_DESIGN.md](LEVEL_DESIGN.md) section 4.
- **M3, level 1 and gameplay objects:** actors, gates with quotas and checkpoints, the
  exit, HUD, pause and level-select cheat, parallax band and screen splits, lives rules,
  frame budget work, level 1 greybox (owner pass: keep as is), style pass.
- Still open from these: fixed pools with free lists for particles and projectiles, once
  the first one is needed (M8).

## M4 — Game Boy Color support

Done when the game runs in color on GBC and unchanged on DMG. Steps in
[CGB_PLAN.md](CGB_PLAN.md) section 7. Can run alongside M2 and M3.

- [x] `src/engine/platform.c`: `is_cgb`. (M0)
- [x] `src/engine/palette.c`: load, fade, flash for both systems. No other file writes
      palette registers. Done 2026-10-04, `tests/test_palette.py`.
- [ ] Load the exported palettes for background, HUD and sprites. Done 2026-10-04 for
      the current assets (level tiles, band, HUD, player); the M5 art adds the rest.
- [ ] Attribute maps behind `is_cgb`, including streamed columns. The band and HUD have
      theirs (2026-10-04); streamed columns wait for per-tile palettes from the M5 tilesets.
- [ ] Brighter palette set when `_is_GBA` is set.
- [x] Invincibility shown with a pale sprite palette on GBC instead of blinking.
      Done 2026-10-04 (sprite palette 1).
- [x] Golden screenshots per mode. `tests/test_golden.py`, `tests/golden/`, rewritten by
      `make goldens` (2026-10-04).
- [ ] **(owner)** Check on DMG and GBC.

## M5 — Art pipeline and the Server Room art pass

Done when level 1 looks like the final game in both modes.

- [ ] **(owner)** Claude Design style guide: palettes and character look.
- [x] `tools/gbconvert.py` with previews ([ART_PIPELINE.md](ART_PIPELINE.md)). Built and
      tested 2026-10-04; `graphics/palettes.json` waits for the style guide's palettes.
- [ ] Player sprite with all animations.
- [ ] Security bot enemy.
- [ ] Server Room tileset with Tiled terrain sets. Swap it in for the greybox tileset.
- [ ] Security gate and exit elevator art.
- [ ] HUD frame and font (the placeholder font's tiles in `src/graphics/font.c` get swapped).
- [ ] Title screen from `apexLogo`. Remove `splashscreen.c`.
- [ ] Parallax band art for the Server Room (32×3 tiles, wraps).
- [ ] Replace every copied placeholder listed under Assets in [SOURCES.md](SOURCES.md):
      programmer sprite, mushroom enemy, background, HUD. Claude Design for the pixel art.

## M6 — Audio

- [x] **(owner)** Decide where the music comes from: Tronimal's Game Boy Music Packs #1
      and #2 (CC BY 4.0), decided 2026-10-02.
- [x] hUGEDriver in the build, songs in banked ROM (`make music`), one step per frame from
      the line interrupt below the band.
- [ ] Sound effects: Tronimal GBFX/GBFX2 converted with untoxa's VGM2GBSFX, played with
      a channel mute mask and priority over the music.
- [ ] Music per level, title, game over and ending. Levels 1 and 2 have songs (Proposed);
      **(owner)** pick the rest by ear from the Tronimal packs.
- [x] `CREDITS.md` with every CC-BY item (title, author, link, license, changes); add
      each new item with it.

## M7 — Saves and game flow

- [ ] Find out why saves don't persist on the EverDrive clone (the M0 test ROM's count
      doesn't go up). MBC1 and MBC3 variants are built (`make savetest`); **(owner)** try
      them (TESTING.md, "Save test on the real Game Boy"). If none keeps the count, use
      passwords instead of battery saves.
- [ ] Save data in cartridge RAM: unlocked levels, best score, 100% marks, a checksum
      for first boot.
- [ ] Level select, game over with continue, level complete screen with score. The level
      complete summary and a game over message exist (2026-10-04), and level complete goes
      on to the next level; the level select and a real "continue" choice wait.
- [ ] Story text: 3-line boot screen, one AI log line per elevator ride, "PATCH n/5"
      progress. Drafted in [STORY.md](STORY.md); **(owner)** edit it.
- [ ] Hard mode, unlocked after clearing the game.
- [ ] Ending and the in-game credits screen.

## M8 — Levels 2 to 5

One level at a time: the new mechanic, the brief, the greybox loop, the art pass, and a
replay test that finishes it.

- [ ] Level 2, Code Testing Lab: one-way platforms, Down + A drops through, fast bots.
      Greybox done ([LEVEL_DESIGN.md](LEVEL_DESIGN.md) section 7); **(owner)** play it.
      Then the art pass.
- [ ] Level 3, AI Research Wing: vertical movers and chasers.
- [ ] Level 4, Network Hub: hazards.
- [ ] Level 5, Mainframe: the KERNEL boss (concept in GAME_DESIGN.md).
      **(owner)** Review the detailed boss design before it's built.

## M9 — Polish and release

- [ ] **(owner)** Full playthrough on DMG and GBC.
- [ ] No lag frames in any level. Sprite flicker below the per-line limit. The level
      replay tests check both.
- [ ] Difficulty and checkpoint tuning.
- [ ] Project README, screenshots, itch.io page.
- [ ] Asset check: every asset in SOURCES.md is original or licensed and credited;
      nothing copied from Mario or other commercial games.
- [ ] Write the final "how it works" document from DECISIONS.md and SOURCES.md: boot,
      main loop, tiles and sprites, banking, collision, physics, palettes and fades,
      parallax, audio, saves.

---

## Order and dependencies

```
M0 ─► M1 ─► M2 ─► M3 ─► M5 ─► M8 ─► M9
         │               ▲
         └─► M4 ─────────┘      (color before the art pass)

             M3 ─► M6 ─► M8      (audio in parallel)
             M3 ─► M7 ─► M9      (saves and flow in parallel)
```

The save test in M0 runs early on purpose, so the cartridge type is settled before save
code is written.

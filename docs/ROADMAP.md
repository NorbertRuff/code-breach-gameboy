# Roadmap

From the current prototype to the full 5-level game, playable in color on GBC and in
4 shades on DMG. Claude writes the code; the owner runs git, reviews, and plays it in
Emulicious and on hardware. Items marked **(owner)** need the owner.

## Status (2026-10-05)

- **Playable:** levels 1-4 (level 1 art pending; levels 2 and 3 greyboxes, kept after the
  owner's passes; the level 4 greybox is new), music and sound effects; passwords with the
  title menu, the password screen, the level select and the game over menu; the story text
  (boot screen, "PATCH n/5", the AI log per elevator ride). The M7 menus and story and
  level 4 wait for the owner's check. Next: the owner reviews the boss design, then
  level 5 is built; M5 art waits for the style guide.
- **Tests:** `make test` 227 tests in about 14 s; `make test-full` 336 in about 19 s.
- **Memory:** bank 0 has 1,365 bytes free (`level_load` reads tables instead of a chain
  of ifs); WRAM uses 1,495 of 4,096 bytes.
- **Frame budget** (worst update, line of the 140 allowed; average): level 1 125 (79),
  level 2 108 (66), level 3 131 (91), level 4 107 (69), greybox route 121 (80). Lifts cost about 10 lines on
  average where they are; the music step about 9, up to 18; a streamed column's write 5.

## Bugs

Open bugs, numbered on from the audit's 22 ([archive/PROJECT_STATE.md](archive/PROJECT_STATE.md)).
A fixed bug leaves this list; its regression test keeps the number.

- None open.

## Waiting for the owner

- Hardware checks of M2 and M4 (streaming, one-way shelf, spikes, pit, respawn, head
  bumps; colors on GBC).
- Claude Design style guide (M5).
- Try the M7 menus in Emulicious and on hardware: the title menu, a password from a level
  complete screen typed back in (`python tools/password.py` lists them), the level select,
  game over CONTINUE and QUIT, the boot screen and the elevator log screens. Agree or
  change the **Proposed** menu and story-screen decisions (DECISIONS.md).
- Play the level 4 greybox and its song ([LEVEL_DESIGN.md](LEVEL_DESIGN.md) section 7,
  "Level 4 brief": the open points).

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
- [x] Sound effects: Tronimal GBFX/GBFX2 (`sounds/`, `make sounds`), our own player with
      a channel mute mask and priority over the music; sound test ROM and WAVs
      (`make soundtest`). Done 2026-10-04; owner approved the picks.
- [ ] Music per level, title, game over and ending. Levels 1 and 2, title, game over and
      level complete have songs (owner approved 2026-10-04; both packs in `music/packs/`,
      all 18 songs in the sound test); level 3 too (owner: keep). Level 4 plays
      "Whirlwind" (2026-10-05, **(owner)** listen). Level 5 and the ending wait for those
      (candidates: "Terminate" for the boss, "Closing" for the ending).
- [x] `CREDITS.md` with every CC-BY item (title, author, link, license, changes); add
      each new item with it.

## M7 — Saves and game flow

- [x] Saves on the EverDrive clone: no save test ROM kept its count (owner,
      2026-10-04), so the game uses passwords (DECISIONS.md "Saves").
- [x] Passwords: one per level, shown at level complete (and game over), entered from
      CONTINUE on the title screen (2026-10-05, `src/game_state/progress.c`,
      `tools/password.py`, `tests/test_passwords.py`). **(owner)** Check them.
- [x] Level select after a password, game over with CONTINUE / QUIT, level complete with
      the fragments and the password (2026-10-05). A score beyond the fragment count isn't
      designed; add one only if wanted.
- [x] Story text: 3-line boot screen on a new game, "PATCH n/5 COMPILED" on level
      complete, the AI log on the elevator ride after levels 1-3 (2026-10-05,
      `src/game_state/story.c`). Log 4 comes with level 4, log 5 with the boss level (M8).
- [ ] Ending and the in-game credits screen.

## M8 — Levels 2 to 5

One level at a time: the new mechanic, the brief, the greybox loop, the art pass, and a
replay test that finishes it.

- [ ] Level 2, Code Testing Lab: one-way platforms, Down + A drops through, fast bots.
      Greybox done and kept after the owner's hardware pass ([LEVEL_DESIGN.md](LEVEL_DESIGN.md)
      section 7). Next: the art pass (M5).
- [ ] Level 3, AI Research Wing: vertical movers and chasers. Lifts done 2026-10-04
      (`src/actor/lift.c`), chasers done (`src/actor/chaser.c`), both tested in
      `levels/test_movers.tmj`; greybox built ([LEVEL_DESIGN.md](LEVEL_DESIGN.md) section 7)
      with a replay test; **(owner)** play it. Then the art pass.
- [ ] Level 4, Network Hub: hazards. Greybox built 2026-10-05 (`levels/level_4.tmj`,
      [LEVEL_DESIGN.md](LEVEL_DESIGN.md) section 7) with a replay test; **(owner)** play it.
      Then the art pass.
- [ ] Level 5, Mainframe: the KERNEL boss. Design drafted 2026-10-05
      ([LEVEL_DESIGN.md](LEVEL_DESIGN.md) section 7, "Level 5 design");
      **(owner)** review it and answer its questions before it's built.

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

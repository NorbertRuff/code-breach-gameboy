# Roadmap

From the current prototype to the full 5-level game, playable in color on GBC and in
4 shades on DMG. Claude writes the code; the owner runs git, reviews, and plays it in
Emulicious and on hardware. Items marked **(owner)** need the owner.

## Status (2026-10-10)

- **Playable from start to end:** levels 1-4 (level 1 art pending; levels 2 and 3
  greyboxes, kept after the owner's passes; level 4's greybox new) and level 5, the KERNEL
  boss (greybox, placeholder face), then the ending and the credits; music and sound
  effects; passwords, the title menu, the level select and the game over menu; the story
  text. New since the owner's last check: the M7 menus and story, level 4, level 5 and the
  ending, and since 2026-10-10 the final player sprite (10 frames, GBC colors from the
  style guide), the bot, chaser, lift and packet sprites, and the HUD and font. Next: the owner's playthrough; the rest of the M5 art.
- **Tests:** `make test` 235 tests in about 16 s; `make test-full` 352 in about 21 s.
- **Memory:** bank 0 has 932 bytes free (KERNEL's packet actor and the boss and ending
  states are in it); WRAM uses 1,504 of 4,096 bytes.
- **Frame budget** (worst update, line of the 140 allowed; average): level 1 124 (79),
  level 2 108 (67), level 3 131 (90), level 4 107 (70), level 5 109 (72), greybox route
  122 (80). KERNEL's own update about 8 lines, up to 25 when it writes tiles. Lifts cost about 10 lines on
  average where they are; the music step about 9, up to 18; a streamed column's write 5.

## Bugs

Open bugs, numbered on from the audit's 22 ([archive/PROJECT_STATE.md](archive/PROJECT_STATE.md)).
A fixed bug leaves this list; its regression test keeps the number.

- None open.

## Waiting for the owner

- Hardware checks of M2 and M4 (streaming, one-way shelf, spikes, pit, respawn, head
  bumps; colors on GBC).
- Claude Design style guide (M5): palettes, the player, the enemies and the lift are in;
  the tilesets are next.
- The new bot, chaser, lift and packet on DMG and GBC (2 frames every 8 frames, the lift
  still): agree or change the **Proposed** enemy sprite decision (DECISIONS.md).
- The player's 10 frames on DMG and GBC (idle breathing, jump, fall, the hurt and death
  poses held 0.5 s on a hit), the white coat on DMG, red enemies and gold lifts on GBC.
- Try the M7 menus in Emulicious and on hardware: the title menu, a password from a level
  complete screen typed back in (`python tools/password.py` lists them), the level select,
  game over CONTINUE and QUIT, the boot screen and the elevator log screens. Agree or
  change the **Proposed** menu and story-screen decisions (DECISIONS.md).
- Play the level 4 greybox and its song ([LEVEL_DESIGN.md](LEVEL_DESIGN.md) section 7,
  "Level 4 brief": the open points).
- Play level 5 and the ending: the boss's feel and timings, its song "Terminate" and the
  ending's "Closing", the credits' text ([LEVEL_DESIGN.md](LEVEL_DESIGN.md) section 7,
  "Level 5 build notes": what changed from the design).

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
      the current assets (level tiles, band, HUD); the player and actors use the style
      guide's sprite palettes since 2026-10-10; the M5 art adds the rest.
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

- [ ] **(owner)** Claude Design style guide: palettes and character look. Palettes for
      all five levels in `graphics/palettes.json` (2026-10-05).
- [x] `tools/gbconvert.py` with previews ([ART_PIPELINE.md](ART_PIPELINE.md)). Built and
      tested 2026-10-04.
- [x] Player sprite with all animations: idle (2), run (4), jump, fall, hurt and death
      (2026-10-10). **(owner)** Check them on hardware.
- [x] Security bot, chaser, lift and KERNEL's packet sprites (2026-10-10). **(owner)**
      Check them on hardware.
- [ ] Server Room tileset with Tiled terrain sets. Swap it in for the greybox tileset.
- [ ] Security gate and exit elevator art.
- [x] HUD frame and font (2026-10-10). **(owner)** Check them on hardware; agree or change
      the **Proposed** HUD palette and HUD decisions (DECISIONS.md).
- [ ] Title screen from `apexLogo`. Remove `splashscreen.c`.
- [ ] Parallax band art for the Server Room (32×3 tiles, wraps).
- [ ] Replace every copied placeholder listed under Assets in [SOURCES.md](SOURCES.md):
      background. Claude Design for the pixel art.

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
      "Whirlwind", level 5 "Terminate", the ending "Closing" (2026-10-05, **(owner)**
      listen).
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
- [x] Ending and the in-game credits screen (2026-10-05, `story_ending` in
      `src/game_state/story.c`): "LOCKDOWN LIFTED", then the credits, then the title.

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
- [ ] Level 5, Mainframe: the KERNEL boss. Built 2026-10-05 from the approved design
      (`levels/level_5.tmj`, `src/game_state/kernel.c`, `tests/test_boss.py`);
      **(owner)** play it. Then the art pass (the face, the packet, the terminals).

## M9 — Polish and release

- [ ] **(owner)** Full playthrough on DMG and GBC.
- [x] No lag frames in any level, at most 10 sprites per line: each level's replay test
      (`tests/test_level_1.py` to `test_level_4.py`, `test_boss.py`) checks both.
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

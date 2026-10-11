# Roadmap

From the current prototype to the full 5-level game, playable in color on GBC and in
4 shades on DMG. Claude writes the code; the owner runs git, reviews, and plays it in
Emulicious and on hardware. Items marked **(owner)** need the owner.

## Status (2026-10-11)

- **Playable from start to end, in final art:** levels 1-5 (KERNEL, the boss, is level
  5), then the ending and the credits; music and sound effects; passwords, the title menu,
  the level select and the game over menu; the story text; the testing cheats (A + SELECT
  and A + B while paused). The owner played it end to end on 2026-10-11 and agreed the
  levels, difficulty, checkpoints, the level 3-5 art and KERNEL's face. Next: the release
  build and the rest of M9; the owner's hardware pass.
- **Visual Polish (2026-10-11, owner):** dual-palette HUD (chassis + ruby heart & gold floppy disk),
  distinct electric cyan chaser drone sprite palette, and multi-palette parallax band depth (Server Room glowing emerald LEDs).
- **Tests:** `make test` 244 tests in about 30 s; `make test-full` 366 in about 60 s (it builds `make release` first).
- **Memory:** bank 0 has 401 bytes free; WRAM uses 1,823 of 4,096 bytes.
- **Frame budget** (worst update on DMG, line of the 140 allowed; average): level 1 127
  (79), level 2 111 (67), level 3 132 (90; 139 on GBC, the tightest), level 4 110 (70),
  level 5 110 (72), greybox route 125 (80). KERNEL's own update about 8 lines, up to 25
  when it writes tiles. Lifts cost about 10 lines on average where they are; the music
  step about 9, up to 18; a streamed column's write 2 in VBlank, 4 with its GBC palettes.

## Bugs

Open bugs, numbered on from the audit's 22 ([archive/PROJECT_STATE.md](archive/PROJECT_STATE.md)).
A fixed bug leaves this list; its regression test keeps the number.

- None open.

## Waiting for the owner

- **Hardware pass on DMG and GBC** with `release/ApexLab.gb` (`make release`), the last
  check before the release (Emulicious
  playthrough done 2026-10-11): streaming, one-way shelves, spikes, pits, respawn, head
  bumps; every level's art and colors; the player's 10 frames, the bots, chasers, lifts
  and packets; the HUD and font; the title screen; a password typed back in
  (`python tools/password.py` lists them), the level select, game over CONTINUE and QUIT;
  KERNEL and the ending; the cheats.
- **The Proposed decisions left** in DECISIONS.md (the rest agreed 2026-10-11): the GBC
  palette layout (0-3 the level, 4 band base, 5 HUD chassis, 6 HUD accents, 7 band accent) and
  the HUD and font art.
- Optional: repaint or move props in Tiled with each level's terrain set
  (`graphics/tilesets/*.tsx`).

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
- Fixed pools for particles and projectiles were never needed: parked (M9, \"Parked ideas\").

## M4 — Game Boy Color support

Done when the game runs in color on GBC and unchanged on DMG. Steps in
[CGB_PLAN.md](CGB_PLAN.md) section 7. Can run alongside M2 and M3.

- [x] `src/engine/platform.c`: `is_cgb`. (M0)
- [x] `src/engine/palette.c`: load, fade, flash for both systems. No other file writes
      palette registers. Done 2026-10-04, `tests/test_palette.py`.
- [x] Load the exported palettes for background, HUD and sprites: the style guide's
      palettes for every level's tileset and band, the HUD, the player and the actors
      (2026-10-11).
- [x] Attribute maps behind `is_cgb`, including streamed columns. The band and HUD have
      theirs (2026-10-04); streamed columns a palette per tile (2026-10-10,
      `tests/test_palette.py`).
- [x] Brighter palette set when `_is_GBA` is set: cut (owner, 2026-10-11).
- [x] Invincibility shown with a pale sprite palette on GBC instead of blinking.
      Done 2026-10-04 (sprite palette 1).
- [x] Golden screenshots per mode. `tests/test_golden.py`, `tests/golden/`, rewritten by
      `make goldens` (2026-10-04).
- [ ] **(owner)** Check on DMG and GBC.

## M5 — Art pipeline and the Server Room art pass

Done when level 1 looks like the final game in both modes.

- [x] Claude Design style guide: palettes and character look. Palettes for all five
      levels in `graphics/palettes.json` (2026-10-05), all in use (2026-10-11).
- [x] `tools/gbconvert.py` with previews ([ART_PIPELINE.md](ART_PIPELINE.md)). Built and
      tested 2026-10-04.
- [x] Player sprite with all animations: idle (2), run (4), jump, fall, hurt and death
      (2026-10-10). **(owner)** Check them on hardware.
- [x] Security bot, chaser, lift and KERNEL's packet sprites (2026-10-10). **(owner)**
      Check them on hardware.
- [x] Server Room tileset with Tiled terrain sets, swapped in for the greybox tileset in
      level 1 (2026-10-10). **(owner)** Check it on hardware.
- [x] Security gate and exit elevator art (Server Room, 2026-10-10).
- [x] HUD frame and font (2026-10-10). **(owner)** Check them on hardware; agree or change
      the **Proposed** HUD palette and HUD decisions (DECISIONS.md).
- [x] Title screen redrawn from Gemini starter art with a blinking warning triangle
      (2026-10-10; `apexLogo.png` kept out of the build); `splashscreen.c`,
      `press_start.png` and the unused `background.png` and `Font.png` removed. The
      owner finished the programmer in Aseprite and agreed the title in Emulicious.
      **(owner)** Check it on hardware.
- [x] Parallax band art for the Server Room (32×3 tiles, wraps; 2026-10-10).
- [x] Replace every copied placeholder listed under Assets in [SOURCES.md](SOURCES.md)
      (2026-10-10: the last, `background.png`, deleted unused).

## M6 — Audio

- [x] **(owner)** Decide where the music comes from: Tronimal's Game Boy Music Packs #1
      and #2 (CC BY 4.0), decided 2026-10-02.
- [x] hUGEDriver in the build, songs in banked ROM (`make music`), one step per frame from
      the line interrupt below the band.
- [x] Sound effects: Tronimal GBFX/GBFX2 (`sounds/`, `make sounds`), our own player with
      a channel mute mask and priority over the music; sound test ROM and WAVs
      (`make soundtest`). Done 2026-10-04; owner approved the picks.
- [x] Music per level, title, game over and ending (both packs in `music/packs/`, all
      18 songs in the sound test). Level 4 \"Whirlwind\", level 5 \"Terminate\" and the
      ending's \"Closing\" heard in the owner's playthrough (2026-10-11).
- [x] `CREDITS.md` with every CC-BY item (title, author, link, license, changes); add
      each new item with it.

## M7 — Saves and game flow

- [x] Saves on the EverDrive clone: no save test ROM kept its count (owner,
      2026-10-04), so the game uses passwords (DECISIONS.md \"Saves\").
- [x] Passwords: one per level, shown at level complete (and game over), entered from
      CONTINUE on the title screen (2026-10-05, `src/game_state/progress.c`,
      `tools/password.py`, `tests/test_passwords.py`). **(owner)** Check them.
- [x] Level select after a password, game over with CONTINUE / QUIT, level complete with
      the fragments and the password (2026-10-05). A score beyond the fragment count isn't
      designed; add one only if wanted.
- [x] Story text: 3-line boot screen on a new game, \"PATCH n/5 COMPILED\" on level
      complete, the AI log on the elevator ride after levels 1-3 (2026-10-05,
      `src/game_state/story.c`). Log 4 comes with level 4, log 5 with the boss level (M8).
- [x] Ending and the in-game credits screen (2026-10-05, `story_ending` in
      `src/game_state/story.c`): \"LOCKDOWN LIFTED\", then the credits, then the title.

## M8 — Levels 2 to 5

One level at a time: the new mechanic, the brief, the greybox loop, the art pass, and a
replay test that finishes it.

- [x] Level 2, Code Testing Lab: one-way platforms, Down + A drops through, fast bots.
      Greybox done and kept after the owner's hardware pass ([LEVEL_DESIGN.md](LEVEL_DESIGN.md)
      section 7); art pass done 2026-10-10 (Code Testing Lab tileset and band).
      **(owner)** Check it on hardware.
- [x] Level 3, AI Research Wing: vertical movers and chasers. Lifts done 2026-10-04
      (`src/actor/lift.c`), chasers done (`src/actor/chaser.c`), both tested in
      `levels/test_movers.tmj`; greybox built ([LEVEL_DESIGN.md](LEVEL_DESIGN.md) section 7)
      with a replay test; owner played it (2026-10-11). Art pass done 2026-10-11 (AI
      Research Wing tileset and band). **(owner)** Check it on hardware.
- [x] Level 4, Network Hub: hazards. Greybox built 2026-10-05 (`levels/level_4.tmj`,
      [LEVEL_DESIGN.md](LEVEL_DESIGN.md) section 7) with a replay test; owner played it
      (2026-10-11). Art pass done 2026-10-11 (Network Hub). **(owner)** Check it on hardware.
- [x] Level 5, Mainframe: the KERNEL boss. Built 2026-10-05 from the approved design
      (`levels/level_5.tmj`, `src/game_state/kernel.c`, `tests/test_boss.py`);
      owner played it (2026-10-11). Art pass done 2026-10-11 (Mainframe tileset and band,
      KERNEL's face and terminals). Agreed (owner). **(owner)** Check it on hardware.

M8 is done: every level is built, has its art and a replay test, and the owner played
them (2026-10-11). The hardware checks are under \"Waiting for the owner\".

## M9 — Polish and release

- [ ] **(owner)** Full playthrough on DMG and GBC.
- [x] No lag frames in any level, at most 10 sprites per line: each level's replay test
      (`tests/test_level_1.py` to `test_level_4.py`, `test_boss.py`) checks both.
- [x] Difficulty and checkpoint tuning: the owner's playthrough found them good as built
      (2026-10-11).
- [x] The testing cheats stay in the release, entered with A held while paused
      (2026-10-11, owner).
- [x] Release build: `make release` builds `release/ApexLab.gb` with the intro and title
      on (`make` keeps skipping them for the tests); `make test-full` builds it and
      `tests/test_release.py` checks it stops at the title (2026-10-11).
- [x] Brighter palettes on GBA: cut (owner, 2026-10-11).
- [x] **Color pass on GBC (2026-10-11, owner):**
      - [x] **Dual-palette HUD:** Chassis uses Palette 5 (`#E0F8D0 #88C070 #346856 #081820`), Heart & Floppy accents use Palette 6 (`#E0F8D0 #F0A820 #D82828 #081820`).
      - [x] **Chaser enemy color differentiation:** Chasers use Sprite Palette 4 (electric cyan/teal `#D0F8F8 #2090A8 #082028`), standing out distinctly from red Patrol Bots (Palette 2).
      - [x] **Multi-palette parallax band depth:** Background Palette 7 assigned as `BAND_ACCENT_PALETTE`, with Server Room band featuring glowing emerald server rack LEDs.
- [ ] GitHub README (what it is, controls and cheats, how to build, playing on hardware,
      credits), screenshots, and the licence files: MIT for code, CC BY 4.0 for art
      (owner, 2026-10-11). Public on GitHub only.
- [ ] Asset check: every asset in SOURCES.md is original or licensed and credited;
      nothing copied from Mario or other commercial games.
- [ ] Write the final \"how it works\" document from DECISIONS.md and SOURCES.md: boot,
      main loop, tiles and sprites, banking, collision, physics, palettes and fades,
      parallax, audio, saves.

**Parked ideas** (not planned; the owner may pick them up): chasers that charge (faster
than the player once they see them); a live glitch flicker on KERNEL's face (about 1
line per frame); a red corrupted floor on GBC (a palette change or attribute writes);
a score beyond the fragment count; fixed pools for particles and projectiles (M3).

---

## Order and dependencies

```
M0 ──► M1 ──► M2 ──► M3 ──► M5 ──► M8 ──► M9
         │               ▲
         └──► M4 ────────┘      (color before the art pass)

             M3 ──► M6 ──► M8      (audio in parallel)
             M3 ──► M7 ──► M9      (saves and flow in parallel)
```

The save test in M0 runs early on purpose, so the cartridge type is settled before save
code is written.

# Roadmap

From the current prototype to the full 5-level game, playable in color on GBC and in
4 shades on DMG. Claude writes the code; the owner runs git, reviews, and plays it in
Emulicious and on hardware. Items marked **(owner)** need the owner.

## Status (2026-10-11): finished, version 1.0 released

- **Done:** levels 1-5 (KERNEL, the boss, is level 5), the ending and the
  credits, music and sound effects, passwords, the title menu, the level select, the
  game over menu, the story text, the cheats (A held + SELECT or B while paused), and the
  title screen demo. The owner played it on real hardware and closed the project
  (2026-10-11). All milestones M0-M9 are done.
- **Version 1.0** (2026-10-11, rebuilt 2026-11-07 with the MegaCorp story before the repo
  went public): the GitHub release `v1.0` with `ApexLab.gb`, published
  by CI from the tag; "Version 1.0" on the intro screen.
- **Tests:** `make test` 165 tests in about 13 s; `make test-full` 286 in about 18 s (it
  builds `make release` first).
- **Memory:** bank 0 has 382 bytes free (42 at its end, after the startup code); WRAM
  uses 1,831 of 4,096 bytes.
- **Frame budget** (worst update on DMG, line of the 140 allowed): level 1 127, level 2
  111, level 3 132 (134 on GBC, the tightest), level 4 110, level 5 110, greybox route
  125. KERNEL's own update about 8 lines, up to 25 when it writes tiles. Lifts cost about
  10 lines on average where they are; the music step about 9, up to 18; a streamed
  column's write 2 in VBlank, 4 with its GBC palettes.

## Bugs

Open bugs, numbered on from the audit's 22 (2026-10-02, in the git history). A fixed bug
leaves this list; its regression test keeps the number.

- None open.

## Open

- Nothing. Optional, if the project is picked up again: repaint or move props in Tiled
  with each level's terrain set (`graphics/tilesets/*.tsx`), and the parked ideas under
  M9.

The physics and level loading (M2) came before color (M4) and art (M5) on purpose: level
layouts depend on the jump, and levels are designed with plain blocks first.

---

## M0–M3 — Done (2026-10-02 to 2026-10-04)

- **M0, toolchain and test loop:** Makefile with MBC5 + autobank and banked assets, ROM
  layout checks, PyBoy harness, CI, save-persistence test ROM.
- **M1, bug fixes:** the audit's bugs fixed with regression tests; review pass and its
  quick wins (2026-10-03).
- **M2, physics and level loading:** sub-pixel physics with coyote time, jump buffer and
  corner correction; camera; tile collision; Tiled levels with `tools/tiled2c.py`; the
  collision ring and column streaming; `tools/levelpreview.py` with the reachability check.
  Jump metrics in [GAME_DESIGN.md](GAME_DESIGN.md), "Jump metrics".
- **M3, level 1 and gameplay objects:** actors, gates with quotas and checkpoints, the
  exit, HUD, pause and level-select cheat, parallax band and screen splits, lives rules,
  frame budget work, level 1 greybox (owner pass: keep as is), style pass.
- Fixed pools for particles and projectiles were never needed: parked (M9, "Parked ideas").

## M4 — Game Boy Color support

Done when the game runs in color on GBC and unchanged on DMG. Can run alongside M2 and M3.

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
      the HUD palette and HUD art.
- [x] Title screen redrawn from Gemini starter art with a blinking warning triangle
      (2026-10-10; `apexLogo.png` kept out of the build); `splashscreen.c`,
      `press_start.png` and the unused `background.png` and `Font.png` removed. The
      owner finished the programmer in Aseprite and agreed the title in Emulicious.
      **(owner)** Check it on hardware.
- [x] Parallax band art for the Server Room (32×3 tiles, wraps; 2026-10-10).
- [x] Replace every copied placeholder listed as copied in the asset records
      (2026-10-10: the last, `background.png`, deleted unused).

## M6 — Audio

- [x] **(owner)** Decide where the music comes from: Tronimal's Game Boy Music Packs #1
      and #2 (CC BY 4.0), decided 2026-10-02.
- [x] hUGEDriver in the build, songs in banked ROM (`make music`), one step per frame from
      the line interrupt below the band.
- [x] Sound effects: Tronimal GBFX/GBFX2 (`sounds/`, `make sounds`), our own player with
      a channel mute mask and priority over the music; a sound test ROM to pick
      them (removed at the release). Done 2026-10-04; owner approved the picks.
- [x] Music per level, title, game over and ending (picked from all 18 songs of both
      packs). Level 4 "Whirlwind", level 5 "Terminate" and the
      ending's "Closing" heard in the owner's playthrough (2026-10-11).
- [x] `CREDITS.md` with every CC-BY item (title, author, link, license, changes); add
      each new item with it.

## M7 — Saves and game flow

- [x] Saves on the EverDrive clone: no save test ROM kept its count (owner,
      2026-10-04), so the game uses passwords.
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

- [x] Level 2, Code Testing Lab: one-way platforms, Down + A drops through, fast bots.
      Greybox done and kept after the owner's hardware pass; art pass done 2026-10-10 (Code Testing Lab tileset and band).
      **(owner)** Check it on hardware.
- [x] Level 3, AI Research Wing: vertical movers and chasers. Lifts done 2026-10-04
      (`src/actor/lift.c`), chasers done (`src/actor/chaser.c`), both tested in
      `levels/test_movers.tmj`; greybox built with a replay test; owner played it (2026-10-11). Art pass done 2026-10-11 (AI
      Research Wing tileset and band). **(owner)** Check it on hardware.
- [x] Level 4, Network Hub: hazards. Greybox built 2026-10-05 (`levels/level_4.tmj`)
      with a replay test; owner played it
      (2026-10-11). Art pass done 2026-10-11 (Network Hub). **(owner)** Check it on hardware.
- [x] Level 5, Mainframe: the KERNEL boss. Built 2026-10-05 from the approved design
      (`levels/level_5.tmj`, `src/game_state/kernel.c`, `tests/test_boss.py`);
      owner played it (2026-10-11). Art pass done 2026-10-11 (Mainframe tileset and band,
      KERNEL's face and terminals). Agreed (owner). **(owner)** Check it on hardware.

M8 is done: every level is built, has its art and a replay test, and the owner played
them (2026-10-11). The hardware checks are under "Waiting for the owner".

## M9 — Polish and release

- [x] **(owner)** Full playthrough on a real GBC (2026-10-11). The DMG pass is under
      "Waiting for the owner".
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
      - [x] **Multi-palette parallax band depth:** Background Palette 7 assigned as `BAND_ACCENT_PALETTE` across all 5 levels (Level 1 Server Room emerald LEDs, Level 2 Code Testing Lab cyan oscilloscopes, Level 3 AI Research Wing violet neural pulses, Level 4 Network Hub amber router LEDs, Level 5 Mainframe crimson tape reels).
      - [x] **Retro green phosphor CRT terminal palette:** Used on GBC for boot screen and elevator logs (`palette_use_terminal()`).
- [x] GitHub README (`Readme.md`: what it is, controls and cheats, how to play and build,
      credits), screenshots in `screenshots/`, `LICENSE` (MIT, code) and `LICENSE-ART`
      (CC BY 4.0, art and levels) (2026-10-11).
- [x] Asset check (2026-10-11): every asset is the owner's own, redrawn by
      hand, or CC BY / public domain and credited; the player sprite is the owner's own
      work (owner). The old placeholder screenshot is gone.
- [x] The final "how it works" document: `docs/ARCHITECTURE.md` (2026-10-11); the
      decision and source records it was written from were then removed (owner).
- [x] Clean-up before the release (2026-10-11): dead code and helpers, the finished
      working docs (plans, research, level briefs, archive), the text-sketch tool, and
      repeated tests removed; 17 docs became 9. Then (owner): the decision and source
      records, the save test and sound test ROMs and their tools, the greybox art scripts,
      the music pack archives and the converters' unit tests removed; GAME_DESIGN.md moved
      into docs/ with the level-making guide; CLAUDE.md and .claude/ kept local; 6 files in docs/.
- [x] Demo on the title screen (2026-10-11, owner): a minute without a press plays a clip
      of the next level, recorded from the replay tests (`make demo`,
      `tests/test_demo.py`).

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

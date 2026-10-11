# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

**ApexLab: Code Breach** is a Game Boy platformer built with GBDK-2020: one ROM that runs
in color on Game Boy Color (GBC) and in 4 shades on the original Game Boy (DMG). Scope:
the 5-level game in `GAME_DESIGN.md`.

Docs are in `docs/` (index: `docs/README.md`). Start with the status at the top of
`docs/ROADMAP.md`; read `docs/DECISIONS.md` before changing anything decided there, and
`docs/ARCHITECTURE.md` for the part of the code you change.

**Workflow:** Claude writes the code. The owner runs git, reviews, and tests on a real DMG
and GBC (EverDrive-style flashcart). Don't run git commands that change state; leave
changes in the working tree and suggest a commit message.

**"Build it" means:** `make`, then `make run` (opens Emulicious). Don't launch Emulicious
otherwise. `make` works from PowerShell, cmd and Git Bash (recipes always run in `cmd.exe`).

## Toolchain

Not in git, installed by the owner: GBDK-2020 **4.5.0** in `gbdk/`, Emulicious in
`emulicious/` (Java 21), GNU Make 4.4.1, Python 3.13. Test tools in `.venv/` from
`make venv` (`tools/requirements.txt`). If `gbdk/` is missing, say so instead of guessing.

CLion runs `make clean` when the Makefile changes, so `obj/` can vanish: run `make` before
using `obj/` files or handing over a ROM.

Every build prints ROM and RAM use per bank and fails if a bank overflows. Bank 0 holds
the per-frame code and is tight (see the ROADMAP status); code that isn't per-frame goes
to a ROM bank (ARCHITECTURE.md, memory map).

## Commands

```bash
make            # build obj/ApexLab.gb (incremental)
make run        # build, then open the ROM in Emulicious
make release    # release/ApexLab.gb, the ROM to share and for the flashcart: intro and title on
make test       # quick tests, in parallel: DMG, plus GBC where the mode matters (~12 s)
make test-full  # every test in DMG and GBC mode, extra start times and seeds (before a commit; CI)
make shots      # screenshots and contact sheets of scripted playthroughs in tests/out/shots/
make goldens    # rewrite the golden screenshots after an intended visual change (look at them)
make budget     # screen lines per part of a gameplay update (tools/frame_budget.py)
make levels     # Tiled maps in levels/ -> src/levels/
make music      # hUGETracker songs in music/ -> src/music/
make sounds     # effect picks in sounds/effects.txt -> src/sound/effects.c
make soundtest  # obj/soundtest.gb with every GBFX/GBFX2 effect, and a WAV of each in tests/out/sounds/
make assets     # PNGs in graphics/ -> src/graphics/ (read png2asset's output for "not recognized")
make preview    # draw every level to tests/out/levels/ and check everything can be reached
make savetest   # the cartridge RAM test ROMs (make OBJDIR=flashcart savetest keeps them)
make usage, make rebuild, make clean, make venv
```

Generated files (`src/levels/`, `src/music/`, `src/graphics/`, `src/sound/effects.*`) are
committed with their sources; tests fail when they are out of date. For a one-off build variant use
`make OBJDIR=<scratch folder> EXTRA_CFLAGS=-D...`, never the normal `obj/`.

## Tests

`make test` after every change; `make test-full` before suggesting a commit (it also
builds `make release`). Details in
`docs/TESTING.md`. Mark a test `both_modes` only when DMG and GBC can differ (palettes,
screens, hardware); extra start times and seeds get `full`. A fixed bug gets a regression
test in `tests/test_regressions.py` that fails on the old code (`tools/test_against.py`);
an open bug can be an `xfail` test in `tests/test_known_bugs.py`. Don't add a test that
repeats what another one checks. Failing tests save screenshots to `tests/out/`.

## Records: one home per fact

- A decision and its reason: `docs/DECISIONS.md` (newest first, short).
- Where an idea, code or asset came from: `docs/SOURCES.md`; third-party assets also in
  `CREDITS.md`.
- What's done, open, and the current numbers (tests, bank 0, frame budget): `docs/ROADMAP.md`.
- How the code works: `docs/ARCHITECTURE.md`, updated only when the structure changes.
- Level briefs and greybox notes: `docs/LEVEL_DESIGN.md`.

Don't repeat a fact in a second doc; link to it. The final "how it works" document (M9) is
written from DECISIONS and SOURCES. No copied art may remain in the final game (SOURCES.md
lists what to replace). Old audits and reviews are in `docs/archive/`.

Project skills (`.claude/skills/`): `fix-bug`, `add-asset` (pixel art from Claude Design),
`close-milestone`. Before/after screenshots: `tools/shots.py --out <dir>` and
`tools/compare_shots.py`. Research on
techniques, licenses and tool flags: `docs/RESEARCH.md` (check a repo's license there
before copying code).

## Code style (owner's rule)

The owner walks through every file and must be able to say what each part does:
- **Names say what it does:** `update_camera`, `RESPAWN_INVINCIBILITY_FRAMES`. Units in
  the name or a short trailing comment (`// 1.5 s at 60 fps`).
- **One short comment above each function:** what it handles, in plain words.
- **Short walk-through comments** only where a step isn't obvious.
- **No long explanations in code:** reasons and sources go to DECISIONS and SOURCES.
- **Keep it short:** small functions, no dead or commented-out code, no duplication.

## Game Boy rules

- **No floats.** **No `/` or `%` in per-frame code** (SDCC's divide takes about 6 screen
  lines): shift, or count by subtraction.
- **Frame budget:** an update must end within 140 lines after VBlank starts (154 per frame).
  Check with `make budget` after anything that adds per-frame work.
- **VRAM:** writes only in VBlank or HBlank (GBDK's `set_bkg_tiles` waits for it, slowly).
- **10 sprites per line, 40 in total.** Tile and sprite allocations: ARCHITECTURE.md.
- **Banked data** is read inside `WITH_BANK` from bank 0 code only.
- **Tiny stack:** no recursion or large locals.
- **SDCC pitfall:** a `?:` passed straight as a function argument can be miscompiled; put
  it in a local. When code misbehaves for no visible reason, read `obj/<file>.asm`.
- **Window:** `move_win()` for absolute positions (`WX = 7` is the left edge).

## GBC + DMG rules (`docs/CGB_PLAN.md`)

- `is_cgb` (`src/engine/platform.h`) guards every VRAM bank 1 / attribute write: on DMG it
  would overwrite the tile map.
- Palettes go through the palette module only, `src/engine/palette.c` and
  `palette_effects.c` (`tests/test_palette.py` fails on any other write); it has both paths
  for loads, fades and flashes. `BGP_REG`/`OBP*_REG` do
  nothing on GBC; GBC palette registers do nothing on DMG.
- Every palette is ordered index 0 lightest → 3 darkest.
- No background tile flips (`png2asset -noflip`). Only VRAM bank 0 for tiles, 8 KB WRAM,
  no double speed.

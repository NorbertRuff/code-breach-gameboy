# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**ApexLab: Code Breach** is a Game Boy platformer built with [GBDK-2020](https://github.com/gbdk-2020/gbdk-2020). Target: one ROM that runs in color on Game Boy Color (GBC) and in 4 shades on the original Game Boy (DMG). Scope: the full 5-level game in `GAME_DESIGN.md`.

Planning docs live in `docs/` (index: `docs/README.md`). Read `docs/ROADMAP.md` for the current milestone and `docs/DECISIONS.md` before changing direction on anything listed there.

**Workflow:** Claude writes the code. The owner runs git (branches, commits, pushes, PRs), reviews, and tests on a real DMG and GBC (EverDrive-style flashcart). Don't run git commands that change state; leave changes in the working tree and suggest a commit message.

**"Build it" means:** run `make`, then `make run` to open the ROM in Emulicious. Don't launch Emulicious otherwise. `make` works the same from PowerShell, cmd and Git Bash (on Windows the Makefile always runs recipes with `cmd.exe`).

## Toolchain

The toolchain is **not** in git. `gbdk/` and `emulicious/` are gitignored and installed by the owner:

- GBDK-2020 **4.5.0** in `gbdk/` (the Makefile uses `./gbdk/bin/lcc` and `./gbdk/bin/png2asset`)
- Emulicious in `emulicious/` (needs Java; Java 21 is installed)
- GNU Make 4.4.1, Python 3.13/3.14
- Test tools in `.venv/` (gitignored), created by `make venv` from `tools/requirements.txt` (PyBoy 2.7.0, pytest)

If `gbdk/` is missing, the build cannot run. Say so instead of guessing.

The owner has the project open in CLion (code reading and commits). When the Makefile changes, CLion runs `make clean` before reloading the project, so `obj/` can vanish. That's expected: run `make` before using `obj/` files, and before handing over a ROM to flash.

Every build prints ROM and RAM use per bank (`romusage -g -R`; the build fails if a bank overflows). Bank 0 holds all code and must keep `_HOME` below `0x4000`; as of 2026-10-03 it has 3,592 bytes free.

## Build Commands

```bash
make            # build obj/ApexLab.gb (incremental; header changes are tracked)
make rebuild    # clean, then build
make run        # build, then open the ROM in Emulicious (Java)
make test       # build, then run the PyBoy tests in DMG and GBC mode
make shots      # build, then a scripted playthrough: screenshots, contact sheets and the level map in tests/out/shots/
make venv       # create .venv with the test tools (once)
make savetest   # build obj/savetest.gb, the cartridge RAM persistence test
make usage      # ROM and RAM use per bank, with areas
make assets     # regenerate src/graphics/*.c from graphics/*.png (and draw the greybox tileset)
make levels     # regenerate src/levels/*.c from the Tiled maps in levels/*.tmj
make preview    # draw every level to tests/out/levels/ and check everything can be reached
make clean      # delete obj/
```

`RELEASE=1` adds size-optimizing compilation (slower). The cartridge is MBC5 + RAM + battery with a GBC-enhanced header (`-Wm-yc`); flags are in the Makefile and `docs/CGB_PLAN.md`. The build writes `obj/ApexLab.sym`, which the tests use to read globals by name.

**Tests** (`tests/`, details in `docs/TESTING.md`): run `make test` after every change. A fixed bug gets a regression test in `tests/test_regressions.py` that fails on the old code (check it with `APEXLAB_OBJ`, see `docs/TESTING.md`). An open bug can be written as a test marked `xfail` in `tests/test_known_bugs.py`; fixing the bug makes that test pass, which fails the run until the test moves to `test_regressions.py`. For a one-off build variant use `make OBJDIR=<scratch folder> EXTRA_CFLAGS=-D...`, never the normal `obj/`. Failing tests save screenshots to `tests/out/`; look at them.

**Level pipeline** (`docs/LEVEL_PIPELINE.md`): maps are made in Tiled and saved as `levels/*.tmj` (13 rows tall = screen rows 3–15). `make levels` runs `tools/tiled2c.py`, which checks the rules and writes banked `LevelData` (`src/level/level_data.h`) to `src/levels/`. Commit the `.tmj` and the generated files together; `tests/test_tiled2c.py` fails when they are out of date.

**Asset pipeline**: PNG files in `graphics/` are converted to GBDK-compatible C source files via `png2asset`. Run `make assets` after modifying any PNG, then commit both the PNG and the regenerated `.c`/`.h` in `src/graphics/`. Assets use `-b 255` (autobank). png2asset only warns about unknown options and still succeeds, so read its output for `not recognized`.

**Vision and records:** the goal is a cozy, polished game that the owner can explain end to end (`GAME_DESIGN.md`, Vision). Record every decision in `docs/DECISIONS.md`. When code or an asset takes an idea from a tutorial, repo, doc or person, add a row to `docs/SOURCES.md` in the same change. The final "how it works" document is written from these two in M9. No copied art (Mario, mushrooms) may remain in the final game; SOURCES.md lists what must be replaced. Use Claude Design for pixel art when needed.

**Project skills** (`.claude/skills/`): `fix-bug` (test-first fix, proven on the old build), `add-asset` (art from Claude Design to banked C data), `close-milestone` (verify, update records, hardware checklist). Use them for those jobs. Helper tools: `tools/test_against.py <commit> [pytest args]` runs today's tests against an older commit's build; `tools/shots.py --out <dir>` plus `tools/compare_shots.py <before> <after>` compare screenshots. `.claude/settings.json` allows make, the venv Python, romusage/png2asset and read-only git, and denies git commands that change state.

Research on techniques, reference code, licensed assets and tool flags is in `docs/RESEARCH.md`. Check a reference repo's license there before copying code: several have none (ideas only). The planned Claude Design → `tools/gbconvert.py` → png2asset flow is in `docs/ART_PIPELINE.md`.

## Code Style (owner's rule)

The owner walks through every file and must be able to say what each part does, so the
code explains itself and stays short:
- **Names say what it does:** `update_camera`, `spawn_enemies`, `RESPAWN_INVINCIBILITY_FRAMES`.
  Units go in the name or a short trailing comment (`// 1.5 s at 60 fps`).
- **One short comment above each function:** what it handles, in plain words.
- **Short walk-through comments** inside a function only where a step isn't obvious, like
  `// Have we jumped? Only on a new press of A.`
- **No long explanations in code.** Reasons, history and sources go to `docs/DECISIONS.md`
  and `docs/SOURCES.md`.
- **Keep it short:** small functions, no dead or commented-out code, no duplication.

## Architecture

### Game State Machine (`src/main.c`)

The main loop is a state machine with these states (defined in `src/utils.h`):
- `INTRO_CREDITS_STATE` → `TITLE_SCREEN_STATE` → `GAMEPLAY_START` → `COREGAMELOOP` → `GAMEPLAY_END` / `LEVEL_COMPLETE`

The per-frame gameplay code is `main_game_loop()` in `main.c`. Global state: in `main.c` `joypad_current`, `joypad_previous`, `current_game_state`, `lives`, `Player player`, `Enemy enemies[MAX_ENEMIES]`; in `level_manager.c` `Level current_level` and `level_index` (the level `GAMEPLAY_START` loads); in `camera.c` `camera_x`.

### Module Breakdown

| Directory | Purpose |
|---|---|
| `src/game_state/` | `prepare_gameboy`, `intro_credits`, `start_screen`, `game_over` (also `level_complete`) |
| `src/engine/` | `platform.c` (`is_cgb`, GBC setup at boot), `banking.h` (`WITH_BANK`, `WITH_ROM_BANK` for banked data), `camera.c` (deadzone, look-ahead) |
| `src/player/` | `Player` struct + physics: `player_update` (run, jump, gravity, tile collision, pickups, animation), `player_draw` |
| `src/input/` | Joypad polling; `input_read_player` copies the d-pad and A into the player's input fields |
| `src/level/` | `level_data.h` (the ROM level format), `level_manager.c` (`level_load`, the `Level` state, HUD drawing), `level_stream.c` (collision ring, column streaming, fragments) |
| `src/enemy/` | Patrol enemies: update, draw (2 OAM entries each, from OAM 4), AABB hit test |
| `src/sound/` | Sound effects written straight to the APU registers |
| `src/graphics/` | Generated asset files from `make assets` (plus the older hand-made `splashscreen.c`), all banked |
| `src/levels/` | Generated level data and tileset collision tables from `make levels`, all banked |
| `graphics/` | Source PNG files for the asset pipeline; `graphics/tilesets/` also holds the Tiled `.tsx` files |
| `levels/` | Tiled maps (`.tmj`) |
| `tests/` | PyBoy tests (`gamelib.py` helpers, `conftest.py` fixtures) |
| `tools/` | `tiled2c.py` (levels), `levelpreview.py` + `physics.py` (level preview and reachability check), `greybox_tileset.py`, screenshot tools, `requirements.txt` for the test tools, `savetest/` save-persistence ROM |

### Player Physics (`src/player/player.h`, `src/player/player.c`)

Physics uses integer arithmetic (no floats — Game Boy has no FPU). Values and their sources are in `player.h` and `docs/RESEARCH.md` 2.2.
- Positions `x`, `y` are `uint16_t` in 1/32 px (`TO_PIXELS(pos)` = `pos >> 5`, tile = `pos >> 8`). `x` is the body's horizontal center, `y` its lowest row; the body box is 16×32 px (`SPRITE_LEFT_W`, `SPRITE_RIGHT_W`, `SPRITE_HEAD_OFFSET`).
- Velocities are `int8_t` in 1/32 px per frame. Running accelerates toward `input_x * RUN_SPEED_MAX`, weaker in the air.
- Jump: `JUMP_SPEED` up, light gravity while rising with A held, strong gravity when falling or after A is released (jump cut), half gravity near the top (apex hang). `coyote_frames` and `jump_buffer_frames` are one-byte countdowns.
- Collision is checked per axis against the tiles the box enters: walls over the full body height, floors and one-way tops only when the feet enter a new tile row going down, ceilings with up to `CORNER_CORRECTION` px of sideways slide. Moves stay under 8 px per frame, so no tile is skipped.
- **`tools/physics.py` is a frame-exact copy of this file** for the level preview. Change both together; `tests/test_levelpreview.py` compares them frame by frame.
- `player_draw` places the metasprite so it covers the box: pivot = box top-left + `programmer_PIVOT_X/Y` + `DEVICE_SPRITE_PX_OFFSET_X/Y`.

### Level Collision and Streaming (`src/level/`)

Levels come from Tiled (`levels/*.tmj` → `src/levels/*.c`), 13 map rows drawn at screen rows 3–15. `collision_ring[32 * 16]` in WRAM holds the collision of the 32 background-map columns, indexed `RING_INDEX(tile_x, tile_y)` with `tile_y` in screen rows; `level_stream_update(camera_x)` draws each column that enters the view (both directions) and fills its ring column. Tile types: `COLLISION_NONE`, `COLLISION_SOLID`, `COLLISION_ONE_WAY`, `COLLISION_HAZARD`, `COLLISION_COLLECTIBLE` (SOLID blocks, ONE_WAY blocks from above, HAZARD costs a life, COLLECTIBLE is a fragment). `LEVEL_COLLISION_AT(level, tile_x, tile_y)` (a macro, pass plain variables) returns a tile's type; outside the level or in a HUD row it is `COLLISION_NONE`. Collected fragments are a bitset, so they stay gone when their column is drawn again.

### Camera

`camera_update()` (`src/engine/camera.c`) keeps the focus (player x + a look-ahead that eases to 24 px in the running direction) within 8 px of screen x 80, clamped to the level; `camera_reset()` snaps it for a level start or respawn, followed by `level_stream_reset()`. `move_bkg()` applies `camera_x`; the player and enemies are drawn at `world_x - camera_x`. Levels are up to 255 tiles wide.

**Frame budget:** the game loop must finish before VBlank on DMG. `tests/test_streaming.py` checks for lag frames; as of M2 a normal frame ends near screen line 88 of 154, a frame that draws a column near line 125.

## Development Feature Flags (`src/utils.h`)

| Flag | Value | Effect |
|---|---|---|
| `SKIP_INTRO_AND_START` | `1` | Skip intro/title screens, jump straight to gameplay. The tests assume `1`. |

## Game Boy Constraints to Keep in Mind

- **No floats**: all physics math uses `int8_t`/`int16_t`/`uint8_t`.
- **VRAM timing**: tile/map writes must happen during VBlank or HBlank. Use `wait_vbl_done()` at end of main loop (already in place).
- **Shared tile memory**: GBDK uses `0x8800` addressing for background tiles, so background tiles 128–255 and sprite tiles 128–255 are the same memory.
- **Sprite limit**: 10 sprites per scanline, 40 total OAM slots. The player uses 8×16 metasprites (`SPRITES_8x16` mode), 4 OAM entries.
- **ROM banking**: assets are made with `png2asset -b 255`, so autobank puts them in a ROM bank (`BANKREF(name)`). Code reads them inside `WITH_BANK(name, ...)` from `src/engine/banking.h`, one asset per `WITH_BANK`. That only works from code in bank 0 (no `#pragma bank` in the file); all game code is in bank 0 for now. Metasprite tables count as banked data too: draw them inside `WITH_BANK`.
- **Stack is tiny**: avoid deep recursion and large stack allocations.
- **Window position**: `scroll_win()` is relative and takes `int8_t`; use `move_win()` for absolute positions. `WX = 7` is the left screen edge.

## GBC + DMG Rules (from `docs/CGB_PLAN.md`)

- Use `is_cgb` from `src/engine/platform.h` (set once at boot from `_cpu == CGB_TYPE`). Every VRAM bank 1 / attribute write must be guarded: on DMG, `VBK_REG` is ignored and the write would overwrite the tile map.
- Until the palette module (M4), `platform_init()` loads gray GBC palettes that match the DMG shades, and fades/flashes only work on DMG.
- `BGP_REG`/`OBP0_REG`/`OBP1_REG` do nothing in GBC mode, and GBC palette registers do nothing on DMG. Palette changes, fades and flashes need both paths.
- Every palette is ordered index 0 lightest → index 3 darkest, so DMG shades come out right.
- No background tile flips (DMG can't flip background tiles): backgrounds use `png2asset -noflip`.
- Only VRAM bank 0 for tiles and 8 KB of WRAM. No double-speed mode: game logic must fit in one DMG frame.

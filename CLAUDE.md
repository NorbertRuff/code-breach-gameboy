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

Every build prints ROM and RAM use per bank (`romusage -g -R`; the build fails if a bank overflows). Bank 0 holds the per-frame code and must keep `_HOME` below `0x4000`; as of 2026-10-04 it has 4,758 bytes free. SDCC's code is bulky (M3 step 3 alone took about 1.3 KB), so code that isn't per-frame moves to a ROM bank (see ROM banking below).

## Build Commands

```bash
make            # build obj/ApexLab.gb (incremental; header changes are tracked)
make rebuild    # clean, then build
make run        # build, then open the ROM in Emulicious (Java)
make test       # build, then run the PyBoy tests in DMG and GBC mode
make shots      # build, then scripted playthroughs (greybox level, level 1): screenshots, contact sheets and the level map in tests/out/shots/
make goldens    # build, then rewrite the golden screenshots in tests/golden/ (after an intended visual change; look at them)
make budget     # build, then print the screen lines each part of a gameplay update takes (tools/frame_budget.py)
make venv       # create .venv with the test tools (once)
make savetest   # build obj/savetest.gb, the cartridge RAM persistence test, plus MBC1/MBC3 variants
make usage      # ROM and RAM use per bank, with areas
make assets     # regenerate src/graphics/*.c from graphics/*.png (and draw the greybox tileset and band)
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
- `COREGAMELOOP` ↔ `PAUSED` on START ("PAUSE" in sprites over the parallax band). While paused, SELECT loads the next level (`LEVEL_SELECT_CHEAT`).
- `GAMEPLAY_END` (a life lost) restarts the current section behind a fade (`restart_behind_fade`), and on game over first refills the lives and puts the section's fragments back. `LEVEL_COMPLETE` (the exit reached) shows the fragment summary (`level_complete` in `game_over.c`), then reloads the level for now.

The per-frame gameplay code is `main_game_loop()` in `main.c`. Global state: in `main.c` `joypad_current`, `joypad_previous`, `current_game_state`, `lives`, `lag_frames`, `Player player`; in `actor.c` `actors[]` and `actor_count`; in `level_manager.c` `Level current_level` and `level_index` (the level `GAMEPLAY_START` loads: `LEVEL_1`, but the tests start in `LEVEL_GREYBOX_TEST`); in `camera.c` `camera_x`.

### Module Breakdown

| Directory | Purpose |
|---|---|
| `src/game_state/` | `prepare_gameboy`, `intro_credits`, `start_screen`, `game_over` (also `level_complete`), `pause` (the label) |
| `src/engine/` | `platform.c` (`is_cgb`, GBC setup at boot), `palette.c` (every palette write: fades and flashes on DMG and GBC), `text.c` (screen text in the game's font; no `printf`), `banking.h` (`WITH_BANK`, `WITH_ROM_BANK` for banked data), `camera.c` (deadzone, look-ahead, left limit), `screen.c` (line interrupts: band and level scroll, sprites off over the HUD) |
| `src/player/` | `Player` struct + physics: `player_update` (run, jump, gravity, tile collision, pickups, animation), `player_draw` |
| `src/input/` | Joypad polling; `input_read_player` copies the d-pad and A into the player's input fields |
| `src/level/` | `level_data.h` (the ROM level format), `level_manager.c` (`level_load`, the `Level` state, HUD drawing), `level_stream.c` (collision ring, column streaming, fragments, gates and the exit drawn into columns), `section.c` (quotas, opening gates, checkpoints) |
| `src/actor/` | `actor.c`: actors spawned from the level's entity list, the type table, update, hit test, drawing; `bot.c`: the patrol bot type |
| `src/sound/` | Sound effects written straight to the APU registers |
| `src/graphics/` | Generated asset files from `make assets` (plus the hand-made `splashscreen.c`, `hud_digits.c` and `font.c`), all banked |
| `src/levels/` | Generated level data and tileset collision tables from `make levels`, all banked |
| `graphics/` | Source PNG files for the asset pipeline; `graphics/tilesets/` also holds the Tiled `.tsx` files |
| `levels/` | Tiled maps (`.tmj`) |
| `tests/` | PyBoy tests (`gamelib.py` helpers, `conftest.py` fixtures) |
| `tools/` | `tiled2c.py` (levels), `sketch2tmj.py` (level text sketch ↔ Tiled map), `gbconvert.py` (art → png2asset-ready PNG), `levelpreview.py` + `physics.py` (level preview and reachability check), `greybox_tileset.py`, screenshot tools, `requirements.txt` for the test tools, `savetest/` save-persistence ROM |

### Player Physics (`src/player/player.h`, `src/player/player.c`)

Physics uses integer arithmetic (no floats — Game Boy has no FPU). Values and their sources are in `player.h` and `docs/RESEARCH.md` 2.2.
- Positions `x`, `y` are `uint16_t` in 1/32 px (`TO_PIXELS(pos)` = `pos >> 5`, tile = `pos >> 8`). `x` is the body's horizontal center, `y` its lowest row; the body box is 16×32 px (`SPRITE_LEFT_W`, `SPRITE_RIGHT_W`, `SPRITE_HEAD_OFFSET`).
- Velocities are `int8_t` in 1/32 px per frame. Running accelerates toward `input_x * RUN_SPEED_MAX`, weaker in the air.
- Jump: `JUMP_SPEED` up, light gravity while rising with A held, strong gravity when falling or after A is released (jump cut), half gravity near the top (apex hang). `coyote_frames` and `jump_buffer_frames` are one-byte countdowns.
- Collision is checked per axis against the tiles the box enters: walls over the full body height, floors and one-way tops only when the feet enter a new tile row going down, ceilings with up to `CORNER_CORRECTION` px of sideways slide. Moves stay under 8 px per frame, so no tile is skipped.
- Collision works in tile coordinates: a position's tile is its high byte (`LEFT_COLUMN`, `RIGHT_COLUMN`, `FEET_ROW`, `head_row`), and `column_has` walks one collision ring column with a pointer.
- **`tools/physics.py` is a frame-exact copy of this file** for the level preview. Change both together; `tests/test_levelpreview.py` compares them frame by frame.
- `player_draw` places the metasprite so it covers the box: pivot = box top-left + `programmer_PIVOT_X/Y` + `DEVICE_SPRITE_PX_OFFSET_X/Y`.

### Level Collision and Streaming (`src/level/`)

Levels come from Tiled (`levels/*.tmj` → `src/levels/*.c`), 13 map rows drawn at screen rows 3–15. `collision_ring[32 * 16]` in WRAM holds the collision of the 32 background-map columns, indexed `RING_INDEX(tile_x, tile_y)` with `tile_y` in screen rows; `level_stream_update(camera_x)` draws each column that enters the view (both directions) and fills its ring column. Tile types: `COLLISION_NONE`, `COLLISION_SOLID`, `COLLISION_ONE_WAY`, `COLLISION_HAZARD`, `COLLISION_COLLECTIBLE`, `COLLISION_EXIT` (SOLID blocks, ONE_WAY blocks from above, HAZARD costs a life, COLLECTIBLE is a fragment, EXIT completes the level). `LEVEL_COLLISION_AT(level, tile_x, tile_y)` (a macro, pass plain variables) returns a tile's type; outside the level or in a HUD row it is `COLLISION_NONE`. Collected fragments are a bitset, so they stay gone when their column is drawn again.

Fragments, gates and the exit are tile objects in Tiled, so `draw_column` puts their marker tiles over the map (a closed gate fills its column from its bottom row up to row 3), and the marker tiles get their object's collision. `current_level` holds the play state: `gates[]` (column, bottom row, quota, rows opened), `section` (0 = A), fragments per section and collected per section. `section.c` opens the current section's gate when the player walks into it with the quota: one row every 2 frames, the next section starts, the camera gets a left limit just past the gate, and a restart puts the player just right of it.

### Actors (`src/actor/`)

Moving level objects (the patrol bots) are actors; the player is not. `actors_spawn(camera_x)` reads the level's entities in x order with a cursor and spawns each one whose type has an `ActorType` once it is within 16 px of the screen's right edge. Each `ActorType` has `spawn`, `update`, a metasprite (pivot = the actor's body center and feet row), its sprite tiles and a hit box. Actors stay packed in `actors[0..actor_count-1]`; one more than a screen behind the camera is removed and comes back only with `actors_reset(section)` (level start, losing a life). The sprite list is rebuilt every frame in `draw_sprites` (`main.c`): the player first (OAM 0–3), then the actors, then `hide_sprites_range`.

### Camera

`camera_update()` (`src/engine/camera.c`) keeps the focus (player x + a look-ahead that eases to 24 px in the running direction) within 8 px of screen x 80, clamped to the level and to `camera_left_limit`; `camera_reset()` snaps it for a level start or respawn, followed by `level_stream_reset()`. `camera_lock_left(x)` sets a new left limit (an opened gate) that takes force once the camera gets there; the player can't go left of it either. Teleport the player (tests) only forward past an opened gate: columns behind the limit aren't drawn. `move_bkg()` applies `camera_x`; the player and enemies are drawn at `world_x - camera_x`. Levels are up to 255 tiles wide.

**Screen splits** (`src/engine/screen.c`, started at boot): the VBlank handler sets `SCX` for the parallax band (half the camera) and turns sprites on; a line interrupt at line 23 sets the level's `SCX` (the camera), and one at line 127 turns sprites off over the HUD, each after waiting for HBlank. Never write `SCX` or `HIDE_SPRITES`/`SHOW_SPRITES` directly: use `screen_scroll(camera_x)` (applied at the next VBlank) and `screen_show_sprites(show)`. The band is drawn once into rows 0–2 at level load.

**Frame budget:** each gameplay update must end before the next VBlank. The ROM counts lag frames in `lag_frames` (`main.c`); `tests/test_frame_budget.py` plays a route through the greybox level and allows an update to end at most 140 lines after VBlank starts, of 154 (`game.track_frame_lines()`). As of M3 step 4 an update ends near line 71, a fragment pickup near 87, one that draws a column near 104; the worst on the route (a pickup in a column update) at 124. On 2026-10-04 three savings (DECISIONS.md) brought the average to about 67 and the worst on both the greybox route and level 1's replay (`tests/test_level_1.py`) to 123. `make budget` shows where the lines go; `.venv/Scripts/python.exe tools/frame_budget.py --route level1` profiles level 1.

## Development Feature Flags (`src/utils.h`)

| Flag | Value | Effect |
|---|---|---|
| `SKIP_INTRO_AND_START` | `1` | Skip intro/title screens, jump straight to gameplay. The tests assume `1`. |
| `LEVEL_SELECT_CHEAT` | `1` | SELECT while paused loads the next level, test rooms included. |

## Game Boy Constraints to Keep in Mind

- **No floats**: all physics math uses `int8_t`/`int16_t`/`uint8_t`.
- **No `/` or `%` in per-frame code**: there is no divide instruction, and SDCC's routine takes about 6 screen lines per call. Shift for powers of two, or count by subtraction.
- **VRAM timing**: tile/map writes must happen during VBlank or HBlank. Use `wait_vbl_done()` at end of main loop (already in place).
- **Shared tile memory**: GBDK uses `0x8800` addressing for background tiles, so background tiles 128–255 and sprite tiles 128–255 are the same memory. Background tiles in use: the level's from 0, the HUD frame from 64, the HUD digits 133–144, the font 160–207.
- **Sprite limit**: 10 sprites per scanline, 40 total OAM slots. The player uses 8×16 metasprites (`SPRITES_8x16` mode), 4 OAM entries; a bot uses 2. Sprite tiles: the player 0–39, the bot 40–43, the pause label 44–53.
- **ROM banking**: assets are made with `png2asset -b 255`, so autobank puts them in a ROM bank (`BANKREF(name)`). Code reads them inside `WITH_BANK(name, ...)` from `src/engine/banking.h`, one asset per `WITH_BANK`. That only works from code in bank 0 (no `#pragma bank` in the file). Code that never runs in a gameplay frame and doesn't use `WITH_BANK` lives in ROM banks: `#pragma bank 255` at the top of the file and `BANKED` on its functions (in the header too). So far: `intro_credits.c`, `game_over.c`, `pause.c`, `prepare_gameboy.c`, and the hand-made HUD digit tiles (`src/graphics/hud_digits.c`). Metasprite tables count as banked data too: draw them inside `WITH_BANK`.
- **Stack is tiny**: avoid deep recursion and large stack allocations.
- **SDCC pitfall**: a `?:` passed straight as a function argument can be miscompiled (DECISIONS.md, `head_row`). Put it in a local or a function. When code misbehaves for no visible reason, read `obj/<file>.asm`.
- **Window position**: `scroll_win()` is relative and takes `int8_t`; use `move_win()` for absolute positions. `WX = 7` is the left screen edge.

## GBC + DMG Rules (from `docs/CGB_PLAN.md`)

- Use `is_cgb` from `src/engine/platform.h` (set once at boot from `_cpu == CGB_TYPE`). Every VRAM bank 1 / attribute write must be guarded: on DMG, `VBK_REG` is ignored and the write would overwrite the tile map.
- Palettes go through `src/engine/palette.c` only (`tests/test_palette.py` fails on any other write): `palette_set_bkg`/`palette_set_sprite` (GBC colors, kept in WRAM), `palette_fade_out`/`palette_fade_in`/`palette_fade_to` and `palette_flash`, on both systems. Every palette starts as the DMG grays; the art's palettes come with M4 step 4. It stays in bank 0: it copies colors out of the assets' banks. GBC sprite palettes: 0 the player (and for now the bots), 1 the player while invincible (pale copy; DMG blinks instead). GBC background palettes: 0 the level (`LEVEL_PALETTE`), 1 the band, 2 the HUD; `palette_use_bkg`/`palette_use_win` set the attributes (GBC only).
- `BGP_REG`/`OBP0_REG`/`OBP1_REG` do nothing in GBC mode, and GBC palette registers do nothing on DMG. Palette changes, fades and flashes need both paths: `src/engine/palette.c` has them.
- Every palette is ordered index 0 lightest → index 3 darkest, so DMG shades come out right.
- No background tile flips (DMG can't flip background tiles): backgrounds use `png2asset -noflip`.
- Only VRAM bank 0 for tiles and 8 KB of WRAM. No double-speed mode: game logic must fit in one DMG frame.

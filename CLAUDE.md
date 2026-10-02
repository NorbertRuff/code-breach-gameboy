# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**ApexLab: Code Breach** is a Game Boy platformer built with [GBDK-2020](https://github.com/gbdk-2020/gbdk-2020). Target: one ROM that runs in color on Game Boy Color (GBC) and in 4 shades on the original Game Boy (DMG). Scope: the full 5-level game in `GAME_DESIGN.md`.

Planning docs live in `docs/` (index: `docs/README.md`). Read `docs/ROADMAP.md` for the current milestone and `docs/DECISIONS.md` before changing direction on anything listed there.

**Workflow:** Claude writes the code. The owner runs git (branches, commits, pushes, PRs), reviews, and tests on a real DMG and GBC (EverDrive-style flashcart). Don't run git commands that change state; leave changes in the working tree and suggest a commit message.

**"Build it" means:** run `make`, then `make run` to open the ROM in Emulicious. Run make from PowerShell: the Makefile uses cmd syntax on Windows and fails under Git Bash's `sh`.

## Toolchain

The toolchain is **not** in git. `gbdk/` and `emulicious/` are gitignored and installed by the owner:

- GBDK-2020 **4.5.0** in `gbdk/` (the Makefile uses `./gbdk/bin/lcc` and `./gbdk/bin/png2asset`)
- Emulicious in `emulicious/` (needs Java; Java 21 is installed)
- GNU Make 4.4.1, Python 3.13/3.14

If `gbdk/` is missing, the build cannot run. Say so instead of guessing.

**ROM bank 0 is nearly full** (85 bytes free as of 2026-10-02). Check `./gbdk/bin/romusage obj/ApexLab.gb -g` after adding code. New code may not fit until the MBC5 + autobank change in roadmap M0.

## Build Commands

```bash
# Full build (clean + compile)
make

# Run in Emulicious emulator (requires Java)
make run

# Regenerate asset .c files from PNG sources
make assets

# Clean build artifacts
make clean
```

The compiled ROM is output to `obj/ApexLab.gb`. `make all` runs `clean` then `prepare` then links — always a fresh build.

Planned in milestone M0 (not there yet): `make test` (PyBoy tests in DMG and GBC mode, see `docs/TESTING.md`), GBC header flags, MBC5 cartridge, and `.sym` output.

**Asset pipeline**: PNG files in `graphics/` are converted to GBDK-compatible C source files via `png2asset`. Run `make assets` after modifying any PNG, then commit both the PNG and the regenerated `.c`/`.h` in `src/graphics/`. **Known broken (fix in M0):** the `assets` target passes `-bank`, `-org` and `-name`, which png2asset ignores with only a warning, so nothing is banked. The real banking option is `-b 255`. Always check png2asset output for `not recognized` warnings.

Research on techniques, reference code, licensed assets and tool flags is in `docs/RESEARCH.md`. Check a reference repo's license there before copying code: several have none (ideas only). The planned Claude Design → `tools/gbconvert.py` → png2asset flow is in `docs/ART_PIPELINE.md`.

## Architecture

### Game State Machine (`src/main.c`)

The main loop is a state machine with these states (defined in `src/utils.h`):
- `INTRO_CREDITS_STATE` → `TITLE_SCREEN_STATE` → `GAMEPLAY_START` → `COREGAMELOOP` → `GAMEPLAY_END` / `LEVEL_COMPLETE`

The per-frame gameplay code is `main_game_loop()` in `main.c`. Global state lives in `main.c`: `joypad_current`, `joypad_previous`, `total_scroll_x` (camera X), `current_game_state`, `lives`, and singleton instances `Player player`, `LevelManager level_manager`, `Enemy enemies[MAX_ENEMIES]`.

### Module Breakdown

| Directory | Purpose |
|---|---|
| `src/game_state/` | `prepare_gameboy`, `intro_credits`, `start_screen`, `game_over` (also `level_complete`). `prepare_core_game_loop.c` is excluded from the build; `core_game_loop.c` is an empty stub. |
| `src/player/` | `Player` struct + physics: `player_handle_movement_physics`, `player_handle_jumping_physics`, `player_draw`, collision and collectible checks |
| `src/input/` | Joypad polling; turns button state into player movement and jump forces |
| `src/level/` | `LevelManager` + `Level` structs, tile collision map (18 rows × 32 columns), level 1 built in code (`level_1_server_room_init`), HUD drawing |
| `src/enemy/` | Patrol enemies: update, draw (2 OAM entries each, from OAM 4), AABB hit test |
| `src/sound/` | Sound effects written straight to the APU registers |
| `src/graphics/` | Generated asset files from `make assets` (plus the older hand-made `splashscreen.c`) |
| `graphics/` | Source PNG files for the asset pipeline |

### Player Physics (`src/player/player.h`, `src/player/player.c`)

Physics uses integer arithmetic (no floats — Game Boy has no FPU). Positions are whole pixels in world coordinates (sub-pixel movement is planned for M2):
- Horizontal: `velocity_x`, `movement_force_x`, `slowing_x` for deceleration over `DECELERATION_FRAMES`
- Vertical: `velocity_y` clamped to `[MIN_VELOCITY_Y, MAX_VELOCITY_Y]`, `jump_frames` counter up to `MAX_JUMP_FRAMES`; falling speed comes from a table indexed by `jump_frame_count`
- Ground detection via `underfoot_state` (`FOOT_ON_LAND` / `FOOT_IN_AIR`)
- `player.x` is the sprite's horizontal center, `player.y` its feet. Collision functions take a `scroll_x` argument, but callers pass `0` because positions are already world coordinates.

### Level Collision (`src/level/level_manager.h`)

`Level.collision_map[LEVEL_HEIGHT][LEVEL_WIDTH]` is an 18×32 byte array in WRAM. Tile types: `COLLISION_NONE`, `COLLISION_SOLID`, `COLLISION_ONE_WAY`, `COLLISION_HAZARD`, `COLLISION_COLLECTIBLE` (only SOLID and COLLECTIBLE are handled). `level_check_collision()` converts world pixel coordinates to tile indices; out of bounds returns `COLLISION_NONE`.

### Camera

`main_game_loop()` sets `total_scroll_x = clamp(player.x - 80, 0, MAX_SCROLL_X)` and applies it with `move_bkg()`. The player and enemies are drawn at `world_x - total_scroll_x`. Levels are at most 32 tiles (256 px) wide until map streaming lands (M2).

## Development Feature Flags (`src/utils.h`)

| Flag | Value | Effect |
|---|---|---|
| `SKIP_INTRO_AND_START` | `1` | Skip intro/title screens, jump straight to gameplay |
| `EMPTY_BACKGROUND` | `1` | **No effect.** Only read by the excluded `prepare_core_game_loop.c`. |
| `DISABLE_BACKGROUND_SCROLLING` | `0` | **No effect.** Nothing reads it. |

## Game Boy Constraints to Keep in Mind

- **No floats**: all physics math uses `int8_t`/`int16_t`/`uint8_t`.
- **VRAM timing**: tile/map writes must happen during VBlank or HBlank. Use `wait_vbl_done()` at end of main loop (already in place).
- **Shared tile memory**: GBDK uses `0x8800` addressing for background tiles, so background tiles 128–255 and sprite tiles 128–255 are the same memory.
- **Sprite limit**: 10 sprites per scanline, 40 total OAM slots. The player uses 8×16 metasprites (`SPRITES_8x16` mode), 4 OAM entries.
- **ROM banking**: assets in bank 1 use `-bank 1 -org 0x8000` flags in `make assets`. Access banked data with `SWITCH_ROM_BANK`.
- **Stack is tiny**: avoid deep recursion and large stack allocations.
- **Window position**: `scroll_win()` is relative and takes `int8_t`; use `move_win()` for absolute positions. `WX = 7` is the left screen edge.

## GBC + DMG Rules (from `docs/CGB_PLAN.md`)

- Check the hardware once with `_cpu == CGB_TYPE`. Every VRAM bank 1 / attribute write must be guarded: on DMG, `VBK_REG` is ignored and the write would overwrite the tile map.
- `BGP_REG`/`OBP0_REG`/`OBP1_REG` do nothing in GBC mode, and GBC palette registers do nothing on DMG. Palette changes, fades and flashes need both paths.
- Every palette is ordered index 0 lightest → index 3 darkest, so DMG shades come out right.
- No background tile flips (DMG can't flip background tiles): backgrounds use `png2asset -noflip`.
- Only VRAM bank 0 for tiles and 8 KB of WRAM. No double-speed mode: game logic must fit in one DMG frame.

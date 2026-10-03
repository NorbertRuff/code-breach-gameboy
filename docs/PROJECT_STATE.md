# Project State — Audit

Snapshot of the codebase on 2026-10-02 at commit `bea580b` (master).
This is a point-in-time audit. The live task list is [ROADMAP.md](ROADMAP.md).

## Status since the audit

- **2026-10-02, M0:** toolchain installed, MBC5 + GBC header, assets banked (bank 0 went
  from 85 to about 4,900 bytes free), dead code removed, PyBoy tests and CI added.
- **2026-10-03, M1:** bugs 1, 2, 3, 4, 8, 9, 10, 11, 13, 14, 15 and 16 fixed, each with a
  test in `tests/test_regressions.py` that fails on the old code.
- **Still open:** bugs 5, 6 and 7 (physics and collision rewrite in M2; 5 and 7 have
  expected-failure tests in `tests/test_known_bugs.py`). Bug 12 is closed: the owner
  chose 1.5 s, and the comment now says so.
- The owner tested on hardware on 2026-10-03: it works, but graphics, logic and visuals
  have many rough parts. A review pass is in the roadmap after M1.

## Summary

The game is a working prototype: one 256 px wide test level with movement, jumping,
collectibles, 3 patrolling enemies, lives, a HUD, and sound effects. It runs on a real
Game Boy. It is not yet a game: there is no real art, no level data format, no music,
no Game Boy Color (GBC) support, and several bugs in the restart path.

About 1,000 lines are hand-written C. The rest of `src/graphics/` is generated asset data.

## Build environment

| Item | State |
|---|---|
| GBDK | GBDK-2020 4.5.0 in `gbdk/` (gitignored, installed by the owner on 2026-10-02). |
| Emulicious | In `emulicious/` (gitignored). `make run` opens the ROM in it. |
| Build shell | `make` works from PowerShell or cmd. It fails from Git Bash: the Makefile detects `OS=Windows_NT` and uses cmd syntax (`if exist ...`), but make then runs it with `sh`. |
| Build result | Builds with GBDK 4.5.0 and many warnings. The compiler flags bug 1 (`level_manager.c:61: overflow in implicit constant conversion`) and bug 13 (`player.c:101: comparison is always true`). |
| **ROM space** | **Bank 0 is 99% full: 16,299 of 16,384 bytes, 85 bytes free.** Bank 1 is 23% used. Almost any new code will not fit until the cartridge has a memory bank controller (MBC) and banked assets. |
| **Asset banking is broken** | `make assets` passes `-bank 1 -org 0x8000 -name ...`. png2asset doesn't know these options: it prints `Warning: Argument "-bank" not recognized` and exits 0. So no asset was ever banked. The right option is `-b 255`. Verified on 2026-10-02. |
| **Code past bank 0** | The linker map shows `_CODE` at `0x4000–0x460A` and `_HOME` at `0x460B–0x4EBA`. This works only because a 32 KB ROM-only cartridge never switches banks. With MBC5 banking, `_HOME` outside bank 0 would crash. Verified with `romusage obj/ApexLab.map -a`. |
| WRAM | 1,994 of 4,096 bytes used in the first WRAM block (49%), mostly the two level structs and the collision view map. |
| GNU Make | 4.4.1 (Chocolatey) |
| Python | 3.13 and 3.14. PyBoy 2.7.0 ships Windows wheels for both. |
| ROM output | `obj/ApexLab.gb`, 32,768 bytes. No cartridge type flag, so it builds as a 32 KB ROM-only cartridge. |
| Header | No GBC flag. The ROM runs in DMG mode on a GBC. |
| Symbols | `-debug` makes `.noi`/`.cdb`/`.map`. No `.sym` yet (PyBoy needs `-Wm-yS`). |

`CLAUDE.md` said the toolchain is bundled in the repo. That was wrong and is now fixed.

## What works

| Feature | Where | Notes |
|---|---|---|
| State machine | `src/main.c` | INTRO_CREDITS → TITLE_SCREEN → GAMEPLAY_START → COREGAMELOOP → GAMEPLAY_END / LEVEL_COMPLETE |
| Horizontal movement | `src/player/player.c` | Integer velocity, 3-frame slide on release. No sub-pixel positions. |
| Jump | `src/input/input.c`, `src/player/player.c` | Hold A up to 15 frames. Gravity is a lookup on frames-in-air. |
| Tile collision | `src/level/level_manager.c` | 32×18 byte map in WRAM. SOLID and COLLECTIBLE work. ONE_WAY and HAZARD are defined but unused. |
| Camera | `src/main.c` | Follows player X, clamped to 0..96. `move_bkg`. |
| Collectibles | `src/player/player.c` | 12 in level 1. Tile is cleared on pickup. |
| HUD | `src/level/level_manager.c` | Window layer at the bottom. Heart + lives, `N/T` counter. |
| Lives and respawn | `src/main.c` | 3 lives, 90 invincibility frames with sprite flashing. |
| Enemies | `src/enemy/` | Patrol between two X bounds, flip by direction, AABB hit test. Mushroom placeholder art. |
| Sound effects | `src/sound/sound.c` | Jump (CH1 sweep), collect (CH2), hit (CH1 + CH4 noise). |
| Fades and flashes | `src/utils.c`, `src/game_state/game_over.c` | DMG only: they write `BGP_REG`/`OBP0_REG`. |

## Bugs

Ordered by how visible they are to the player.

| # | Bug | Location | Cause | Fix |
|---|---|---|---|---|
| 1 | After game over or level complete, the HUD window covers the whole screen on every second load. This is the "game-over reload visual bug" in GAME_DESIGN.md. | `src/level/level_manager.c:61` | `scroll_win(0, 128)` moves the window *relative* to its current position. Its parameters are `int8_t`, so `128` is `-128`. Each load flips `WY` between 128 and 0. | `move_win(7, 128)`. `WX=7` is the left screen edge. Today `WX=0`, which also hides 7 px of the heart icon. |
| 2 | HUD shows `9/9` for a level with 12 fragments. | `src/level/level_manager.c:87-89` | Single-digit display caps both numbers at 9. | Two-digit counter. |
| 3 | The level reload is visible on screen. | `src/main.c:136` | `GAMEPLAY_START` calls `fade_in_screen` *before* `level_load_level`. The screen fades in on the old frame, then tiles change while visible. | Load while the screen is dark, then fade in. |
| 4 | 3 KB of junk written to video RAM (VRAM) on every level load. | `src/level/level_manager.c:132` | `set_bkg_data(0, 256, ...)` copies 256 tiles from a 64-tile array (`background_TILE_COUNT` is 64). | Use `background_TILE_COUNT`. |
| 5 | Long falls can turn upward mid-air. | `src/player/player.c:204` | `jump_frame_count` wraps with `% 60`. After 60 frames in the air it goes back to 0, and the gravity table gives `velocity_y = -2` again. | Saturate the counter. This goes away with the sub-pixel physics rewrite. |
| 6 | Releasing A does not cut the jump short. | `src/player/player.c:195-217` | On release, the counter restarts at 0, so the table keeps the player rising for 10 more frames. | Replace the table with velocity + gravity, and cut upward velocity on release. |
| 7 | Player edges can sink into walls. | `src/player/player.c:174` | Horizontal collision checks one point: the center X, one tile above the feet. The sprite is 16×32. | Check the leading edge at several heights. |
| 8 | Title screen blink speed depends on CPU speed. | `src/main.c:128`, `src/game_state/start_screen.c:18-24` | Busy loop without `wait_vbl_done()`, counting to 8000 iterations. It would run 2× fast in GBC double-speed mode. | Count frames instead. |
| 9 | Sprite shades can change after the first game-over flash. | `src/game_state/game_over.c:15` | The flash "restores" `OBP0_REG` to `0xD0`, but nothing sets it to `0xD0` at boot. | One palette module that owns all palette state. |
| 10 | `performant_delay(n)` loops forever for `n > 255`. | `src/utils.c:5-7` | `uint16_t time` parameter, `uint8_t i` counter. | Make the counter `uint16_t`. No current caller passes more than 200. |
| 11 | Falling out of the level never kills the player. | `src/level/level_manager.c` `level_check_collision` | Out-of-bounds tiles return `COLLISION_NONE`. Level 1 has no pits, so it is hidden today. | Kill below the bottom row. |
| 12 | Invincibility lasts 1.5 s, not 3 s. | `src/main.c:174` | The comment assumes 30 fps. The loop runs at 60 fps. | Decide the intended length, then fix the constant or the comment. |
| 13 | Releasing left makes the player slide right for a moment. | `src/player/player.c:93`, `player.h` (`last_movement_x`, `slowing_x`) | Both fields are `uint8_t` but store `-1`, which becomes 255. `last_movement_x > 0` is then true after moving left, so `slowing_x` gets `1`. The compiler warns at `player.c:101`: "comparison is always true". | Make both fields `int8_t`. |
| 14 | The level-complete flash is too fast for photosensitive players. | `src/game_state/game_over.c:28` | `palette_flash(6, 8)` flashes once every 16 frames: 3.75 per second. The WCAG guideline is at most 3. The game-over flash (`hold` 12) is 2.5 per second, which is fine. | Hold at least 10 frames per half-flash. |
| 15 | The player is drawn with obsolete API calls. | `src/player/player.c:282,286` | `move_metasprite` and `move_metasprite_vflip` are marked "Obsolete" in GBDK 4.5's `metasprites.h`. | `move_metasprite_ex` and `move_metasprite_flipx`. |
| 16 | The player starts inside platform A, and 1 px inside the ground. | `src/level/level_manager.c` `level_1_server_room_init` | Start x = 16 puts the 16 px sprite over platform A (x 16–47) at body height. Start y = 136 is the ground tile's top row; every landing puts the feet at 135. | Start at x = 8, y = 135. Found by the PyBoy jump test on 2026-10-02. |

## Tech debt

- **Dead file in the tree.** `src/game_state/prepare_core_game_loop.c` is excluded in the Makefile (`filter-out`). It duplicates `set_hud_window_tiles` and `set_programmer_metasprite`.
- **`EMPTY_BACKGROUND` does nothing.** The only reader is the excluded file above. `utils.h` sets it to `1`. The old CLAUDE.md said `0`.
- **Empty stub.** `src/game_state/core_game_loop.c` holds an empty `CoreGameLoop()`. The real loop is `main_game_loop()` in `main.c`.
- **Unused global wastes WRAM.** `LevelManager g_level_manager` (`level_manager.c:53`) is never used and takes about 640 bytes.
- **WRAM cost of the level.** One 32-tile level uses the 576-byte collision map in `Level`, plus a 576-byte static `collision_vis_map`. A 128-tile wide level would need 2.3 KB for each. The DMG has 8 KB of WRAM in total.
- **Level 1 is C code.** Platforms and collectibles are `for` loops in `level_1_server_room_init`. Enemy spawns are hardcoded twice in `main.c:144-146` and `main.c:175-177`. `Level.spawns[]`, `exit_x/y` and `required_keys` are set but never read.
- **The level is a debug view.** Solid tiles are drawn as mushroom tile 128 over the background image.
- **Duplicate constant.** `MAX_JUMP_FRAMES` is defined in both `input.h:9` and `player.h:29`.
- **Sub-pixel movement is not in the code.** PR #2 (`feat-smooth-movement`) was merged, but positions are still whole pixels.
- **Makefile.** One copy-pasted rule per source folder. No cartridge type, no GBC flag, no `.sym` output.
- **Leftover files.** `scripts/*.sh` come from another project (they reference `assets/sprites/mario.png`). `compile.bat` is stale. `graphics/start_screen/*.c|.h` are generated files in the source-art folder. `programmerMario.png` and `apexLogo3.*` are unused. `src/graphics/apexLogo.c` is generated but never drawn. The title screen uses `splashscreen.c` instead.
- **`Readme.md`** is a GitHub profile template, not a project readme.

## VRAM use today

GBDK puts background tiles in the `0x8800` (signed) region. Background tiles 128–255 share memory with sprite tiles 128–255.

| Range | Layer | Content |
|---|---|---|
| BG 0–63 | Background | Level background (64 tiles) |
| BG 64–77 | Window | HUD frame (14 tiles) |
| BG 128–131 | Background | Mushroom block (solid-tile debug view) |
| BG 132 | Background | Collectible diamond |
| BG 133–144 | Window | Digits 0–9, `/`, heart |
| Sprite 0–39 | Sprites | Player, 5 frames of 16×32 (40 tiles) |
| Sprite 40–43 | Sprites | Enemy mushroom, reordered for 8×16 mode |

OAM: player uses entries 0–3, enemies use 4–11 (2 each, 4 enemies max).

## GBC readiness

Nothing GBC-specific runs today. What has to change is in [CGB_PLAN.md](CGB_PLAN.md). The main blockers:

- No `-Wm-yc` header flag, so a GBC runs the ROM in DMG mode.
- No palettes are loaded. `png2asset` already exports them (the background has 3), but only the dead file calls `set_bkg_palette`.
- Every fade and flash writes `BGP_REG`/`OBP0_REG`. GBC color mode ignores those registers.
- The source art was drawn for DMG. The player palette is ordered transparent, black, gray, white (dark to light). The DMG needs light to dark.

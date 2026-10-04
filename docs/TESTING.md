# Testing with PyBoy

Every change is checked on the PC, headless, in both DMG and GBC mode, before it goes to
the flashcart.

[PyBoy](https://github.com/Baekalfen/PyBoy) 2.7.0 is a Game Boy emulator with a Python
API. It runs without a window, far faster than real time. Tests press buttons, step
frames, read memory and take screenshots. The whole suite runs in about 20 seconds.

## The loop

1. `make test` builds `obj/ApexLab.gb` and `obj/savetest.gb`, then runs `pytest`.
   Most tests run twice: once in DMG mode, once in GBC mode.
2. A failing test saves a screenshot to `tests/out/`. Claude reads these PNGs when
   debugging. The folder is emptied at the start of every run.
3. At each milestone, the owner checks the ROM on the real DMG and GBC.

Emulicious stays the tool for manual debugging: breakpoints, VRAM viewer, tile map
viewer, and source-level debugging with the `.cdb` file.

## Setup (once)

```bash
make venv
```

This creates `.venv/` and installs `tools/requirements.txt`: PyBoy 2.7.0, pytest,
Pillow, NumPy. `make test` uses `.venv` when it exists, otherwise `python` from PATH
(as in CI).

## Files

| File | Holds |
|---|---|
| `pytest.ini` | Test folder, strict expected failures, warning filter |
| `tests/conftest.py` | Fixtures: `game` (one booted ROM per test and mode, in the greybox level or the one `@pytest.mark.level` names), `mode`, `rom_dir`, `rom_bytes` |
| `tests/gamelib.py` | `boot()`, the `Game` helper class, `Pilot` (level replays), the `Player` struct mirror, constants copied from the C headers |
| `tests/test_rom.py` | Header bytes, checksums, ROM size, bank 0 layout from the linker map |
| `tests/test_boot.py` | Hardware detection, reaching gameplay, walking, jumping, screen drawn |
| `tests/test_regressions.py` | One test per fixed bug |
| `tests/test_physics.py` | Jump heights, coyote time, jump buffer, ceiling corners, one-way platforms, hazards, never overlapping solid tiles |
| `tests/test_tiled2c.py` | The level converter: committed output up to date, the test level's contents, the level data in the ROM, and every rule check |
| `tests/test_jump_metrics.py` | The jump metrics in LEVEL_DESIGN.md section 4: climb heights and gap widths, measured in the test room `levels/test_jumps.tmj` |
| `tests/test_levelpreview.py` | `tools/physics.py` matches the ROM frame by frame; the reachability check agrees with the jump metrics, passes the greybox level and reports a broken one |
| `tests/test_streaming.py` | Screen and collision ring match the map while scrolling both ways, collected fragments stay gone, respawn redraws, camera look-ahead and speed |
| `tests/test_actors.py` | Actors: bots in reach spawn at the start, a bot spawns as the camera nears, patrols, removal a screen behind until a restart, hits and invincibility, the sprite list every frame |
| `tests/test_screen.py` | The screen splits: band and level scroll per screen line, the band drawn once and kept by streaming, no sprite over the HUD, the band palette light to dark |
| `tests/test_sections.py` | Gates as solid columns, closed below the quota, opening on touch, the opened gate as checkpoint, the camera and player lock, lives and game over per section, the exit |
| `tests/test_frame_budget.py` | A route that finishes the greybox level (both gates, the exit): no lag frames, every update within `FRAME_LINE_LIMIT` lines; the ROM's lag counter counts right |
| `tests/test_level_1.py` | The level complete summary shows the fragments and START goes on. `Pilot` replays finish level 1 without losing a life, through section B's high and low routes, from two start times, with no lag frames, every update within `FRAME_LINE_LIMIT`, and at most 10 sprites on any screen line (`game.track_sprites_per_line()`) |
| `tests/test_pause.py` | START pauses (nothing moves, the label shows) and resumes; SELECT while paused cycles the levels |
| `tests/test_palette.py` | Only `src/engine/palette.c` writes palettes (a source scan); losing a life fades out and back in through the same steps on DMG and GBC; the invincible player blinks on DMG and is pale on GBC; the band and HUD use their own GBC palettes |
| `tests/test_golden.py` | Key screens (level 1 start, the rack stair, the pause, the invincible player) match `tests/golden/<screen>_<mode>.png` pixel for pixel; a mismatch saves the new picture and a mask of the changed pixels to `tests/out/`. `make goldens` rewrites them after an intended change |
| `tests/test_gbconvert.py` | The art converter on generated pictures: upscale and the given size, snapping, palette per tile, flipped sprite tiles, tile budget, every rule break, previews |
| `tests/test_sketch2tmj.py` | Level sketches: level 1 and the greybox level survive the trip to a sketch and back; a sketch with every legend character converts; errors |
| `tests/test_fuzz.py` | Random play from level 1 with fixed seeds, pause and level-select included: after every frame a valid state, no lag frame, the body in no solid tile, at most 10 sprites on a line, the camera inside the level, the HUD count right; and, when each update ends, every screen column matching the map (markers only where the level has that object) and the collision ring matching the tiles. For a bigger scan, loop over more seeds with `play`, `problems` and `screen_mismatch` |
| `tests/test_text.py` | Screen text in the game's font: the intro credits (reached from play, since the test build skips the intro) are the right font tiles in the right places |
| `tests/test_known_bugs.py` | Open bugs, as expected failures |
| `tests/test_savetest.py` | Cartridge RAM survives power cycles, for `obj/savetest.gb` (MBC5) and the MBC1 and MBC3 variants; each header names its mapper |

## How PyBoy is set up

- **Forcing DMG mode.** PyBoy picks its boot ROM from the cartridge header. Our header
  says "GBC-enhanced", so PyBoy runs GBC hardware even with `cgb=False`. `boot()` passes
  PyBoy's own DMG boot ROM (`pyboy/core/bootrom_dmg.bin`) to force real DMG mode.
- **Boot time.** PyBoy's boot ROMs take about 65 frames. `boot()` runs until the
  cartridge's first instruction (`0x100`), so every test starts at the game's power-on.
  After that, GBDK has set `_cpu`: `0x01` on DMG, `0x11` on GBC.
- **Isolated save files.** PyBoy loads `<rom>.ram` if it exists and writes it when a
  battery cartridge stops. The tests run from a copy of the ROM in a temp folder, and
  every `game` fixture stops with `save=False`.
- **Symbols.** The build writes `obj/ApexLab.sym`; PyBoy loads it from next to the ROM.
  `pyboy.symbol_lookup("_player")` returns `(bank, address)`. C globals start with `_`.

## Reading and changing game state

- `game.player()` reads the `Player` struct through `PLAYER_FIELDS` in `gamelib.py`.
  SDCC does not pad structs, so the offsets follow `player.h`. The ROM exports
  `player_struct_size`; `test_player_struct_size_matches` fails if the C struct changes
  without the Python mirror.
- **When a read happens.** A PyBoy frame ends a few lines into VBlank, while the game is
  in the middle of its update. So `game.player()` returns a copy of the struct taken when
  the game calls `player_draw`, after that frame's update is complete. The copy is dropped
  at every new frame: in a frame without a draw (a respawn, a state change) `player()`
  reads memory directly. The struct then shows the frame that finished during the last
  `frames()` step; a button pressed now reaches the game one frame later.
- **Replaying from one frame.** `game.save()` returns a snapshot of the whole machine and
  `game.load(snapshot)` goes back to it, so a test can try several inputs from the same
  frame (coyote time, jump buffer). Buttons are not part of the snapshot: press the held
  ones again after `load()`.
- **Writing the player.** `place_player` and `set_player_y` wait for the same point
  (the start of `player_draw`), so a move never lands in the middle of an update.
  `place_player` then runs 10 frames: a far move makes the camera jump, and redrawing a
  whole screen of columns takes about 6 frames. A 1-frame button press in a lag frame
  is never read.
- **Levels.** The game starts in level 1, but `Game` writes `level_index` when `main()`
  starts, so tests play `levels/greybox_test.tmj`. Builds from before level 1 (an old
  commit under `tools/test_against.py`) number the levels otherwise and start in the
  greybox level anyway, so `Game` leaves them alone. `@pytest.mark.level(LEVEL_1)` on a
  test starts it in another level. `game.load_level(LEVEL_TEST_JUMPS)`
  restarts in the test room with the jump metric fixtures. Test geometry lives in Tiled
  levels: writing collision from a test doesn't last once columns stream.
- `game.actors()` reads the actors in play through `ACTOR_FIELDS` (checked like the
  Player mirror, with `actor_struct_size`); `bot` says whether one is a patrol bot.
  `game.visible_sprites()` lists the OAM entries that are switched on.
- **Rendered frames.** `game.tick(render=True)` renders a frame; then
  `pyboy.screen.tilemap_position_list` gives SCX, SCY, WX and WY for each screen line,
  and `pyboy.screen.ndarray` the pixels. A frame shows the scroll and sprites of the
  update before it (they are latched at VBlank). PyBoy's first rendered frame after
  unrendered ones starts the window one line early: compare from the second one.
- `game.collision(x, y)` reads the collision ring (screen rows; only the columns around
  the camera), `game.bkg_tile(x, y)` the background map at a level column,
  `game.camera_x()` the camera, `game.window_tile(col, row)` the HUD window map,
  `game.reg("WY")` a hardware register.
- `game.play_route(route)` plays `(frames, buttons)` steps, each holding exactly the
  buttons named, like `(26, "a right")`. Routes are input, so they stay valid when game
  state moves in memory, but a physics or enemy change can break them: a route test
  checks that its route still gets where it should.
- **Level replays.** `Pilot(game)` plays a level with moves that watch the player every
  frame: `run_to(x)`, `jump(hold, steer)`, `drop_off()`, `stop()`, `wait_for_bot`,
  `pass_bot` (wait until the bot walks away, follow it, jump over it when it turns back),
  `open_gate(section)` and `reach_exit()`. x is in world pixels (Tiled's x); a bot is
  named by its patrol rectangle's left edge. A move that loses a life fails with the
  place and the fragments so far. `Pilot(game, print)` logs every landing, which helps
  when writing a route.
- **Frame budget.** The ROM counts lag frames in `lag_frames` (`src/main.c`): every
  VBlank that passes during an update is a frame shown without one.
  `game.track_frame_lines()` returns a list that fills with, for every update, the screen
  lines from the start of VBlank to the end of the update: `LY` when the game starts
  waiting for VBlank, plus 154 for every VBlank in between (from `sys_time`). A frame
  has 154 lines; `test_frame_budget.py` allows 140, a margin because PyBoy's timing is
  close to the hardware's but not exact. `make budget` (`tools/frame_budget.py`) plays the
  same route and prints the lines each part of an update takes, and the worst update
  part by part.
- Some situations would take minutes of play: finishing the level, losing all lives.
  For those, tests write the game's variables directly with `set_state`, `set_lives`,
  `set_player_y` and `place_player`. `start_on_open_ground()` puts the player at x = 156,
  where nothing is in the way: at the level start, platform A blocks the path at body
  height. It also picks up the fragment at column 19, which opens gate A (quota 1).
  `lose_life()` drops the player below the screen, which costs a life even while
  invincible. Keep this rare, and prefer input. Only move the player forward past an
  opened gate: behind the camera's left limit, columns aren't drawn and the collision
  ring holds other columns. `game.level()` reads the level's state (section, gates,
  fragments per section) through `LEVEL_FIELDS`.

## Conventions

- **A fixed bug gets a regression test** in `test_regressions.py`. Check that the test
  fails without the fix, on the last commit's build:

  ```bash
  .venv/Scripts/python.exe tools/test_against.py HEAD tests/test_regressions.py -k <test name>
  ```

  The tool exports the commit with `git archive` into a temporary folder (no checkout),
  builds it there, and runs the current tests against that build through
  `APEXLAB_OBJ`. Tests that read the `Player` struct give no answer on builds from before
  M2 (commit `4d93eb2` and older): the struct changed, and only `x` and `y` stayed in
  place, with `y` in different units. Compare raw values within one build instead, as
  the bug 6 test does. The `fix-bug` skill in `.claude/skills/` walks through the whole routine.
- **An open bug can get an expected-failure test** in `test_known_bugs.py`, marked
  `@pytest.mark.xfail(reason=...)`. `pytest.ini` sets `xfail_strict = true`: when the bug
  is fixed, the test passes and the run fails, so the test moves to
  `test_regressions.py` without the marker.
- **Fades and flashes run in both modes** since the palette module. Check them with
  `game.background_is_black()` (DMG: `BGP` is `0xFF`; GBC: every background color is 0),
  `game.sprite_palettes()` (DMG: `OBP0`; GBC: the sprite colors) and
  `game.gbc_colors(sprites=False)`, which reads GBC palette memory through its index
  register and puts the register back. A test that only makes sense on one system uses
  `@pytest.mark.parametrize("mode", ["dmg"])`.
- **Build variants** for one-off checks: `make OBJDIR=<folder> EXTRA_CFLAGS=-D...`. For
  example, `EXTRA_CFLAGS=-DSKIP_INTRO_AND_START=0` builds a ROM with the intro and title
  screen. Always use a separate `OBJDIR` so `obj/` keeps the normal build.

## Save test on the real Game Boy

`obj/savetest.gb` counts boots in cartridge RAM. On the flashcart:

1. Copy `obj/savetest.gb` to the SD card and start it. It shows `NO SAVE: NEW ONE` and
   `BOOT COUNT: 1`.
2. Power off, wait a few seconds, power on, and start it again. The count must be 2.
   Repeat once more.
3. Try both ways of leaving: going back to the flashcart menu, and switching off
   directly. Some EverDrive clones only write the save to the SD card on one of them.
4. Press A to clear the save when done.

If the count never goes up, try the same with `obj/savetest_mbc1.gb` and
`obj/savetest_mbc3.gb` (built by `make savetest` too): the same test with an MBC1 or MBC3
header, named on the second line of the screen. Some clones keep saves only for some
mappers. If no variant keeps the count, battery saves don't work on this cartridge, and
the game falls back to passwords (docs/DECISIONS.md).

## Continuous integration

`.github/workflows/build.yml` runs on every push and pull request, on `ubuntu-24.04`:
download and cache GBDK 4.5.0, `make all savetest` (the build fails if a ROM bank
overflows), `make test`, then upload the ROMs (the game and the three save test ROMs). On failure it uploads `tests/out/`.

## Still planned

| Test | When |
|---|---|
| Golden copies of the `make shots` contact sheets, to spot visual changes. For now, compare by hand: `tools/shots.py --out tests/out/shots_before` before a change, `make shots` after, then `tools/compare_shots.py tests/out/shots_before tests/out/shots` | M4 |

## What PyBoy cannot prove

- How colors and motion look on the real LCDs.
- Whether saves persist on the flashcart. EverDrive clones differ here.
- Exact VRAM timing edge cases. Use Emulicious, which is more accurate, then hardware.

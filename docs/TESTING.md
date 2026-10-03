# Testing with PyBoy

Every change is checked on the PC, headless, in both DMG and GBC mode, before it goes to
the flashcart.

[PyBoy](https://github.com/Baekalfen/PyBoy) 2.7.0 is a Game Boy emulator with a Python
API. It runs without a window, far faster than real time. Tests press buttons, step
frames, read memory and take screenshots. The whole suite runs in about 2 seconds.

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
| `tests/conftest.py` | Fixtures: `game` (one booted ROM per test and mode), `mode`, `rom_dir`, `rom_bytes` |
| `tests/gamelib.py` | `boot()`, the `Game` helper class, the `Player` struct mirror, constants copied from the C headers |
| `tests/test_rom.py` | Header bytes, checksums, ROM size, bank 0 layout from the linker map |
| `tests/test_boot.py` | Hardware detection, reaching gameplay, walking, jumping, screen drawn |
| `tests/test_regressions.py` | One test per fixed bug |
| `tests/test_physics.py` | Jump heights, coyote time, jump buffer, ceiling corners, one-way platforms, hazards, never overlapping solid tiles |
| `tests/test_tiled2c.py` | The level converter: committed output up to date, the test level's contents, the level data in the ROM, and every rule check |
| `tests/test_jump_metrics.py` | The jump metrics in LEVEL_DESIGN.md section 4: climb heights and gap widths, measured in the test room `levels/test_jumps.tmj` |
| `tests/test_levelpreview.py` | `tools/physics.py` matches the ROM frame by frame; the reachability check agrees with the jump metrics, passes the greybox level and reports a broken one |
| `tests/test_streaming.py` | Screen and collision ring match the map while scrolling both ways, collected fragments stay gone, respawn redraws, camera look-ahead and speed |
| `tests/test_frame_budget.py` | A route through the greybox level from the start to the exit: no lag frames, every update within `FRAME_LINE_LIMIT` lines; the ROM's lag counter counts right |
| `tests/test_known_bugs.py` | Open bugs, as expected failures |
| `tests/test_savetest.py` | Cartridge RAM survives power cycles (`obj/savetest.gb`) |

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
- **Levels.** The game plays `levels/greybox_test.tmj`. `game.load_level(LEVEL_TEST_JUMPS)`
  restarts in the test room with the jump metric fixtures. Test geometry lives in Tiled
  levels: writing collision from a test doesn't last once columns stream.
- `game.collision(x, y)` reads the collision ring (screen rows; only the columns around
  the camera), `game.bkg_tile(x, y)` the background map at a level column,
  `game.camera_x()` the camera, `game.window_tile(col, row)` the HUD window map,
  `game.reg("WY")` a hardware register.
- `game.play_route(route)` plays `(frames, buttons)` steps, each holding exactly the
  buttons named, like `(26, "a right")`. Routes are input, so they stay valid when game
  state moves in memory, but a physics or enemy change can break them: a route test
  checks that its route still gets where it should.
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
  height. `lose_life()` runs into the pit at column 35. Keep this rare, and prefer input.

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
- **DMG-only effects** (fades and flashes through `BGP_REG`, until the palette module in
  M4) use `@pytest.mark.parametrize("mode", ["dmg"])` instead of running in both modes.
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

If the count never goes up, battery saves don't work on this cartridge, and the game
falls back to passwords (docs/DECISIONS.md).

## Continuous integration

`.github/workflows/build.yml` runs on every push and pull request, on `ubuntu-24.04`:
download and cache GBDK 4.5.0, `make all savetest` (the build fails if a ROM bank
overflows), `make test`, then upload the ROMs. On failure it uploads `tests/out/`.

## Still planned

| Test | When |
|---|---|
| Golden screenshots per screen and mode, `pytest --update-golden` to accept changes | M4 |
| Replays: recorded inputs in `tests/replays/*.json` that must still finish a level | M3 |
| Golden copies of the `make shots` contact sheets, to spot visual changes. For now, compare by hand: `tools/shots.py --out tests/out/shots_before` before a change, `make shots` after, then `tools/compare_shots.py tests/out/shots_before tests/out/shots` | M4 |

## What PyBoy cannot prove

- How colors and motion look on the real LCDs.
- Whether saves persist on the flashcart. EverDrive clones differ here.
- Exact VRAM timing edge cases. Use Emulicious, which is more accurate, then hardware.

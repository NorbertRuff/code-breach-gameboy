# Testing with PyBoy

[PyBoy](https://github.com/Baekalfen/PyBoy) 2.7.0 runs the ROM headless, far faster than
real time. Tests press buttons, step frames, read memory and take screenshots. Emulicious
stays the tool for manual debugging (breakpoints, VRAM viewer, source-level debugging).

## The loop

| Command | When | What runs |
|---|---|---|
| `make test` | After every change | The quick tier, in parallel (pytest-xdist): ROM tests in DMG mode, plus GBC where the mode changes the result. |
| `make test-full` | Before a commit, in CI | Every ROM test in DMG and GBC mode, plus the tests marked `full` (extra replay start times and random-play seeds). |

A failing test saves a screenshot to `tests/out/` (emptied at the start of every run).
The owner checks the ROM on the real DMG and GBC.

**Tiers** (`tests/conftest.py`, `pytest.ini`): a test that boots the ROM gets a `mode`.
- `@pytest.mark.both_modes`: DMG and GBC in both tiers. For palettes, fades, screens and
  hardware detection, where the two systems can differ.
- `@pytest.mark.mode("cgb")` (or `"dmg"`): one system only, in both tiers.
- `@pytest.mark.full`, usually on a parameter (`pytest.param(5, marks=pytest.mark.full)`):
  only in the full tier.

Don't add a test that repeats what another test checks; extend that one instead.

## Setup (once)

`make venv` creates `.venv/` from `tools/requirements.txt` (PyBoy, pytest, pytest-xdist,
Pillow, NumPy). `make test` uses `.venv` when it exists, otherwise `python` from PATH.

## Files

- `pytest.ini`: test folder, `tools/` on the import path, strict expected failures, the
  markers.
- `tests/conftest.py`: the tiers, and the fixtures `game` (one booted ROM, in the greybox
  level or the one `@pytest.mark.level(...)` names), `rom_dir`, `rom_bytes`.
- `tests/gamelib.py`: `boot()`, the `Game` helper class, `Pilot` (level replays), the
  struct mirrors and constants copied from the C headers, the limits
  (`FRAME_LINE_LIMIT`, `SPRITES_PER_LINE`) and `Game.assert_within_limits` for replays.
- `tests/test_*.py`: each file's docstring says what it covers. `test_regressions.py`
  holds one test per fixed bug; open bugs go in `test_known_bugs.py` (create it when needed).

## How PyBoy is set up

- **DMG mode:** our header says "GBC-enhanced", so PyBoy would run GBC hardware even with
  `cgb=False`. `boot()` passes PyBoy's own DMG boot ROM to force real DMG mode, and runs
  to the cartridge's first instruction (about 65 frames).
- **Isolated saves:** tests run from a copy of the ROM in a temp folder; every `game`
  fixture stops with `save=False`.
- **Symbols:** PyBoy loads `obj/ApexLab.sym`; `pyboy.symbol_lookup("_player")` returns
  `(bank, address)`. C globals start with `_`.

## Reading and changing game state

- **When a read happens.** A PyBoy frame ends a few lines into VBlank, in the middle of
  the game's update. `game.player()` returns a copy of the `Player` struct taken when the
  game calls `player_draw`, after that frame's update. A button pressed now reaches the
  game one frame later. Struct mirrors (`PLAYER_FIELDS`, `ACTOR_FIELDS`, `LEVEL_FIELDS`)
  are checked against `*_struct_size` exported by the ROM.
- **When the screen is complete.** A new column's tiles are written at the start of the
  next update (`level_stream_flush`). Screen checks against the map run from
  `game.at_screen_ready(callback)`, not after `frames()`.
- **Replaying from one frame:** `game.save()` / `game.load(snapshot)`. Buttons aren't in
  the snapshot: press held ones again after `load()`.
- **Writing state:** `place_player`, `set_player_y` (applied at the start of `player_draw`,
  then 10 frames for the camera and columns), `set_state`, `set_lives`, `lose_life()`.
  Keep it rare and prefer input. Move the player only forward past an opened gate:
  behind the camera's left limit, columns aren't drawn. `start_on_open_ground()` puts the
  player at x = 156 in the greybox level (and picks up the fragment that opens gate A).
- **Levels:** tests start in `levels/greybox_test.tmj`; `@pytest.mark.level(LEVEL_1)`
  picks another, `game.load_level(...)` restarts in one. Test geometry lives in Tiled levels.
- **Rendered frames:** `game.tick(render=True)`; then `pyboy.screen.tilemap_position_list`
  gives the scroll per line. A frame shows the scroll and sprites of the update before it.
  PyBoy's first rendered frame after unrendered ones starts the window one line early.
- **Menus and screen text:** `game.text(x, y, length)` reads the font's characters from
  the background map; `game.wait_for_text(x, y, text)` waits until a menu shows it and
  reads buttons (after the fade-in); `game.tap("down start")` presses buttons in turn;
  `game.answer_game_over(quit=False)`. Passwords come from `tools/password.py`.
- **Readers:** `game.collision(x, y)`, `game.bkg_tile(x, y)` (at a level column),
  `game.window_tile(col, row)`, `game.camera_x()`, `game.reg("WY")`, `game.actors()`,
  `game.visible_sprites()`, `game.level()`. Palettes on both systems:
  `game.background_is_black()`, `game.sprite_palettes()`, `game.gbc_colors()`.
- **Routes and replays:** `game.play_route([(frames, "a right"), ...])` holds exact
  buttons. `Pilot(game)` plays a level with moves that watch the player every frame:
  `run_to(x)`, `jump(hold, steer, direction)`, `drop_off()`, `drop_through()`, `stop()`,
  `wait_for_bot`, `pass_bot`, `open_gate(section)`, `reach_exit()`. x is in world pixels;
  a bot is named by its patrol rectangle's left edge. `Pilot(game, print)` logs landings.
- **Frame budget:** `game.track_frame_lines()` fills with the screen lines from the start
  of VBlank to the end of each update; the tests allow 140 of 154 (PyBoy's timing is close
  to the hardware's, not exact). `make budget` prints the lines per part of an update.

## Conventions

- **A fixed bug gets a regression test** that fails on the last commit's build:
  `.venv/Scripts/python.exe tools/test_against.py HEAD tests/test_regressions.py -k <name>`.
  It builds the commit in a temporary folder and runs today's tests against it
  (`APEXLAB_OBJ`). The `fix-bug` skill walks through it.
- **An open bug can be an expected failure** in `tests/test_known_bugs.py` (create the file
  for it; `xfail_strict = true`): fixing it fails the run until the test moves to
  `test_regressions.py`.
- **Build variants:** `make OBJDIR=<folder> EXTRA_CFLAGS=-D...`, never in `obj/`.
  `make release` is one: `release/ApexLab.gb` with the intro and title on, the ROM for the
  flashcart; `make test-full` builds it and `test_release.py` checks it.
- **Golden screenshots** (`tests/golden/`): `make goldens` after an intended visual change;
  look at the new pictures. For a wider before/after: `tools/shots.py --out
  tests/out/shots_before`, change, `make shots`, `tools/compare_shots.py`.

## Save test on the real Game Boy

`obj/savetest.gb` counts boots in cartridge RAM. Build it outside `obj/` for the
flashcart (`make OBJDIR=flashcart savetest`, gitignored), so CLion's `make clean` keeps it.

1. Start it from the SD card: `NO SAVE: NEW ONE`, `BOOT COUNT: 1`.
2. Power off, wait, power on, start it again: the count must be 2. Repeat once.
3. Try both ways of leaving: back to the flashcart menu, and switching off directly.
4. Press A to clear the save.

If the count never goes up, try `savetest_mbc1.gb` and `savetest_mbc3.gb` (the mapper is
named on screen). On the owner's EverDrive clone none kept it (2026-10-04); what the game
does about it is in DECISIONS.md, "Saves".

## Sound test

`make soundtest` builds `obj/soundtest.gb` with every effect of the GBFX and GBFX2 packs,
played by the game's effect player, and every song of the two music packs, and writes a
WAV of each to `tests/out/sounds/` (songs: their first 45 s).
On the Game Boy: SELECT switches between effects and songs, LEFT/RIGHT step, UP/DOWN
jump 10, A plays, B stops the song. The game's effects are in `sounds/effects.txt` (then
`make sounds`), its songs in `music/` (then `make music`).
`tests/test_soundtest.py` checks that every effect holds its channels for its length and
that every song plays.

## Continuous integration

`.github/workflows/build.yml` runs on every push and pull request on `ubuntu-24.04`:
GBDK 4.5.0 (cached), `make all savetest`, `make test-full`, then uploads the ROMs (and
`tests/out/` on failure).

## What PyBoy cannot prove

How colors and motion look on the real screens, whether saves persist on the flashcart,
exact VRAM timing edge cases (use Emulicious, then hardware), and how the music and the
effects sound (PyBoy's sound is close, not exact).

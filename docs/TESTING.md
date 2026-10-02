# Testing with PyBoy

Goal: every change is checked on the PC, headless, in both DMG and GBC modes, before it
goes to the flashcart.

[PyBoy](https://github.com/Baekalfen/PyBoy) 2.7.0 is a Game Boy emulator with a Python
API. It runs without a window, faster than real time. Tests can press buttons, step
frames, read memory and take screenshots.

## The loop

1. `make`: builds `obj/ApexLab.gb` and `obj/ApexLab.sym`.
2. `make test`: runs `pytest tests/`. Every test runs twice, once in DMG mode and once in
   GBC mode.
3. A failing test saves screenshots to `tests/out/`. Claude reads these PNGs directly
   when debugging.
4. At each milestone, the owner checks the ROM on the real DMG and GBC.

Emulicious stays the tool for manual debugging: breakpoints, VRAM viewer, tile map
viewer, and source-level debugging with the `.cdb` file.

## Setup

```bash
py -3.13 -m venv .venv
```

```bash
.venv/Scripts/pip install -r tools/requirements.txt
```

`tools/requirements.txt` pins `pyboy==2.7.0`, `pytest`, `pillow` and `numpy`. `.venv/`
and `tests/out/` are gitignored.

## How tests read game state

- The build adds `-Wm-yS`, which writes `obj/ApexLab.sym` next to the ROM. PyBoy loads a
  `.sym` file next to the ROM automatically.
- `pyboy.symbol_lookup("_player")` returns `(bank, address)`. C globals get a leading
  underscore.
- Struct fields are read by offset. SDCC does not pad structs on the Game Boy CPU, so the
  offsets follow the field order in `player.h`. The Python mirror lives in
  `tests/gamelib.py`.
- To catch struct changes, the ROM exports
  `const uint8_t player_struct_size = sizeof(Player);`. A test compares it with the Python
  mirror's size and fails if they differ.
- Hardware registers are read directly: `WY` is `0xFF4A`, `WX` is `0xFF4B`, `BGP` is
  `0xFF47`.

## Harness

`tests/conftest.py` provides a `game` fixture, parametrized over `dmg` and `cgb`:

```python
pyboy = PyBoy("obj/ApexLab.gb", window="null", cgb=(mode == "cgb"), sound_emulated=False)
pyboy.set_emulation_speed(0)   # no real-time limit
```

`tests/gamelib.py` wraps it in a `Game` class:

| Method | Does |
|---|---|
| `frames(n)` | Step `n` frames without rendering (fast). |
| `hold(button, n)` | Hold a button for `n` frames, then release it. |
| `press(button)` | Press for one frame. |
| `state()` | Read `_current_game_state`. |
| `wait_for_state(s, timeout)` | Step frames until the state machine reaches `s`, or fail. |
| `player()` | Read the `Player` struct into a Python object. |
| `reg(name)` | Read a hardware register by name. |
| `screenshot(name)` | Save `tests/out/<test>_<mode>_<name>.png`. |

Tests drive the release ROM from power-on with button input. There is no test-only build.
Once the game has a level-select cheat (milestone M3), tests use it to jump to a level.

## Test groups

| Group | Examples |
|---|---|
| Boot and header | Header `0x143 == 0x80`, cartridge type `0x147 == 0x1B`. `_is_cgb` matches the mode. Gameplay is reached within N frames. The GBC screen has more than one color. |
| Regressions | One test per fixed bug. Example: after two game-over restarts, `WY == 128`. |
| Physics | Walking right for 60 frames moves X by the expected amount. Jump height is in range. A tap jump is lower than a held jump. The player never overlaps a solid tile. |
| Gameplay | A collected fragment adds 1 to the count and clears the tile. Touching an enemy costs one life. Game over restarts in a clean state. |
| Replays | Recorded input sequences in `tests/replays/*.json` must still finish the level. This catches physics or level edits that make a level impossible. |
| Golden screenshots | Title, level start and HUD, per mode, compared pixel for pixel. `pytest --update-golden` rewrites them after an intended change. |
| Frame budget | The game counts frames where the main loop overran one VBlank. A 10-second scripted run in DMG mode must have 0 lag frames. |

## Visual review

`make shots` runs a fixed script and writes a contact sheet, `tests/out/contact_dmg.png`
and `tests/out/contact_cgb.png`, with the key screens side by side. Claude reviews these
after art or rendering changes.

## Continuous integration

A GitHub Actions workflow on `ubuntu-latest`:

1. Download and cache `gbdk-linux64.tar.gz` 4.5.0.
2. `make`
3. `pip install -r tools/requirements.txt`
4. `pytest`
5. On failure, upload `tests/out/` as a build artifact.

## What PyBoy cannot prove

- How colors and motion look on the real LCDs.
- Whether saves persist on the flashcart. EverDrive clones differ here.
- Exact VRAM timing edge cases. Use Emulicious, which is more accurate, then hardware.

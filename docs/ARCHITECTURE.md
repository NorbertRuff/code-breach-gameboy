# Architecture

How the code is put together. Read the section for the part you change. Reasons behind
each choice are in [DECISIONS.md](DECISIONS.md); where an idea came from is in
[SOURCES.md](SOURCES.md).

## Game state machine (`src/main.c`)

States are defined in `src/utils.h`:
- `INTRO_CREDITS_STATE` → `TITLE_SCREEN_STATE` → `GAMEPLAY_START` → `COREGAMELOOP` → `GAMEPLAY_END` / `LEVEL_COMPLETE`
- `COREGAMELOOP` ↔ `PAUSED` on START ("PAUSE" in sprites over the parallax band). While
  paused, SELECT loads the next level (`LEVEL_SELECT_CHEAT`).
- `GAMEPLAY_END` (a life lost) restarts the current section behind a fade
  (`restart_behind_fade`); on game over it first shows "GAME OVER" (`game_over` in
  `game_over.c`), then refills the lives and puts the section's fragments back.
- `LEVEL_COMPLETE` (the exit reached) shows the fragment summary (`level_complete`), then
  loads the next game level with full lives (`go_to_next_level`: level 1 again after the
  last; a test room restarts).

The per-frame gameplay code is `main_game_loop()`. Global state: in `main.c`
`joypad_current`, `joypad_previous`, `current_game_state`, `lives`, `lag_frames`,
`Player player`; in `actor.c` `actors[]` and `actor_count`; in `level_manager.c`
`Level current_level` and `level_index` (the game's levels `LEVEL_1`, `LEVEL_2` first,
`GAME_LEVEL_COUNT`, then the test rooms; the game starts in `LEVEL_1`, the tests in
`LEVEL_GREYBOX_TEST`); in `camera.c` `camera_x`.

Development flags in `src/utils.h`: `SKIP_INTRO_AND_START` (1: straight to gameplay; the
tests assume it), `LEVEL_SELECT_CHEAT` (1: SELECT while paused loads the next level).

## Modules

| Directory | Purpose |
|---|---|
| `src/game_state/` | `prepare_gameboy`, `intro_credits`, `start_screen`, `game_over` (also `level_complete`), `pause` (the label) |
| `src/engine/` | `platform.c` (`is_cgb`), `palette.c` (every palette write), `text.c` (screen text in the game's font), `banking.h` (`WITH_BANK`, `WITH_ROM_BANK`), `camera.c`, `screen.c` (line interrupts) |
| `src/player/` | `Player` struct and physics: `player_update`, `player_draw` |
| `src/input/` | Joypad polling; `input_read_player` copies the d-pad, Down and A into the player |
| `src/level/` | `level_data.h` (ROM level format), `level_manager.c` (`level_load`, `Level` state, HUD), `level_stream.c` (collision ring, column streaming, fragments, gates, exit), `section.c` (quotas, gates, checkpoints) |
| `src/actor/` | `actor.c` (spawning, type table, update, hit test, drawing), `bot.c` (the patrol bot), `lift.c` (the lift) |
| `src/sound/` | `sound.c` (the effect player), `effects.c` (generated: `make sounds`), `music.c` (the level's song through hUGEDriver) |
| `src/graphics/`, `src/levels/`, `src/music/` | Generated, banked: `make assets`, `make levels`, `make music` (plus hand-made `splashscreen.c`, `hud_digits.c`, `font.c`) |
| `graphics/`, `levels/`, `music/`, `sounds/` | Sources: PNGs and Tiled `.tsx`, Tiled maps `.tmj`, hUGETracker songs `.uge`, FX Hammer effect packs `.sav` and the effect list `effects.txt` |
| `lib/hUGEDriver/` | The music driver's header and GBDK library (6.1.3, public domain) |
| `tests/` | PyBoy tests: `gamelib.py` helpers, `conftest.py` fixtures and tiers ([TESTING.md](TESTING.md)) |
| `tools/` | `tiled2c.py`, `sketch2tmj.py`, `uge2c.py`, `fxhammer2c.py`, `gbconvert.py`, `levelpreview.py` + `physics.py`, `frame_budget.py`, `render_sounds.py`, screenshot tools, `savetest/`, `soundtest/` |

## Player physics (`src/player/`)

Integer arithmetic only. Values and their sources are in `player.h` and RESEARCH.md 2.2.
- Positions `x`, `y` are `uint16_t` in 1/32 px (`TO_PIXELS(pos)` = `pos >> 5`, tile =
  `pos >> 8`). `x` is the body's center, `y` its lowest row; the body is 16×32 px.
- Velocities are `int8_t` in 1/32 px per frame. Running accelerates toward
  `input_x * RUN_SPEED_MAX`, weaker in the air.
- Jump: `JUMP_SPEED` up, light gravity while rising with A held, strong gravity when
  falling or after A is released (jump cut), half gravity near the top (apex hang).
  `coyote_frames` and `jump_buffer_frames` count down.
- Drop through: Down + A with nothing solid under the feet moves the feet 1 px down into
  the one-way row (`drop_through`); otherwise Down + A jumps.
- Collision per axis against the tiles the box enters: walls over the full body height,
  floors and one-way tops only when the feet enter a new tile row going down, ceilings
  with up to `CORNER_CORRECTION` px of sideways slide. Moves stay under 8 px per frame.
- Tile coordinates: a position's tile is its high byte (`LEFT_COLUMN`, `FEET_ROW`,
  `head_row`); `column_has` walks one collision ring column with a pointer.
- **`tools/physics.py` is a frame-exact copy** for the level preview. Change both
  together; `tests/test_levelpreview.py` compares them frame by frame.
- `player_draw`: pivot = box top-left + `programmer_PIVOT_X/Y` + `DEVICE_SPRITE_PX_OFFSET_X/Y`.

## Level collision and streaming (`src/level/`)

Levels come from Tiled ([LEVEL_PIPELINE.md](LEVEL_PIPELINE.md)), 13 map rows drawn at
screen rows 3–15. `collision_ring[32 * 16]` in WRAM holds the collision of the 32
background-map columns (`RING_INDEX(tile_x, tile_y)`, `tile_y` in screen rows).
`level_stream_update(camera_x)` draws each column that enters the view, both directions:
its ring column at once, its tiles at the start of the next update (`level_stream_flush`,
first in `main_game_loop`: straight into VRAM while VBlank lasts, else `set_bkg_tiles`). Tile types: `COLLISION_NONE`, `SOLID`, `ONE_WAY`, `HAZARD`,
`COLLECTIBLE` (a fragment), `EXIT`. `LEVEL_COLLISION_AT(level, x, y)` (a macro, pass plain
variables) returns a tile's type; outside the level or in a HUD row it is `NONE`.
Collected fragments are a bitset, so they stay gone when their column is drawn again.

Fragments, gates and the exit are tile objects in Tiled: `draw_column` puts their marker
tiles over the map (a closed gate fills its column from its bottom row up to row 3) with
their object's collision. `current_level` holds the play state: `gates[]`, `section`
(0 = A), fragments and collected per section. `section.c` opens the current gate when the
player walks into it with the quota: one row every 2 frames, the next section starts, the
camera gets a left limit just past the gate, and a restart puts the player right of it.

## Actors (`src/actor/`)

Moving level objects (patrol bots and lifts) are actors; the player is not. `actors_spawn`
reads the level's entities in x order with a cursor and spawns each one that has an
`ActorType` once it is within 16 px of the screen's right edge. A bot walks at its
entity's `speed` (1 or 2 px per frame), kept as the signed `velocity_x`. An `ActorType`
has `spawn`, `update`, a metasprite (pivot = body center, feet row), its tiles, a hit
box and a half width (to skip actors off screen). Actors stay packed in `actors[0..actor_count-1]`; one more than a screen behind the
camera is removed until `actors_reset(section)`. `draw_sprites` (`main.c`) rebuilds the
sprite list every frame: the player (OAM 0–3), the actors, then `hide_sprites_range`.

Lifts (`is_lift` in their type) move in `actors_move_lifts`, before `player_update`; the
others in `actors_update`, after it; with no lift in play (`lifts_in_play`), the lift code
returns at once. A lift moves by `velocity_y` (1 px up or down) within `min_y`..`max_y`
(its top row), waits `wait_frames` at each end, and keeps the step it took this frame in
`step_y` (0 while it waits). The player
finds the lift under its body with `actors_lift_at(body_x)` (lifts never overlap in x):
`ride_lift` adds the lift's step when the feet stood on the row above its top before the
step, `move_y` lands on that row when the feet reach it falling, and `update_ground`
counts it as ground. Lifts don't hurt (`actors_touch` skips them).

## Camera and screen splits

`camera_update()` keeps the focus (player x + a look-ahead that eases to 24 px in the
running direction) within 8 px of screen x 80, clamped to the level and to
`camera_left_limit`; `camera_reset()` snaps it for a level start or respawn, followed by
`level_stream_reset()`. `camera_lock_left(x)` sets a new left limit (an opened gate) that
takes force once the camera gets there; the player can't go left of it either. Tests
teleport the player only forward past an opened gate. Levels are up to 255 tiles wide.

`src/engine/screen.c`, started at boot: the VBlank handler sets `SCX` for the band (half
the camera) and turns sprites on; the line interrupt at line 23 sets the level's `SCX`
and then runs the frame's music step; the one at line 127 turns sprites off over the HUD.
Never write `SCX` or `HIDE_SPRITES`/`SHOW_SPRITES` directly: use `screen_scroll(camera_x)`
(applied at the next VBlank) and `screen_show_sprites(show)`. The band is drawn once
into rows 0–2 at level load.

## Music and sound (`src/sound/`)

`music/*.uge` → `make music` (`tools/uge2c.py`) → banked `hUGESong_t` in `src/music/`.
`music_play_level` starts a level's song (`hUGE_init` with its bank switched in), and
`music_play_title`, `_game_over` and `_level_complete` the screens' songs;
`music_step` runs once per frame from the line-23 interrupt, with the song's bank switched
in and restored. Credit every song and effect pack in `CREDITS.md`.

Sound effects: `sounds/effects.txt` (name, pack, effect number, priority) → `make sounds`
(`tools/fxhammer2c.py`) → banked `sound_effects[]` in `src/sound/effects.c` and
`EFFECT_*` numbers in `effects.h`. An effect is a list of 8-byte steps (frames, channels,
then the CH2 and CH4 register values). `sound_play(EFFECT_...)` starts one unless a
higher-priority one plays, and sets its channels in `hUGE_mute_mask` so the music leaves
them alone; `sound_step`, called by `music_step` before the song's step, writes the next
step when its frames are up, and after the last one silences the channels and unmutes
them. `make soundtest` builds `obj/soundtest.gb` (every effect of both packs with the same
player, and every song of `music/packs/*.zip` through `uge2c.py --table`) and renders each
to `tests/out/sounds/*.wav` (`tools/render_sounds.py`).

## Frame budget

Each gameplay update must end before the next VBlank. The ROM counts lag frames in
`lag_frames`; the tests allow an update to end at most 140 lines after VBlank starts, of
154 (`game.track_frame_lines()`). `make budget` shows where the lines go;
`tools/frame_budget.py --route level1` (or `level2`) profiles a level's replay. The music
step costs about 9 lines per frame (up to 18). The current numbers are in the ROADMAP
status.

## Memory map

- **Background tiles** (GBDK uses `0x8800` addressing, so background tiles 128–255 share
  memory with sprite tiles 128–255): the level's from 0, the HUD frame from 64, the HUD
  digits 133–144, the font 160–207.
- **Sprite tiles:** the player 0–39, the bot 40–43, the pause label 44–53, the lift 54–55. The player is
  4 OAM entries (8×16 mode), a bot 2.
- **ROM banks:** assets are made with `png2asset -b 255` (autobank, `BANKREF(name)`) and
  read inside `WITH_BANK(name, ...)`, one asset per `WITH_BANK`, only from bank 0 code.
  Code that never runs in a gameplay frame and doesn't use `WITH_BANK` lives in a ROM bank
  (`#pragma bank 255`, `BANKED`): `intro_credits.c`, `game_over.c`, `pause.c`,
  `prepare_gameboy.c`, `hud_digits.c`. Metasprite tables are banked data too.
- **GBC palettes:** background 0 the level, 1 the band, 2 the HUD; sprite 0 the player
  (and for now the bots), 1 the player while invincible.

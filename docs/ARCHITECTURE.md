# How it works

How ApexLab: Code Breach runs on a Game Boy: one cartridge ROM, written in C with
GBDK-2020, that plays in color on a Game Boy Color and in 4 shades on the original Game
Boy. Each section says what the game does, how the code does it, and where the idea came
from. The reasons behind each choice are one line each in [DECISIONS.md](DECISIONS.md);
every source with its license is in [SOURCES.md](SOURCES.md) and [../CREDITS.md](../CREDITS.md).
For a code change, read the section for the part you change.

## The hardware in one paragraph

A Game Boy draws a 160×144 screen from 8×8 tiles: a 32×32-tile background map that
scrolls, a window layer on top (the HUD), and up to 40 sprites, at most 10 per screen
line. The CPU may write video memory (VRAM) only while the screen isn't drawing a line:
in VBlank (10 of the 154 lines of a frame, 60 frames per second) or briefly in HBlank.
The cartridge's code and data live in 16 KB ROM banks; bank 0 is always mapped, one other
bank at a time is switched in at `0x4000`. Work RAM is 8 KB. A Game Boy Color adds a
second VRAM bank for per-tile attributes (palette, flips) and 8 palettes of 4 colors each
for the background and 8 for sprites; the original Game Boy has one 4-shade palette for
the background and two for sprites. (Pan Docs is the reference for all of this.)

## The cartridge and the build

- One ROM, `obj/ApexLab.gb`, built by `make` with GBDK-2020 4.5.0. The header says
  "GBC-enhanced" (`-Wm-yc`, not the GBC-only `-yC`): a GBC runs it in color, a DMG
  ignores the flag. The title in the header (`-Wm-ynAPEXLAB`) can be 11 characters at
  most with the GBC flag.
- The cartridge is an MBC5 without RAM (`-Wl-yt0x19`), the ROM size picked by the
  linker (`-Wm-yoA`). No battery saves: the game uses passwords (see Saves).
- `-autobank` places every file marked `#pragma bank 255` into a ROM bank; the build
  prints ROM and RAM use per bank (`romusage`) and fails if a bank overflows.
- Generated C files (levels, music, art, sound effects) are made from their sources by
  Python tools and committed with them; the tests fail when they are out of date.
- `make release` builds `release/ApexLab.gb`, the same build with the intro and title
  screen on (`make` skips them for the tests: `SKIP_INTRO_AND_START`).

| Directory | What's in it |
|---|---|
| `src/game_state/` | Screens and game flow: `prepare_gameboy` (boot), `intro_credits`, `start_screen` (the title picture), `title_menu` (NEW GAME / CONTINUE, the password screen, the level select), `menu` (blank menu screen, button presses, `menu_choose`), `progress` (furthest level, 100% marks, the password), `game_flow` (level start, section restart, losing a life), `game_over` (also `level_complete`), `story` (the boot screen, the AI logs, the ending and credits), `kernel` (the boss), `pause` (the label). All banked; the title picture's loader `draw_title_picture` is `NONBANKED`. |
| `src/engine/` | `platform.c` (`is_cgb`), `palette.c` and `palette_effects.c` (every palette write), `text.c` (screen text in the game's font), `banking.h` (`WITH_BANK`, `WITH_ROM_BANK`), `camera.c`, `screen.c` (line interrupts) |
| `src/player/` | The `Player` struct and physics: `player_update`, `player_draw` |
| `src/input/` | Joypad polling; `input_read_player` copies the d-pad, Down and A into the player |
| `src/level/` | `level_data.h` (the ROM level format), `level_manager.c` (`level_load`, the `Level` state, the HUD), `level_stream.c` (collision ring, column streaming, fragments, gates, exit), `section.c` (quotas, gates, checkpoints) |
| `src/actor/` | `actor.c` (spawning, type table, update, hit test, drawing), `bot.c`, `chaser.c`, `lift.c`, `packet.c` (KERNEL's falling packet) |
| `src/sound/` | `sound.c` (the effect player), `effects.c` (generated), `music.c` (songs through hUGEDriver) |
| `src/graphics/`, `src/levels/`, `src/music/` | Generated, banked: `make assets`, `make levels`, `make music` |
| `graphics/`, `levels/`, `music/`, `sounds/` | Sources: indexed PNGs and Tiled `.tsx`, Tiled maps `.tmj`, hUGETracker songs `.uge`, FX Hammer effect packs `.sav` and the picks in `effects.txt` |
| `lib/hUGEDriver/` | The music driver's header and GBDK library (6.1.3, public domain) |
| `tests/`, `tools/` | PyBoy tests ([TESTING.md](TESTING.md)) and the Python converters and checkers |

## Boot and the game states (`src/main.c`)

`main` calls `prepare_gameboy` once: it detects the console (`is_cgb`, from
`_cpu == CGB_TYPE`), loads the palettes before the display turns on (so a GBC never
flashes white), turns on the display in 8×16 sprite mode, starts the screen-split
interrupts and turns on the sound chip. Then it runs one game state per frame and waits
for VBlank (`wait_vbl_done`).

States are in `src/utils.h`:
- `INTRO_CREDITS_STATE` → `TITLE_SCREEN_STATE` → `GAMEPLAY_START` → `COREGAMELOOP` → `GAMEPLAY_END` / `LEVEL_COMPLETE`.
- `TITLE_SCREEN_STATE` (`run_title_screen`, then `title_menu`) returns once a level is
  chosen: NEW GAME (progress reset, the boot screen, level 1) or CONTINUE (the password
  screen, then the level select). B on an empty password shows the title again.
- `COREGAMELOOP` ↔ `PAUSED` on START ("PAUSE" in sprites over the parallax band). While
  paused, with A held, SELECT loads the next level (`LEVEL_SELECT_CHEAT`) and B toggles
  `no_death_cheat` (`NO_DEATH_CHEAT`).
- `GAMEPLAY_END` (a life lost): `lose_life` restarts the current section behind a fade
  (`restart_behind_fade`) and returns the next state. On game over, `game_over` asks
  CONTINUE (the lives refill, the section's fragments come back) or QUIT (the title).
- In level 5 (`LEVEL_BOSS`), `main_game_loop` first calls `kernel_update`; a finished
  upload goes to `BOSS_HIT` (`kernel_hit`: hit-stop, shake, flashes, the next phase), the
  third to `ENDING` (`story_ending`, then the title). The uploads count as the arena's
  fragments, so the HUD shows them, a lost life keeps them and a game over clears them.
- `LEVEL_COMPLETE` (the exit reached) notes the level in the progress, shows the patch,
  the fragment summary and the password, then the elevator ride's log and the next level
  with full lives (`go_to_next_level`).

Global state: in `main.c` `joypad_current`, `joypad_previous`, `current_game_state`,
`lives`, `lag_frames`, `Player player`; in `actor.c` `actors[]` and `actor_count`; in
`level_manager.c` `Level current_level` and `level_index` (the game's levels first,
`GAME_LEVEL_COUNT`, then the test rooms; the tests start in `LEVEL_GREYBOX_TEST`); in
`camera.c` `camera_x`; in `progress.c` `furthest_level` and `perfect_levels`.

## One frame of play

`main_game_loop` runs once per frame: write last frame's streamed column to VRAM (while
VBlank lasts), check START, run KERNEL in level 5, read the joypad, move the lifts, move
the player, check falls and the exit, open gates, move the camera and stream the level,
move the actors, update the HUD, check hits, and rebuild the sprite list.

**Frame budget.** The update must end before the next VBlank, or the screen shows the
same picture twice (a lag frame). The tests allow it to end at most 140 lines after
VBlank starts, of 154, and the ROM counts lag frames in `lag_frames`: every VBlank
during an update (from GBDK's `sys_time`) is one. `make budget` shows where the lines go;
the current numbers are in the ROADMAP status. Rules that keep it there: no floats, no
`/` or `%` in per-frame code (SDCC's divide costs about 6 lines), tile coordinates from a
position's high byte, and code that doesn't run every frame kept out of bank 0.

## Tiles, sprites and VRAM

- **Background tiles** (GBDK uses `0x8800` addressing, so background tiles 128–255 share
  memory with sprite tiles 128–255): the level's tileset from 0 and its band right after
  (together at most 160), the font 160–208 (1-bit, loaded in color 3 on 0; the HUD's
  digits are the font's), the HUD frame 209–218; 219–255 free.
- **Sprite tiles:** the player 0–67 (all 10 frames stay loaded, so no per-frame copy),
  the bot 68–75, the chaser 76–83, the lift 84–89, KERNEL's packet 90–97
  (`src/actor/actor.h`), the pause label 98–107.
- **Sprites:** 8×16 mode. The 16×32 player is a metasprite of 4 hardware sprites
  (`move_metasprite_ex`, flipped with `move_metasprite_flipx`); a bot is 2.
  `draw_sprites` rebuilds the sprite list every frame: the player (OAM 0–3), the actors,
  the pause label, then `hide_sprites_range` hides only what the last frame used and this
  one doesn't.
- **Writing VRAM:** GBDK's `set_bkg_tiles` waits for each tile until VRAM is free, which
  is slow mid-frame. The streamed column is written straight into VRAM at the start of the
  next update, while VBlank lasts (2 lines instead of 12).
- **The HUD** is the window layer at an absolute position (`move_win(7, 128)`; `WX = 7`
  is the left edge), 2 tile rows.
- **Art** comes from indexed PNGs (palettes in groups of 4, light to dark) through
  `tools/gbconvert.py` and GBDK's `png2asset` (`make assets`, `-noflip`, tileset tiles in
  image order); [ART_PIPELINE.md](ART_PIPELINE.md) has the steps.

## ROM banks

- Art, levels, songs and the code that isn't per-frame are in ROM banks (`png2asset -b
  255`, `#pragma bank 255` with `BANKED` functions); autobank places them.
- Banked data is read from bank 0 code inside `WITH_BANK(name, ...)` (`banking.h`): map
  the asset's bank in (`SWITCH_ROM`, the bank from `BANKREF(name)`), read, map the old
  bank back. One asset per `WITH_BANK`.
- Bank 0 holds what runs every frame plus the loaders that read banked data; its free
  bytes are in the ROADMAP status. Banked code: `intro_credits.c`, `game_over.c`,
  `pause.c`, `prepare_gameboy.c`, `game_flow.c`, `menu.c`, `title_menu.c`, `progress.c`,
  `story.c`, `kernel.c`, `palette_effects.c`. png2asset's metasprite tables are banked
  data; the actors' are written by hand in bank 0.
- Screen text uses the game's own font (`text.c`) instead of `printf`, which took 2.6 KB
  of bank 0.

## Levels and collision (`src/level/`)

**From Tiled to the ROM.** Each level is a Tiled map (`levels/*.tmj`) on its tileset
(`graphics/tilesets/*.tsx`, a tile's class = its collision type).
`tools/tiled2c.py` (`make levels`) writes banked C: the map columns, the entities sorted
by x, the sections, and per tileset a `TilesetData` with each tile's collision type and
GBC palette (read back from the indexed PNG: color index / 4). It also checks the level
rules (pit widths, room right of each gate). `make preview` draws every level with its
jump arcs and checks that every fragment and the exit can be reached, by simulating
jumps with `tools/physics.py`, a frame-exact copy of the player physics.
[LEVELS.md](LEVELS.md) has the Tiled setup and how to add a level.

**Streaming.** A level is up to 255 tiles wide, the background map only 32. Maps are 13
rows drawn at screen rows 3–15. `level_stream_update(camera_x)` draws each column that
enters the view, both directions: its collision into the ring at once, its tiles at the
start of the next update (`level_stream_flush`, straight into VRAM while VBlank lasts,
else `set_bkg_tiles`), and on GBC their palettes into VRAM bank 1 right after them.
(From GBDK's `large_map` example and Larold's "Drawing Large Maps".)

**Collision.** `collision_ring[32 * 16]` in WRAM holds the collision of the 32 map
columns (`RING_INDEX(tile_x, tile_y)`), filled from the tileset's table (copied to WRAM
at level load, with the palette table: `tile_collision`, `tile_palette`, 256 bytes each).
Types: `COLLISION_NONE`, `SOLID`, `ONE_WAY`, `HAZARD`, `COLLECTIBLE` (a fragment),
`EXIT`. `LEVEL_COLLISION_AT(level, x, y)` (a macro, pass plain variables) returns a
tile's type; outside the level or in a HUD row it is `NONE`. Collected fragments are a
bit per level column, so they stay gone when their column is drawn again.

**Sections, gates, the exit.** Fragments, gates and the exit are tile objects in Tiled:
`draw_column` puts their marker tiles over the map with their collision (a closed gate
fills its column from its bottom row up to row 3). `section.c` opens the current gate
when the player walks into it with the quota: one row every 2 frames, the next section
starts, the camera gets a left limit just past the gate, and a restart puts the player
right of it. The exit is the elevator; `level_complete` draws the open one
(`exit_open` in `TilesetData`). `level_show_tile` writes one cell with its palette (a
pickup, a gate row, the open elevator).

## Player physics (`src/player/`)

Integer arithmetic only, as in the GB Studio engine; values in `player.h`.
- Positions `x`, `y` are `uint16_t` in 1/32 px (`TO_PIXELS(pos)` = `pos >> 5`, tile =
  `pos >> 8`). `x` is the body's center, `y` its lowest row; the body is 16×32 px.
- Velocities are `int8_t` in 1/32 px per frame. Running accelerates toward
  `input_x * RUN_SPEED_MAX`, weaker in the air.
- Jump (from Kyle Pittman's "Building a Better Jump", Celeste and Super Mario Bros.):
  `JUMP_SPEED` up, light gravity while rising with A held, strong gravity when falling or
  after A is released (jump cut), half gravity near the top (apex hang). Coyote time
  (6 frames) and a jump buffer (5) count down; a new press of A is needed to jump.
- Drop through: Down + A with nothing solid under the feet moves the feet into the
  one-way row (`drop_through`); otherwise Down + A jumps.
- Collision per axis against the tiles the box enters: walls over the full body height,
  floors and one-way tops only when the feet enter a new tile row going down, ceilings
  with up to `CORNER_CORRECTION` px of sideways slide. Moves stay under 8 px per frame,
  so no tile is skipped. `column_has` walks one collision ring column with a pointer.
- **`tools/physics.py` is a frame-exact copy** for the level preview. Change both
  together; `tests/test_levelpreview.py` compares them frame by frame.
- `player_draw`: pivot = box top-left + `programmer_PIVOT_X/Y` +
  `DEVICE_SPRITE_PX_OFFSET_X/Y` (hardware sprite coordinates start at (8, 16)).

## Actors and the boss (`src/actor/`, `kernel.c`)

Moving level objects (patrol bots, chasers, lifts, KERNEL's packets) are actors; the
player is not. One `ActorType` table entry per kind instead of copied code: `spawn`,
`update`, a metasprite (pivot = body center, feet row), its first sprite tile, the tile
step to its second animation frame (`frame_tiles`, every other 8 frames), a hit box and
a half width (to skip actors off screen).
- `actors_spawn` reads the level's entities in x order with a cursor and spawns each
  when it is within 16 px of the screen's right edge. Actors stay packed in
  `actors[0..actor_count-1]`, at most 8; one more than a screen behind the camera is
  removed until `actors_reset(section)`.
- A bot walks at its entity's `speed` (1 or 2 px per frame, `velocity_x`). A chaser reads
  the player's place from `actor_player_x` and `actor_player_feet` and walks toward it
  while it is in sight.
- Lifts (`is_lift`) move in `actors_move_lifts`, before `player_update`; the others in
  `actors_update`, after it; with no lift in play (`lifts_in_play`) the lift code returns
  at once. A lift moves 1 px per frame within `min_y`..`max_y`, waits `wait_frames` at
  each end, and keeps this frame's step in `step_y`. The player finds the lift under its
  body with `actors_lift_at(body_x)`: `ride_lift` adds the lift's step when the feet
  stood on it, `move_y` lands on it, `update_ground` counts it as ground. Lifts don't hurt.
- **KERNEL**, the boss of level 5, is drawn with background tiles: its face, eyes,
  glitch and terminals are cells of the mainframe tileset, painted in level 5's map.
  `kernel.c` swaps tiles by tileset id (each keeps the palette of the cell it replaces,
  so no attribute writes), drops packet volleys into lanes (packets are actors), runs the
  lift and the corrupted floor per phase, and counts the 3 uploads (hold Up on the lit
  terminal). As sprites, the face would need 42, more than the 40 there are. A volley's
  packets start 8 frames apart at 2 px per frame, so they are 16 px apart and never share
  a line. The shake moves `SCX` only: a vertical shake would move the band and HUD splits.
  The attack patterns are fixed tables, so the replay test always wins.

## Camera, parallax and screen splits

- `camera_update()` keeps the focus (player x + a look-ahead that eases to 24 px in the
  running direction) within 8 px of screen x 80, clamped to the level and to
  `camera_left_limit` (from the GB Studio camera and Itay Keren's "Scroll Back").
  `camera_reset()` snaps it for a level start or respawn, followed by
  `level_stream_reset()`. `camera_lock_left(x)` sets a new left limit (an opened gate)
  that takes force once the camera gets there; the player can't go left of it either.
- The screen has three parts: a 3-row parallax band at the top, the play area, and the
  HUD. `src/engine/screen.c` changes the scroll mid-frame with screen-line (LYC)
  interrupts (Larold's "Parallax Backgrounds with GBDK-2020"): the VBlank handler sets
  `SCX` for the band (half the camera) and shows sprites; the interrupt at line 23 waits
  for HBlank, sets the level's `SCX`, then runs the frame's music step; the one at line
  127 hides sprites over the HUD. The band is drawn once into rows 0–2 at level load.
- Never write `SCX` or `HIDE_SPRITES`/`SHOW_SPRITES` directly: use
  `screen_scroll(camera_x)` (applied at the next VBlank, so a frame never mixes two
  updates) and `screen_show_sprites(show)`.

## Palettes and fades (`src/engine/palette.c`, `palette_effects.c`)

Only the palette module writes palettes (`tests/test_palette.py` fails on any other
write), and it has both paths: DMG registers `BGP_REG`/`OBP0_REG`/`OBP1_REG` and the GBC
palette RAM. Every palette is ordered lightest to darkest, so the same tile data shows
the right shades on both.
- **GBC background palettes:** 0–3 the level's tileset (a tile's attribute is its
  tileset palette, from the per-tile table), 4 the band base, 5 the HUD chassis, 6 the
  HUD accents (red heart, gold floppy), 7 the band accent. **Sprite palettes:** 0 the
  player, 1 the pale invincible player (DMG blinks instead), 2 the bots and packets, 3
  the lifts, 4 the chasers. Menu screens use background 0 with grays
  (`palette_use_grays`); the story screens a green terminal palette on GBC
  (`palette_use_terminal`).
- `is_cgb` guards every VRAM bank 1 write: on DMG it would overwrite the tile map.
- **Fades** in 4 steps: on DMG by stepping `BGP_REG` (`0xE4 → 0xF9 → 0xFE → 0xFF`); on
  GBC by scaling all three RGB555 channels at once, shifting the color word and masking
  off the bits that slid into the next channel (`(c >> 2) & 0x1CE7`). Multiplying took 2
  frames per step.
- **Flashes** (game over, level complete, a boss upload) stay at 3 per second or fewer (WCAG 2.3.1).

## Music and sound (`src/sound/`)

- Songs: `music/*.uge` (hUGETracker) → `make music` (`tools/uge2c.py`, a Python port of
  hUGETracker's own export) → banked `hUGESong_t` in `src/music/`. hUGEDriver plays them:
  `music_play_level` and the screen songs call `hUGE_init` with the song's bank switched
  in; `music_step` runs once per frame from the line-23 interrupt (not VBlank, where it
  delayed every update), with the song's bank switched in and restored. About 9 lines
  per frame, up to 18.
- Sound effects: Tronimal's GBFX packs (FX Hammer `.sav`) → `sounds/effects.txt` (name,
  pack, effect number, priority) → `make sounds` (`tools/fxhammer2c.py`) → banked
  `sound_effects[]` and `EFFECT_*` numbers. An effect is a list of 8-byte steps (frames,
  channels, then the CH2 and CH4 register values). `sound_play(EFFECT_...)` starts one
  unless a higher-priority one plays, and mutes its channels in `hUGE_mute_mask` so the
  music leaves them alone; `sound_step`, called by `music_step`, writes the next step and
  unmutes the channels after the last (the pattern of VGM2GBSFX's music manager).
- `make soundtest` builds `obj/soundtest.gb` with every effect and every song of both
  music packs, and renders each to a WAV (`tools/render_sounds.py`).

## Saves: passwords

The owner's flashcart kept no battery save in any test ROM (MBC5, MBC1, MBC3; the
test ROMs are `make savetest`), so the game uses passwords. `progress.c` keeps one
progress byte: the furthest level and a 100% mark per finished level. A password is 4
letters from `BCDFGHJKLMNPRTWX`: the progress byte and a check byte that is a one-to-one
mix of it, so every single wrong letter is refused. The level complete and game over
screens show it; CONTINUE on the title asks for it, then offers the level select.
`tools/password.py` is a Python copy that lists and checks them.

## Testing

PyBoy runs the ROM headless in DMG and GBC mode: symbols come from the `.sym` file,
game state is read at a fixed point in the frame (a hook on `player_draw`), level replays
are moves that watch the game ("run to x, jump"), and golden screenshots catch visual
changes. Each level's replay also checks for lag frames and sprites per line.
[TESTING.md](TESTING.md) has the commands and conventions.

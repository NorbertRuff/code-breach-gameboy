# Sources

Where each technique, rule and asset came from, and where it lives in the project.
Together with [DECISIONS.md](DECISIONS.md) (what was decided and why), this is the raw
material for the final "how it works" document (roadmap M9) and for the credits.

**Keep it current:** when code or an asset takes an idea from a tutorial, a repo, a
document or a person, add a row here in the same change. Status is *in use* or
*planned*. Licenses of reference repos are in [RESEARCH.md](RESEARCH.md) section 5.

## Engine and hardware techniques

| Technique | Source | Used in | Status |
|---|---|---|---|
| Reading banked data: map the asset's bank in, read, map the old bank back (`WITH_BANK`) | GBDK docs, ROM banking (`NONBANKED` loader pattern, `SWITCH_ROM`, `CURRENT_BANK`) | `src/engine/banking.h`, `level_manager.c`, `player.c`, `start_screen.c` | In use |
| Autobank build flags (`-autobank -Wb-ext=.rel`, `-Wm-yoA`), MBC5 header (`-Wl-yt0x19`; `0x1B` with RAM + battery for the save test ROMs), GBC-compatible header (`-Wm-yc`), `.sym` output (`-Wm-yS`) | GBDK docs, toolchain settings; checked by the research agent on GBDK 4.5.0 | `Makefile` | In use |
| One Makefile for PowerShell, cmd and Git Bash: force `cmd.exe` on Windows | Own finding (make picks `sh.exe` only when Git Bash is on PATH) | `Makefile` | In use |
| Hardware detection `_cpu == CGB_TYPE`; guard VRAM bank 1 writes on DMG | GBDK docs (supported consoles), Pan Docs | `src/engine/platform.c` | In use |
| Gray GBC palettes matching the DMG shades, attribute maps cleared at boot | Own design, from Pan Docs on CGB palettes and attributes | `src/engine/palette.c` `palette_init`, `src/engine/platform.c` | In use (until the M5 palettes) |
| Window HUD at an absolute position: `move_win(7, 128)`, `WX = 7` is the left edge | GBDK docs (`move_win`, `scroll_win` is relative), flozz/gameboy-examples example 08 | `level_manager.c` `set_hud_window_tiles` | In use |
| Metasprites for the 16×32 player (4 hardware sprites in 8×16 mode), `move_metasprite_ex` / `move_metasprite_flipx` | GBDK docs, `metasprites.h`; Larold's metasprite tutorial | `player.c` `player_draw` | In use |
| DMG fades by stepping `BGP_REG` (`0xE4 → 0xF9 → 0xFE → 0xFF`) | The project's early versions; same technique in flozz example 09 | `src/engine/palette.c` `palette_fade_to` | In use (DMG) |
| Flashes at most 3 per second | WCAG 2.3.1 "Three Flashes or Below Threshold" (found by the game-feel research) | `src/engine/palette.c` `palette_flash` | In use |
| GBC fades and the pale invincible player: scale all three RGB555 channels at once by shifting the whole color word and masking off the bits that slid into the next channel (`(c >> 2) & 0x1CE7`) | Own design; the masking is the usual trick for packed values ("SIMD within a register"); palette format from Pan Docs, CGB registers | `src/engine/palette.c` | In use |
| Code that isn't per-frame in ROM banks: `#pragma bank 255` and `BANKED` functions, placed by autobank | GBDK docs, ROM/RAM banking and banked functions | `intro_credits.c`, `game_over.c`, `pause.c` | In use |
| Load a level while the screen is black, then fade in | Own fix (review of the early versions, bug 3) | `main.c` `GAMEPLAY_START` | In use |
| Parallax: a screen-line (LYC) interrupt changes `SCX` per band | Larold's Retro Gameyard, "Parallax Backgrounds with GBDK-2020"; GBDK examples `gb/lcd_isr_wobble`, `cross-platform/irq` | `src/engine/screen.c` `on_split_line` | In use |
| Hide sprites over the bottom HUD with the same interrupt (`HIDE_SPRITES` at line 127) | Larold's Flappy Bird tutorial; BlackCastle `src/main.c` (idea only, no license) | `src/engine/screen.c` `on_split_line` | In use |
| Wait for HBlank (`STAT` mode bits) before a mid-frame register write, so it starts on the next line | Pan Docs, STAT and rendering timing; GBDK `wait_int_handler` notes | `src/engine/screen.c` | In use |
| Level files: Tiled JSON maps (`.tmj`, CSV tile data, object layers, tile objects with `gid`, flip flags in the top 3 bits of a gid) and TSX tilesets (tile `type` = class, custom properties) | Tiled documentation, "JSON Map Format" and "TMX Map Format" | `tools/tiled2c.py`, `levels/`, `graphics/tilesets/greybox.tsx` | In use |
| Tileset tiles in image order, one per PNG tile: `png2asset -map -keep_duplicate_tiles -tiles_only` | `png2asset` help (GBDK 4.5.0) | `Makefile` `assets`, `src/graphics/greybox.c` | In use |
| Map streaming: draw the column entering the view when the camera's tile column changes, on either side | GBDK example `cross-platform/large_map`; Larold's "Drawing Large Maps" | `src/level/level_stream.c` | In use (with `set_bkg_tiles`, see DECISIONS.md) |
| Collision kept in a 32×16 RAM ring, filled from a per-tileset ROM table while streaming | BlackCastle `include/global.h` (idea only), marioland-gbdk, CrossZGB `scroll_collisions[256]` (MIT) | `src/level/level_stream.c` | In use |
| Positions in 1/32 px; coyote time, jump buffer, hold-to-extend jump as small countdowns | GB Studio engine `gbvm/src/states/platform.c` (MIT) | `player.c` | In use |
| Jump values: two gravities (jump cut), apex hang, coyote 6 frames, buffer 5 frames, corner correction | Kyle Pittman "Building a Better Jump" (GDC 2016); Celeste `Player.cs` (Maddy Thorson); SMBpedia movement data | `player.h`, `player.c` | In use |
| Strong gravity on every fall, light gravity only while rising with A held | Super Mario Bros. (SMBpedia movement data) | `player.c` `choose_gravity` | In use |
| Collision per axis; check points no more than 8 px apart along the box edge (here: every tile row and column the edge spans) | Larold's collision tutorial; RESEARCH.md 2.2 | `player.c` `move_x`, `move_y`, `column_has`, `area_has` | In use |
| Metasprite position in hardware sprite coordinates: screen origin at (8, 16), `DEVICE_SPRITE_PX_OFFSET_X/Y` | GBDK `gb/metasprites.h` and `gb/hardware.h`; Pan Docs, OAM | `player.c` `player_draw` | In use |
| Jump only on a new press of A | Own fix (review P1); edge detection as in flozz example 02 | `input.c` | In use |
| Spawn list sorted by X, read with a cursor, start index per section | BlackCastle `src/monster_data.c` (idea only), marioland-gbdk (idea only) | `tools/tiled2c.py`, `src/actor/actor.c` `actors_spawn`, `actors_reset` | In use |
| Removing actors that are off screen | GB Studio engine `src/core/actor.c` (MIT): off-screen actors are deactivated (here: removed a screen behind the camera) | `src/actor/actor.c` `actors_update` | In use |
| One table entry per actor type (update, art, hit box) instead of copied code per enemy | RESEARCH.md 2.4: BlackCastle's copied draw blocks as the counter-example | `src/actor/actor.c` `actor_types`, `src/actor/bot.c` `bot_type` | In use |
| Moving platforms move before the player, and carry a player who stood on them by the step they took; one-way, so the player jumps up through them | Common platformer practice (own implementation; no code copied) | `src/actor/lift.c`, `actors_move_lifts`, `src/player/player.c` `ride_lift` | In use |
| Rebuild the sprite list every frame, then `hide_sprites_range` | zelda-gbdk `objects.c` (idea only); GBDK `metasprites` example | `main.c` `draw_sprites`, `src/actor/actor.c` `actors_draw` | In use |
| Object pools with a free list, fixed sprite ranges | cuphead_gb `bullet_pool.c` (MIT) | Projectiles, particles | Planned (M3/M8) |
| Boss drawn with background tiles, poses in the off-screen map, switched by scrolling | cuphead_gb README and maps (MIT) | KERNEL boss | Planned (M8) |
| Sound effects over music: one at a time with a priority, its channels muted in `hUGE_mute_mask` until it ends | untoxa/VGM2GBSFX `musicmanager.c` (MIT); used the same way in GB Studio and CrossZGB | `src/sound/sound.c` (own C player, not VGM2GBSFX's) | In use |
| Reading FX Hammer `.sav` files: effect channels at 0x300, 32 steps of 8 bytes per effect from 0x400, the note-to-frequency table, which registers a step writes | untoxa/VGM2GBSFX `utils/fxhammer2data.py` (MIT) | `tools/fxhammer2c.py` (rewritten for our step format) | In use |
| hUGEDriver: `hUGE_init` with the song's bank switched in, `hUGE_dosound` once per frame from an interrupt, with the song's bank switched in | Larold's hUGEDriver tutorial; hUGEDriver `gbdk_example` (public domain) | `src/sound/music.c` (called from the band split in `src/engine/screen.c`) | In use |
| `.uge` file format (version 6) and its conversion to hUGEDriver C data | hUGETracker `src/song.pas`, `src/hugedatatypes.pas`, `src/instruments.pas`, `src/codegen.pas` `RenderSongToGBDKC` (public domain), ported to Python | `tools/uge2c.py` | In use |
| Writing a streamed column's tiles at the start of the next update, while VBlank lasts, straight into VRAM | Own finding (`make budget`: GBDK's `set_bkg_tiles` waits for each tile; direct writes in VBlank need no wait, Pan Docs "VRAM access") | `src/level/level_stream.c` `level_stream_flush` | In use |
| Save header: magic value, version, size, checksum | CrossZGB `SRAM.c` (MIT); Larold's saving tutorial | Saves | Planned (M7); test ROM in use |
| Counting lag frames with the VBlank counter: every VBlank during an update is a frame without one | GBDK docs, `sys_time` in `gb/gb.h` | `src/main.c` `lag_frames` | In use |
| Look-ahead camera, deadzone, lock behind opened gates | GB Studio engine `src/core/camera.c` (MIT), camera locks; Itay Keren, "Scroll Back" | `src/engine/camera.c` | In use |
| A gate that retracts one row at a time when it opens | Game-feel research, RESEARCH.md section 3 ("bars retract one tile every 3 frames") | `src/level/section.c`, `level_stream.c` `level_open_gate_row` | In use |

## Testing

| Technique | Source | Used in | Status |
|---|---|---|---|
| PyBoy headless tests, symbol lookup from the `.sym` file | PyBoy 2.7.0 docs and source | `tests/gamelib.py`, `tests/conftest.py` | In use |
| Forcing DMG mode with PyBoy's own DMG boot ROM; waiting for the cartridge entry point | Own finding in PyBoy's `core/mb.py` and `core/bootrom.py` | `tests/gamelib.py` `boot()` | In use |
| Save power-cycle test: stop with `ram_file`, boot again with it | struktured-labs/quintra `scripts/test_suspend.py` (MPL-2.0, idea only) | `tests/test_savetest.py` | In use |
| ROM layout checks: `_HOME` below `0x4000`, header bytes, checksums | quintra `scripts/check_rom_layout.py`, `check_cartridge.py` (idea only) | `tests/test_rom.py` | In use |
| Known bugs as strict expected failures; regression tests checked against the old build | Own convention | `tests/test_known_bugs.py`, `tests/test_regressions.py` | In use |
| Replaying from one frame with `save_state` / `load_state` | PyBoy 2.7.0 docs | `tests/gamelib.py` `save`, `load`; `tests/test_physics.py` | In use |
| Golden screenshots: render a screen and compare it pixel for pixel with a stored picture, rewritten on purpose (`make goldens`) | Snapshot testing, a common test technique; PyBoy 2.7.0 `screen.image` | `tests/test_golden.py` | In use |
| Level replays as moves that watch the game (run to x, jump, follow a bot) instead of fixed frame counts | Own design, after frame-exact routes broke with small layout changes | `tests/gamelib.py` `Pilot`, `tests/test_level_1.py` | In use |
| Reading game state at a fixed point in the frame: copy the struct from a `hook_register` callback on `player_draw`; test writes to the player wait for the same point | Own finding (PyBoy frames end mid-update) | `tests/gamelib.py` `player()`, `place_player()` | In use |
| Reachability by simulating jumps with a copy of the game's physics, from every reachable surface; jump arcs drawn from the simulated paths | Own design (the plan in LEVEL_DESIGN.md section 1, step 3) | `tools/levelpreview.py`, `tools/physics.py` | In use |
| Finding lag frames: count game-loop calls per frame with a PyBoy hook; time code by reading `LY` at function entries (114 CPU cycles per line) | Own method | `tests/gamelib.py` `track_frame_lines`, `tests/test_frame_budget.py`, `tools/frame_budget.py` | In use |
| GitHub Actions: cached GBDK download, build, test | GitHub-hunt research (sample workflow); mdeclerk/Bananas (MIT) | `.github/workflows/build.yml` | In use |

## Design

| Idea | Source | Where | Status |
|---|---|---|---|
| Story: a programmer escapes ApexLab after the AI security goes haywire | Owner | GAME_DESIGN.md | In use |
| Key-gated sections, fragment quota, gates as checkpoints, elevator exit | Owner decisions 2026-10-02; Donkey Kong '94 key-to-door idea (game-feel research) | LEVEL_DESIGN.md, `src/level/section.c` | In use |
| 16×32 multi-sprite player | Owner (early decision, kept 2026-10-03) | `player.c`, ART_PIPELINE.md | In use |
| "PATCH n/5" story tie-in, boot screen lines, AI log per elevator ride | Game-feel research suggestion, accepted by the owner | GAME_DESIGN.md, STORY.md, `src/game_state/story.c` | In use (logs 4-5 with levels 4-5) |
| KERNEL boss concept | Game-feel research suggestion, accepted by the owner | GAME_DESIGN.md | Planned (M8) |
| Hard mode after clearing (quota = all fragments) | Kirby's Dream Land extra mode (game-feel research), accepted by the owner | — | Cut (owner, 2026-10-05) |
| 4-letter passwords with a check byte that refuses every single wrong letter | Owner decision (contents, 2026-10-04); own design for the encoding | `src/game_state/progress.c`, `tools/password.py` | In use |
| Brighter palettes on GBA | Shantae's GBA enhancement (game-feel research), accepted by the owner | CGB_PLAN.md | Planned (M4) |
| Readability: light shades behind the play lane, black outlines | GB Studio Central sprite style notes (game-feel research) | ART_PIPELINE.md | Planned (M5) |
| Indexed PNG with palettes in groups of 4, light to dark, for `png2asset -keep_palette_order`; per-tile palettes in the attribute map | GBDK png2asset docs (`-keep_palette_order`, `-use_map_attributes`) | `tools/gbconvert.py` | In use (tool ready, art in M5) |

## Assets

Every asset in the final game is original (drawn by the owner or made with Claude
Design) or openly licensed and credited. Nothing copied from Mario or other commercial
games ships (decision 2026-10-03).

| Asset | Origin | Status |
|---|---|---|
| `graphics/player/programmer.png` | Early prototype; partly based on a Mario sprite (owner, 2026-10-03) | **Replace** in M5 (new art from Claude Design) |
| `graphics/enemies/Mushroom.png`, mushroom tiles in `src/actor/bot.c` | Copied from Mario | **Replace** in M5 (security bot) |
| `graphics/background/background.png` | Early placeholder, origin unclear | **Replace** in M5 (Server Room tileset and parallax band) |
| `graphics/hud/hud_a.png` | Early placeholder, Mario-like bricks and coin | **Replace** in M5 |
| `graphics/hud/Font.png` | Unknown | Check origin, or use a CC0 font (RESEARCH.md section 4) |
| `graphics/start_screen/apexLogo.png`, `press_start.png`, `src/graphics/splashscreen.c` | Owner (ApexLab logo) | Keep the logo; redo the title screen in M5 |
| Greybox tiles (`graphics/tilesets/greybox.png`, drawn by `tools/greybox_tileset.py`), HUD digits and heart (hand-written in `level_manager.c`) | Project code | Placeholders until M5 |
| Greybox parallax band (`graphics/bands/greybox_band.png`, drawn by `tools/greybox_band.py`) | Project code | Placeholder until M5 |
| Placeholder font (`src/graphics/font.c`): A-Z, 0-9, punctuation, 8×8 bold | Project code, drawn in the style of the existing HUD digits | Placeholder until the M5 font |
| Music: `music/*.uge` (Tech, Neurotic Robonaut, Observing Jupiter, Decampment, Gone Missing, Delight); both packs in `music/packs/` for the sound test | Tronimal, Game Boy Music Packs #1 and #2 (itch.io), CC BY 4.0 | In use; credited in `CREDITS.md` |
| hUGEDriver 6.1.3 (`lib/hUGEDriver/`) | SuperDisk, github.com/SuperDisk/hUGEDriver release, public domain | In use; credited in `CREDITS.md` |
| Sound effects: `sounds/GBFX.sav`, `sounds/GBFX2.sav` (picks in `sounds/effects.txt`) | Tronimal, GBFX and GBFX2 (itch.io), CC BY 4.0 | In use; credited in `CREDITS.md` |

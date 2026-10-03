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
| Autobank build flags (`-autobank -Wb-ext=.rel`, `-Wm-yoA`), MBC5 + RAM + battery header (`-Wl-yt0x1B`), GBC-compatible header (`-Wm-yc`), `.sym` output (`-Wm-yS`) | GBDK docs, toolchain settings; checked by the research agent on GBDK 4.5.0 | `Makefile` | In use |
| One Makefile for PowerShell, cmd and Git Bash: force `cmd.exe` on Windows | Own finding (make picks `sh.exe` only when Git Bash is on PATH) | `Makefile` | In use |
| Hardware detection `_cpu == CGB_TYPE`; guard VRAM bank 1 writes on DMG | GBDK docs (supported consoles), Pan Docs | `src/engine/platform.c` | In use |
| Gray GBC palettes matching the DMG shades, attribute maps cleared at boot | Own design, from Pan Docs on CGB palettes and attributes | `src/engine/platform.c` | In use (until M4) |
| Window HUD at an absolute position: `move_win(7, 128)`, `WX = 7` is the left edge | GBDK docs (`move_win`, `scroll_win` is relative), flozz/gameboy-examples example 08 | `level_manager.c` `set_hud_window_tiles` | In use |
| Metasprites for the 16×32 player (4 hardware sprites in 8×16 mode), `move_metasprite_ex` / `move_metasprite_flipx` | GBDK docs, `metasprites.h`; Larold's metasprite tutorial | `player.c` `player_draw` | In use |
| DMG fades by stepping `BGP_REG` (`0xE4 → 0xF9 → 0xFE → 0xFF`) | The project's early versions; same technique in flozz example 09 | `utils.c` | In use (DMG) |
| Flashes at most 3 per second | WCAG 2.3.1 "Three Flashes or Below Threshold" (found by the game-feel research) | `game_over.c` `palette_flash` | In use |
| Load a level while the screen is black, then fade in | Own fix (review of the early versions, bug 3) | `main.c` `GAMEPLAY_START` | In use |
| Parallax: a screen-line (LYC) interrupt changes `SCX` per band | Larold's Retro Gameyard, "Parallax Backgrounds with GBDK-2020"; GBDK examples `gb/lcd_isr_wobble`, `cross-platform/irq` | Top 3-row band (decision 2026-10-03) | Planned (M3) |
| Hide sprites over the bottom HUD with the same interrupt (`HIDE_SPRITES` at line 127) | Larold's Flappy Bird tutorial; BlackCastle `src/main.c` (idea only, no license) | Interrupt handler | Planned (M3) |
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
| Spawn list sorted by X, read with a cursor, start index per section | BlackCastle `src/monster_data.c` (idea only), marioland-gbdk (idea only) | Level converter, entity system | Planned (M3) |
| Rebuild the sprite list every frame, then `hide_sprites_range` | zelda-gbdk `objects.c` (idea only); GBDK `metasprites` example | Entity system | Planned (M3) |
| Object pools with a free list, fixed sprite ranges | cuphead_gb `bullet_pool.c` (MIT) | Projectiles, particles | Planned (M3/M8) |
| Boss drawn with background tiles, poses in the off-screen map, switched by scrolling | cuphead_gb README and maps (MIT) | KERNEL boss | Planned (M8) |
| Sound effects over music: channel mute mask and priority | untoxa/VGM2GBSFX (MIT); used the same way in GB Studio and CrossZGB | Audio | Planned (M6) |
| hUGEDriver music from the VBlank handler, `NONBANKED` wrapper for banked songs | Larold's hUGEDriver tutorial; hUGEDriver `gbdk_example` | Audio | Planned (M6) |
| Save header: magic value, version, size, checksum | CrossZGB `SRAM.c` (MIT); Larold's saving tutorial | Saves | Planned (M7); test ROM in use |
| Counting lag frames with the VBlank counter: every VBlank during an update is a frame without one | GBDK docs, `sys_time` in `gb/gb.h` | `src/main.c` `lag_frames` | In use |
| Look-ahead camera, deadzone, lock behind opened gates | GB Studio engine `src/core/camera.c` (MIT); Itay Keren, "Scroll Back" | `src/engine/camera.c` | In use (gate lock planned, M3) |

## Testing

| Technique | Source | Used in | Status |
|---|---|---|---|
| PyBoy headless tests, symbol lookup from the `.sym` file | PyBoy 2.7.0 docs and source | `tests/gamelib.py`, `tests/conftest.py` | In use |
| Forcing DMG mode with PyBoy's own DMG boot ROM; waiting for the cartridge entry point | Own finding in PyBoy's `core/mb.py` and `core/bootrom.py` | `tests/gamelib.py` `boot()` | In use |
| Save power-cycle test: stop with `ram_file`, boot again with it | struktured-labs/quintra `scripts/test_suspend.py` (MPL-2.0, idea only) | `tests/test_savetest.py` | In use |
| ROM layout checks: `_HOME` below `0x4000`, header bytes, checksums | quintra `scripts/check_rom_layout.py`, `check_cartridge.py` (idea only) | `tests/test_rom.py` | In use |
| Known bugs as strict expected failures; regression tests checked against the old build | Own convention | `tests/test_known_bugs.py`, `tests/test_regressions.py` | In use |
| Replaying from one frame with `save_state` / `load_state` | PyBoy 2.7.0 docs | `tests/gamelib.py` `save`, `load`; `tests/test_physics.py` | In use |
| Reading game state at a fixed point in the frame: copy the struct from a `hook_register` callback on `player_draw`; test writes to the player wait for the same point | Own finding (PyBoy frames end mid-update) | `tests/gamelib.py` `player()`, `place_player()` | In use |
| Reachability by simulating jumps with a copy of the game's physics, from every reachable surface; jump arcs drawn from the simulated paths | Own design (the plan in LEVEL_DESIGN.md section 1, step 3) | `tools/levelpreview.py`, `tools/physics.py` | In use |
| Finding lag frames: count game-loop calls per frame with a PyBoy hook; time code by reading `LY` at function entries (114 CPU cycles per line) | Own method | `tests/gamelib.py` `track_frame_lines`, `tests/test_frame_budget.py`, `tools/frame_budget.py` | In use |
| GitHub Actions: cached GBDK download, build, test | GitHub-hunt research (sample workflow); mdeclerk/Bananas (MIT) | `.github/workflows/build.yml` | In use |

## Design

| Idea | Source | Where | Status |
|---|---|---|---|
| Story: a programmer escapes ApexLab after the AI security goes haywire | Owner | GAME_DESIGN.md | In use |
| Key-gated sections, fragment quota, gates as checkpoints, elevator exit | Owner decisions 2026-10-02; Donkey Kong '94 key-to-door idea (game-feel research) | LEVEL_DESIGN.md | Planned (M3) |
| 16×32 multi-sprite player | Owner (early decision, kept 2026-10-03) | `player.c`, ART_PIPELINE.md | In use |
| "PATCH n/5" story tie-in, boot screen lines, AI log per elevator ride | Game-feel research suggestion, accepted by the owner | GAME_DESIGN.md | Planned (M7) |
| KERNEL boss concept | Game-feel research suggestion, accepted by the owner | GAME_DESIGN.md | Planned (M8) |
| Hard mode after clearing (quota = all fragments) | Kirby's Dream Land extra mode (game-feel research), accepted by the owner | GAME_DESIGN.md | Planned (M7) |
| Brighter palettes on GBA | Shantae's GBA enhancement (game-feel research), accepted by the owner | CGB_PLAN.md | Planned (M4) |
| Readability: light shades behind the play lane, black outlines | GB Studio Central sprite style notes (game-feel research) | ART_PIPELINE.md | Planned (M5) |

## Assets

Every asset in the final game is original (drawn by the owner or made with Claude
Design) or openly licensed and credited. Nothing copied from Mario or other commercial
games ships (decision 2026-10-03).

| Asset | Origin | Status |
|---|---|---|
| `graphics/player/programmer.png` | Early prototype; partly based on a Mario sprite (owner, 2026-10-03) | **Replace** in M5 (new art from Claude Design) |
| `graphics/enemies/Mushroom.png`, mushroom tiles in `src/enemy/enemy.c` | Copied from Mario | **Replace** in M5 (security bot) |
| `graphics/background/background.png` | Early placeholder, origin unclear | **Replace** in M5 (Server Room tileset and parallax band) |
| `graphics/hud/hud_a.png` | Early placeholder, Mario-like bricks and coin | **Replace** in M5 |
| `graphics/hud/Font.png` | Unknown | Check origin, or use a CC0 font (RESEARCH.md section 4) |
| `graphics/start_screen/apexLogo.png`, `press_start.png`, `src/graphics/splashscreen.c` | Owner (ApexLab logo) | Keep the logo; redo the title screen in M5 |
| Greybox tiles (`graphics/tilesets/greybox.png`, drawn by `tools/greybox_tileset.py`), HUD digits and heart (hand-written in `level_manager.c`) | Project code | Placeholders until M5 |
| Sound effects in `src/sound/sound.c` | Project code (register values) | In use |
| Music | Tronimal, Game Boy Music Packs #1 and #2, CC BY 4.0 | Planned (M6); credit required |
| Sound effect packs | Tronimal GBFX / GBFX2, CC-BY | Planned (M6); credit required |

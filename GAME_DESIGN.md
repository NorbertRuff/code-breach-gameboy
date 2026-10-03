# ApexLab: Code Breach - Game Design Document

## Game Concept
**ApexLab: Code Breach** is a Game Boy platformer where you play as a programmer at ApexLab who must escape when the AI security systems go haywire. The lab's protocols are corrupted, and you need to collect code fragments and get out while avoiding malfunctioning security bots.

## Vision
A **cozy little game**: it looks good, its logic is solid, it runs on a real Game Boy and Game Boy Color, and its owner can explain how every part works. It started as a proof of concept for learning C on old hardware; the revival (2026-10) turns it into a finished game.

What that means in practice:
- **Forgiving rules.** 3 lives; losing one returns you to the last opened gate with your fragments kept. Game over refills your lives and restarts the current section, not the level.
- **Readable, original visuals.** A 16×32 multi-sprite programmer, a parallax band at the top of the screen, fades between screens. No copied art: no Mario, no mushrooms.
- **Explainable code.** Decisions and the sources of ideas are recorded as we go (docs/DECISIONS.md, docs/SOURCES.md); a full "how it works" document is written at the end.

---

## Story & Setting
- **Setting**: ApexLab programming facility
- **Protagonist**: ApexLab programmer/employee (16×32 sprite, lab coat + glasses)
- **Conflict**: AI security breach — corrupted security bots are patrolling the facility
- **Goal**: Each level has 3 sections. Collect enough code fragments in a section to open its security gate, then reach the elevator at the end. Extra fragments are bonus. The camera locks behind an opened gate. Details in [docs/LEVEL_DESIGN.md](docs/LEVEL_DESIGN.md).
- **Theme**: Cyberpunk tech lab, programming, AI gone wrong
- **Story through the fragments ("PATCH n/5")**: each level's fragments compile into one part of a patch. In the final fight the player uploads the patch into the Mainframe AI.
- **Text, kept minimal**:
  - Boot screen: three terminal lines, "ANOMALY / STAFF #042 / LOCKDOWN".
  - Each elevator ride: one line from the AI's log, more corrupted on every floor.
  - Ending: the elevator goes up, the palette fades to light, "LOCKDOWN LIFTED".
- **Hard mode**: unlocked after clearing the game. Every gate needs all of its section's fragments.

---

## Current Build Status

### What Is Done
- [x] Player movement — left/right with smooth acceleration and deceleration
- [x] Jump mechanics — variable height, jump_frames counter, coyote-style landing
- [x] Physics engine — integer-only (no floats), velocity_x/y, deceleration over DECELERATION_FRAMES
- [x] Tile-based collision detection — SOLID, ONE_WAY (defined), COLLECTIBLE, HAZARD, NONE
- [x] Horizontal camera follow — player centered at 80px, clamped to level bounds
- [x] Horizontal background scroll — 32-tile wide level (256px), move_bkg camera
- [x] World-coordinate player position — draw_x computed as player.x - total_scroll_x
- [x] Collectibles system — 12 collectibles in level 1, tile cleared on pickup
- [x] Win condition — all collectibles collected triggers LEVEL_COMPLETE
- [x] HUD — bottom window layer, lives (heart + digit), collectible counter (N/T), custom digit tiles
- [x] Lives system — 3 lives, death → GAMEPLAY_END, respawn or game over
- [x] Invincibility frames — 90 frames after respawn, sprite flashes (hide every 2 frames)
- [x] Enemy system (MVP) — patrol AI, bounces between min/max world X, placeholder mushroom sprite
- [x] Enemy direction flip — horizontal sprite flip + column swap for correct mirroring
- [x] Enemy–player AABB collision — world coordinates, skipped while invincible
- [x] Game over effect — palette flash (BGP_REG toggle, no LCDC changes) → fade out → full restart
- [x] Level complete effect — faster palette flash → fade out → GAMEPLAY_START
- [x] Sound effects — jump (CH1 sweep), collect (CH2 blip), hit (CH1 low + CH4 noise)
- [x] State machine — INTRO_CREDITS → TITLE_SCREEN → GAMEPLAY_START → COREGAMELOOP → GAMEPLAY_END / LEVEL_COMPLETE
- [x] Intro credits + title screen with animated "PRESS START"
- [x] Asset pipeline — PNG → png2asset → C source, ROM bank 1 for large assets
- [x] Feature flags — SKIP_INTRO_AND_START, EMPTY_BACKGROUND, DISABLE_BACKGROUND_SCROLLING

### Known Issues / In Progress
Full bug list with causes: [docs/PROJECT_STATE.md](docs/PROJECT_STATE.md).
- [ ] Game-over reload visual bug — **cause found**: `scroll_win(0, 128)` is relative, so every second level load moves the HUD window to the top and it covers the screen. Fix: `move_win(7, 128)` (roadmap M1).
- [ ] Enemy sprites are mushroom placeholders — need proper security-bot sprites per theme
- [ ] ONE_WAY platform collision defined but not implemented
- [ ] Sub-pixel smooth movement — PR #2 was merged, but positions are still whole pixels (roadmap M2)

---

## Level Design

### Level 1: Server Room *(implemented)*
- **Width**: 32 tiles (256px), single horizontal scroll
- **Ground**: Full bottom row
- **Platforms**: 5 platforms at varying heights (staircase path upward right)
- **Enemies**: 3 patrol bots, fixed patrol ranges
- **Collectibles**: 12 code fragments, placed above each surface cluster
- **Objective**: Collect all 12 fragments to trigger level complete

### Level 2: Code Testing Lab *(not started)*
- **Theme**: Computer workstations, code displays, broken terminals
- **Enemies**: Faster patrol bots, consider vertical movers
- **Collectibles**: Bug reports, debug tools
- **New mechanic**: ONE_WAY platforms (jump up through, land on top)
- **Width**: Consider exceeding 256px → requires dynamic tile streaming (set_bkg_submap)

### Level 3: AI Research Wing *(planned)*
- **Theme**: Advanced AI systems, neural network displays
- **Enemies**: Malfunctioning AI entities with more complex patterns
- **Collectibles**: AI cores, security protocols

### Level 4: Network Hub *(planned)*
- **Theme**: Network infrastructure, data flow visualization
- **Enemies**: Network bots, data corruption hazards
- **New mechanic**: Hazard tiles (COLLISION_HAZARD, instant death)

### Level 5: Mainframe *(planned)*
- **Theme**: Core system, central processing
- **Boss: "KERNEL"** (concept accepted 2026-10-02; details in M8)
  - **Arena:** one screen. KERNEL is a monitor face of about 7×6 tiles, drawn with background tiles, so it uses no sprites. A screen-line split moves the face while the floor stays still.
  - **How to hurt it:** the player has no attack. Three kill-switch terminals light up in turn; standing on the lit one and holding Up for about half a second uploads part of the patch.
  - **Hit feedback:** a short freeze, screen shake, and a wobble on the face.
  - **Phases:** 2 adds lifts (level 3's mechanic), 3 corrupts floor tiles (level 4's).
  - **Attacks:** packet rain with a column warning, spawned level 1 bots, and a purge sweep escaped on one-way platforms.
  - Research notes: [docs/RESEARCH.md](docs/RESEARCH.md) section 3.

---

## Gameplay Mechanics

### Core Movement
- [x] Left/right movement with acceleration
- [x] Jump mechanics (variable height)
- [x] Smooth deceleration
- [x] Wall/ground collision
- [ ] ONE_WAY platform pass-through
- [ ] Sub-pixel resolution for smoother movement (in progress)

### Collectibles
- [x] Code fragments (basic collectibles, clear tile on pick-up)
- [x] Collectible counter HUD
- [ ] Different collectible types per level
- [ ] Security gates that open at a fragment quota; opened gates are checkpoints

### Enemies
- [x] Patrol AI (bounce between world X bounds)
- [x] Player collision + lives loss
- [x] Direction-aware sprite flip
- [ ] Security bot sprites (replace mushroom placeholder)
- [ ] Vertical patrol enemies
- [ ] Chase/aggro AI

### HUD & Progression
- [x] Lives display (heart + digit)
- [x] Collectible counter (N/T format)
- [x] Game over (full restart)
- [x] Level complete
- [ ] Score counter (points per collectible)
- [ ] Level select / transition between levels
- [ ] Proper game over text ("GAME OVER" on screen)
- [ ] Timer / speed-run mode

### Audio
- [x] Jump sound (CH1 sweep)
- [x] Collect sound (CH2 blip)
- [x] Hit/death sound (CH1 + CH4 noise)
- [ ] Background music: Tronimal's Game Boy Music Packs #1 and #2 (CC BY 4.0) with hUGEDriver
- [ ] Menu music
- [ ] Level complete jingle
- [ ] Sound effects from Tronimal's GBFX/GBFX2 (CC-BY) through untoxa's VGM2GBSFX player, with priority over the music
- [ ] Credits for every CC-BY item in `CREDITS.md` and on the in-game credits screen

---

## Technical Architecture

### Physics (src/player/)
Integer-only, no floats. Horizontal: `velocity_x`, `movement_force_x`, `slowing_x` over DECELERATION_FRAMES. Vertical: `velocity_y` clamped to [MIN_VELOCITY_Y, MAX_VELOCITY_Y], `jump_frames` up to MAX_JUMP_FRAMES. Ground state via `underfoot_state`.

**Next**: Sub-pixel resolution — scale positions by 16 (or power-of-2), store face value, draw `face >> 4`. This is the current feat-smooth-movement branch goal.

### Camera (src/main.c)
`total_scroll_x = clamp(player.x - 80, 0, MAX_SCROLL_X)`. Applied via `move_bkg(total_scroll_x, 0)`. Player draws at `draw_x = player.x - total_scroll_x`.

**For Level 2+ wider than 256px**: Implement `set_bkg_submap` streaming — copy visible columns into the 32×32 BG map as the camera scrolls (see Larold's "Drawing Large Maps" tutorial).

### Collision (src/level/)
`collision_map[18][32]` byte array. `level_check_collision()` converts world pixel → tile index. Collision types: NONE, SOLID, ONE_WAY, HAZARD, COLLECTIBLE.

**ONE_WAY todo**: When landing from above (`velocity_y > 0`), treat as solid. When moving upward or already overlapping, ignore.

### Actors (src/actor/)
Enemies are actors, spawned from the level's entity list as the camera reaches them and removed a screen behind it. One type table entry per kind (spawn, update, metasprite, hit box); the patrol bot is the first. The sprite list is rebuilt every frame: the player first, then the actors on screen. Mushroom tiles currently used as placeholder — to be replaced with security bot PNG via png2asset.

### State Machine (src/main.c)
```
INTRO_CREDITS → TITLE_SCREEN → GAMEPLAY_START → COREGAMELOOP
                                     ↑                  ↓
                                GAMEPLAY_END ← enemy/pit/hazard hit
                                LEVEL_COMPLETE ← all collectibles collected
```

### VRAM Layout
- Tiles 0–63: Background tiles (bank 1)
- Tiles 64–77: HUD window tiles (bank 1)
- Tiles 128–131: Enemy mushroom placeholder (to be replaced)
- Tile 132: Collectible (diamond shape, hand-drawn)
- Tiles 133–142: Digit tiles 0–9 (hand-drawn 8×8)
- Tile 143: Slash `/`
- Tile 144: Heart `♥`
- Sprite VRAM 0–39: Player programmer (5 frames of 16×32)
- Sprite VRAM 40–43: Patrol bot

### ROM Banking
Assets compiled with `-bank 1 -org 0x8000` (background, hud, mushroom, apexLogo, press_start). Accessed via GBDK's banked data mechanism. Code runs from bank 0.

---

## Upcoming Work

Moved to [docs/ROADMAP.md](docs/ROADMAP.md) (milestones M0–M9). Project decisions are in [docs/DECISIONS.md](docs/DECISIONS.md).

---

## Visual Style
- **Aesthetic**: Cyberpunk tech lab — circuits, code symbols, digital effects
- **Colors**: Game Boy Color palettes per environment (up to 8 background + 8 sprite palettes), with a 4-shade fallback on the original Game Boy. Every palette is ordered light → dark so the DMG version reads correctly. See [docs/CGB_PLAN.md](docs/CGB_PLAN.md) and [docs/ART_PIPELINE.md](docs/ART_PIPELINE.md).
- **Character**: 16×32 programmer sprite (current: placeholder; complete animations planned)
- **Enemies**: Corrupted security bots — NOT mushrooms; need 16×16 or 16×32 bot sprites
- **Environment**: Server racks, computer banks, lab equipment, neural displays

---

## Research Notes — Larold's Retro Gameyard (laroldsretrogameyard.com)

This site has a complete GBDK-2020 tutorial series. All directly applicable to this project:

### Already covered by our implementation
- Getting Started, Background/Window Layers, Sprites, Joypad, Sounds, PNG2Asset

### High-value tutorials to follow next

**Tilemap Collision Detection**
URL: `/tutorials/gb/tilemap-collision-detection-in-game-boy-games/`
Their approach: `column = worldX >> 3; row = worldY >> 3` (bit shift instead of divide). Multi-point edge checks (check all 4 corners of player bounding box per axis). Out-of-bounds treated as solid. Separate X and Y collision steps for wall-sliding. Our current implementation is close; could add multi-point checks for large sprites.

**Smooth Movement / Sub-Pixel Resolution**
URL: `/tutorials/gb/smooth-movement-using-sub-pixel-resolution/`
Store position as `face` integer = true_position × 16. Update physics in face units. Draw at `face >> 4` pixels. Always use powers-of-2 as scale (bit-shift = free division on SM83). This is exactly what feat-smooth-movement branch should implement.

**Drawing Large Maps**
URL: `/tutorials/gb/drawing-large-maps-in-gbdk/`
Key insight: BG layer is 32×32 tiles (256×256px); for wider worlds use `set_bkg_submap()` to stream tiles into the wrap-around buffer as camera moves. Track `camera_x`, `map_pos_x = camera_x >> 3`, update a 21-column strip when `map_pos_x` changes. Only update tiles on camera movement (use redraw flag). **Required for Level 2+ if map exceeds 256px.**

**PNG2Asset for Sprites**
URL: `/tutorials/gb/png2asset-graphics-creation-for-gbdk-2020/`
For enemy/character sprites: PNG width/height must be multiples of 8 (height multiples of 16 for 8x16 mode). No padding between frames on sprite sheet. Use `-keep_palette_order` for consistent 4-color palette. Use `-px`/`-py` pivot flags for proper flip behavior. Run: `png2asset enemy.png -sw 16 -sh 16 -spr8x16 -keep_palette_order -name security_bot -o src/graphics/security_bot.c`

**Playing Music with hUGEDriver**
URL: `/tutorials/gb/playing-music-in-game-boy-games-with-huge-driver/`
Steps:
1. Create music in hUGETracker (free, export as `.uge`)
2. Download hUGEDriver from GitHub, place `gbdk/` and `include/` under `hugedriver/`
3. Add to Makefile: `-Ihugedriver/include` and `-Wl-lhugedriver/gbdk/hUGEDriver.lib`
4. In code: `#include "hUGEDriver.h"`, declare `extern const hUGESong_t my_song;`
5. Init in `__critical` block: `hUGE_init(&my_song);`
6. Register VBL: `add_VBL(hUGE_dosound);`
Songs in bank 1+ need a NONBANKED wrapper that switches ROM bank before calling dosound.

**Saving Games**
URL: `/tutorials/gb/how-to-save-data-in-game-boy-games/`
Requires MBC with SRAM + battery (e.g., MBC-5, type 0x1B in Makefile). API: `ENABLE_RAM` / `DISABLE_RAM` around SRAM reads/writes. Define saveable vars in a separate `.c` compiled with `-Wf-ba0`. Include a magic checksum to detect first-boot (no save). Only save what can't be reconstructed: high score, unlocked levels, settings. Keep SRAM disabled normally to prevent corruption.

**Parallax Backgrounds**
URL: `/tutorials/gb/parallax-backgrounds-with-gbdk-2020/`
Relevant for visual polish in later levels. Uses HBlank interrupt (STAT interrupt) to change `SCX_REG` mid-scanline, creating the illusion of depth. Adds atmosphere to Server Room / Network Hub environments.

**Drawing Text**
URL: `/tutorials/gb/drawing-text-in-game-boy-games/`
Needed for: "GAME OVER" screen, "LEVEL COMPLETE" text, score display. GBDK provides `set_bkg_based_tiles` with a font tileset or use custom digit tiles (already partially done in our HUD).

---

## Feature Flags (src/utils.h)

| Flag | Default | Purpose |
|---|---|---|
| `SKIP_INTRO_AND_START` | `1` | Skip intro/title for faster dev iteration |
| `EMPTY_BACKGROUND` | `1` | No effect: only read by the excluded `prepare_core_game_loop.c` |
| `DISABLE_BACKGROUND_SCROLLING` | `0` | No effect: nothing reads it |

---

## GB Hardware Constraints

- **No floats**: all math uses int8_t / int16_t / uint8_t
- **Sprite limit**: 40 OAM slots total, 10 per scanline. Player = 4 slots (OAM 0–3), each bot = 2 slots, given out in order each frame
- **VRAM timing**: tile writes must happen during VBlank — GBDK buffers this; `wait_vbl_done()` at end of main loop
- **Stack**: tiny — avoid deep recursion, declare large arrays as `static`
- **ROM banking**: bank 0 fixed (code), bank 1 switchable (large assets) — always `SWITCH_ROM_BANK` before accessing banked data if explicitly needed
- **SM83 CPU**: no hardware multiply/divide; use bit-shifting for ×2/÷2 operations

---

*Last updated: 2026-10-02*

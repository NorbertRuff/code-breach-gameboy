# Research Findings

Collected on 2026-10-02 by 7 research agents, plus checks run on this machine. Covers:
Larold's Retro Gameyard tutorials, the GBDK-2020 docs and local examples, 8 open-source
game codebases, game-feel and design sources, and openly licensed art and audio.

How to read this doc:
- **Section 1** is the short list: findings that change our plans, ranked.
- **Sections 2–5** hold the details by topic, for use when the matching milestone starts.
- **Section 6** lists plan changes that need the owner's OK. Nothing in this doc is a
  decision until it moves to [DECISIONS.md](DECISIONS.md).
- "(S)" marks a suggestion from the research, not a sourced fact.
- "Verified" means it was run or checked on this machine with GBDK 4.5.0.

---

## 1. Findings that change our plans

| # | Finding | Action | Milestone |
|---|---|---|---|
| 1 | **`make assets` has been silently broken.** png2asset doesn't know `-bank`, `-org` or `-name`. It prints `Warning: Argument "-bank" not recognized` and exits 0, so every asset lands in bank 0. Verified. | Use `-b 255` (autobank); symbol names come from the `-o` file name. | M0 |
| 2 | **Code already spills past bank 0.** The linker map shows `_CODE` at `0x4000–0x460A` and `_HOME` at `0x460B–0x4EBA`. This works on today's 32 KB cartridge, but breaks once bank switching is on: `_HOME` must stay in bank 0. Verified with `romusage obj/ApexLab.map -a`. | Move assets out with `-b 255` before turning on MBC5 banking. Add a CI check that `_HOME` ends below `0x4000`. | M0 |
| 3 | **Keep collision in a 32×16 RAM ring (512 bytes)**, filled from the per-tileset ROM table as each column streams in. Used by BlackCastle, marioland-gbdk and CrossZGB (`scroll_collisions[256]`). Opening a gate or collecting a fragment is then a single write to the ring plus one tile in VRAM. | Replaces our plan of reading ROM on every check and special-casing gates. Proposed change P1. | M2 |
| 4 | **Proven jump-feel numbers and code.** The GB Studio engine (`src/states/platform.c`, MIT) has coyote time, jump buffering, hold-to-extend and drop-through as small per-frame countdowns. Celeste and Super Mario Bros. give starting values (section 2.2). | Base the M2 physics rewrite on these. | M2 |
| 5 | **The camera should look ahead.** With the player fixed at x = 80, a full running jump (about 65 px, (S)) is nearly as long as the view ahead (80 px). A 24 px look-ahead raises the view ahead to 104 px. | Look-ahead with a 16 px threshold before it flips direction. | M2 |
| 6 | **Rebuild the sprite list every frame.** Each object draws with `move_metasprite_ex` / `move_metasprite_flipx` into a running count, then `hide_sprites_range(count, MAX_HARDWARE_SPRITES)` hides the rest. Our `player.c:282,286` uses `move_metasprite` / `move_metasprite_vflip`, marked "Obsolete" in GBDK 4.5's `metasprites.h:166`. Verified. | Use this pattern in the entity system. | M3 |
| 7 | **A spawn list sorted by X, read with a cursor**, plus the start index of each section. A checkpoint restart then only resets the cursor (BlackCastle, marioland-gbdk). | Matches our plan; add the per-section index. | M3 |
| 8 | **A ready-made sound effect player with music priority.** untoxa's VGM2GBSFX (MIT) plays effects over hUGEDriver music with a channel mute mask and priority. GB Studio, CrossZGB and marioland-gbdk all use it. | Use it rather than writing our own. | M6 |
| 9 | **Music and sound effects with clear licenses exist.** Tronimal's Game Boy Music Packs #1 and #2 are hUGETracker files under CC BY 4.0, with fitting tracks ("Neurotic Robonaut", "Tech", "Terminate"). Tronimal's GBFX and GBFX2 sound effects are also CC-BY. | Candidate answer to the open music decision. | M6 |
| 10 | **Save tests can run in PyBoy.** quintra (MPL-2.0, GBDK 4.5.0, MBC5) writes a save, calls `pyboy.stop(save=True, ram_file=...)`, starts a new PyBoy with that `ram_file`, and checks the data survived. | Use this for the M0 save test on PC, before testing on the flashcart. | M0 |
| 11 | **The level-complete flash is too fast for photosensitive players.** `palette_flash(6, 8)` makes one flash every 16 frames, which is 3.75 per second. The WCAG guideline is no more than 3 per second. The game-over flash (`hold` 12) is 2.5 per second, which is fine. | Slow it down. | M1 |
| 12 | **The Mainframe boss can be built from background tiles.** cuphead_gb stores the boss poses in the off-screen part of the 32×32 background map and switches pose by changing the scroll registers. That costs no sprites and no tile copies. A screen-line interrupt keeps the floor still. | Plan the boss as a one-screen arena with this technique. | M8 |
| 13 | **GBC screens wash colors out.** Per channel, values `$10–$1F` all look very bright, so mid and dark colors should stay in `$00–$0F` (Pan Docs). | Add this to the palette rules. Preview with SameBoy's color correction or `rgbgfx -C`. | M5 |
| 14 | **Brighter palettes on GBA are cheap.** GBDK exposes `_is_GBA`. Shantae swaps in brighter palettes on a GBA. | Optional palette set, chosen at boot. | M4 |

---

## 2. Engine techniques by topic

### 2.1 Build and banking

Recommended flags. All verified on 4.5.0 except where marked.

```make
LCCFLAGS += -Wl-yt0x1B -Wm-yoA -Wm-ya1 -Wm-yc -Wm-yn"APEXLAB"
LCCFLAGS += -autobank -Wb-ext=.rel
LCCFLAGS += -debug -Wm-yS
CFLAGS   += -Wf--max-allocs-per-node50000 -Wf-MMD -Wf-Wp-MP
# after linking: romusage obj/ApexLab.map -g -R   (-R returns an error code for CI)
```

| Flag | Why |
|---|---|
| `-Wl-yt0x1B` | MBC5 + RAM + battery. bankpack refuses to autobank without a cartridge type. |
| `-Wb-ext=.rel` | The GBDK docs recommend it: bankpack rewrites object files, and without this they no longer count as autobanked. |
| `-Wf--max-allocs-per-node50000` | Smaller code: `player.c` went from 2215 to 2158 bytes, but it compiles in about 8 s instead of 2. Adding `--opt-code-speed` gave 2077 bytes. |
| `-Wf-MMD -Wf-Wp-MP` | Header dependency files, so changing a header rebuilds what includes it. |
| `-Wm-yn` | Up to 15 characters; the GBC flag at `0x143` survives. |
| `-Wb-random -Wb-max=3` | Test-only: random bank placement, to flush out banking bugs (from GBDK's hicolor example). |

Also sort the source file list so autobank places things the same way on every build.

**Banking rules from the GBDK docs:**
- Data: png2asset `-b 255` writes `#pragma bank 255` and `BANKREF(name)`. Callers use
  `BANKREF_EXTERN(name)` and `BANK(name)`.
- Load banked data through a `NONBANKED` function that saves `CURRENT_BANK`, calls
  `SWITCH_ROM`, copies, then restores the bank.
- Code: `#pragma bank 255` at the top of a file, and `BANKED` on functions called from
  other files.
- `main`, interrupt handlers, loaders, and functions that read map data across banks
  stay in bank 0.
- "Banks cannot be switched manually from inside a BANKED function."
- One source file goes into one bank, so each level's data gets its own `.c` file under 16 KB.
- A banked call goes through a trampoline and passes arguments on the stack. Avoid banked
  calls inside per-entity loops.
- Avoid one global "previous bank" variable. BlackCastle and zelda-gbdk both do this, and
  zelda-gbdk pushes twice but pops once. Save the bank in a local variable.

**Useful local GBDK examples** (in `gbdk/examples/`):

| Example | What to learn |
|---|---|
| `cross-platform/large_map` | Column streaming with `set_bkg_submap`. `map_w` is a `uint8_t`, so 255 tiles is the width limit. |
| `cross-platform/platformer_template` | Banked level and camera code with `NONBANKED` accessors. Its Readme mentions `-tileset_only`, which doesn't exist; the real flag is `-tiles_only`. |
| `cross-platform/banks_autobank` | `BANKREF` / `BANKED`, mixing fixed and autobanked files |
| `cross-platform/metasprites` | `move_metasprite_ex`, `base_prop` palettes, `hide_sprites_range` |
| `cross-platform/sram_banks` | Cartridge RAM saves |
| `cross-platform/emu_debug` | `EMU_printf`, `EMU_PROFILE_BEGIN` / `END` (Emulicious only) |
| `gb/lcd_isr_wobble`, `cross-platform/irq` | Screen-line interrupts: the pattern for parallax and HUD splits |
| `gb/crash` | `<gb/crash_handler.h>`, which shows registers on a crash |

### 2.2 Physics and game feel

Values are in pixels per frame at 60 fps. "Sub" means sub-pixel units. The scale (1/16 or
1/32 px) is proposed change P3.

| Technique | Reference | ApexLab starting value |
|---|---|---|
| Jump from height and time | Initial speed = 2h / t, gravity = 2h / t² (Kyle Pittman, GDC "Building a Better Jump") | (S) start speed 3 px/f |
| Jump cut | Super Mario Bros. uses about 3.5× more gravity when A is released while rising (SMBpedia) | (S) gravity 0.125 px/f² with A held, 0.375 without. About 5 tiles for a full jump, 1.5 for a tap |
| Apex hang | Celeste halves gravity near the top of the jump while it is held | (S) half gravity while vertical speed is under 0.5 px/f and A is held |
| Max fall speed | Celeste: 2.7 px/f | (S) 3.5 px/f. Stay below 8 px/f, or the player can pass through a tile in one frame. |
| Coyote time | Celeste: 6 frames | 6 frames |
| Jump buffer | Celeste, GMTK Platformer Toolkit | (S) 5 frames |
| Head-corner correction | Celeste: up to 4 px sideways | 4 px |
| Ledge pop | (S) | 2 px up when the feet catch a ledge top |
| Run | Super Mario Bros. walk cap 1.5 px/f. Celeste: about 5 frames to full speed, 0.65× control in the air | (S) cap 1.5 px/f, 6 frames to full speed, 8 frames to stop, 0.65× in the air |
| Response | Steve Swink, *Game Feel*: about 100 ms counts as real-time | The jump starts on the frame A is pressed |
| Look-ahead camera | Super Mario Bros. sits about 25% off-center; Super Mario World uses a forward focus | (S) 24 px lead, easing at most 1 px/f faster than the player, flipping after 16 px of travel the other way |

GB Studio engine details to copy (`gbvm`, MIT): positions in 1/32 px, so the tile index is
the high byte (`>> 8`). Coyote time, jump buffer and hold-to-extend are each a one-byte
countdown. Camera with a deadzone and clamping (`src/core/camera.c`). Off-screen actors
are deactivated unless flagged persistent (`src/core/actor.c`).

Collision: test X and Y separately so the player slides along walls. A 16×32 body needs
check points no more than 8 px apart: 5 down each side, 3 across (Larold's collision
tutorial).

### 2.3 Map streaming and camera

- Redraw only when the camera crosses a tile: `map_pos_x = camera_x >> 3`. Start with
  `old_map_pos_x = 255` to force the first draw (GBDK `large_map`).
- Column write: `set_bkg_submap(x, 3, 1, 13, map, map_width)`. Our column is 13 tall:
  rows 0–2 are the parallax band and the HUD covers rows 16–17 (decision 2026-10-03).
  On GBC, also call `set_bkg_submap_attributes`.
- Spread the work across frames. Decode the next column on quiet frames and push it on
  the frame the boundary is crossed (BlackCastle's `preload_pending`). GB Studio queues
  columns and copies them in batches.
- **Section gates can lock the camera** (GB Studio's camera locks): once a gate opens,
  the camera can't scroll back past it. This removes backtracking, which simplifies
  spawning and streaming. It also means bonus fragments missed in an earlier section are
  gone. That is proposed change P2.

### 2.4 Entities and sprites

- **Fixed pools with a free list** for projectiles and particles, each with a reserved
  sprite range (cuphead_gb `bullet_pool.c`). Always check for a full pool. zelda-gbdk
  writes through a null pointer when its pool is full. That write lands at address
  `0x0000`, the cartridge RAM-enable register, and risks corrupting saves.
- **Type table instead of copied code:** `{update, width, height, hp, metasprites}` per
  enemy type. BlackCastle copy-pastes one draw block per enemy, which caused index bugs.
- **Animation as frame-index tables** in ROM, with repeated indices for timing
  (cuphead_gb `player_animation.c`). Authored motion tables suit boss swoops and movers.
- **Swap enemy tiles at each gate.** Load the next section's enemy tiles when its gate
  opens (zelda-gbdk loads them per screen section).
- **Stream player frames into one VRAM slot** instead of keeping all frames loaded
  (zelda-gbdk `Link.c`). That frees sprite tile space for enemies.
- **Mirrored frames are free:** png2asset merges left/right copies into `S_FLIPX`. Draw
  right-facing frames only.
- **Animate background details by rewriting one tile.** For example,
  `set_bkg_data(13, 1, ...)` every 8 frames animates every server LED on screen at once
  (BlackCastle).
- **Chaser AI** (Larold's Pac-Man): pick a target tile and choose the direction with the
  smallest squared distance, with no square root. Fits the level 3 Seeker.

### 2.5 HUD, text and numbers

- Window HUD at `move_win(7, 128)` with a 20×2 map (flozz example 08). The HUD stays still
  while the background scrolls.
- **Keep sprites out of the HUD rows.** A screen-line interrupt at line 127 can call
  `HIDE_SPRITES`, and `SHOW_SPRITES` runs again at line 0 (Larold's Flappy Bird). The
  other option is to keep sprites from going below row 15. The GBC-only tile priority
  bit doesn't help on DMG.
- **BCD counters:** add with decimal adjust, and draw each nibble as a digit tile, with no
  division (BlackCastle `misc.c`). Redraw only the field that changed.
- **Font:** load a 1-bit font with `-bpp 1` and `set_bkg_1bpp_data` (half the ROM). Map
  characters to tiles, with a "tofu" fallback tile for unknown characters (flozz example 11).
- **Terminal messages:** a short text box that slides in above the HUD. Don't block while
  it's open: a waiting loop stalls the music driver.

### 2.6 Screen effects

| Technique | Cost | Risk | Use in ApexLab |
|---|---|---|---|
| Screen shake (SCX/SCY ±1–2 px) | Almost none | Can reveal unstreamed edge columns. Sprites need the same offset. The window HUD doesn't shake. | Damage, gate slam, boss hits (8–12 frames) |
| Hit-stop (freeze frames) | Saves CPU | Input must still be buffered | (S) 4 frames when a quota is met, 8 on damage |
| Palette flash | 1 register write on DMG; 8 bytes per palette on GBC, outside the busy mode | At most 3 flashes per second (WCAG 2.3.1) | Damage, boss hits |
| Invincibility blink | None | Harsh on GBC, GBA and IPS screens | (S) On GBC, use a pale sprite palette instead of blinking |
| Particle pool | 1 sprite and 2 tiles each | The 10-per-line limit near the player's feet | (S) 4 slots: landing dust, "+1", gate sparks |
| Screen-line split | 1–3 interrupts per frame, roughly 2–3% CPU | An interrupt can push map-streaming writes into the busy mode and corrupt them | Boss arena, a parallax ceiling band |
| Per-line wobble | Up to 144 interrupts per frame | Expensive in C, same risk as above | Only while nothing is streaming: death effect, boss hits |
| Tile animation | 16 bytes per tile, in VBlank | Shares VBlank time with streaming | Server LEDs, data streams, glitch hazards |

**Parallax** (Larold's tutorial): set `STAT_REG |= STATF_LYC`, register the handler with
`add_LCD(...)`, enable `LCD_IFLAG | VBL_IFLAG`. Each band's handler sets the next `LYC_REG`
and a scaled `SCX_REG`. It works with our bottom HUD if the last band ends at line 127.
Parallax rows must be a 32-tile repeating strip that streaming never writes to, for
example a distant wall along the top rows. The GBDK docs warn that LYC may not fire
reliably on line 0; use the VBlank handler for start-of-frame work.

### 2.7 Audio

- **hUGEDriver** setup: `__critical { hUGE_init(&song); add_VBL(hUGE_dosound); }`. For a song
  in a banked file, the VBlank handler is a `NONBANKED` wrapper that switches to the song's
  bank first. Switch banks before `hUGE_init`.
- **Sound effects over music:** VGM2GBSFX's player with a mute mask and priority (3 levels
  in GB Studio). With plain hUGEDriver: `hUGE_mute_channel(HT_CH1, HT_CH_MUTE)` during the
  effect, then `HT_CH_PLAY`. After using channel 3 for an effect, set
  `hUGE_current_wave = hUGE_NO_WAVE` so the music reloads its wave.
- **Sound effect sources:** VGM2GBSFX converts VGM, UGE, FX Hammer `.sav` and 8 kHz WAV.
  CBT-FX is archived (last commit July 2022) and doesn't support banks; skip it.
- **Music volume fades:** `NR50_REG` at 0 is still audible. Also set `NR51_REG = 0`.

### 2.8 Saves

- Save variables live in a separate `.c` file compiled with `-Wf-ba0`. Wrap every access
  in `ENABLE_RAM` … `DISABLE_RAM` (Larold's saving tutorial).
- Header: magic value + version + size + checksum. Clear the save if any of these doesn't
  match (CrossZGB `SRAM.c`).
- µCity checks how many RAM banks the cartridge really has. That's useful against a
  flashcart clone that doesn't give what the header asks for.

### 2.9 Testing and CI

- **PyBoy:** `PyBoy(rom, window="null", cgb=False/True)`, `symbol_lookup`,
  `memory[bank, addr]`, `hook_register`, `stop(save=True, ram_file=...)`. Not verified yet:
  whether it loads our `.sym` automatically; quintra reads symbol addresses from the
  `.noi` file instead.
- **One test per physics bug:** recorded input + a golden screenshot + memory checks (the
  GB Studio engine's `test/` folder, run there in BGB).
- **ROM layout checks** (quintra): `_HOME` ends below `0x4000`, the banked-call trampoline
  is in bank 0, each bank has headroom, and header bytes `0x143 = 0x80`, `0x147 = 0x1B`
  and the checksums are correct. `romusage -R` fails CI when a bank overflows.
- **Pure logic tests on the PC:** pogo-panic compiles game logic with the PC's C compiler
  for fast unit tests. That fits physics math.
- **GitHub Actions:** cache the `gbdk-linux64.tar.gz` 4.5.0 download, build with
  `make GBDK_HOME=$HOME/gbdk/`, run `romusage -R`, then pytest. No maintained
  "setup-gbdk" action exists.

---

## 3. Design ideas (all suggestions)

### Enemies per level
| Level | Ideas |
|---|---|
| 1 Server Room | Patrol bot that pauses 8 frames with an LED blink before turning |
| 2 Code Testing Lab | "Unit-tester" bot: crouches 16 frames, then hops between one-way platforms. "Debug drone": flies a sine path from a 32-byte table. |
| 3 AI Research Wing | Lift pads. "Seeker": shows "!" for 20 frames, then chases at 1 px/f (slower than the player) and stays in its zone. |
| 4 Network Hub | Animated corrupted-data hazard tiles. Packet lasers on a 2-second cycle with a half-second flicker warning. Firewall turret: charges 24 frames, fires a 1 px/f shot. |

Telegraphing rule: every attack has a wind-up, the attack, and a recovery. Wind-ups of at
least 24 frames. Never spawn an enemy next to the player. Mega Man GB was criticized
because its large hero had little room to dodge, and our 16×32 hero has the same risk.

### Gate presentation
- A column of laser bars next to a terminal showing `2/3`.
- Touching the gate early: a buzz, and the HUD count blinks.
- Quota met: 4 frames of hit-stop, a chime, the HUD icon blinks.
- Opening: the bars retract one tile every 3 frames, and "CHECKPOINT" shows above the HUD.

### Mainframe boss: "KERNEL"
- One-screen arena, so no streaming. A monitor face about 7×6 tiles drawn in the
  background. A screen-line split moves the face while the floor stays still.
- The player has no attack, so damage comes from kill-switch terminals that light up in
  turn. Standing on the lit one and holding Up for about 30 frames uploads a patch.
- Each hit: 8 frames of hit-stop, screen shake, a wobble on the face.
- Phase 2 adds lifts (level 3's mechanic). Phase 3 corrupts floor tiles (level 4's).
- Attacks: packet rain with a 30-frame column warning, spawned level 1 bots, and a purge
  sweep the player escapes by jumping onto one-way platforms.

### Story with very little text
- Boot screen: three terminal lines, "ANOMALY / STAFF #042 / LOCKDOWN".
- Each elevator ride: one line from the AI's log, more corrupted each floor.
- Each level's fragments compile into "PATCH n/5", uploaded during the boss fight. The
  collectibles become part of the plot.
- Ending: the elevator goes up, the palette fades to light, "LOCKDOWN LIFTED".

### Replay value
- After the game is cleared, unlock a hard mode where every gate's quota is all of its
  section's fragments (Kirby's Dream Land unlocks an extra mode).
- Save after each level is cleared (Super Mario Land 2).

### GBC extras that stay cosmetic
- Color-code tile types on GBC (red hazards, blue one-way platforms, gold fragments). Each
  type also has a unique shape, so it reads on DMG.
- Palette cycling for the Network Hub data flow on GBC; DMG swaps tiles instead.
- Keep gameplay identical on both. The DX version of Wario Land II plays the same on DMG.
  Link's Awakening DX locks only an optional extra dungeon to GBC.

### Readability on a small screen
- Shades 0–1 behind the play lane. Shade 3 for outlines, solid edges and hazards.
- Black outlines separate sprites from light backgrounds.

---

## 4. Art and audio materials

### Usable assets (check the license again before shipping)

| Name | URL | License | Contains | Fit |
|---|---|---|---|---|
| 1-Bit Sci-Fi Tileset (Omniclause) | omniclause.itch.io/1-bit-sci-fi | CC0 1.0 | 16×16 industrial platformer tiles, monitors, checkpoint, conveyors, acid | High |
| 16x16 Industrial Tileset (0x72) | 0x72.itch.io/16x16-industrial-tileset | CC0 1.0 | Pipes, spikes, platforms, small robot hero and enemies | High |
| Sci-fi platformer tileset (Buch) | opengameart.org/content/sci-fi-platformer-tileset | CC0 | 16×16 tiles, animated robots, levers | High |
| Mini Sci-Fi (Matt!) | mattnot.itch.io/mini-sci-fi | CC BY 4.0 | 8×8, 256 tiles: pipes, doors, levers, creatures | High |
| Green Robot + Enemy (darkagent) | opengameart.org/content/green-robot-enemy | CC0 | Robot and enemy in a Game Boy palette | Medium |
| Game Boy Platformer Tileset (Chloe Wolfe) | opengameart.org/content/game-boy-platformer-tileset | CC0 | Tiles, spikes, an animated character | Medium |
| 4 Colour Interior Tileset (stealthix) | opengameart.org/content/4-colour-interior-tileset | CC0 | 8×8 Game Boy-style interiors | Medium |
| Sci-fi Lab (Foozle) | foozlecc.itch.io/sci-fi-lab-tileset-decor-traps | CC0 | 32×32 lab tiles, turrets, laser barriers | Medium, for ideas |
| Free Robot Warfare Pack (MattWalkden) | mattwalkden.itch.io/free-robot-warfare-pack | CC0 1.0 | Robot insects, explosions, UI | Medium |
| Kenney Pico-8 Platformer, 1-Bit Pack | kenney.nl | CC0 | Generic 8×8 and 16×16 props and UI | Low–medium |

Avoid: GB Studio SciFi Tiles (The Pixel Nook, no license stated), Micro Space (no
redistribution), Retro Space! (PetricakeGames, may ship but not an open license).

All of these need redrawing or recoloring to our palettes and 8×8 grid. They are a base
for Claude Design and `gbconvert.py`, not drop-in files.

### Fonts
| Font | URL | License |
|---|---|---|
| Intrepid Monochrome | opengameart.org/content/intrepid-monochrome-8-bit-font | CC0, full ASCII 8×8 |
| Public Pixel | ggbot.itch.io/public-pixel-font | CC0 1.0 (TTF on an 8×8 grid, render at 8 px) |
| Den Of Snakes Pixel Fonts | bbbbbr.itch.io/den-of-snakes-pixel-fonts | CC0, PNG and TTF |
| Squont8 | opengameart.org/content/squont8 | CC0, uppercase and digits |

### Starting palettes (lospec, light → dark)
| Palette | Colors | Mood |
|---|---|---|
| 2bit demichrome | `e9efec a0a08b 555568 211e20` | "Robotic or mechanical" |
| Technobike | `07ef5c 13b37e 2a616e 1d2938` | Cyberpunk / tech |
| Kirokaze Gameboy | `e2f3e4 94e344 46878f 332c50` | Cool green |
| Nintendo Game Boy (BGB) | `e0f8d0 88c070 346856 081820` | Classic |

### Music and sound effects
| Item | URL | License |
|---|---|---|
| Tronimal Game Boy Music Pack #1 (8 songs, .uge) | yogi-tronimal.itch.io/game-boy-music-pack | CC BY 4.0 |
| Tronimal Game Boy Music Pack #2 (10 songs, .uge) | yogi-tronimal.itch.io/game-boy-music-pack-2 | CC BY 4.0 |
| GBFX (48 effects) and GBFX2 (59 effects), FX Hammer `.sav` | Tronimal on itch.io | CC-BY |
| VGM2GBSFX | github.com/untoxa/VGM2GBSFX | MIT |
| hUGEDriver, hUGETracker | github.com/SuperDisk | Public domain (stated in the READMEs) |

Avoid: Mute Audio (CC BY-ND, no edits allowed), TipTopTomCat (conflicting terms), the
hUGETracker sample songs (some are covers of commercial music).

### Study references (copyrighted, never ship)
From The Spriters Resource, Game Boy / GBC "S" page:
- **Shantae (GBC):** best-in-class animation, a large player sprite, the Tinkerbat Factory tileset.
- **Spider-Man (GBC):** a "Lab Enemies" sheet and tech bosses.
- **Super Mario Land 2:** readable 4-shade art, Space Zone tiles, HUD.
- **Super Mario Bros. Deluxe:** GBC tileset, font and HUD.
- **Solar Striker:** DMG sci-fi stages and mechanical enemies.
- **Space Station Silicon Valley:** 23 robot sprites, an idea bank for security bots.
- **Space-Net:** the same monsters in raw and LCD-adjusted GBC color.

### Tools
- **Aseprite + Palette Guard** (CC0 plugin, Aseprite 1.3.1+): flags 8×8 tiles that break
  the 4-colors-from-one-palette rule and snaps colors to 5-bit RGB. Free editors:
  LibreSprite, Pixelorama.
- **SuperFamiconv** (MIT): `-M gbc` packs colors into up to 8 palettes. Use it to
  cross-check our palette validator.
- **Color preview:** SameBoy's color correction modes, `rgbgfx -C`.

### Credits rule
Every CC-BY item goes into a `CREDITS.md` file and the in-game credits screen. List the
title, author, link, license, license link, and what we changed. CC0 items need no
credit, but listing them is courteous.

---

## 5. Reference codebases

| Repo | License | Activity | Use for | Copy code? |
|---|---|---|---|---|
| [chrismaltby/gbvm](https://github.com/chrismaltby/gbvm) (GB Studio engine) | MIT | 2026-08 | Platformer physics, camera, streaming queue, actors, color mode, music manager | Yes, with the notice |
| [gbdk-2020/CrossZGB](https://github.com/gbdk-2020/CrossZGB) | MIT | 2026-09 | Collision table per tile, spawning from streamed tiles, saves, examples `slopes`, `screen_shake`, `sfx` | Yes, with the notice |
| [untoxa/VGM2GBSFX](https://github.com/untoxa/VGM2GBSFX) | MIT | 2024-10 | Sound effect player and music manager | Yes, with the notice |
| [SimonLarsen/tobutobugirl-dx](https://github.com/SimonLarsen/tobutobugirl-dx) | MIT code, CC BY 4.0 assets | 2023-01 | A GBDK game for DMG, GBC and Super Game Boy with runtime asset sets; sound effect priority | Yes, with the notice |
| [deluxturtle/cuphead_gb](https://github.com/deluxturtle/cuphead_gb) | MIT (art and song derived from Cuphead: don't use) | 2023 | Background-layer boss, projectile pool, animation tables | Code yes, art no |
| [flozz/gameboy-examples](https://github.com/flozz/gameboy-examples) | WTFPL | Old | Window HUD, font mapping, DMG fades (written for the old gbdk-n) | Yes |
| [struktured-labs/quintra](https://github.com/struktured-labs/quintra) | MPL-2.0 | 2026-09 | PyBoy save tests, ROM layout checks, GBDK 4.5.0 CI | File-level copyleft: keep changes to copied files open |
| [mdeclerk/Bananas](https://github.com/mdeclerk/Bananas) | MIT | 2026-05 | GBDK 4.5.0 + `.sym` + PyBoy pytest smoke test | Yes, with the notice |
| [untoxa/BlackCastle](https://github.com/untoxa/BlackCastle) | None | Old | Collision ring, spawn cursor, BCD, sound effect priority | No: ideas only |
| [LaroldsJubilantJunkyard/zelda-gbdk](https://github.com/LaroldsJubilantJunkyard/zelda-gbdk) | None (Nintendo art) | Old | Sprite list rebuilt per frame, streaming with attributes | No: ideas only |
| [odrevet/marioland-gbdk](https://github.com/odrevet/marioland-gbdk) | None | 2026-10 | Horizontal scroller with a collision ring and spawn cursor | No: ideas only |
| [AntonioND/ucity](https://github.com/AntonioND/ucity) | GPL-3.0 | 2026-01 | Save RAM probing and checksums (asm) | No: GPL |

Larold's tutorials (laroldsretrogameyard.com/tutorials/gb/) cover 27 topics. The most
useful for us: tilemap collision, sub-pixel movement, large maps, metasprites, png2asset,
hUGEDriver, saving, parallax, text. Bugs in its sample code: `1+marioFrame>>4` parses as
`(1+marioFrame)>>4`, and the samples still call the obsolete `move_metasprite_vflip`.

---

## 6. Proposed plan changes

**Status (2026-10-02): all of P1–P7 were accepted.** They are recorded in
[DECISIONS.md](DECISIONS.md) and worked into the roadmap and design docs.

| # | Change | Affects | Trade-off |
|---|---|---|---|
| P1 | **Collision in a 32×16 RAM ring (512 bytes of WRAM)**, filled from the per-tileset table while streaming | LEVEL_PIPELINE section 3 | Costs 512 bytes, but opening gates and collecting fragments become simple writes, and collision checks never switch banks. |
| P2 | **The camera locks behind an opened gate.** No going back to earlier sections. | LEVEL_DESIGN section 2 | Simpler streaming, spawning and checkpoints. Bonus fragments missed in an earlier section are lost until a replay. |
| P3 | **Positions in 1/32 px** (GB Studio style) instead of 1/16 | M2 physics | The tile index is a free high-byte read. A 160-tile level is 40,960 units, so positions must be `uint16_t`. |
| P4 | **Music: Tronimal packs (CC BY 4.0)** as the default source, with VGM2GBSFX for sound effects | DECISIONS (music is "Open") | Ready now, fitting titles. Needs credits. Your own songs can still replace them. |
| P5 | **Story tie-in: "PATCH n/5"** from each level's fragments, used in the boss fight | GAME_DESIGN | Gives the fragments a purpose. Needs a little extra text and a final-boss mechanic. |
| P6 | **Hard mode after clearing the game** (quota = every fragment) | M7 | Cheap replay value; one flag and a menu entry. |
| P7 | **Brighter GBA palettes** when `_is_GBA` is set | M4 | A second palette table per asset. |

Kept as is after the research:
- **No GBC double-speed mode.** The GB Studio engine turns it on (`cpu_fast()`) for GBC,
  but that makes CPU timing differ between DMG and GBC. Our rule that logic fits a DMG
  frame stays.
- **Tiled with our own converter.** The research found no maintained Tiled-to-GBDK
  converter that handles GBC attributes.

---

## 7. Notes on the research itself

- The agents' clones are in the session scratchpad, not in the project. Nothing from them
  was built or run.
- tcrf.net returned text aimed at AI agents instead of its article. The agent ignored it,
  and nothing from it is cited here.
- Licenses were read from each source page or LICENSE file on 2026-10-02. Check again
  before shipping any asset.
- Not verified: whether PyBoy loads the GBDK `.sym` file automatically (M0 will show it),
  the cycle cost of a banked call, and the hUGEDriver license beyond its README.

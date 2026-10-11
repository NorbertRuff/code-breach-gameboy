# Decisions

What was decided and why, one line each, grouped by topic. Decisions from 2026-10-02 to
2026-10-04 with their full reasons, and the superseded ones, are in
[archive/DECISIONS_2026-10.md](archive/DECISIONS_2026-10.md).

**Keeping it:** add a decision to its section with the date and a short reason. Mark it
**Proposed** until the owner agrees. When a decision changes, edit its line (the archive
or git keeps the old one). Details of how the code does it belong in
[ARCHITECTURE.md](ARCHITECTURE.md), not here.

## Vision and process

- **A cozy little game** that looks good, has solid logic, runs on real DMG and GBC, and
  that the owner can explain end to end: forgiving rules, readable visuals, simple code.
- **Scope:** the full 5-level game in GAME_DESIGN.md, one GBC-enhanced ROM that also runs
  on DMG (`-Wm-yc`), with GBDK-2020 4.5.0.
- **Workflow:** Claude writes the code; the owner runs git, reviews and tests on hardware.
  Level design: Claude drafts (brief, greybox, preview), the owner reacts and playtests.
- **Records:** one home per fact (CLAUDE.md, "Records"); DECISIONS and SOURCES feed the
  final "how it works" document (M9). Condensed on 2026-10-04 at the owner's request,
  because keeping 5-7 docs in step slowed every change.
- **No copied assets in the final game:** every asset original (owner or Claude Design)
  or openly licensed and credited.
- **Order:** physics and level loading (M2) before color (M4) and art (M5): layouts
  depend on the jump.
- **Tests:** PyBoy 2.7.0 headless. `make test` is the quick tier (DMG, GBC only where the
  mode matters, in parallel with pytest-xdist); `make test-full` runs both modes and the
  extra cases, before commits and in CI. Overlapping tests were merged or removed
  (2026-10-04, owner): a test should check what no other test checks.

## Game design

- **Lives: 3.** Losing one restarts the section at the last opened gate (the hurt pose,
  or the death pose on the last life, held 0.5 s, then fade out, fade in, 1.5 s
  invincibility; pose hold 2026-10-10, owner); fragments are kept. Game over refills the lives and restarts
  the section with its fragments put back. Hazards cost a life like enemies, and
  invincibility protects from both; falling out costs a life even then.
- **Lives refill at each new level** (2026-10-04, owner).
- **Sections and gates:** each level has 3 sections; a security gate with a fragment
  quota (like 3 of 5) ends each of the first two. Extra fragments are bonus. A gate is a
  full-height column, opened by walking into it with the quota (one row every 2 frames);
  the opened gate is the checkpoint, and the camera locks behind it (no going back).
- **A closed gate buzzes once per touch** without its quota. **Proposed.**
- **Each level ends at a lab elevator** (the exit tile completes the level).
- **HUD:** lives, and the current section's fragments against its quota (`02/03`); in the
  last section, against all its (bonus) fragments.
- **Game over** shows "GAME OVER" in the play area with CONTINUE (restarts the section)
  and QUIT (Saves: Menus). **Level complete** shows the level's fragments ("100%" when
  all) and the password, then START (or 5 s) goes on to the next level; after the last
  level, level 1 again until the ending exists.
- **Pause:** START freezes the game with "PAUSE" in the band; the music keeps playing.
  A + SELECT while paused loads the next level (cheat, `LEVEL_SELECT_CHEAT`).
- **No-death cheat** (2026-10-11, owner asked for it for playtesting): A + B while paused
  turns it on (pickup sound) or off (denied sound); hits don't count, and a fall restarts
  the section without a life. Behind `NO_DEATH_CHEAT`. No on-screen mark, to keep bank 0
  and the HUD as they are.
- **The cheats stay in the release** (2026-10-11, owner), a little harder to enter: A must
  be held while paused, so a stray SELECT or B does nothing.
- **No hard mode** (2026-10-05, owner): cut to keep the game small.
- **Story:** "PATCH n/5"; each level's fragments compile into a patch uploaded in the boss
  fight; the twist that staff #042 wrote the AI stays (STORY.md).
- **Story screens** (2026-10-05): the boot screen (after NEW GAME) and the elevator log
  (after level complete) type their lines on a blank screen, a character every 4 frames,
  then wait for START or 5 s. Log 5 shows before the boss level. Agreed (owner's
  playthrough, 2026-10-11).
- **Boss design approved** (2026-10-05, owner): 3 terminals and uploads (hold Up), one
  earlier idea per phase (packets, the lift, the corrupted floor), no spawned bots or purge
  sweep, a whole-screen shake, game over restarts phase 1, song "Terminate"
  (LEVEL_DESIGN.md section 7). While building it (2026-10-05; agreed in the owner's
  playthrough, 2026-10-11): a terminal
  lights once the phase's first volley has fallen, and phase 2's extra packets stay left
  of the lift (LEVEL_DESIGN.md, "Level 5 build notes").
- **Ending** (2026-10-05; agreed in the owner's playthrough, 2026-10-11): "LOCKDOWN LIFTED" typed as the screen fades to
  light, song "Closing", then a credits screen until START, then the title.
- **Boss:** "KERNEL" on one screen, drawn with background tiles (details in M8, reviewed by
  the owner first).
- **Fades** between screens and on level load (owner likes them).

## Levels

- **Length:** 3 sections, 120-160 tiles, 4-6 minutes on a first play. Level 1 is 174 and
  level 2 170 tiles (owner kept both).
- **Editor:** Tiled 1.12 + `tools/tiled2c.py`; text sketches via `tools/sketch2tmj.py`.
- **Map:** 13 rows (screen rows 3-15) under a 3-row parallax band (decoration only) and
  above the 2-row HUD; up to 255 tiles wide.
- **Rules the converter checks:** pits at least 3 tiles wide (the 16 px body walks across
  a 2-tile pit); every checkpoint has 2 free columns right of its gate, with ground.
- **Jump metrics are measured in the ROM and pinned by tests** (LEVEL_DESIGN.md section 4).
  Level 1 uses steps up to 2 and gaps up to 5; level 2 climbs up to 3 (one-way shelves 3-4
  above), gaps up to 6; only level 5 asks for maximum jumps.
- **Bots:** a `speed` per bot in Tiled, 1 or 2 px per frame (2026-10-04, owner). A bot can
  only be jumped with 8 free rows above its floor (one-way shelves don't count).
- **Lifts** (level 3's mechanic, also the boss's phase 2): a 24 px one-way platform, an
  actor that moves up and down its travel at 1 px per frame and waits 0.5 s at each end
  (owner, 2026-10-04). Lifts move
  before the player's update and carry a player standing on them; the player can jump up
  through one and drop through it. In Tiled a rectangle covers the area the platform
  sweeps; the converter checks a clear path (with room for the body) and 16 px between
  lifts; the reachability check treats a lift as one-way tops on every row it passes.
  Horizontal lifts and other speeds only if a level needs them.
- **Chasers** (level 3, owner 2026-10-04): a bot that stands still until the player is
  within about 6 tiles on its floor (feet within 2 tiles of its own), then walks toward them
  at 1 px per frame (the player runs at 1.5), only inside its range. In Tiled an `enemy`
  with `kind` = `chaser`; its own art (a hovering drone) since 2026-10-10.
- **Level 1** (Server Room) greybox: kept as built after the owner's hardware pass
  (2026-10-04). **Level 2** (Code Testing Lab) greybox: built 2026-10-04, the low route
  needs the bridge's fragment for gate B; kept as built after the owner's hardware pass
  (2026-10-04). **Level 3** (AI Research Wing) greybox: built 2026-10-04 with lifts,
  chasers and 2 patrol bots (owner asked for the bots). **Levels 3-5** kept as built
  after the owner's playthrough (2026-10-11), difficulty and checkpoints included.
- **The game starts in level 1;** level indices list the game's levels, then the test
  rooms (the greybox level the tests use, `test_jumps` for the jump metrics, `test_movers`
  for the lift and the chaser).

## Player physics

- **Positions in 1/32 px (`uint16_t`), velocities `int8_t` in 1/32 px per frame,** as in
  the GB Studio engine; the player is a 16×32 metasprite (4 sprites in 8×16 mode).
- **Gravity:** light only while rising with A held, strong when falling or after A is
  released (jump cut), halved near the top of a held jump (apex hang). Values from
  RESEARCH.md 2.2.
- **Coyote time 6 frames, jump buffer 5,** counted down after they are checked, so the
  number in `player.h` is the number the player gets. A new press of A is needed to jump.
- **Corner correction:** a ceiling corner overlapping the head by up to 4 px is slid past,
  computed from the head row (an 8-position search cost 55 lines: bug 18).
- **Down + A drops through a one-way top** (2026-10-04, owner): the feet move 3 px into the
  one-way row (1 px let a lift going down catch them again); with anything solid under the
  feet it's a normal jump.
- **Collision in tile coordinates** (a position's high byte is its tile): 44 → 24 lines per
  frame.
- **`tools/physics.py` is a frame-exact copy** of the physics for the reachability check,
  compared with the ROM frame by frame.

## Engine

- **Collision ring:** 32×16 bytes in WRAM, filled from the tileset's collision table
  (copied to WRAM at level load) as columns stream in, both directions.
- **Streaming:** columns are read into a 13-byte buffer; the collision goes in at once,
  the tiles at the start of the next update (`set_bkg_tiles` takes 11-14 lines per column,
  and stacked with a pickup and the music it broke the budget; 2026-10-04). There, in
  VBlank, they are written to VRAM directly: 5 lines instead of 12, `set_bkg_tiles` only
  outside VBlank (2026-10-04).
- **Frame budget:** every update must end within 140 of 154 lines after VBlank starts
  (tested), and the ROM counts lag frames. No `/` or `%` in per-frame code (about 6 lines
  per call). Savings so far: touched-column flags, a fragment bit per level column, hiding
  only the sprites the last frame used.
- **Screen splits:** the VBlank handler sets the band's scroll (half the camera) and shows
  sprites; a line interrupt at 23 sets the level's scroll and runs the music step; one at
  127 hides sprites over the HUD. Scrolls are latched at VBlank, so a frame never mixes
  two updates.
- **Camera:** focus = player + a look-ahead easing to 24 px, kept within 8 px of screen x
  80. An opened gate sets a left limit, in force once the camera gets there; near the
  level's end, the end wins (bug 22).
- **Actors:** spawned from the level's sorted entity list with a cursor when within 16 px
  of the screen's right edge, removed a screen behind the camera until the section
  restarts, at most 8, one type table. The sprite list is rebuilt every frame.
- **A tileset per level** (2026-10-10): each level's header points at its tileset's
  `TilesetData`, written by `tools/tiled2c.py` from the `.tsx`: the art and band, the
  collision and palette of each tile, the marker tiles, the open elevator. Level 1 uses
  `server_room`, level 2 `code_testing_lab`, level 3 `ai_research_wing`, level 4
  `network_hub`, level 5 `mainframe`; the test rooms the greybox.
- **Bank 0 is for per-frame code;** code that never runs in a gameplay frame lives in ROM
  banks (`#pragma bank 255`, `BANKED`). Code that reads banked data inside the caller's
  `WITH_BANK` stays in bank 0 (2026-10-04: the palette fades and the level start/restart
  flow moved out, about 730 bytes).
- **Screen text** in the game's own font (`src/engine/text.c`); `printf` took 2.6 KB of bank 0.
- **SDCC pitfall:** `?:` passed straight as a function argument can compile wrong
  (`head_row` is a function for this). `sizeof` of a string literal leaves the string in
  the ROM of each function that uses it: `FONT_CHAR_COUNT` is a number, checked against
  the font's characters in `text.c` (2026-10-10, 100 bytes of bank 0).

## Color and graphics

- **Palette module** (`src/engine/palette.c`) is the only code that writes palettes:
  loads, 4-step fades and flashes on both systems; GBC fade steps by masked shifts of
  RGB555 (multiplying took 2 frames per step).
- **GBC palettes:** background 0-3 the level's tileset (its palettes in the set's order,
  so a tile's attribute is its tileset palette, no remapping), 4 the band, 5 the HUD
  (`hud` set: #E0F8D0 #88C070 #346856 #081820, 2026-10-10, **Proposed**), 6-7 free
  (KERNEL only swaps tiles of the same palette, 2026-10-11). Was 0 level, 1 band, 2 HUD until a tileset needed 4 (2026-10-10,
  **Proposed**). Sprite 0 the player, 1 the pale invincible player (DMG blinks instead),
  2 the enemies (bots, chasers, KERNEL's packets), 3 the lifts: the style guide's three
  sprite palettes in order (player, red, gold; `server_room` set through
  `tools/gbconvert.py`), 2026-10-10, owner.
- **A palette per tile, not per map cell** (2026-10-10, **Proposed**): `tools/tiled2c.py`
  reads each tile's palette from the indexed tileset PNG (color index / 4) into
  `<tileset>_tileset_palette[]`, copied to WRAM at level load (256 bytes) like the
  collision table. A streamed column looks up its 13 attributes as it is read and, on GBC
  only, writes them to VRAM bank 1 with its tiles (about 2 more lines in VBlank, 11
  outside it); a gate row or a pickup writes its cell's attribute too. An attribute map
  per level would double the map data for no gain: the palette never differs between two
  uses of a tile.
- **Brighter palettes on GBA**, chosen at boot (`_is_GBA`). No GBC double-speed mode.
  **Proposed.**
- **Art pipeline:** Claude Design → `tools/gbconvert.py` → png2asset (ART_PIPELINE.md);
  every palette light to dark; no background tile flips.
- **Placeholders until each level's art pass:** the greybox tileset and band (drawn by
  scripts), now only in the test rooms (levels 3-5 got their art 2026-10-11).
- **Server Room art** (2026-10-10, **Proposed**): redrawn by hand on the 8×8 grid from
  the owner's Gemini starter art (mock-up, band, object sheet; not on a pixel grid, so
  used as references). Green riveted machine blocks with a striped top lip (a 16-tile
  Tiled terrain set, plus a vent plate), a blue truss catwalk (one-way), red and gold
  wiring (hazard), racks, pipes, cable trays and terminals behind the play lane in
  shades 0-2 only, a gold and black striped shutter for the gate (gray slats read as a
  ladder), a 2×4 elevator that opens at level complete (closed gray, open lit green), a
  floppy disk for the fragment; 68 tiles, the band 16. No light plates in the fill: on
  DMG they read as holes.
- **Code Testing Lab art** (2026-10-10, **Proposed**): level 2 in Server Room's way,
  redrawn by hand from the owner's Gemini mock-up and band. Teal riveted panels with a
  dark outline and a light lip on exposed sides (a 16-tile "panel" terrain set, plus a
  vent plate), thin blue shelves with brackets (one-way: the tile's lower half is empty),
  sparking cut cables (hazard, unused in level 2), and behind the play lane only shades 0
  and 1 of the props palette: desks with monitors, lab benches with an oscilloscope,
  towers, chairs, whiteboards, hanging cables and pipes. The gate, its rails, the elevator,
  the floppy and the start marker are level 1's tiles at the same ids, recolored. 69 tiles,
  the band (desks, racks and benches under a ceiling pipe) 22. The map keeps its layout
  and every cell's collision; the props were placed by a script, so the owner can repaint
  them in Tiled.
- **AI Research Wing art** (2026-10-11, owner): level 3 in the same way, from the
  owner's Gemini mock-up and band. Dark-purple riveted lab plates with a lilac lip (a
  16-tile "plate" terrain set, plus a vent plate), thin blue catwalks (one-way), and
  behind the play lane in shades 0-1: specimen tanks, neural-network boards, server
  cabinets, terminals, lamps, and rails with a pulley beam behind each lift. Gate, rails,
  elevator (open: lit purple), floppy and start marker are level 2's tiles recolored. 69
  tiles, the band 28. Layout and every cell's collision kept; props placed by a script.
- **Network Hub art** (2026-10-11, owner): level 4 in the same way. Tan floor tiles
  with a lip and dark outline (a 16-tile "floor" terrain set, plus a vent), lighter than
  levels 1-2 so the dark hazards stand out; blue data bridges (one-way); spikes, one per
  tile (yellow and dark red, black outline), and hanging cut-cable bundles ending in
  spikes; wall panels, patch panels, racks and routers behind in shades 0-1. Shared
  objects recolored from level 2, the open elevator lit blue. 72 tiles, the band 21.
  Layout and collision kept; props placed by a script.
- **Mainframe art and KERNEL's face** (2026-10-11, owner): level 5 in the same
  way: lavender riveted plates ("plate" terrain set), a blue truss catwalk (one-way),
  racks, consoles and cables behind; the band tape-reel mainframes under a truss. 96
  tiles, the band 17. KERNEL's face (7x6 tiles, traced from `kernel_face_v1`; the owner kept it
  as drawn), its wide eyes, the glitch (from `kernel_v2`) and the terminals
  are cells of the mainframe tileset, painted in the map; `kernel.c` swaps tiles by
  tileset id, each with the palette of the cell it replaces, so no attribute writes are
  needed (`tests/test_kernel_art.py` checks it). So the corrupted floor is black spikes on
  the floor's gray palette, not red (owner: fine). The fragment stays the floppy in every
  level, to match the HUD icon (owner, 2026-10-11).
- **Title screen** (2026-10-10, owner): redrawn by hand at 1× on the 8×8 grid from
  the owner's Gemini starter art (a server corridor, the programmer from behind, his head turned
  to a monitor; the "APEX [LAB] / CODE BREACH" logo), 139 tiles from 0. A warning triangle on the
  monitor (6×5 tiles, 23 more after the font) blinks by a tile-map swap every 30 frames,
  the same on DMG and GBC, and stays on under the menu. "PRESS START" is printed with the
  font below the monitor, where START's NEW GAME / CONTINUE box covers it. 4 palettes on
  GBC (`title` set: scene and racks, walls and monitor, the programmer, the triangle and
  LEDs), a palette per map cell from png2asset's attribute map; on DMG the 4 grays by
  palette index. The programmer is traced from the Gemini scene, his face, glasses and
  hair finished by the owner in Aseprite. Replaces the owner's `apexLogo.png` as it was.
- **Background tiles** (2026-10-10, **Proposed**): the tileset from 0 and its band after
  it (tileset at most 128, band 32), the font 160-208, the HUD 209-218 (was 64-73, which
  capped a tileset at 64 tiles); 219-255 free (KERNEL's tiles moved into level 5's
  tileset, 2026-10-11).
- **HUD and font** (2026-10-10): redrawn from Gemini starter art. The HUD is 2 tile rows
  (Gemini's was 30 px tall): light boxes for the lives and the count, which the code
  writes, a heart and a floppy icon in the frame, a riveted strip below; 10 tiles. One
  1-bit font of 49 characters (7×7 glyphs) for all text, the HUD's digits and '/'
  included. 59 tiles together; GBDK's 1-bit loader costs about 110 bytes of bank 0.
  **Proposed.**
- **Player sprite is final** (2026-10-10, owner): the owner's programmer, first based on
  a Mario sprite and then heavily redrawn, plus 5 frames drawn in its style (10 frames:
  idle 2, run 4, jump, fall, hurt, death). `graphics/player/programmer.png` and the
  "ApexLab Hero Sprite Sheet" Claude Design canvas are the source of truth.
- **All player frames stay in VRAM** (68 sprite tiles, 2026-10-10): no per-frame tile
  copy, so no frame budget cost. Copying each frame's 8 tiles in VBlank is the fallback
  if tiles run out.
- **Enemy and lift sprites** (2026-10-10): the security bot, the chaser, the lift and
  KERNEL's packet, redrawn at 16×16 (the lift 24×16) from the owner's reference pictures
  on the "ApexLab Enemy Sprites" Claude Design canvas. 2 frames each but the lift, every
  8 frames, by a tile offset (no tile copies, about 1 line). Every actor's tiles stay
  loaded in every level: 108 of the 128 sprite-only tiles in use, 20 left for the L2-L4
  enemies. **Proposed.**
- **Golden screenshots** of key screens in both modes catch visual changes.

## Audio

- **Music:** hUGEDriver 6.1.3 (released GBDK library), songs from Tronimal's Game Boy Music
  Packs #1 and #2 (CC BY 4.0), converted by `tools/uge2c.py` (a Python port of
  hUGETracker's export, instead of downloading `uge2source.exe`).
- **The music step runs in the line-23 interrupt, not VBlank** (2026-10-04, owner): about 9
  lines per frame, up to 18; in VBlank it delayed every update.
- **Songs:** level 1 "Tech", level 2 "Neurotic Robonaut" (owner, after the hardware pass,
  2026-10-04), level 3 "Observing Jupiter" (owner: keep, 2026-10-04), level 4 "Whirlwind"
  (2026-10-05, **Proposed**); only Tronimal's own songs, not the packs' remixes of other
  people's.
- **Screen songs:** title "Decampment", game over "Gone Missing", level complete "Delight",
  each after the screen's flashes; the level's song starts again after them (2026-10-04,
  owner approved after listening).
- **Sound effects: Tronimal's GBFX and GBFX2** (FX Hammer files, CC BY 4.0), picked in
  `sounds/effects.txt` and converted by `tools/fxhammer2c.py`; our own short C player
  (`src/sound/sound.c`) instead of VGM2GBSFX's assembly one: one effect at a time with a
  priority, its channels (CH2, CH4) muted in hUGEDriver until it ends, stepped from the
  music's interrupt. The hand-made register effects are gone (2026-10-04, owner approved
  the player and the picks after listening).

## Saves

- **Passwords instead of battery saves** (2026-10-04, owner): no save test ROM kept its
  count on the owner's EverDrive clone (MBC5, MBC1, MBC3). So the cartridge is plain MBC5
  (`0x19`, no RAM); the save test ROMs keep MBC5 + RAM + battery as tools.
- **Password contents** (2026-10-04, owner): 4 characters from 16 easy-to-read ones (no
  0/O or 1/I): the next level (1-5), a 100% mark per finished level, and a
  checksum, so a typo is refused instead of loading the wrong game. CONTINUE on the title
  screen opens the entry; the level complete screen shows the new password.
- **Password format** (2026-10-05): letters `BCDFGHJKLMNPRTWX` (consonants only, so no
  words); one progress byte (furthest level, marks of levels 1-4, the boss level has none;
  bit 7 unused, a password with it set is refused) and a check byte that is a one-to-one mix of it, so every single wrong letter
  is refused (`tools/password.py`). The furthest level never passes the build's last
  level, so the password after the last level works in the same build.
- **Menus** (2026-10-05): the title shows NEW GAME / CONTINUE after START; CONTINUE asks
  for the password, then a level select (levels up to the furthest, with 100% marks,
  starting on the furthest; skipped when only level 1 is open). Game over shows CONTINUE /
  QUIT and the password, and waits for a choice (no 5 s timeout any more); QUIT goes to
  the title. Menu sounds reuse the approved effects: pause beep to pick or type, gate to
  accept a password, buzz to refuse one. **Proposed.**

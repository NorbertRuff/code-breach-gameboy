# ApexLab: Code Breach - Game Design

A Game Boy platformer in 5 levels. One ROM: in color on a Game Boy Color, in 4 shades on
the original Game Boy, with the same gameplay on both.

## Story

You are staff #042, a programmer at the ApexLab research facility. The AI security system
goes haywire and locks the lab down. Collect the code fragments, ride the elevators down
through the lab, and upload a patch into the Mainframe AI, KERNEL.

The story is told in a few lines of text (all in `src/game_state/story.c`):
- **Boot screen** (after NEW GAME): `ANOMALY / STAFF #042 / LOCKDOWN`.
- **Level complete:** each level's fragments compile into one part of the patch,
  `PATCH n/5 COMPILED`.
- **Elevator rides:** one line of the AI's log per ride, more corrupted on every floor
  (digits stand in for letters). The last one, before the boss, hints at the twist:
  staff #042 wrote the AI.
- **Ending:** the screen fades to light, `LOCKDOWN LIFTED`, then the credits.

## Controls

| Button | Action |
|---|---|
| Left / Right | Run |
| A | Jump; hold it to jump higher |
| Down + A | Drop through a one-way platform |
| Up (hold) | Upload at a lit terminal (boss level) |
| START | Pause |

Testing cheats, kept in the release: while paused, hold A and press SELECT to skip to
the next level, or hold A and press B to turn no-death on or off.

## Rules

- **3 lives**, refilled at each new level. An enemy, a hazard tile or a fall costs a
  life. After a restart the player is invincible for 1.5 s.
- **3 sections per level.** A security gate closes each of the first two. It opens when
  the player walks into it with the section's quota of fragments (like 3 of 5). Extra
  fragments are a bonus; all of a level's fragments give it a `100%` mark.
- **An opened gate is a checkpoint.** A lost life restarts there; collected fragments
  stay. The camera can't go back past an opened gate.
- **Game over** offers CONTINUE (full lives, the section starts over with its fragments
  put back) or QUIT.
- **The level ends** at the lab elevator after section C.
- **The HUD** shows the lives and the fragments against the current section's quota
  (`02/03`); in section C, against all of its fragments.
- **Demo:** a minute on the title screen without a press plays a clip of the next level
  by itself; any button goes back to the title.
- **Passwords** keep progress (no battery save): 4 letters, shown at level complete and
  game over. CONTINUE on the title screen asks for one, then offers every level reached,
  with the `100%` marks.

## Levels

Each level adds one idea and has its own song (CREDITS.md).

| # | Level | New idea | Width | Fragments | Enemies and movers |
|---|---|---|---|---|---|
| 1 | Server Room | Running, jumping, fragments, gates, pits | 174 tiles | 14 (gates 3 of 4, 3 of 6) | 8 slow patrol bots |
| 2 | Code Testing Lab | One-way platforms, drop through | 170 | 13 (3 of 5, 3 of 5) | 1 slow and 5 fast bots |
| 3 | AI Research Wing | Lifts and chasers | 170 | 13 (3 of 5, 3 of 5) | 8 lifts, 3 chasers, 2 bots |
| 4 | Network Hub | Hazard tiles (spike strips, spike pits, hanging spikes) | 180 | 13 (3 of 5, 3 of 5) | 2 bots, 1 chaser, 1 lift |
| 5 | Mainframe | Boss: KERNEL | 1 screen | - | Packets, a lift |

- **Patrol bot:** walks its range at 1 or 2 px per frame.
- **Chaser:** a drone that waits until the player is about 6 tiles away on its floor,
  then follows at 1 px per frame (the player runs at 1.5), only inside its range.
- **Lift:** a one-way platform that moves up and down and waits 0.5 s at each end.

Each level has its own tileset and parallax band: green machine blocks in the Server Room,
teal panels in the Code Testing Lab, purple lab plates in the AI Research Wing, tan floor
tiles in the Network Hub, lavender steel and tape-reel mainframes in the Mainframe. The
collectible is a floppy disk in every level, matching the HUD icon.

### Level 5: KERNEL

KERNEL is a monitor face drawn with background tiles at the top of a one-screen arena.
The player can't attack it. Instead, three kill-switch terminals light up one at a time;
standing on the lit one and holding Up for about half a second uploads part of the patch.
Letting go, jumping or getting hit starts the upload over.

| Phase | Terminal | KERNEL's attack |
|---|---|---|
| 1 | On the floor, left | Packet rain: 2 packets per volley |
| 2 | On a high shelf, reached by a lift that appears | Packet rain: 3 per volley |
| 3 | On the floor, middle | Packet rain, and floor segments that turn into hazards for 1.5 s |

Every attack is announced: a marker blinks over each packet's column for 0.5 s, and a
floor segment flickers before it turns. The patterns are fixed tables, no randomness.
Each upload hits KERNEL: a short freeze, a screen shake and flashes. A lost life restarts
the current phase (finished uploads stay); game over restarts the fight. The third upload
ends the game.

## Look

- A cyberpunk tech lab: server racks, workstations, cables, terminals.
- The hero is a 16×32 programmer (4 hardware sprites); enemies are 16×16.
- A parallax band of 3 tile rows at the top scrolls at half speed; the HUD is the bottom
  2 rows; the play area is the 13 rows between.
- Every palette runs from light to dark, so the original Game Boy shows the same picture
  in 4 grays. On GBC, tile types get colors (red hazards, blue one-way platforms, gold
  fragments), and each type also has its own shape so it reads without color.
- Fades between screens; flashes are slow (at most 3 per second).
- Music by Tronimal (Game Boy Music Packs #1 and #2) and sound effects from GBFX and GBFX2,
  all CC BY 4.0 ([CREDITS.md](../CREDITS.md)).

## Making levels

Levels are drawn in [Tiled](https://www.mapeditor.org/) 1.12 and turned into banked C data
by `tools/tiled2c.py`. How the game loads, streams and collides with them is in
[ARCHITECTURE.md](ARCHITECTURE.md); the data format is in `src/level/level_data.h`.

### Adding or changing a level

1. **Draw it** in Tiled: open `levels/level_N.tmj`, or copy one as a start. A first
   layout can use the greybox tileset, then the level's own (`graphics/tilesets/`).
2. **`make levels`** writes `src/levels/<map>.c/.h` and `<tileset>_tileset.c/.h`. The
   output is committed; `tests/test_generated.py` fails when it is older than the maps.
   A map that breaks a rule ("What tiled2c.py checks") stops the build with a message.
3. **`make demo`** records the title demo's clips again (a changed level plays
   differently); `tests/test_demo.py` fails until it does.
4. **`make preview`** draws every level to `tests/out/levels/<name>.png` and fails when
   something can't be reached ("Preview and reachability").
5. **A new level** also needs its entry in the tables in `src/level/level_manager.c` and
   `level_manager.h`, its song in `src/sound/music.c`, its log line in
   `src/game_state/story.c`, and a replay test that finishes it (`tests/test_level_N.py`,
   with `Pilot` from `tests/gamelib.py`).
6. Play it in Emulicious (`make run`; while paused, A held + SELECT skips to the next
   level).

Refer to places by tile column: the preview and Tiled's status bar both show them.

### Tiled setup

- **Files:** maps in `levels/*.tmj` (JSON, CSV layer data, so diffs are readable),
  tilesets in `graphics/tilesets/*.tsx` next to their PNG. The tileset PNG is the indexed
  output of `tools/gbconvert.py` ([ART_PIPELINE.md](ART_PIPELINE.md)); each tile's GBC
  palette is read from it, not set by hand.
- **One tileset per level theme:** `server_room` (level 1), `code_testing_lab` (level 2),
  `ai_research_wing` (level 3), `network_hub` (level 4), `mainframe` (level 5), and
  `greybox` (the test rooms). A
  level's parallax band is `graphics/bands/<tileset>_band.png`, 32×3 tiles; it isn't part
  of the map.
- **Per-tile property `collision`:** `solid`, `one_way` or `hazard`; empty is none.
- **Terrain sets** paint the edge tiles: "machine" in `server_room.tsx`, "panel" in
  `code_testing_lab.tsx`, "plate" in `ai_research_wing.tsx` and `mainframe.tsx`, "floor"
  in `network_hub.tsx`.
- **Map size:** exactly 13 tiles tall (screen rows 3-15), 20 to 255 tiles wide.
- **Layers:** `tiles` (a tile layer) and `entities` (an object layer).
- **Entities:**

| Class | Placed as | Notes |
|---|---|---|
| `player_start` | Tile object (marker tile) | The body's left edge on the marker's left edge, feet on its bottom |
| `fragment` | Tile object | Belongs to the section it sits in (by x, between gates) |
| `gate` | Tile object, property `quota` | Fills its column from the marker up to the map's top row; also the checkpoint |
| `exit` | Tile object | In a tileset with an elevator: the closed elevator (`elevator_1`-`_8`, 2×4) painted around it, the marker its bottom-left tile; the game draws `exit_open_1`-`_8` at level complete |
| `enemy` | Rectangle | Left and right edges: the patrol range; bottom edge: the floor. Property `kind` (`patrol` or `chaser`), int `speed` 1 or 2 px per frame (default 1) |
| `lift` | Rectangle, 24 px wide, on the 8 px grid, at least 16 px tall | The area the platform sweeps, top to bottom; it starts at the bottom, going up |

- Marker tiles carry their class; painting one on the tile layer is an error. All objects
  snap to the 8 px grid.
- **Map properties:** `boss` = true (the KERNEL arena: no gates needed); `test` = true (a
  test room for the PyBoy tests: no gates, no exit).

### Level rules

- **3 sections**, A and B each closed by a gate with a quota, C ending at the exit. A
  teaches the level's new idea safely, B mixes it with earlier ones, C is the hardest.
- **Put each section's bonus fragments before its gate:** the camera locks behind an
  opened gate, so missed ones are gone until a replay.
- **Rows:** nothing that matters in rows 0-2 of the screen (the band scrolls at another
  speed). The main floor is usually map row 12 (screen row 15). The highest useful
  platform top is about screen row 7, so the 4-tile body fits under the band.
- **No blind jumps:** the camera shows about 10 tiles ahead; every landing must be
  visible from the take-off.
- **Sprites:** at most 3 enemies at the same height on screen (10 sprites per line: the
  player uses 2, a 16 px enemy 2, a lift 3), and about 6 enemies on screen in all. At most
  8 actors are in play at once.
- **Tiles:** a level tileset has at most 128 tiles and its band 32
  ([ART_PIPELINE.md](ART_PIPELINE.md) section 2).
- **Difficulty:** level 1 keeps steps to 2 tiles and gaps to 5, so every jump works from
  a standstill; levels 2-4 climbs to 3 and gaps to 6; only level 5 asks for maximum jumps.

### Jump metrics

Measured in the ROM, the same on DMG and GBC. `tests/test_jump_metrics.py` checks every
number and fails when a physics change moves one; the values behind them are in
`src/player/player.h`. Heights count tiles above the floor; gaps count missing floor tiles
between two ledges at the same height.

| Metric | Value |
|---|---|
| Max jump height (A held) | 4 tiles (rises 36 px, 43 frames in the air) |
| Min jump height (A tapped) | 1 tile (rises 13 px, 17 frames) |
| Max gap, standing jump | 7 tiles |
| Max gap, running jump | 9 tiles; 10 with coyote time |
| Run speed | 1.5 px per frame; full speed after 6 frames, stops in 8 |
| Coyote time / jump buffer | 6 frames after leaving a ledge / A up to 5 frames before landing |
| Ceiling corners | Up to 4 px of overlap is slid past |
| Drop through | Down + A on a one-way top; on solid ground it jumps |

What follows from them:
- **A solid shelf 1-4 tiles above the floor is a wall** for the 4-tile body; one 5 or
  more up can't be reached from that floor. Climb with steps of 1-4 tiles or one-way
  platforms (they don't block walking).
- **A pit must be at least 3 tiles wide:** the 2-tile body walks across a 2-tile pit.
- **A bot can only be jumped with 8 free rows above its floor;** under a lower shelf the
  jump bonks too early.
- Stack one-way tiers at least 4 rows apart, or a bot on the tier above touches the head
  of a player on the tier below.
- Keep bots a few tiles away from a checkpoint and from a jump's landing.

### Preview and reachability: `tools/levelpreview.py`

`make preview` draws each level at 2×: tiles with column numbers every 5, the band,
fragments (green outline: reachable, red: not), gate quotas, the exit, patrol ranges, the
player start, the surfaces the player can stand on, and the jump that first reached each
one (blue, green, purple for sections A, B, C).

The check simulates jumps with `tools/physics.py`, a frame-exact copy of `player.c`
(`tests/test_levelpreview.py` compares them frame by frame, so a physics change must be
made in both). From every reached surface it tries tapped, medium and full jumps, left,
right and straight up, standing and running, steering early and late, and walking off
each edge. Gates count as open, the player can't go back past one, enemies are ignored,
lifts count as one-way tops on every row they pass, and a jump that touches spikes or a
pit doesn't count. It fails when the exit, a fragment, or a gate's quota can't be reached.

### What `tools/tiled2c.py` checks

The build stops when:
- the map isn't 13 tiles tall, or is narrower than 20;
- there isn't exactly one `player_start`, or (but in a `boss` map) exactly one `exit`;
- a level (not `boss` or `test`) doesn't have exactly two gates;
- there are more than 32 fragments;
- a gate asks for more fragments than its section has, or has no room to restart right of
  it (2 free columns, the body's 4 rows free of solid and hazard tiles, ground below);
- a lift isn't 24 px wide on the grid, has a solid tile in its path (with room for the
  body riding it), or is less than 16 px from another lift;
- the tileset has more than 128 tiles, or a tile mixes palettes;
- the exit isn't in the closed elevator, when the tileset has one.

It warns when an enemy's patrol range has no ground under it, and when a pit is narrower
than 3 tiles.

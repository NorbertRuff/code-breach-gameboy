# Level Design

How we design levels together, the rules every level follows, and the level briefs.
Tools and file formats are in [LEVEL_PIPELINE.md](LEVEL_PIPELINE.md).

## 1. How we work

Agreed on 2026-10-02: **Claude drafts, the owner reacts.**

| Step | Who | Output |
|---|---|---|
| 1. Brief | Claude proposes, owner edits in chat | Section-by-section plan, stored in this file |
| 2. Greybox | Claude | Text sketch in chat, then a Tiled map (`levels/level_N.tmj`) using the plain block tileset: `tools/sketch2tmj.py sketch.txt -o levels/level_N.tmj` turns the sketch into the map |
| 3. Preview and checks | Claude | `make preview` → `tests/out/levels/<name>.png`: the whole level with column numbers, enemies, fragments and jump arcs. Plus a reachability report: can every fragment, gate and the exit be reached with the real jump? ([LEVEL_PIPELINE.md](LEVEL_PIPELINE.md) section 5) |
| 4. Owner pass | Owner | Edits in Tiled, or notes by column number. Plays it in Emulicious. |
| 5. Tune | Claude | Applies the notes. A PyBoy replay test proves the level can be finished. |
| 6. Art pass | Claude | Swaps the block tileset for the real one (roadmap M5). |

**Feedback format:** refer to tile columns. Both the preview and Tiled's status bar show
them. Examples: "col 42: gap too wide", "section B drags", "the bot at col 88 is unfair".

**Text sketch legend** (used for quick drafts in chat; `tools/sketch2tmj.py` converts a
sketch to a Tiled map, and `--from-tmj levels/<map>.tmj` prints a map as a sketch, so the
sketches in this file can be redrawn after an edit in Tiled):

| Char | Meaning |
|---|---|
| `.` | Air |
| `#` | Solid |
| `=` | One-way platform (from level 2) |
| `^` | Hazard (from level 4) |
| `*` | Code fragment |
| `E` | Enemy: a run of `E`s is one patrol bot standing on the row below, the run is its patrol range |
| `P` | Player start |
| `G` | Security gate (section end and checkpoint) |
| `D` | Exit elevator |

## 2. Level structure

- **3 sections** (A, B, C), each 40–55 tiles wide. A level is 120–160 tiles in total and
  takes 4–6 minutes on a first play.
- **A security gate closes each of sections A and B.** The gate opens when the player has
  collected the section's **quota** of code fragments, for example 3 of 5. Extra
  fragments are a bonus: score, and a 100% mark on the level-select screen.
- **A gate fills its column** from its tile up to the top of the playfield, so it can't
  be jumped. It opens when the player walks into it with the quota collected.
- **An opened gate is a checkpoint.** Losing a life restarts at the last opened gate,
  just right of it: leave 2 free columns there with ground below (the converter checks).
  Collected fragments stay collected. 3 lives. Game over refills the lives and restarts
  the current section: fragments collected since the last gate are reset.
- **The camera locks behind an opened gate.** The player can't go back to an earlier
  section. Bonus fragments missed there can only be collected by replaying the level, so
  put each section's bonus fragments where an attentive player will find them before
  the gate.
- **Hard mode** (unlocked after clearing the game): every gate needs all of its
  section's fragments. Every fragment must therefore be reachable without going back.
- **Section C ends at the exit elevator.** It has no quota. Its fragments are all bonus.
- **Inside a level:** section A teaches the level's new idea safely, B mixes it with
  earlier ideas, and C is the hardest.
- **One new mechanic per level** (from GAME_DESIGN.md): L1 basics, L2 one-way platforms,
  L3 vertical movers and chasers, L4 hazard tiles, L5 boss.
- **The HUD shows** lives, plus fragments collected against the quota of the current
  section, for example `◆ 2/3`. In section C, which has no gate, it counts against all
  of C's fragments.
- **The level ends** when the player walks into the exit elevator.

## 3. Screen and hardware limits

| Limit | Rule for layouts |
|---|---|
| Rows 0–2 are the parallax band, rows 3–15 the playfield (13 rows), rows 16–17 the HUD. No vertical scrolling. | Gameplay fits in rows 3–15: nothing solid, no enemies and no fragments in rows 0–2, which scroll at a different speed. The main floor is usually row 15. The highest usable platform top is about row 7, so a 4-tile player still fits under the band. |
| The player is 4 tiles tall and 2 wide. | Walkable passages need at least 4 tiles of headroom; passages you jump in need more. |
| The camera keeps the player at x = 80 px, so the player sees 10 tiles ahead. | The landing spot of every jump must be visible from where the jump starts. No blind jumps. |
| 10 sprites per scanline. The player uses 2 per line; a 16 px enemy uses 2. | At most 3 enemies visible at the same height at the same time. |
| 40 sprites total, shared with pickups and effects. | At most about 6 enemies on screen at once. |
| Tileset budget: about 190 unique tiles per level. | Visual variety per level, not per section. |

Level width has no hardware limit: map streaming copies columns in as the camera moves,
and collision is kept in a 32-column RAM ring that follows the camera.

## 4. Jump metrics

Measured in the ROM on 2026-10-03, the same on DMG and GBC. `tests/test_jump_metrics.py`
checks every number below and fails when a physics change moves one. The values behind
them are in `src/player/player.h`.

Heights count tiles above the floor the player stands on. Gaps count missing floor
tiles between two ledges at the same height; a landing counts with 1 px of the body
over the far ledge.

| Metric | Value |
|---|---|
| Max jump height (A held) | 4 tiles: lands on a ledge 4 tiles up, not 5. Rises 36 px, 43 frames in the air |
| Min jump height (A tapped) | 1 tile: rises 13 px, 17 frames in the air |
| Max gap, standing jump | 7 tiles: front of the body at the ledge's edge, A and right pressed together |
| Max gap, running jump | 9 tiles, taking off from the ledge. Coyote time allows 10 with a jump up to 6 frames late |
| Safe landing width | At least 3 tiles |
| Run speed | 1.5 px per frame, about 11 tiles per second. Full speed after 6 frames, stops in 8 |
| Coyote time / jump buffer | A jump works up to 6 frames after leaving a ledge / an A press up to 5 frames before landing still jumps |
| Ceiling corners | A corner that overlaps the body by up to 4 px is slid past instead of bumped |

What follows from these numbers:

- **A solid shelf the player can reach blocks walking under it.** The body is 4 tiles tall
  and the highest climb is 4 tiles. A solid shelf 1–4 tiles above the floor is a wall at
  body height; one 5 or more tiles up can't be reached from that floor. Go up with
  stairs (steps of 1–4 tiles), or with a one-way platform: it doesn't block walking and
  can be jumped through from below.
- A climb of 2 or more tiles needs A held; a tap only clears 1-tile steps.
- **A pit must be at least 3 tiles wide.** The body is 2 tiles wide: it walks across a
  2-tile pit without falling in. `tools/tiled2c.py` warns about narrower pits.
- **A bot can only be jumped with 8 free rows above its floor** (a shelf over a floor on
  row 12 at row 3 or higher). Under a lower shelf the jump bonks too early: with 7 free
  rows the body clears the bot for about 11 frames, which covers about 27 px against a
  bot walking the other way, and passing a 16 px bot takes about 30.
- **Level 1 limits** (difficulty rule below): steps up to 2 tiles, gaps up to 5 tiles
  (the standing maximum minus 2), so every jump also works from a standstill.

Difficulty rule: level 1 uses gaps and heights at least 2 tiles below the maximum. Only
level 5 asks for maximum-distance jumps.

## 5. Building blocks

Names to use in briefs and feedback.

| Name | Meaning |
|---|---|
| Step | A ledge 1–2 tiles high |
| Stair | Steps in a row going up |
| Gap-N | A pit N tiles wide. Falling in costs a life. |
| Shelf | A floating platform |
| High route / low route | Two parallel paths. The harder one has bonus fragments. |
| Patrol lane | Flat ground with an enemy, long enough to wait for it to turn |
| Alcove | A small hidden pocket with a bonus fragment |
| Gate | Security gate at the end of section A or B |

## 6. Level 1 brief: Server Room

**Status: section plan accepted on 2026-10-02. Greybox drafted on 2026-10-04**
(`levels/level_1.tmj`, below); waiting for the owner pass (step 4 in section 1).

Checked against the jump metrics (section 4) on 2026-10-03. On the main path, steps are
at most 2 tiles and gaps at most 5; the longest jump in section C is a 5-tile gap. Two
items need a change, to settle with the owner in the greybox:
- A4: the alcove needs a held jump of 3–4 tiles, above the level 1 limit of 2.
  Proposal: keep it, since it is a bonus fragment off the main path.
- C3: "shelves above to escape onto" can't work as written. A solid shelf the player
  can reach blocks the floor under it, and level 1 has no one-way platforms.
  Proposal: low crates (2-tile steps) along the run that the player hops onto.

Teaches: running, jumping, fragments, enemies, gates. No one-way platforms and no hazard
tiles. Two narrow cable trenches introduce pits.

**Section A, "Boot sequence"** (about 45 tiles, 1.5 minutes)
1. Start on flat floor with 6 tiles of safe space.
2. A 1-tile step, then a 2-tile step: learn to jump. Fragment 1 on top of the second step.
3. A server-rack stair of 3 shelves going up. Fragment 2 at the top.
4. Bonus fragment in an alcove above the rack. It needs a held jump, which teaches that
   jump height varies.
5. Gap-2 cable trench: the first pit, with a wide landing.
6. First enemy: a slow security bot on a long patrol lane, with room to wait for it to turn.
7. Fragment 4 after the bot. **Gate A: 3 of 4.**

**Section B, "Rack aisles"** (about 50 tiles, 2 minutes)
1. The path splits: a low route on the floor with 2 bots, and a high route over the rack
   tops with fragments and gap jumps.
2. The routes merge at a Gap-3 trench with a shelf in the middle.
3. A bot patrols a shelf the player must land on: a timing test.
4. A bonus alcove high up.
5. **Gate B: 3 of 5.**

**Section C, "Core cooling"** (about 45 tiles, 1.5 minutes)
1. A stair going up, with bots on alternating steps.
2. The longest jump of the level.
3. A final run past 2 bots on flat ground, with shelves above to escape onto.
4. A last bonus fragment off the path, then the exit elevator.

**Totals:** about 140 tiles. 13 fragments: 6 needed for the gates, 7 bonus. 8 enemies
placed, no more than 3 on screen at once.

**Settled with the owner (2026-10-02):**
- Gates open with a fragment quota, not a key card.
- Level 1 has the two narrow cable trenches.
- Every level ends at a lab elevator that takes the player deeper into the facility,
  toward the Mainframe.

### Level 1 greybox (2026-10-04)

`levels/level_1.tmj`, 174 tiles wide (sketch below: `tools/sketch2tmj.py --from-tmj
levels/level_1.tmj`). `make preview` reaches every surface and all 14
fragments. `tests/test_level_1.py` plays it from the start to the exit without losing a
life, once through section B's high route and once through its low route, each collecting
11 fragments (`tests/out/levels/level_1.png` is the full map).

Columns 0–112 (sections A and B; the top line counts columns in tens):

```
0         1         2         3         4         5         6         7         8         9         0         1  
.................................................................................................................
.................................................................................................................
....................................................................*......*.....................................
............................*.....................................####...####....................................
...........................###..............................................................................*....
...........................................................................................................###...
........................*.....................................###................................................
.......................###................................................................*.....EEEEEEEEE........
....................######........................................................####...###...##########........
............*.......########............................###......................*####.........##########........
...........####..###########..........................#####.....................######.........##############....
.P......#######..###########...........EEEEEEE.*.G....#####........EEEEEE*EEEEEE######.........##############...G
#################################...##################################################.........##################
```

Columns 113–173 (section C):

```
       2         3         4         5         6         7   
.............................................................
.............................................................
.............................................................
.............................................................
.............................................................
...................EEEEE.......*.............................
................########.....####.......................*....
.............*..########.....####......................###...
............############.....######..........................
.......EEEEE############.....######..........*...............
....####################.....########.......##......##.......
....####################.....########.EEEEEE##EEEEEE##.....D.
########################.....################################
```

What changed from the brief, and why:
- **The first trench is 3 tiles wide, not 2.** The body is 2 tiles wide, so it walks
  across a 2-tile pit without falling in (section 4). The converter now warns about pits
  narrower than 3.
- **A4 kept:** the alcove above the rack stair needs a held jump of 3 tiles. It is bonus.
- **C3 as proposed:** two low crates (2-tile steps) between the final bots instead of
  shelves to escape onto. A fragment sits on the first one.
- **C1, bots on alternating steps:** each bot step is 8 tiles wide, and its bot patrols
  only the right 5, so there are 3 safe tiles to land on before jumping over it.
- **B3, the bot's shelf:** 10 tiles wide. At 7, the bot's whole patrol (80 frames) was
  barely longer than a full jump.
- **B1, the split:** two steps up to a plateau where the routes part. The high route
  climbs rack tops at rows 6, 3 and 3 (2 fragments). The low route drops into a floor
  lane with 2 bots and 2 fragments. Either route plus the middle shelf opens gate B
  (3 of 6). The low route's bots only patrol under the row-3 rack tops or open sky:
  under a lower shelf a bot can't be jumped (section 4). The first rack top leaves 3
  open columns next to the plateau, so the player can drop into the lane.
- **Width: 174 tiles**, over the 120–160 target. Sections: A 50, B 63, C 61. The owner
  pass decides where to trim (B's low lane and C's final run have the most slack).

Fragments: A 4 (gate 3 of 4), B 6 (gate 3 of 6), C 4 (bonus): 14, one more than the brief,
so each route of B has 2. 8 bots. An expert run takes about 25 seconds; the replay's
worst frame ends at line 123 of the 140 allowed.

Questions for the owner pass:
- Is the high route in B worth its climb (3-tile jumps, above the level 1 limit of 2)?
- Are the bot steps in C too hard for a first level?
- Where to cut 14+ tiles.

## 7. Levels 2–5

Briefed one at a time, after level 1 is playable. Themes and mechanics are in
GAME_DESIGN.md and [LEVEL_PIPELINE.md](LEVEL_PIPELINE.md) section 5.

### Level 2 brief: Code Testing Lab (draft)

**Status: draft by Claude, 2026-10-04, for the owner to react to.** Nothing is built.
Level 1's greybox pass comes first.

Teaches: **one-way platforms**: jump up through them from below, land on top, walk under
them. They also fix level 1's problem with split routes: a high route of one-way shelves
can sit above a low route without blocking it. Bots are faster than in level 1.

Proposed limits for level 2 (one tile more than level 1): climbs up to 3 tiles, gaps up
to 6. Every one-way shelf is 3–4 tiles above the surface you jump from, so a held jump
reaches it.

**Section A, "Test bench"** (about 45 tiles)
1. Flat start. A one-way shelf 3 tiles up with fragment 1: jump through it from below.
   Floor everywhere under it, so missing costs nothing.
2. Three one-way shelves stacked 3 tiles apart, like a bookcase. Fragment 2 on the top.
3. A Gap-4 pit with a one-way shelf over its middle: the first time the shelf is the
   only way across.
4. A bot on the floor under a long one-way shelf: hop up and let it pass beneath. Shelves
   as safe spots is the section's last lesson. Fragment 3 on the shelf, bonus fragment 4
   high above the pit (a held jump from the top of the bookcase).
5. **Gate A: 3 of 4.**

**Section B, "Unit tests"** (about 50 tiles)
1. A zig-zag climb: one-way shelves alternating left and right up to row 4, a fragment
   halfway.
2. The split, done right: a floor route with 2 fast bots and a fragment, and above it a
   route of one-way shelves with 2 fragments and gaps of 3–5.
3. The routes meet at a long one-way bridge over a Gap-8 pit, with 2-tile holes in it.
4. A bonus alcove above the high route.
5. **Gate B: 3 of 5.**

**Section C, "Regression suite"** (about 50 tiles)
1. Three tiers of one-way shelves with fast bots on two of them: pick the tier whose bot
   is away.
2. The longest jump: a Gap-6 from the end of a one-way shelf.
3. A last bonus fragment off the path, then the exit elevator on the top tier.

**Totals:** about 145 tiles. 13 fragments (A 4, B 5, C 4): 6 needed for the gates, 7 bonus.
About 9 bots, at most 3 on screen.

**Questions for the owner:**
- **Down + A to drop through a one-way platform?** Many platformers have it, and it lets
  a level send the player back down. Without it, descending needs a gap in the shelf or
  its end. It changes the physics (`player.c` and `tools/physics.py` together).
- **Faster bots:** 1.5 px per frame (the player's run speed, so they can't be outrun, only
  jumped), or a speed property per bot in Tiled?
- The collectible is called a bug report in GAME_DESIGN.md: only a new picture, or also a
  new name in the HUD?

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
| `F` | Fast enemy: like `E`, at 2 px per frame instead of 1 (the bot's `speed` property in Tiled) |
| `L` | Lift (from level 3): a block of `L`s 3 wide, from the row of its highest place to its lowest |
| `C` | Chaser (from level 3): like `E`; the run is the range it may walk in (`kind` = `chaser` in Tiled) |
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
| Drop through | Down + A on a one-way top falls through it (from level 2). With anything solid under the feet, Down + A jumps as usual |

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

**Status: section plan accepted on 2026-10-02. Greybox built on 2026-10-04**
(`levels/level_1.tmj`, below). Owner pass on DMG and GBC the same day: "it is good",
kept as is. Next: the art pass (M5).

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

Owner pass (2026-10-04, after playing it on hardware): keep the high route's 3-tile jumps
(it's optional), keep the bot steps in C, keep the 174 tiles.

## 7. Levels 2–5

Briefed one at a time, after level 1 is playable. Themes and mechanics are in
GAME_DESIGN.md and [LEVEL_PIPELINE.md](LEVEL_PIPELINE.md) section 5.

### Level 2 brief: Code Testing Lab (draft)

**Status: draft by Claude, 2026-10-04. Greybox built the same day** (`levels/level_2.tmj`,
below), after the owner's answers. Owner's hardware pass, 2026-10-04: keep it as built.

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

**Owner's answers (2026-10-04):**
- **Down + A drops through a one-way platform.** A physics change in `player.c` and
  `tools/physics.py` together, before the level 2 greybox.
- **Bots get a speed property per bot in Tiled** (1 or 2 px per frame), so a level can mix
  slow and fast bots.
- The collectible stays the floppy in every level, matching the HUD icon (owner,
  2026-10-11).

### Level 3 brief: AI Research Wing (draft)

**Status: draft by Claude, 2026-10-04. Greybox built the same day** (`levels/level_3.tmj`,
below), after the owner's answers. Lifts and chasers are tested in `levels/test_movers.tmj`.
The owner played it end to end (2026-10-11): kept as built. Art: AI Research Wing
(2026-10-11).

Teaches: **lifts** (vertical movers) and **chasers** (bots that follow the player). Lifts
reach heights no jump can: the level goes up, with shelves 5-9 tiles above the floor.
Limits on fixed ground as in level 2 (climbs up to 3, gaps up to 6).

**Section A, "Elevator lobby"** (about 50 tiles)
1. A lift next to a high shelf with fragment 1, over solid floor: missing costs nothing.
2. Two lifts in a row, out of step, with a shelf between them: wait for the second.
3. A lift in the middle of a wide pit (gaps of 4 to it): hop on when it's high, off at
   the other side.
4. A bonus fragment on a shelf only the highest point of a lift reaches.
5. **Gate A: 3 of 5.**

**Section B, "Neural lab"** (about 55 tiles)
1. The first chaser on an open floor: it walks toward the player once the player is near,
   slower than running. A shelf to escape to.
2. A chaser under a lift: ride up and let it wait below.
3. A high route of lifts and shelves, and a low route past two chasers.
4. **Gate B: 3 of 5.**

**Section C, "Training core"** (about 55 tiles)
1. Lifts over chasers: time the drop.
2. A tall shaft of three lifts to the top of the level.
3. The exit elevator at the top.

**Totals:** about 160 tiles, 13 fragments, 3-4 chasers, 3-4 patrol bots, 8-10 lifts.

**Owner's answers (2026-10-04):**
- **Chaser:** stands still until the player is within about 6 tiles on its floor, then
  walks toward them at 1 px per frame, only inside its range in Tiled.
- **Lifts:** wait about half a second at each end (built). Vertical only for now.

### Level 3 greybox (2026-10-04)

`levels/level_3.tmj`, 170 tiles wide (`L` lifts, `C` chasers' ranges). `make preview`
reaches every surface and all 13 fragments; `tests/test_level_3.py` plays it from the start
to the exit without losing a life, collecting all 13.

Columns 0-83 (section A and the start of B):

```
0         1         2         3         4         5         6         7         8   
....................................*...............................................
..............................LLL======.............................................
..............................LLL..............*....................................
..............*...............LLL...................................................
........LLL=======....LLL====.LLL...................................................
........LLL...........LLL.....LLL...................................................
........LLL...........LLL.....LLL.............LLL...................................
........LLL...........LLL.....................LLL.......................*...........
........LLL...........LLL.....................LLL.....................======........
........LLL...........LLL.....................LLL...................................
........LLL...........LLL.....................LLL...................................
.P...........EEEEE.*...................................*.G........CCCCCCCCCCCCCC....
##########################################...........###############################
```

Columns 84-169 (the rest of B, and C):

```
      9         0         1         2         3         4         5         6
.............................................................*........................
.....*................................................................................
...........*.....................................LLL======............................
....LLL=========...........................*.....LLL........LLL.......................
....LLL.............................LLL========..LLL........LLL.....*..EEEEEEEE...D...
....LLL.............................LLL..........LLL........LLL...####################
....LLL.............................LLL..........LLL........LLL...####################
....LLL.............................LLL..........LLL........LLL...####################
....LLL.............................LLL..........LLL........LLL...####################
....LLL.............................LLL..........LLL........LLL...####################
............................................................LLL...####################
CCCCCCCCCCCCCC......*...*...G...........CCCCCCCCCCCCC.............####################
######################################################################################
```

What changed from the brief, and why:
- **Two patrol bots** (the brief had 3-4): one on the floor under the first high shelf,
  clear of L1 where the player waits; one on the top block before the exit. Lifts and
  chasers are this level's lessons, and every one of them is an actor too (at most 8 in
  play, 3 sprites per lift).
- **A1, the first lifts wait at the bottom 2 tiles above the floor:** stand under one and
  jump straight up while it waits; the shelves they reach are 7 rows up, out of a jump's reach.
- **A2, between L2 and L3 the shelf is at L2's top;** L3 waits 2 rows lower, so the player
  steps off the shelf onto it, then rides up to the high shelf (fragment 3).
- **A3, the pit is 11 tiles wide** with L4 in the middle (gaps of 4); fragment 4 hangs over
  L4's highest place, so riding it collects it.
- **B1, chaser 1's shelf is 4 tiles up:** from it, a running jump clears the chaser (it
  waits at its range's right end, as it can't see the player on the shelf).
- **B2, L5 starts 3 tiles up,** so a player boarding it is never in chaser 2's sight; its
  high shelf ends past chaser 2's range.
- **C, three lifts:** L6 to a shelf over chaser 3, L7 higher, and L8 up the shaft beside
  the top block (fragment 12 at its highest place). The block's top is 8 rows below the
  screen's top, so its bot can be jumped; the exit is past the bot.
- **Fragments:** A 5 (gate 3 of 5), B 5 (gate 3 of 5), C 3 (bonus): 13. **Actors:** 8 lifts,
  3 chasers, 2 bots; at most 4 in play at once. The replay's worst frame ends at line 131
  of the 140 allowed (average 91; `tools/frame_budget.py --route level3`): lifts cost about
  10 lines on average.

### Level 2 greybox (2026-10-04)

`levels/level_2.tmj`, 170 tiles wide (sketch below: `tools/sketch2tmj.py --from-tmj
levels/level_2.tmj`; `F` is a fast bot). `make preview` reaches every surface and all 13
fragments. `tests/test_level_2.py` plays it from the start to the exit without losing a
life, once through section B's high route (9 fragments) and once through its low route (8).

Columns 0–123 (sections A and B):

```
0         1         2         3         4         5         6         7         8         9         0         1         2
...............*............................................................................................................
............................................................................................................................
.............................................................................*..............*................*..............
..............====..................*............................====.......................................................
.......................................................................*...====............====.............................
...................................................................................====.....................................
.........*....====....................................................====..................................................
...................................................*........................................................................
............................................................................................................................
.......====...====....#====#..................==========.........====..............................===...===...===...===....
......................#....#.......====.....................................................................................
.P....................#.*..#.................EEEEEEEEEEEEEE..G............FFFFFFFFF..*..FFFFFFFFFF.........................G
###############################...........############################################################..............########
```

Columns 124–169 (section C):

```
      3         4         5         6
..............................................
..............................................
................*.............................
..........FFFFFFFFFFF.....................D...
........=====================......###########
...................................###########
......................*............###########
..............FFFFFFFFFF..*........###########
....=====================..........###########
...................................###########
...................................###########
........FFFFFFFFFFF................###########
#########################..........###########
```

What changed from the brief, and why:
- **A3, the room under a lid (new):** a fragment on the floor between two 3-tile walls with
  a one-way lid between their tops. The only way in is Down + A on the lid; the way out is
  a jump up through it. It teaches the drop where missing costs nothing. Section A has 5
  fragments, gate A asks for 3.
- **A4, the pit is 11 tiles wide, not Gap-4,** with the one-way shelf in its middle (gaps of
  4 and 3 to it). A Gap-4 can be jumped without the shelf; a running jump clears up to 10
  with coyote time (section 4). The bonus fragment sits 7 rows above that shelf: a held
  jump from it, not from the bookcase.
- **A2, the bookcase's fragment is at row 0,** 3 rows above the top shelf. The body is 4
  tiles tall: a fragment just above the top shelf is touched from the middle one.
- **B1, the zig-zag:** 3 shelves to row 3, a fragment on the middle one. The low route drops
  back to the floor through it.
- **B3, the bridge's holes are 3 tiles wide,** not 2: the body walks across 2-tile holes.
  Its bonus fragment hangs over the second hole (a held jump across it).
- **B quota:** the high route has 4 fragments (zig-zag, 2 on the shelves, the bridge's
  bonus), the low route 3 (zig-zag, 1 between the bots, the bridge's bonus). Gate B asks
  for 3, so the low route needs the bridge's fragment. Owner, 2026-10-04: keep it so.
- **C1, the tiers are 4 rows apart** (floor, rows 8 and 4 of the map), not 3: with 3, a bot
  on the tier above touches the head of a player standing on the tier below. Going up a
  tier takes the full held jump (4 tiles), as the brief's 3–4 allows.
- **C, the floor bot starts 9 tiles past gate B:** the first draft put the checkpoint next
  to a fast bot.
- **C2, the Gap-6 is from tier 3's end onto the exit block.** C has 3 bonus fragments, not 4.
- **Bots: 6** (1 slow in A, 5 fast in B and C), fewer than the brief's 9; at most 3 on
  screen. **Width: 170 tiles** (A 62, B 62, C 46), over the brief's 145, like level 1.

Fragments: A 5 (gate 3 of 5), B 5 (gate 3 of 5), C 3 (bonus): 13. The replay's worst frame
ends at line 115 of the 140 allowed (average 61; `tools/frame_budget.py --route level2`).

### Level 4 brief: Network Hub (draft)

**Status: draft by Claude, 2026-10-05, built the same day** (`levels/level_4.tmj`, below)
with Claude's answers to its open points. The owner played it end to end (2026-10-11):
kept as built, the answers below included. Art: Network Hub (2026-10-11).

Teaches: **hazard tiles** ("corrupted data"): never solid, and touching one with any part
of the body costs a life, like a bot. Three shapes: **spike strips** on a floor (jump
over), **spike pits** (a hazard floor, crossed on shelves or a lift), and **hanging
hazards** from the top of the screen (walk under them; a full jump under one hits it, a
low hop doesn't). Earlier mechanics come back: one-way shelves, bots, a chaser, a lift.
Limits as in levels 2 and 3 (climbs up to 3, gaps up to 6); no jump near the maximum.

**Section A, "Patch panel"** (59 tiles)
1. A 1-tile spike strip on the floor, a fragment over it: the first hazard, with room
   before and after.
2. A 3-tile strip.
3. A 2-tile step with a fragment, spikes right after it: jump off the step over them.
4. A spike pit 10 wide with a one-way shelf in its middle (fragment), a bonus fragment
   high above the shelf.
5. A slow bot with a fragment over it. **Gate A: 3 of 5.**

**Section B, "Router farm"** (66 tiles)
1. Two hanging hazards with a 1-tile strip between them: a low hop over the strip; a
   full jump under a hanger costs a life. Fragment over the strip.
2. The packet bridge: 4 one-way shelves over a 28-tile spike floor (gaps of 3), a
   fragment on shelf 2 with a bonus fragment high above it, one on shelf 4.
3. A fast bot with a fragment over it. **Gate B: 3 of 5.**

**Section C, "Backbone"** (55 tiles)
1. A chaser under a one-way shelf (fragment), a 2-tile strip just past its range: from
   the shelf, one jump clears the chaser and the spikes.
2. A lift in a 12-tile spike pit up to the top block; a fragment at its highest place.
3. A last strip on the block with a fragment over it, then the exit elevator.

**Open points, answered by Claude for the greybox (owner: keep or change):**
- **Static hazards only:** no timed packet lasers or turrets (RESEARCH.md section 3). They
  would be a new actor; the static shapes already make the level.
- **Song: "Whirlwind"** (Tronimal, Game Boy Music Pack #2), not heard yet; any unused
  Tronimal song from the packs fits (`make soundtest` renders them all).

### Level 4 greybox (2026-10-05)

`levels/level_4.tmj`, 180 tiles wide (A 59, B 66, C 55; like levels 1-3, over the 160
target). `make preview` reaches every surface and all 13 fragments; `tests/test_level_4.py`
plays it from the start to the exit without losing a life, collecting all 13.

Columns 0-89 (section A and most of B):

```
0         1         2         3         4         5         6         7         8
................................................................^^^.......^^^.............
................................................................^^^.......^^^.............
..........................................*.....................^^^.......^^^.............
................................................................^^^.......^^^.............
................................................................^^^.......^^^.............
..........................................................................................
.....................................................*....................................
..........*...............................*...........................*.................==
..........................................................................................
............................*............===....................................=====.....
.........................######...........................................................
.P........^.......^^^....######^^^.................EEEEE..G...........^...................
#####################################^^^^^^^^^^#################################^^^^^^^^^^
```

Columns 90-179 (the end of B, and C):

```
9         0         1         2         3         4         5         6         7
*.....................................................................*...................
..........................................................................................
..................................................................................*.......
.....................................................................LLL..................
.....................................................................LLL..................
*....................................................................LLL..........^....D..
................*...........*....................*...................LLL.....#############
===..................................................................LLL.....#############
..............=====............................=====.................LLL.....#############
......=====..........................................................LLL.....#############
.....................................................................LLL.....#############
........................FFFFFFFF..G.......CCCCCCCCCCCCC.^^...................#############
^^^^^^^^^^^^^^^^^^###############################################^^^^^^^^^^^^#############
```

What changed while building it, and why:
- **A2 moved 4 tiles right:** with 4 tiles between the first two strips, the landing
  after the first left 2 tiles for a 2-tile body.
- **A4's pit is 10 wide and A5's bot starts 4 tiles after it:** the jump off the shelf
  landed next to a bot that patrolled up to the pit's edge.
- **B3's bot starts 6 tiles after the bridge:** a fast bot reached the bridge's landing.
- **C1's shelf ends 3 tiles before the spikes:** from 5, clearing the chaser and the
  spikes took a jump near the maximum.

Fragments: A 5 (gate 3 of 5), B 5 (gate 3 of 5), C 3 (bonus): 13. **Actors:** 2 bots
(1 fast), 1 chaser, 1 lift. The replay's worst frame ends at line 107 of the 140 allowed
(average 69; `tools/frame_budget.py --route level4`).

### Level 5 design: Mainframe, the KERNEL boss

**Status: by Claude, 2026-10-05, from the concept in GAME_DESIGN.md and RESEARCH.md
section 3; approved by the owner the same day, with every proposal below (the answers to
the questions at the end). Built the same day: the build notes below say what changed.**

**The fight in one line:** KERNEL can't be hit, so the player uploads the patch at three
kill-switch terminals that light up one at a time, while KERNEL attacks. Each phase adds
one earlier level's idea: packets (hazards that fall), then a lift, then a floor that
corrupts.

**Arena** (one screen, 20 tiles wide; legend as in section 1, plus `K` KERNEL's face,
background only, and `1` `2` `3` the terminals in the order they light up):

```
          1
01234567890123456789
......KKKKKKK.......
......KKKKKKK.......
......KKKKKKK.......
......KKKKKKK.......
......KKKKKKK.....2.
......KKKKKKKLLL####
.............LLL....
.............LLL....
...====......LLL....
.............LLL....
....................
.1...P...3..........
####################
```

- **The face**, 7×6 background tiles at the top middle. It doesn't block or hurt; the
  player and the packets are drawn over it. Its eyes change before every attack (the
  wind-up), and it looks more corrupted in each phase (a few tile swaps).
- **Terminal 1** on the floor at the left, **terminal 2** on the high shelf at the right,
  reached only by the lift, **terminal 3** in the middle of the floor. A terminal is one
  tile; the lit one has a different shape (it reads on DMG) and blinks.
- **The lift** (level 3's) appears in phase 2 and stays; it waits at its bottom 2 tiles
  above the floor, like level 3's first lifts.
- **The one-way shelf** at the left is the safe place in phase 3.
- No camera movement and no column streaming: the arena is drawn once.
- Checked with `tools/levelpreview.py` on a stand-in map (fragments for the terminals):
  everything can be reached, and terminal 2 only with the lift.

**Uploading:** stand on the lit terminal and hold Up for 32 frames (about half a second).
Its screen fills in 4 steps, with a beep each. Letting go of Up, jumping or getting hit
starts it over. The HUD's counter shows the uploads, `00/03`, in place of the fragments.

**Flow:**

| Step | What happens |
|---|---|
| Intro | Log 5 is typed under the face: `L0G 5: WH0 WR0T3 M3` / `...#042? Y0U?`. After 2 s it clears, terminal 1 lights, and the first attack comes 1 s later. |
| Phase 1 (terminal 1) | Packet rain: 2 packets per volley. |
| Hit 1 | 8 frames of hit-stop, 16 frames of screen shake, 2 slow flashes. 1 s without attacks; the lift appears; terminal 2 lights. |
| Phase 2 (terminal 2) | Packet rain with 3 packets per volley. Ride the lift to the shelf. |
| Hit 2 | Same feedback; terminal 3 lights. |
| Phase 3 (terminal 3) | Packet rain with 2 per volley, and the floor corrupts in turns. |
| Hit 3 | Hit-stop, 3 flashes, the face breaks up, the music stops, fade out: the ending (M7: "LOCKDOWN LIFTED", the credits). |

**Attacks** (fixed pattern tables, no randomness, so the replay test can win every time):

| Attack | Wind-up | Attack | Recovery |
|---|---|---|---|
| **Packet rain** (all phases) | The face's eyes change, and a marker blinks at the top of each chosen column for 30 frames. One column is the player's at the start of the warning. | A 16×16 packet falls at 2 px per frame from the top to the floor (about 48 frames) and vanishes there. Touching it costs a life. Packets of one volley start 8 frames apart. | 60 frames to the next volley. |
| **Corrupted floor** (phase 3) | One 4-tile floor segment flickers for 30 frames, still solid. | It turns into hazard tiles for 90 frames. | Back to floor; 30 frames later the next segment from the table. Terminal 3's segment is in the table too, so the upload waits for a safe moment. |

**Lives:** losing one fades out, puts the player back at the start and restarts the current
phase (uploads done stay done, packets and the floor reset). **Game over** with CONTINUE
restarts the fight from phase 1, as a section restarts with its fragments put back.
The password after level 4 already opens level 5.

**Hardware budget:**
- **Sprites:** the player 4, at most 3 packets (2 each), the lift 3: at most 13 of 40.
  Per screen line: a volley's packets start 8 frames apart at 2 px per frame, so they
  are 16 px apart and never share a line: the player 2 + a packet 2 + the lift 3 = 7 of
  10. (KERNEL as sprites would need 42, more than the 40 there are: hence background.)
- **Frame budget:** no streaming saves the camera's 21 lines; the boss update (one
  volley's actors, one floor segment's tiles) should stay under about 15 lines, so the
  worst update should end well under level 3's 131.
- **Bank 0:** the boss code goes to a ROM bank; bank 0 gets only the call from the game
  loop (about 30 bytes).
- **Tiles:** about 80 background tiles (face 42 plus a few swaps per phase, terminals,
  corrupted floor, marker, the greybox and band tiles) of about 190 free; they don't fit
  in the 64 slots before the HUD frame, so the face goes after it (a memory map change).
  The packet's 4 sprite tiles.
- **Screen shake** is horizontal only (`SCX`); a vertical one would move the band and HUD
  splits. The map columns just outside the arena are drawn too, so a shake shows no
  garbage at the edges.

**What it reuses and what's new:**
- Reused: the lift actor, hazard tiles (the corrupted floor is the same as level 4's
  spikes), the HUD counter, `palette_flash`, the fades, the story's typed text, the
  `boss` map property in `tools/tiled2c.py` (no gates needed).
- New: `src/game_state/kernel.c` (phases, terminals, uploads, attack tables, the floor),
  a packet actor, Up in the input, the face tiles and the boss song ("Terminate",
  owner to listen), the ending and credits screens (M7).
- Tests: a replay that wins without losing a life; every attack's warning lasts at least
  24 frames; releasing Up resets an upload; a lost life keeps the uploads; game over
  restarts phase 1; the frame budget and sprites per line.

**Left out to keep the boss small** (RESEARCH.md had them):
- **Spawned level 1 bots** and the **purge sweep:** two more attack types to build,
  tune and test. The packets and the floor already use the hazard lessons.
- **A screen split that moves only the face:** the whole screen shakes instead, with the
  same `SCX` the camera uses; no new line interrupt.

**Questions for the owner** (2026-10-05: all as proposed):
1. Is the fight right at 3 uploads, one per phase, with the hit feedback above?
2. Hold Up for half a second at a terminal, or a single press?
3. Cut the spawned bots and the purge sweep, as proposed?
4. Shake the whole screen instead of moving only the face?
5. Game over restarts the fight from phase 1: or keep the uploads done?
6. The song: "Terminate" (Tronimal, pack 1), after listening (`make soundtest`).

### Level 5 build notes (2026-10-05)

Built from the design above: `levels/level_5.tmj` (the arena's floor and shelves, map
property `boss`), `src/game_state/kernel.c` (the face, the terminals, the lift, the volley
and floor tables), `src/actor/packet.c`. `tests/test_boss.py` beats it without losing a
life (and checks the warnings, the uploads and the lives); the worst frame ends at line
111 of 140. What changed while building it, and why:
- **A terminal lights 2.5 s into its phase,** once the first volley has fallen, not right
  away: with the terminal lit at once, phase 1 was over before KERNEL had attacked.
- **Phase 2's extra packets fall left of the lift** (lanes 0-5): with lanes 6 and 7 in the
  table, packets landed on the lift almost every volley, and boarding it was luck. Over
  the lift and the shelf only the packet aimed at the player falls.
- **A waiting packet blinks over the band,** above the highest head (the shelf and the
  lift's top), so it can't hurt before it falls; it falls about 48 frames to the floor.
- **The map columns right of the arena are drawn** (empty, with floor), so the shake shows
  no old tiles at the edges.

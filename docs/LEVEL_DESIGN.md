# Level Design

How we design levels together, the rules every level follows, and the level briefs.
Tools and file formats are in [LEVEL_PIPELINE.md](LEVEL_PIPELINE.md).

## 1. How we work

Agreed on 2026-10-02: **Claude drafts, the owner reacts.**

| Step | Who | Output |
|---|---|---|
| 1. Brief | Claude proposes, owner edits in chat | Section-by-section plan, stored in this file |
| 2. Greybox | Claude | Text sketch in chat, then a Tiled map (`levels/level_N.tmj`) using the plain block tileset |
| 3. Preview and checks | Claude | `build/levels/level_N.png`: the whole level with column numbers, enemies, fragments and jump arcs. Plus a reachability report: can every fragment, gate and the exit be reached with the real jump? |
| 4. Owner pass | Owner | Edits in Tiled, or notes by column number. Plays it in Emulicious. |
| 5. Tune | Claude | Applies the notes. A PyBoy replay test proves the level can be finished. |
| 6. Art pass | Claude | Swaps the block tileset for the real one (roadmap M5). |

**Feedback format:** refer to tile columns. Both the preview and Tiled's status bar show
them. Examples: "col 42: gap too wide", "section B drags", "the bot at col 88 is unfair".

**Text sketch legend** (used for quick drafts in chat):

| Char | Meaning |
|---|---|
| `.` | Air |
| `#` | Solid |
| `=` | One-way platform (from level 2) |
| `^` | Hazard (from level 4) |
| `*` | Code fragment |
| `E` | Enemy (patrol range set in Tiled) |
| `P` | Player start |
| `G` | Security gate (section end and checkpoint) |
| `D` | Exit elevator |

## 2. Level structure

- **3 sections** (A, B, C), each 40–55 tiles wide. A level is 120–160 tiles in total and
  takes 4–6 minutes on a first play.
- **A security gate closes each of sections A and B.** The gate opens when the player has
  collected the section's **quota** of code fragments, for example 3 of 5. Extra
  fragments are a bonus: score, and a 100% mark on the level-select screen.
- **An opened gate is a checkpoint.** Losing a life restarts at the last opened gate.
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
  section, for example `◆ 2/3`.

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
and collision is read from ROM.

## 4. Jump metrics

To be measured after the physics rewrite (roadmap M2). All layouts are built on these numbers.

| Metric | Value |
|---|---|
| Max jump height (A held) | TBD tiles |
| Min jump height (A tapped) | TBD tiles |
| Max gap, standing jump | TBD tiles |
| Max gap, running jump | TBD tiles |
| Safe landing width | At least 3 tiles |

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

**Status: section plan accepted on 2026-10-02.** Exact gap sizes and heights wait for
the jump metrics from M2. The greybox comes next, in M3.

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

## 7. Levels 2–5

Briefed one at a time, after level 1 is playable. Themes and mechanics are in
GAME_DESIGN.md and [LEVEL_PIPELINE.md](LEVEL_PIPELINE.md) section 5.

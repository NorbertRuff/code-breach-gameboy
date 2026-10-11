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
  all CC BY 4.0 ([CREDITS.md](CREDITS.md)).

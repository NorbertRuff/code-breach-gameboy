# ApexLab: Code Breach - Game Design Document

What the game *is*. How it's built: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md); what's
done and open: [docs/ROADMAP.md](docs/ROADMAP.md); why: [docs/DECISIONS.md](docs/DECISIONS.md).

## Game Concept
**ApexLab: Code Breach** is a Game Boy platformer where you play as a programmer at ApexLab who must escape when the AI security systems go haywire. The lab's protocols are corrupted, and you need to collect code fragments and get out while avoiding malfunctioning security bots.

## Vision
A **cozy little game**: it looks good, its logic is solid, it runs on a real Game Boy and Game Boy Color, and its owner can explain how every part works. It started as a proof of concept for learning C on old hardware; the revival (2026-10) turns it into a finished game.

What that means in practice:
- **Forgiving rules.** 3 lives; losing one returns you to the last opened gate with your fragments kept. Game over refills your lives and restarts the current section, not the level.
- **Readable, original visuals.** A 16×32 multi-sprite programmer, a parallax band at the top of the screen, fades between screens. No copied art.
- **Explainable code.** Decisions and the sources of ideas are recorded as we go (docs/DECISIONS.md, docs/SOURCES.md); a full "how it works" document is written at the end.

---

## Story & Setting
- **Setting**: ApexLab programming facility
- **Protagonist**: ApexLab programmer, staff #042 (16×32 sprite, lab coat + glasses)
- **Conflict**: AI security breach — corrupted security bots are patrolling the facility
- **Goal**: Each level has 3 sections. Collect enough code fragments in a section to open its security gate, then reach the elevator at the end. Extra fragments are bonus. The camera locks behind an opened gate. Details in [docs/LEVEL_DESIGN.md](docs/LEVEL_DESIGN.md).
- **Theme**: Cyberpunk tech lab, programming, AI gone wrong
- **Story through the fragments ("PATCH n/5")**: each level's fragments compile into one part of a patch. In the final fight the player uploads the patch into the Mainframe AI, KERNEL.
- **Text, kept minimal** (all of it in [docs/STORY.md](docs/STORY.md)):
  - Boot screen: three terminal lines, "ANOMALY / STAFF #042 / LOCKDOWN".
  - Each elevator ride: one line from the AI's log, more corrupted on every floor.
  - Ending: "LOCKDOWN LIFTED", then the credits.

---

## The Five Levels

One new idea per level; earlier ones come back. Briefs, layouts and build notes are in
[docs/LEVEL_DESIGN.md](docs/LEVEL_DESIGN.md) section 7.

| # | Level | New mechanic | Enemies | Look |
|---|---|---|---|---|
| 1 | Server Room | Run, jump, fragments, gates, pits | Patrol security bots | Green machine blocks, racks, wiring |
| 2 | Code Testing Lab | One-way platforms (Down + A drops through) | Faster bots | Teal panels, desks, monitors |
| 3 | AI Research Wing | Lifts and chasers | Chasers, bots | Purple lab plates, specimen tanks |
| 4 | Network Hub | Hazard tiles (spikes, cut cables) | Bots, a chaser | Tan floor, patch panels, routers |
| 5 | Mainframe | Boss fight: KERNEL | KERNEL's packets | Lavender steel, tape-reel mainframes |

The collectible is a floppy disk (a code fragment) in every level, matching the HUD icon.

**KERNEL** (level 5): a monitor face of 7×6 background tiles on one screen. The player has
no attack: three terminals light up in turn, and holding Up on the lit one uploads part of
the patch. Each upload freezes and shakes the screen and glitches the face. Phase 1 drops
packets with a column warning, phase 2 adds a lift, phase 3 corrupts floor segments.

---

## Rules and Game Flow
- **Lives:** 3. A bot, a chaser, a packet or a hazard tile costs a life unless you're
  invincible after a respawn; falling out of the level always does.
- **Sections and gates:** walk into a gate with its quota and it opens; it becomes the
  checkpoint. The HUD shows the lives and the section's fragments against its quota.
- **Level complete:** the fragments ("100%" with all of them) and a password; then the
  elevator ride with its log line.
- **Passwords** instead of saves (the owner's flashcart doesn't keep cartridge RAM):
  CONTINUE on the title screen takes a password and opens the level select.
- **Game over:** CONTINUE restarts the section with full lives, QUIT goes to the title.
- **Pause:** START. Testing cheats while paused: A + SELECT skips to the next level,
  A + B turns no-death on or off (they stay in the release).

---

## Visual Style
- **Aesthetic**: Cyberpunk tech lab — circuits, server racks, lab equipment, neural displays.
- **Colors**: a GBC palette set per level from the Claude Design style guide, with the 4 grays on the original Game Boy. Every palette is ordered light → dark so the DMG version reads correctly ([docs/CGB_PLAN.md](docs/CGB_PLAN.md), [docs/ART_PIPELINE.md](docs/ART_PIPELINE.md)).
- **Art**: redrawn by hand on the 8×8 grid from Claude Design / Gemini references; faces traced and finished by the owner. Origins in [docs/SOURCES.md](docs/SOURCES.md).

## Audio
Music from Tronimal's Game Boy Music Packs #1 and #2 and sound effects from GBFX/GBFX2
(CC BY 4.0), played by hUGEDriver and our own effect player; credits in
[CREDITS.md](CREDITS.md) and on the in-game credits screen.

---

*Last updated: 2026-10-11*

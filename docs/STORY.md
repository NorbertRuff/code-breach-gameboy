# Story Text

**Status: written by Claude 2026-10-04, approved by the owner 2026-10-05.** Roadmap M7,
story text. The owner keeps the twist (staff #042 wrote the AI).
The concept is in GAME_DESIGN.md (Story & Setting). All of it is in the game since
2026-10-05 (`src/game_state/story.c`, `game_over.c`; log 5 is typed under KERNEL's face as
the boss level starts).

A Game Boy text screen is 20 characters wide. Every line below fits.

## Boot screen (from GAME_DESIGN.md)

```
ANOMALY
STAFF #042
LOCKDOWN
```

## Level complete

The game shows `LEVEL COMPLETE` and the fragment count since 2026-10-04 (`100%` when all
were found). The patch line would join them, before the elevator ride:

```
PATCH 1/5 COMPILED
FRAGMENTS 11/14
```

All of a level's fragments add a mark: `100%`.

## Elevator rides: the AI's log

One entry per ride, a little more corrupted each floor. The player first suspects the AI
is hunting them; the last line hints that staff #042 wrote it, which sets up the patch.

| After | Log line (2 × 20 characters) |
|---|---|
| Level 1, Server Room | `LOG 1: STAFF #042` / `NOT FOUND IN RACKS.` |
| Level 2, Code Testing Lab | `LOG 2: ALL TESTS` / `PA55ED. ALL T35TS P` |
| Level 3, AI Research Wing | `LOG 3: I AM LEARN1NG` / `1 AM L3ARN1NG 1 AM` |
| Level 4, Network Hub | `L0G 4: TH3 N3TW0RK` / `1S M1N3. 4LL 0F 1T.` |
| Before the boss | `L0G 5: WH0 WR0T3 M3` / `...#042? Y0U?` |

## Ending (from GAME_DESIGN.md)

The elevator goes up and the palette fades to light:

```
LOCKDOWN LIFTED
```

## Corruption

Digits stand in for letters (as above): the font already has them, so no extra tiles.

---
name: fix-bug
description: Fix a bug in the ApexLab Game Boy game test-first, and prove the test catches it on the old build. Use when fixing anything broken in src/, when picking up a bug from the Bugs list in docs/ROADMAP.md, or when the owner reports a problem seen on hardware or in Emulicious.
---

# Fix a bug, test first

Conventions this follows are in `docs/TESTING.md`. The owner runs git; don't commit.

1. **Record it.** Find the bug in the Bugs list of `docs/ROADMAP.md`. If it's new, add it
   with the next number (bugs 1-22 are in `docs/archive/PROJECT_STATE.md`).
2. **Write the test first** in `tests/test_regressions.py`, under a `# Bug N ---` header,
   using the `Game` helpers in `tests/gamelib.py`.
   - Drive the game with input (`game.hold`, `game.press`). Write game state
     (`set_state`, `set_lives`, `set_player_y`) only when input can't reach the situation.
   - Fades, flashes and palettes work on both systems (`src/engine/palette.c`): check them
     with `game.background_is_black()`, `game.sprite_palettes()` and `game.gbc_colors()`.
     A test where DMG and GBC can differ gets `@pytest.mark.both_modes`; a bug on one
     system only gets `@pytest.mark.mode("cgb")` (or `"dmg"`).
   - If the bug already has an `xfail` test in `tests/test_known_bugs.py`, move it to
     `test_regressions.py` and drop the marker.
3. **See it fail** on the current code:
   `.venv/Scripts/python.exe -m pytest tests/test_regressions.py -k <test name>`
4. **Fix it** in `src/`, following Code Style in `CLAUDE.md`: clear names, one short
   comment per function, short walk-through comments only where needed.
   Banked asset data is only readable inside `WITH_BANK(name, ...)`.
5. **Run everything:** `make test-full`. Watch the `romusage` lines: bank 0 must keep headroom.
6. **Prove the test catches the bug** on the last commit (without your fix):
   `.venv/Scripts/python.exe tools/test_against.py HEAD tests/test_regressions.py -k <test name>`
   It must **fail** there. If it passes, the test doesn't detect the bug: rework it.
   If the fix is already committed, use the commit before it instead of `HEAD`.
7. **Look at it** when the bug was visible: `make shots`, then read
   `tests/out/shots/dmg_contact.png` and `cgb_contact.png`.
8. **Update the records:**
   - `docs/ROADMAP.md`: remove the bug from the Bugs list (the regression test's header
     keeps its number), tick the item if there is one.
   - `docs/SOURCES.md`: a row if the fix uses an idea from a tutorial, repo or doc.
   - `docs/DECISIONS.md`: a row if the fix chose a behavior (for example a timing).
9. **Report:** what was wrong, the fix, the test, the old-build result, and a suggested
   commit message.

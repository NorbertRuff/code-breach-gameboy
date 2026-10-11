---
name: close-milestone
description: Wrap up a roadmap milestone (M0–M9) of the ApexLab Game Boy game - full build and test, screenshots, roadmap and records bookkeeping, a hardware checklist for the owner and a commit message. Use when a milestone's tasks are done, or the owner says to finish or close a milestone.
---

# Close a milestone

1. **Verify from scratch:** `make rebuild`, then `make test-full`. Everything passes; tests
   in `tests/test_known_bugs.py`, if any, still fail for their stated reason (or moved to
   `test_regressions.py` if fixed). Note bank 0 free bytes and WRAM use from `romusage`.
2. **Look:** `make shots`, then read `tests/out/shots/dmg_contact.png` and
   `cgb_contact.png`. Anything wrong becomes a task or an entry in the ROADMAP's Bugs list.
3. **`docs/ROADMAP.md`:** tick what's done. Leave anything unfinished unticked, with one
   line saying why and where it moves. Owner tasks stay marked **(owner)**. Update the
   status block at the top (tests, bank 0 free bytes, WRAM, frame budget).
4. **`docs/ARCHITECTURE.md`:** how new parts work, with the idea's source where one was
   used; `CREDITS.md` for new third-party material.
5. **`docs/TESTING.md` and `CLAUDE.md`:** update them if commands, tools or conventions
   changed.
6. **Hardware checklist for the owner:** flash `obj/ApexLab.gb` (run `make` first;
   `obj/` is build output and can disappear). List what to try on DMG and on GBC for
   this milestone, and what correct looks like.
7. **Report:** what changed, how it was verified, what's left, the checklist, and a
   suggested commit message. The owner runs git.


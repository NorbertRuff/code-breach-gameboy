# Decisions

Project-level decisions. Add new entries at the top. Change the status instead of
deleting an entry.

| Date | Decision | Reason | Status |
|---|---|---|---|
| 2026-10-02 | **Mainframe boss: "KERNEL".** One-screen arena; the boss is drawn with background tiles. Kill-switch terminals light up in turn; hold Up on the lit one to upload a patch. 3 phases reuse the level 3 and 4 mechanics. | Works within the sprite limits, and the player doesn't need an attack. [RESEARCH.md](RESEARCH.md) section 3. | Accepted (concept; details in M8) |
| 2026-10-02 | **Brighter palettes on GBA**, chosen at boot when `_is_GBA` is set. | A GBA shows GBC games darker. Costs one extra palette table per asset. | Accepted |
| 2026-10-02 | **Hard mode after clearing the game:** every gate needs all of its section's fragments. | Cheap replay value: one flag and a menu entry. | Accepted |
| 2026-10-02 | **Story: "PATCH n/5".** Each level's fragments compile into a patch, uploaded in the boss fight. A 3-line boot screen, and one corrupted AI log line per elevator ride. | Gives the fragments a purpose with very little text. | Accepted |
| 2026-10-02 | **Music: Tronimal's Game Boy Music Packs #1 and #2 (CC BY 4.0), sound effects from GBFX/GBFX2 (CC-BY), played with hUGEDriver and untoxa's VGM2GBSFX player.** Needs `CREDITS.md` and an in-game credits screen. | Ready now, fitting tracks, clear license. Any track can still be replaced by the owner's own songs. | Accepted. Replaces the "song source decided later" entry. |
| 2026-10-02 | **The camera locks behind an opened gate.** No going back to earlier sections. | Simpler streaming, spawning and checkpoints, and each gate feels like progress. Missed bonus fragments can be collected on a replay. | Accepted |
| 2026-10-02 | **Positions in 1/32 px** (`uint16_t`), as in the GB Studio engine. | The tile index is the high byte, so tile lookups are cheap. A 160-tile level is 40,960 units, which fits `uint16_t`. | Accepted (Claude's proposal; the owner didn't object) |
| 2026-10-02 | **Collision kept in a 32×16 RAM ring (512 bytes)**, filled from the per-tileset table in ROM while columns stream in. | Opening gates and collecting fragments become single writes, and collision checks never switch ROM banks. Used by BlackCastle, marioland-gbdk and CrossZGB. | Accepted (Claude's proposal; the owner didn't object) |
| 2026-10-02 | **Each level ends at a lab elevator.** | Moving deeper into the facility, floor by floor, toward the Mainframe. | Accepted |
| 2026-10-02 | **Gates open with a fragment quota** (for example 3 of 5). Extra fragments are bonus. | Lets players skip the hardest fragment and still progress; bonus fragments reward exploring. Rejected alternative: one key card per section. | Accepted |
| 2026-10-02 | **Level length: 4–6 minutes on a first play, 120–160 tiles, 3 sections.** | About 30–40 minutes for the whole game on a first play. | Accepted |
| 2026-10-02 | **Level style: key-gated sections.** A security gate ends each of the first two sections. An opened gate is a checkpoint. | Middle ground between a linear run and collect-everything exploration, and gives natural checkpoints. Details in [LEVEL_DESIGN.md](LEVEL_DESIGN.md). | Accepted |
| 2026-10-02 | **Level design: Claude drafts (brief, greybox, preview), the owner reacts and playtests.** | Owner's choice; level design isn't the owner's strength. | Accepted |
| 2026-10-02 | **Physics and level loading before color and art** (roadmap M2 before M4 and M5). | Level layouts depend on the final jump. Greybox levels let design start before the art exists. | Accepted |
| 2026-10-02 | **Level editor: Tiled 1.12 + `tools/tiled2c.py`.** | Visual editing, object layers for spawns, actively maintained. LDtk has better auto-tiling but no release since January 2024. Comparison in [LEVEL_PIPELINE.md](LEVEL_PIPELINE.md). | Proposed. Owner wants it researched; confirm before M2. |
| 2026-10-02 | **No GBC double-speed mode.** | The DMG sets the CPU budget. Logic that only fits at double speed would lag on DMG. | Proposed |
| 2026-10-02 | **Music: hUGEDriver. Song source decided later.** | Standard GBDK music driver. The integration doesn't depend on who writes the songs. | Superseded: Tronimal packs chosen (see above) |
| 2026-10-02 | **Saves: MBC5 + RAM + battery (`0x1B`).** Fallback: password system. | The test flashcart is an EverDrive-style clone with microSD. Clones differ in save support, so M0 tests save persistence on it before save code is written. | Accepted, pending hardware test |
| 2026-10-02 | **Art: Claude Design → `tools/gbconvert.py` → png2asset.** | Claude Design for look and sprites. The script enforces hardware rules and makes DMG and GBC previews. Details in [ART_PIPELINE.md](ART_PIPELINE.md). | Accepted |
| 2026-10-02 | **Testing: PyBoy 2.7.0 headless tests with pytest, every test in DMG and GBC mode.** | Fast PC loop. Symbol lookup reads game state by name. Details in [TESTING.md](TESTING.md). | Accepted |
| 2026-10-02 | **Toolchain: GBDK-2020 4.5.0, installed by the owner into `gbdk/` (gitignored).** | Owner's choice. CI downloads the same version. | Accepted |
| 2026-10-02 | **Scope: the full 5-level game in GAME_DESIGN.md.** | Owner's goal. | Accepted |
| 2026-10-02 | **Target: one GBC-enhanced ROM that also runs on DMG (`-Wm-yc`).** | Color on GBC, playable on the original hardware. Test devices: DMG and GBC. Details in [CGB_PLAN.md](CGB_PLAN.md). | Accepted |
| 2026-10-02 | **Workflow: Claude writes the code. The owner runs git (branches, commits, PRs), reviews, and tests in Emulicious and on hardware.** | Owner's choice. | Accepted |

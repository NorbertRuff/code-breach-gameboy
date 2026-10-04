# Credits

Everything in the game that someone else made, with its license. The in-game credits
screen (roadmap M7) is written from this file. The full list of sources, including code
ideas, is in [docs/SOURCES.md](docs/SOURCES.md).

## Music

| Title | Author | Link | License | Changes | Used for |
|---|---|---|---|---|---|
| Tech (Game Boy Music Pack #2, `07_tech.uge`) | Tronimal (Yogi) | https://yogi-tronimal.itch.io/game-boy-music-pack-2 | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) | Converted to hUGEDriver C data by `tools/uge2c.py`; notes unchanged | Level 1, the test rooms |
| Neurotic Robonaut (Game Boy Music Pack #1, `free_04_neurotic_robonaut.uge`) | Tronimal (Yogi) | https://yogi-tronimal.itch.io/game-boy-music-pack | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) | Converted to hUGEDriver C data by `tools/uge2c.py`; notes unchanged | Level 2 |
| Observing Jupiter (Game Boy Music Pack #1, `free_07_observing_jupiter.uge`) | Tronimal (Yogi) | https://yogi-tronimal.itch.io/game-boy-music-pack | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) | Converted to hUGEDriver C data by `tools/uge2c.py`; notes unchanged | Level 3 |
| Decampment (Game Boy Music Pack #2, `01_decampment.uge`) | Tronimal (Yogi) | https://yogi-tronimal.itch.io/game-boy-music-pack-2 | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) | Converted to hUGEDriver C data by `tools/uge2c.py`; notes unchanged | Title screen |
| Gone Missing (Game Boy Music Pack #2, `08_gone_missing.uge`) | Tronimal (Yogi) | https://yogi-tronimal.itch.io/game-boy-music-pack-2 | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) | Converted to hUGEDriver C data by `tools/uge2c.py`; notes unchanged | Game over |
| Delight (Game Boy Music Pack #1, `free_06_delight.uge`) | Tronimal (Yogi) | https://yogi-tronimal.itch.io/game-boy-music-pack | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) | Converted to hUGEDriver C data by `tools/uge2c.py`; notes unchanged | Level complete |

The songs the game uses are in `music/` as downloaded, without the pack's number in the
file name (`tech.uge`, `decampment.uge`, ...). Both packs are in `music/packs/` as
downloaded (renamed `game_boy_music_pack_1.zip` and `_2.zip`), for the sound test.

## Sound effects

| Title | Author | Link | License | Changes | Used for |
|---|---|---|---|---|---|
| GBFX - sound effects for Game Boy (`GBFX.sav`) | Tronimal (Yogi) | https://yogi-tronimal.itch.io/gbfx | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) | Converted to C data by `tools/fxhammer2c.py`; sounds unchanged | Jump, fragment pickup, closed gate (picks in `sounds/effects.txt`) |
| GBFX2 - more sound effects for Game Boy (`GBFX2.sav`) | Tronimal (Yogi) | https://yogi-tronimal.itch.io/gbfx2 | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) | Converted to C data by `tools/fxhammer2c.py`; sounds unchanged | Gate opening, hit, pause (picks in `sounds/effects.txt`) |

The packs are in `sounds/` as downloaded, with their `.txt` effect lists.

## Code

| Name | Author | Link | License | Changes |
|---|---|---|---|---|
| hUGEDriver 6.1.3 (music driver) | SuperDisk (Nick Faro) and contributors | https://github.com/SuperDisk/hUGEDriver | Public domain | None: `lib/hUGEDriver/` holds the release's header and GBDK library |
| VGM2GBSFX (`utils/fxhammer2data.py`: the FX Hammer file layout and note-to-frequency table) | Toxa (untoxa) | https://github.com/untoxa/VGM2GBSFX | MIT | Rewritten in `tools/fxhammer2c.py` for our own effect player; the note table copied |
| GBDK-2020 4.5.0 (compiler and library) | The GBDK-2020 team | https://github.com/gbdk-2020/gbdk-2020 | GBDK's library license (see its repository) | None; not in this repository |

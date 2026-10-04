# Credits

Everything in the game that someone else made, with its license. The in-game credits
screen (roadmap M7) is written from this file. The full list of sources, including code
ideas, is in [docs/SOURCES.md](docs/SOURCES.md).

## Music

| Title | Author | Link | License | Changes | Used for |
|---|---|---|---|---|---|
| Tech (Game Boy Music Pack #2, `07_tech.uge`) | Tronimal (Yogi) | https://yogi-tronimal.itch.io/game-boy-music-pack-2 | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) | Converted to hUGEDriver C data by `tools/uge2c.py`; notes unchanged | Level 1, the test rooms |
| Neurotic Robonaut (Game Boy Music Pack #1, `free_04_neurotic_robonaut.uge`) | Tronimal (Yogi) | https://yogi-tronimal.itch.io/game-boy-music-pack | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) | Converted to hUGEDriver C data by `tools/uge2c.py`; notes unchanged | Level 2 |

The source songs are in `music/` as downloaded (renamed `tech.uge` and
`neurotic_robonaut.uge`).

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

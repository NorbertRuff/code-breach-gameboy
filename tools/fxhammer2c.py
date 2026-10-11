"""Turn FX Hammer sound effects (.sav) into banked C data for src/sound/sound.c.

    python tools/fxhammer2c.py sounds/effects.txt -o src/sound

Writes the game's effects, listed in sounds/effects.txt (one per line: name, pack file,
effect number, priority), to effects.c and effects.h ("make sounds").

An FX Hammer effect is up to 32 steps on CH2 (square) and CH4 (noise). Each step becomes
8 bytes: frames, channels, NR21, NR22, NR23, NR24, NR42, NR43. The file layout and the
note table come from fxhammer2data.py in github.com/untoxa/VGM2GBSFX (MIT License,
Copyright (c) 2022 Toxa; CREDITS.md).
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path

MAGIC = b"FX HAMMER"
MAGIC_AT = 0x09
CHANNELS_AT = 0x300         # one byte per effect: CH2 in the high nibble, CH4 in the low
STEPS_AT = 0x400            # 256 bytes per effect: 32 steps of 8 bytes
STEPS_PER_EFFECT = 32

CH2, CH4 = 0x02, 0x08       # hUGE_mute_mask bits (sound.h: SOUND_CH2, SOUND_CH4)
TRIGGER = 0x80              # NRx4: restart the channel

# Square channel frequency register per FX Hammer note (2048 - 131072 / Hz), C-3 upward.
NOTE_FREQUENCIES = [
    44, 156, 262, 363, 457, 547, 631, 710, 786, 854, 923, 986,
    1046, 1102, 1155, 1205, 1253, 1297, 1339, 1379, 1417, 1452, 1486, 1517,
    1546, 1575, 1602, 1627, 1650, 1673, 1694, 1714, 1732, 1750, 1767, 1783,
    1798, 1812, 1825, 1837, 1849, 1860, 1871, 1881, 1890, 1899, 1907, 1915,
    1923, 1930, 1936, 1943, 1949, 1954, 1959, 1964, 1969, 1974, 1978, 1982,
    1985, 1988, 1992, 1995, 1998, 2001, 2004, 2006, 2009, 2011, 2013, 2015,
]
FIRST_NOTE = 0x40           # FX Hammer stores notes as 0x40 + 2 * index


@dataclass
class Effect:
    name: str               # C name suffix and, in the sound test, the label
    channels: int           # CH2 | CH4
    steps: list[bytes]      # 8 bytes each, as sound.c reads them
    priority: int = 0


def read_effect(data: bytes, number: int, name: str) -> Effect:
    """One effect of an FX Hammer file, as player steps."""
    if data[MAGIC_AT:MAGIC_AT + len(MAGIC)] != MAGIC:
        raise ValueError("not an FX Hammer file")
    used = data[CHANNELS_AT + number]
    if not used:
        raise ValueError(f"effect {number:#04x} is empty")
    channels = (CH2 if used & 0xF0 else 0) | (CH4 if used & 0x0F else 0)
    steps = []
    for i in range(STEPS_PER_EFFECT):
        at = STEPS_AT + number * 256 + i * 8
        frames, ch2_pan, ch2_volume, ch2_duty, ch2_note, ch4_pan, ch4_volume, ch4_noise = data[at:at + 8]
        if frames == 0:
            break
        step_channels, nr21, nr22, nr23, nr24, nr42, nr43 = 0, 0, 0, 0, 0, 0, 0
        if ch2_pan:
            frequency = NOTE_FREQUENCIES[(ch2_note - FIRST_NOTE) >> 1]
            step_channels |= CH2
            nr21, nr22, nr23, nr24 = ch2_duty, ch2_volume, frequency & 0xFF, TRIGGER | frequency >> 8
        if ch4_pan:
            step_channels |= CH4
            nr42, nr43 = ch4_volume, ch4_noise
        steps.append(bytes([frames, step_channels, nr21, nr22, nr23, nr24, nr42, nr43]))
    return Effect(name, channels, steps)


def read_pick_list(path: Path) -> list[Effect]:
    """The game's effects from sounds/effects.txt, in the file's order."""
    effects = []
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        words = line.split("#")[0].split()
        if not words:
            continue
        if len(words) != 4:
            raise ValueError(f"{path}:{line_number}: expected name, pack, number, priority")
        name, pack, number, priority = words
        effect = read_effect((path.parent / pack).read_bytes(), int(number, 0), name)
        effect.priority = int(priority)
        effects.append(effect)
    return effects


def c_identifier(name: str) -> str:
    return re.sub(r"\W+", "_", name).strip("_").lower()


def effects_c(effects: list[Effect]) -> str:
    """The banked C file: each effect's steps, then the sound_effects table (sound.h)."""
    out = ["// Made by tools/fxhammer2c.py from FX Hammer files. Don't edit.", "",
           "#pragma bank 255", "", "#include <gb/gb.h>", '#include "sound.h"', ""]
    for effect in effects:
        out.append(f"static const uint8_t effect_{c_identifier(effect.name)}[] = {{")
        out += ["    " + ", ".join(f"0x{b:02X}" for b in step) + "," for step in effect.steps]
        out += ["    0x00    // end", "};", ""]
    out += ["BANKREF(sound_effects)", "const sound_effect_t sound_effects[] = {"]
    out += [f"    {{ effect_{c_identifier(e.name)}, 0x{e.channels:02X}, {e.priority} }},"
            for e in effects]
    out += ["};", ""]
    return "\n".join(out)


def effects_h(effects: list[Effect]) -> str:
    """The game's effect numbers, for sound_play()."""
    out = ["// Made by tools/fxhammer2c.py from sounds/effects.txt. Don't edit.", "",
           "#ifndef EFFECTS_H", "#define EFFECTS_H", ""]
    out += [f"#define EFFECT_{c_identifier(e.name).upper()} {i}" for i, e in enumerate(effects)]
    out += ["", "#endif // EFFECTS_H", ""]
    return "\n".join(out)


def write_outputs(pick_list: Path, out_dir: Path) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    effects = read_pick_list(pick_list)
    (out_dir / "effects.c").write_text(effects_c(effects), encoding="utf-8", newline="\n")
    (out_dir / "effects.h").write_text(effects_h(effects), encoding="utf-8", newline="\n")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("pick_list", type=Path, help="sounds/effects.txt")
    parser.add_argument("-o", "--out", type=Path, required=True, help="output folder")
    args = parser.parse_args()
    try:
        write_outputs(args.pick_list, args.out)
    except (OSError, ValueError, IndexError) as error:
        print(f"fxhammer2c: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

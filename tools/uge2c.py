"""Turn hUGETracker songs (.uge) into banked C data for hUGEDriver.

    python tools/uge2c.py music/tech.uge [...] -o src/music
    python tools/uge2c.py music/packs/*.zip -o obj/soundtest --table obj/soundtest/song_table.c

For each song it writes <name>.c and <name>.h: a const hUGESong_t named after the file,
in a ROM bank (#pragma bank 255, BANKREF(name)). "make music" runs it on every song.
A .zip stands for every .uge in it. --table also writes the list of the songs for the
sound test ROM ("make soundtest").

The output matches hUGETracker's own GBDK export (uge2source; RenderSongToGBDKC in
src/codegen.pas of github.com/SuperDisk/hUGETracker), written in Python so the build
needs no extra program. Only .uge format version 6 is read (hUGETracker 1.0 and GB Studio
4). Sources and reasons: docs/SOURCES.md and docs/DECISIONS.md.
"""

from __future__ import annotations

import argparse
import re
import struct
import sys
import zipfile
from dataclasses import dataclass
from pathlib import Path

UGE_VERSION = 6
ROWS = 64                   # rows per pattern
SUBPATTERN_ROWS = 32
NO_NOTE = 90                # hUGEDriver's ___
LAST_NOTE = 71              # B_8
INSTRUMENTS_PER_TYPE = 15
WAVES = 16


@dataclass
class Cell:
    note: int
    instrument: int
    volume: int             # in a subpattern: the row to jump to
    effect: int             # code << 8 | params


@dataclass
class Instrument:
    kind: int               # 0 duty, 1 wave, 2 noise
    length: int
    length_enabled: bool
    initial_volume: int
    volume_sweep_up: bool
    volume_sweep_amount: int
    sweep_time: int
    sweep_down: bool
    sweep_shift: int
    duty: int
    output_level: int
    waveform: int
    seven_step: bool
    subpattern: list[Cell] | None


@dataclass
class Song:
    name: str
    artist: str
    ticks_per_row: int
    timer_enabled: bool
    instruments: list[list[Instrument]]   # duty, wave, noise; 15 each, instrument 1 first
    waves: list[list[int]]                # 16 waves of 32 4-bit samples
    patterns: dict[int, list[Cell]]
    orders: list[list[int]]               # pattern numbers per channel


class Reader:
    """Reads the little-endian Free Pascal records of a .uge file in order."""

    def __init__(self, data: bytes):
        self.data, self.at = data, 0

    def take_all(self, fmt: str) -> tuple:
        values = struct.unpack_from("<" + fmt, self.data, self.at)
        self.at += struct.calcsize("<" + fmt)
        return values

    def take(self, fmt: str):
        """Like take_all, but a single value comes back alone."""
        values = self.take_all(fmt)
        return values if len(values) > 1 else values[0]

    def short_string(self) -> str:
        """A ShortString: a length byte and 255 bytes of room."""
        raw = self.take("256s")
        return raw[1:1 + raw[0]].decode("latin-1")

    def ansi_string(self) -> str:
        return self.take(f"{self.take('i')}s").decode("latin-1")

    def cells(self, count: int) -> list[Cell]:
        cells = []
        for _ in range(count):
            note, instrument, volume, code, params = self.take_all("iiiiB")
            cells.append(Cell(note, instrument, volume, code << 8 | params))
        return cells

    def instrument(self) -> Instrument:
        kind = self.take("i")
        self.short_string()                                     # name
        length, length_enabled, volume, sweep_up, amount = self.take("i?BiB")
        sweep_time, sweep_down, shift, duty = self.take("iiiB")
        output_level, waveform, counter_step, has_subpattern = self.take("iii?")
        subpattern = self.cells(ROWS)
        return Instrument(kind, length, length_enabled, volume, sweep_up == 0, amount,
                          sweep_time, sweep_down == 1, shift, duty, output_level, waveform,
                          counter_step == 1, subpattern if has_subpattern else None)


def read_uge(data: bytes) -> Song:
    """Parse a version 6 .uge file (ReadSongFromStreamV6 in hUGETracker's song.pas)."""
    r = Reader(data)
    version = r.take("i")
    if version != UGE_VERSION:
        raise ValueError(f"only .uge version {UGE_VERSION} is supported, this is version {version}")
    name, artist, _comment = r.short_string(), r.short_string(), r.short_string()
    instruments = [[r.instrument() for _ in range(INSTRUMENTS_PER_TYPE)] for _ in range(3)]
    waves = [list(r.take_all("32B")) for _ in range(WAVES)]
    ticks_per_row, timer_enabled, _timer_divider = r.take("i?i")
    patterns = {}
    for _ in range(r.take("i")):
        key = r.take("i")
        patterns[key] = r.cells(ROWS)
    # Each order list has one entry too many (hUGETracker's "off-by-one" note): drop it.
    orders = [list(r.take_all(f"{r.take('i')}i"))[:-1] for _ in range(4)]
    for _ in range(16):
        r.ansi_string()                                         # routines: not used
    if r.at != len(data):
        raise ValueError(f"{len(data) - r.at} bytes left after the song: not a valid .uge file")
    return Song(name, artist, ticks_per_row, timer_enabled, instruments, waves, patterns, orders)


def used_parts(song: Song) -> tuple[list[int], list[int], int]:
    """The highest instrument used per type (duty, wave, noise; 0 = none) in the order
    lists' patterns, and the highest waveform (FindUsedStuff in codegen.pas)."""
    highest = [0, 0, 0]
    waveform = 0
    for channel, order in enumerate(song.orders):
        kind = (0, 0, 1, 2)[channel]
        for key in set(order):
            for cell in song.patterns[key]:
                if channel == 2 and cell.effect >> 8 == 0x9:      # 9xx sets the wave
                    waveform = max(waveform, cell.effect & 0xFF)
                if 1 <= cell.instrument <= INSTRUMENTS_PER_TYPE:
                    highest[kind] = max(highest[kind], cell.instrument)
                    if channel == 2:
                        waveform = max(waveform, song.instruments[1][cell.instrument - 1].waveform)
    for instrument in song.instruments[1]:
        for cell in instrument.subpattern or []:
            if cell.effect >> 8 == 0x9:
                waveform = max(waveform, cell.effect & 0xFF)
    return [max(h, 1) for h in highest], waveform


def unique_orders(song: Song) -> list[list[int]]:
    """The order lists with each pattern replaced by the first one with the same cells
    (OptimizeSong in song.pas), so duplicates aren't stored twice."""
    first_of = {}
    for key in sorted(song.patterns):
        first_of.setdefault(tuple((c.note, c.instrument, c.volume, c.effect) for c in song.patterns[key]), key)
    same = {key: first_of[tuple((c.note, c.instrument, c.volume, c.effect) for c in cells)]
            for key, cells in song.patterns.items()}
    return [[same[key] for key in order] for order in song.orders]


def note_name(note: int) -> str:
    """hUGEDriver.h's name for a note: C_3 is 0, Cs3 is 1, ... B_8 is 71."""
    if not 0 <= note <= LAST_NOTE:
        return "___"
    names = ["C_", "Cs", "D_", "Ds", "E_", "F_", "Fs", "G_", "Gs", "A_", "As", "B_"]
    return f"{names[note % 12]}{note // 12 + 3}"


def pattern_c(name: str, cells: list[Cell]) -> str:
    rows = [f"    DN({note_name(c.note)},{c.instrument if 0 <= c.instrument <= 15 else 0},0x{c.effect:03X}),"
            for c in cells]
    return f"static const unsigned char {name}[] = {{\n" + "\n".join(rows) + "\n};"


def subpattern_c(name: str, cells: list[Cell]) -> str:
    """32 rows; the middle value is the row to jump to, and the last row jumps back to row
    1 unless it says otherwise."""
    rows = []
    for i, c in enumerate(cells[:SUBPATTERN_ROWS]):
        note = "___" if c.note == NO_NOTE else str(c.note)
        jump = 1 if i == SUBPATTERN_ROWS - 1 and c.volume == 0 else min(max(c.volume, 0), 32)
        rows.append(f"    DN({note},{jump},0x{c.effect:03X}),")
    return f"static const unsigned char {name}[] = {{\n" + "\n".join(rows) + "\n};"


def instrument_c(i: Instrument, subpattern: str) -> str:
    """One instrument in hUGEDriver's layout: the register values it starts a note with
    (SquareInstrumentToRegisters etc. in instruments.pas)."""
    trigger = 0x80 | (0x40 if i.length_enabled else 0)              # NR14/NR34: start, length
    envelope = i.initial_volume << 4 | i.volume_sweep_up << 3 | i.volume_sweep_amount & 7
    if i.kind == 0:
        sweep = (i.sweep_time & 7) << 4 | i.sweep_down << 3 | i.sweep_shift & 7
        values = [sweep, i.duty << 6 | i.length & 63, envelope, subpattern, trigger]
    elif i.kind == 1:
        values = [i.length & 0xFF, (i.output_level & 3) << 5, i.waveform, subpattern, trigger]
    else:
        high_mask = i.length & 63 | (0x40 if i.length_enabled else 0) | (0x80 if i.seven_step else 0)
        values = [envelope, subpattern, high_mask, 0, 0]
    return "{" + ",".join(str(v) for v in values) + "}"


def song_c(song: Song, name: str, source: str) -> tuple[str, str]:
    """The .c and .h text for one song."""
    orders = unique_orders(song)
    highest, last_wave = used_parts(song)
    used_patterns = sorted({key for order in orders for key in order})
    parts = [pattern_c(f"P{key}", song.patterns[key]) for key in used_patterns]

    kinds = ("duty", "wave", "noise")
    banks = []
    for kind, (label, instruments) in enumerate(zip(kinds, song.instruments)):
        rows = []
        for number, instrument in enumerate(instruments[:highest[kind]], 1):
            sp = "0"
            if instrument.subpattern:
                sp = f"{label}_sp{number}"
                parts.append(subpattern_c(sp, instrument.subpattern))
            rows.append("    " + instrument_c(instrument, sp) + ",")
        struct_name = ("hUGEDutyInstr_t", "hUGEWaveInstr_t", "hUGENoiseInstr_t")[kind]
        banks.append(f"static const {struct_name} {label}_instruments[] = {{\n" + "\n".join(rows) + "\n};")
    for channel, order in enumerate(orders, 1):
        parts.append(f"static const unsigned char* const order{channel}[] = {{"
                     + ",".join(f"P{key}" for key in order) + "};")
    waves = ["    " + ",".join(str(w[j] << 4 | w[j + 1]) for j in range(0, 32, 2)) + ","
             for w in song.waves[:last_wave + 1]]

    c = f"""// Generated by tools/uge2c.py from {source}. Don't edit: change the song in
// hUGETracker and run "make music".
// "{song.name}" by {song.artist}.
#pragma bank 255

#include "{name}.h"

BANKREF({name})

static const unsigned char order_cnt = {max(len(o) for o in orders) * 2};

{chr(10).join(parts)}

{chr(10).join(banks)}

static const unsigned char waves[] = {{
{chr(10).join(waves)}
}};

const hUGESong_t {name} = {{
    {song.ticks_per_row}, &order_cnt, order1, order2, order3, order4,
    duty_instruments, wave_instruments, noise_instruments, 0, waves
}};
"""
    h = f"""// Generated by tools/uge2c.py from {source}. Don't edit.
#ifndef SONG_{name.upper()}_H
#define SONG_{name.upper()}_H

#include <gbdk/platform.h>
#include "hUGEDriver.h"

BANKREF_EXTERN({name})
extern const hUGESong_t {name};

#endif
"""
    return c, h


def write_song(data: bytes, name: str, source: str, out_dir: Path) -> Song:
    """Converts one song into <out_dir>/<name>.c and .h."""
    song = read_uge(data)
    c, h = song_c(song, name, source)
    (out_dir / f"{name}.c").write_text(c, encoding="utf-8", newline="\n")
    (out_dir / f"{name}.h").write_text(h, encoding="utf-8", newline="\n")
    return song


def write_outputs(path: Path, out_dir: Path) -> Song:
    """Converts one song file into <out_dir>/song_<file name>.c and .h."""
    return write_song(path.read_bytes(), "song_" + path.stem, path.as_posix(), out_dir)


def song_files(path: Path) -> list[tuple[bytes, str, str]]:
    """(data, C name, source) of a .uge file, or of every .uge in a .zip."""
    if path.suffix != ".zip":
        return [(path.read_bytes(), "song_" + path.stem, path.as_posix())]
    with zipfile.ZipFile(path) as pack:
        return [(pack.read(member), "song_" + re.sub(r"\W+", "_", Path(member).stem).lower(),
                 f"{path.as_posix()}:{member}")
                for member in sorted(pack.namelist()) if member.endswith(".uge")]


def song_table_c(songs: list[tuple[str, Song]]) -> str:
    """The sound test's song list: each song, its bank and its title."""
    includes = "\n".join(f'#include "{name}.h"' for name, _ in songs)
    entries = "\n".join(f'    {{ &{name}, BANK({name}), "{song.name.upper()}" }},' for name, song in songs)
    return f"""// Generated by tools/uge2c.py --table. Don't edit.
#include "song_table.h"
{includes}

const uint8_t song_count = {len(songs)};
const song_entry_t song_table[] = {{
{entries}
}};
"""


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("songs", nargs="+", type=Path)
    parser.add_argument("-o", "--out", type=Path, required=True)
    parser.add_argument("--table", type=Path, help="also write the sound test's song list here")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    converted = []
    for path in args.songs:
        for data, name, source in song_files(path):
            song = write_song(data, name, source, args.out)
            converted.append((name, song))
            print(f"{source} -> {args.out}: \"{song.name}\" by {song.artist}")
    if args.table:
        args.table.write_text(song_table_c(converted), encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())

"""Record every effect and song of the sound test ROM (obj/soundtest.gb) to WAV files.

    python tools/render_sounds.py [--out tests/out/sounds]

PyBoy plays each effect with the game's effect player and records it until the player
ends it, plus a short tail ("gbfx2_2a_hit.wav", named like the sound test shows it), and
the first SONG_SECONDS of each song ("song_05_hideout.wav", mono at half the rate, to
keep the files small). Effects and songs each get one gain, so their loudness compares.
For picking effects (sounds/effects.txt) and songs (music/) by ear.
"""

from __future__ import annotations

import argparse
import re
import sys
import wave
from pathlib import Path

import numpy as np
from pyboy import PyBoy

ROOT = Path(__file__).resolve().parent.parent
ROM = ROOT / "obj" / "soundtest.gb"
GENERATED = ROOT / "obj" / "soundtest"
SAMPLE_RATE = 48000
TAIL_FRAMES = 10            # recorded after an effect ends, for its last note's fade
LONGEST_EFFECT_FRAMES = 32 * 255
SONG_SECONDS = 45
FRAMES_PER_SECOND = 60
LOUDEST_SAMPLE = 24000      # the loudest recording's peak in the WAVs (16-bit)


def effect_names() -> list[str]:
    """The sound test's effect names ("GBFX2 2A HIT"), from the generated C file."""
    source = (GENERATED / "soundtest_effects.c").read_text(encoding="utf-8")
    return re.findall(r'"([^"]+)"', source[source.index("sound_effect_names"):])


def song_names() -> list[str]:
    """The sound test's songs by C name ("song_05_hideout"), from the generated table."""
    return re.findall(r"\{ &(\w+),", (GENERATED / "song_table.c").read_text(encoding="utf-8"))


def select(pyboy: PyBoy, songs: bool, number: int) -> None:
    pyboy.memory[pyboy.symbol_lookup("_showing_songs")[1]] = songs
    pyboy.memory[pyboy.symbol_lookup("_selected")[1]] = number
    pyboy.button("a")


def record_effect(pyboy: PyBoy, number: int) -> np.ndarray:
    """Plays one effect; returns its sound, stereo 8-bit signed samples."""
    select(pyboy, False, number)
    taken_channels = pyboy.symbol_lookup("_hUGE_mute_mask")[1]   # 0 once the effect ends
    chunks, tail = [], TAIL_FRAMES
    for frame in range(LONGEST_EFFECT_FRAMES):
        pyboy.tick(1, False, True)
        chunks.append(pyboy.sound.ndarray.copy())
        if frame and not pyboy.memory[taken_channels]:
            tail -= 1
            if not tail:
                break
    return np.concatenate(chunks)


def record_song(pyboy: PyBoy, number: int) -> np.ndarray:
    """Plays a song from its start; returns its first SONG_SECONDS, mono at half the rate."""
    select(pyboy, True, number)
    chunks = []
    for _ in range(SONG_SECONDS * FRAMES_PER_SECOND):
        pyboy.tick(1, False, True)
        chunks.append(pyboy.sound.ndarray.copy())
    pyboy.button("b")
    stereo = np.concatenate(chunks).astype(np.int32)
    return (stereo[::2, 0] + stereo[::2, 1]) // 2


def write_wavs(out: Path, recordings: dict, channels: int, rate: int) -> None:
    """Writes each recording to <out>/<name>.wav, all with one gain."""
    gain = LOUDEST_SAMPLE // max(int(np.abs(samples).max()) for samples in recordings.values())
    for name, samples in recordings.items():
        file_name = re.sub(r"\W+", "_", name).strip("_").lower() + ".wav"
        with wave.open(str(out / file_name), "wb") as wav:
            wav.setnchannels(channels)
            wav.setsampwidth(2)
            wav.setframerate(rate)
            wav.writeframes((samples.astype(np.int32) * gain).astype(np.int16).tobytes())


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--out", type=Path, default=ROOT / "tests" / "out" / "sounds")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    pyboy = PyBoy(str(ROM), window="null", sound_emulated=True, sound_sample_rate=SAMPLE_RATE)
    pyboy.set_emulation_speed(0)
    pyboy.tick(60, False, False)        # boot and the menu
    effects = {name: record_effect(pyboy, number) for number, name in enumerate(effect_names())}
    songs = {name: record_song(pyboy, number) for number, name in enumerate(song_names())}
    pyboy.stop(save=False)
    write_wavs(args.out, effects, 2, SAMPLE_RATE)
    write_wavs(args.out, songs, 1, SAMPLE_RATE // 2)
    print(f"render_sounds: {len(effects)} effects and {len(songs)} songs in {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

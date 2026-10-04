"""tools/uge2c.py: the committed song files, and the .uge reading and the C it writes
(hUGETracker's own GBDK export is the reference)."""

import struct
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import uge2c  # noqa: E402

SONGS = sorted((ROOT / "music").glob("*.uge"))


def test_committed_song_files_are_up_to_date(tmp_path):
    # Fails when a song in music/ changed without "make music".
    for song in SONGS:
        uge2c.write_outputs(song.relative_to(ROOT), tmp_path)
    for generated in tmp_path.iterdir():
        committed = ROOT / "src" / "music" / generated.name
        assert committed.read_text(encoding="utf-8") == generated.read_text(encoding="utf-8"), \
            f"{committed.name} is out of date: run make music"


@pytest.mark.parametrize("path", SONGS, ids=[s.stem for s in SONGS])
def test_songs_read_to_their_last_byte(path):
    # read_uge stops on any byte left over, so a wrong record size can't pass.
    song = uge2c.read_uge(path.read_bytes())
    assert song.artist and song.ticks_per_row > 0 and not song.timer_enabled
    assert len({len(order) for order in song.orders}) == 1, "the 4 channels' order lists differ in length"
    assert all(key in song.patterns for order in song.orders for key in order)


def test_other_format_versions_are_refused():
    with pytest.raises(ValueError, match="version 5"):
        uge2c.read_uge(struct.pack("<i", 5) + bytes(100))


def test_note_names_follow_hugedriver_h():
    assert [uge2c.note_name(n) for n in (0, 1, 12, 71, 72, 90)] == ["C_3", "Cs3", "C_4", "B_8", "___", "___"]


def test_instruments_become_the_driver_s_register_bytes():
    # A 50% duty square at volume 15 fading 3 steps, sweeping down by shift 2 every 1.
    duty = uge2c.Instrument(kind=0, length=0, length_enabled=False, initial_volume=15,
                            volume_sweep_up=False, volume_sweep_amount=3, sweep_time=1,
                            sweep_down=True, sweep_shift=2, duty=2, output_level=0, waveform=0,
                            seven_step=False, subpattern=None)
    assert uge2c.instrument_c(duty, "0") == "{26,128,243,0,128}"       # NR10, NR11, NR12, -, NR14
    noise = uge2c.Instrument(kind=2, length=10, length_enabled=True, initial_volume=8,
                             volume_sweep_up=True, volume_sweep_amount=1, sweep_time=0,
                             sweep_down=False, sweep_shift=0, duty=0, output_level=0, waveform=0,
                             seven_step=True, subpattern=None)
    assert uge2c.instrument_c(noise, "noise_sp1") == "{137,noise_sp1,202,0,0}"   # NR42, -, length | flags


def test_identical_patterns_are_stored_once():
    cells = [uge2c.Cell(uge2c.NO_NOTE, 0, 0, 0)] * uge2c.ROWS
    song = uge2c.Song("t", "a", 6, False, [], [], {0: cells, 1: list(cells), 2: cells}, [[0, 1], [2, 2], [1], [0]])
    assert uge2c.unique_orders(song) == [[0, 0], [0, 0], [0], [0]]

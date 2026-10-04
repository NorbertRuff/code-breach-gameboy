"""tools/fxhammer2c.py: the committed effect files, and how FX Hammer steps become sound
register bytes (fxhammer2data.py in untoxa/VGM2GBSFX is the reference)."""

import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import fxhammer2c  # noqa: E402

PICK_LIST = ROOT / "sounds" / "effects.txt"


def fx_hammer_file(number: int, channels: int, steps: list[bytes]) -> bytes:
    """A minimal FX Hammer file with one effect."""
    data = bytearray(0x400 + 0x3C * 256)
    data[0x09:0x12] = b"FX HAMMER"
    data[0x300 + number] = channels
    for i, step in enumerate(steps):
        at = 0x400 + number * 256 + i * 8
        data[at:at + 8] = step
    return bytes(data)


def test_committed_effect_files_are_up_to_date(tmp_path):
    # Fails when sounds/effects.txt changed without "make sounds".
    fxhammer2c.write_outputs([PICK_LIST], tmp_path, whole_packs=False)
    for name in ("effects.c", "effects.h"):
        committed = ROOT / "src" / "sound" / name
        assert committed.read_text(encoding="utf-8") == (tmp_path / name).read_text(encoding="utf-8"), \
            f"{name} is out of date: run make sounds"


def test_steps_become_register_bytes():
    # Step 1: CH2 at the lowest note (frequency 44), 2 frames. Step 2: CH4 only, 1 frame.
    data = fx_hammer_file(5, 0x33, [bytes([2, 0x22, 0xF1, 0x80, 0x40, 0, 0, 0]),
                                    bytes([1, 0, 0, 0, 0, 0x88, 0xA2, 0x57])])
    effect = fxhammer2c.read_effect(data, 5, "test")
    assert effect.channels == fxhammer2c.CH2 | fxhammer2c.CH4
    assert effect.steps == [bytes([2, 0x02, 0x80, 0xF1, 44, 0x80, 0, 0]),
                            bytes([1, 0x08, 0, 0, 0, 0, 0xA2, 0x57])]


def test_empty_effects_and_other_files_are_refused():
    with pytest.raises(ValueError, match="empty"):
        fxhammer2c.read_effect(fx_hammer_file(0, 0x30, []), 1, "test")
    with pytest.raises(ValueError, match="FX Hammer"):
        fxhammer2c.read_effect(bytes(0x10000), 0, "test")


@pytest.mark.parametrize("pack, count", [("GBFX.sav", 49), ("GBFX2.sav", 60)])
def test_whole_packs_read_with_their_names(pack, count):
    # GBFX: 48 effects and "Nintendo" ($30). GBFX2: $00 to $3B (its .txt says 59).
    effects = fxhammer2c.read_whole_pack(ROOT / "sounds" / pack)
    assert len(effects) == count
    assert all(effect.steps and len(effect.steps) <= 32 for effect in effects)
    assert effects[4].name == ("GBFX 04 PITCH UP" if pack == "GBFX.sav" else "GBFX2 04 BLEEP")

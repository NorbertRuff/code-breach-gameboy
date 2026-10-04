"""Boot: the hardware is detected in DMG and GBC mode, and the Player mirror matches."""

import pytest

from gamelib import PLAYER_SIZE

pytestmark = pytest.mark.both_modes   # the mode changes the result: run both in the quick tier too


def test_hardware_detected(game):
    # GBDK sets _cpu from the CPU's A register at power-on: 0x11 on GBC, 0x01 on DMG.
    game.frames(5)
    expected_cpu = 0x11 if game.mode == "cgb" else 0x01
    assert game.u8("__cpu") == expected_cpu
    assert game.u8("_is_cgb") == (1 if game.mode == "cgb" else 0)


def test_player_struct_size_matches(game):
    # If this fails, update PLAYER_FIELDS in tests/gamelib.py to match player.h.
    assert game.u8("_player_struct_size") == PLAYER_SIZE

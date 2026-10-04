"""Cartridge RAM survives a power cycle (obj/savetest.gb, built by "make savetest").

PyBoy writes the cartridge RAM into a buffer when it stops, and a new PyBoy loads it.
This proves the ROM side of saving. Whether the EverDrive clone keeps the RAM is checked
by hand on the real Game Boy (docs/TESTING.md).
"""

import io

import pytest

from gamelib import boot, read_symbol


# The ROM name and the cartridge type its header must hold (byte 0x147).
SAVETESTS = {"savetest": 0x1B, "savetest_mbc1": 0x03, "savetest_mbc3": 0x13}


@pytest.fixture(params=SAVETESTS)
def savetest_rom(request, rom_dir):
    """The game's cartridge type (MBC5), then the MBC1 and MBC3 variants for the
    flashcart (roadmap M7)."""
    rom = rom_dir / f"{request.param}.gb"
    if not rom.exists():
        pytest.skip(f"obj/{request.param}.gb is missing: run 'make savetest' or 'make test'")
    return rom


def run_once(rom, mode, ram_in=None):
    """Boot, let the save code run, then power off. Returns the RAM and what the ROM saw."""
    pyboy = boot(rom, mode, ram_file=ram_in)
    pyboy.tick(60, False)
    seen = {
        "save_found": read_symbol(pyboy, "_save_found")[0],
        "boot_count": int.from_bytes(read_symbol(pyboy, "_boot_count", 2), "little"),
        "write_ok": read_symbol(pyboy, "_write_ok")[0],
    }
    ram_out = io.BytesIO()
    pyboy.stop(save=True, ram_file=ram_out)
    ram_out.seek(0)
    return ram_out, seen


def test_save_survives_power_cycles(savetest_rom, mode):
    ram, first = run_once(savetest_rom, mode)
    assert first == {"save_found": 0, "boot_count": 1, "write_ok": 1}

    ram, second = run_once(savetest_rom, mode, ram)
    assert second == {"save_found": 1, "boot_count": 2, "write_ok": 1}

    _, third = run_once(savetest_rom, mode, ram)
    assert third["boot_count"] == 3


def test_savetest_headers_name_their_mapper(savetest_rom):
    assert savetest_rom.read_bytes()[0x147] == SAVETESTS[savetest_rom.stem]

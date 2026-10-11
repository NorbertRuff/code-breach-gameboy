"""Checks on the built ROM file and linker map. No emulation needed."""

import re

# Areas that must stay in ROM bank 0. _HOME holds library routines that run while
# other banks are switched in; if it spills past 0x4000, bank switching crashes the game.
BANK0_AREAS = ("_HOME", "_CODE", "_GSINIT", "_GSFINAL", "_INITIALIZER", "_BASE")


def test_header_gbc_enhanced_and_dmg_compatible(rom_bytes):
    # 0x80: uses GBC features, still runs on DMG. 0xC0 would be GBC-only.
    assert rom_bytes[0x143] == 0x80


def test_cartridge_is_mbc5_without_ram(rom_bytes):
    # Progress is kept in passwords (docs/ARCHITECTURE.md, "Saves: passwords").
    assert rom_bytes[0x147] == 0x19   # MBC5
    assert rom_bytes[0x149] == 0x00   # no cartridge RAM


def test_title(rom_bytes):
    assert rom_bytes[0x134:0x13F].rstrip(b"\x00") == b"APEXLAB"


def test_header_checksum(rom_bytes):
    # The boot ROM refuses to start a cartridge with a wrong header checksum.
    checksum = 0
    for byte in rom_bytes[0x134:0x14D]:
        checksum = (checksum - byte - 1) & 0xFF
    assert rom_bytes[0x14D] == checksum


def test_global_checksum(rom_bytes):
    total = (sum(rom_bytes) - rom_bytes[0x14E] - rom_bytes[0x14F]) & 0xFFFF
    assert int.from_bytes(rom_bytes[0x14E:0x150], "big") == total


def test_rom_size_matches_header(rom_bytes):
    # Header byte 0x148: ROM size is 32 KB << n.
    assert len(rom_bytes) == 0x8000 << rom_bytes[0x148]


def test_bank0_areas_end_below_0x4000(rom_dir):
    text = (rom_dir / "ApexLab.map").read_text(errors="replace")
    areas = {}
    for name, addr, size in re.findall(r"^(_\w+)\s+([0-9A-F]{8})\s+([0-9A-F]{8})\s+=", text, re.M):
        areas[name] = (int(addr, 16), int(size, 16))
    assert "_HOME" in areas, "no _HOME area in the map"
    for name in BANK0_AREAS:
        if name in areas:
            addr, size = areas[name]
            assert addr + size <= 0x4000, f"{name} ends at {addr + size:#x}, past bank 0"

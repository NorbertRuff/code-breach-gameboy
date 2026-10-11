"""The ROM to share (release/ApexLab.gb, built by "make release" and by "make test-full"):
it shows the intro and the title screen, which the tests' own build skips."""

import shutil

import pytest

from conftest import ROOT
from gamelib import TITLE_SCREEN_STATE, boot, read_symbol

RELEASE_ROM = ROOT / "release" / "ApexLab.gb"
RELEASE_SYM = ROOT / "release" / "obj" / "ApexLab.sym"   # PyBoy reads symbols next to the ROM
INTRO_FRAMES = 900   # generous: the intro credits, then the title is drawn


@pytest.fixture
def release_rom(tmp_path):
    """A copy of the release ROM and its symbols, so PyBoy never writes next to the real one."""
    if not RELEASE_ROM.exists():
        pytest.skip("release/ApexLab.gb is missing: run 'make release' or 'make test-full'")
    shutil.copy(RELEASE_SYM, tmp_path / RELEASE_SYM.name)
    return shutil.copy(RELEASE_ROM, tmp_path / RELEASE_ROM.name)


def test_the_release_rom_stops_at_the_title_screen(release_rom, mode):
    pyboy = boot(release_rom, mode)
    try:
        pyboy.tick(INTRO_FRAMES, False)
        assert read_symbol(pyboy, "_current_game_state")[0] == TITLE_SCREEN_STATE
    finally:
        pyboy.stop(save=False)

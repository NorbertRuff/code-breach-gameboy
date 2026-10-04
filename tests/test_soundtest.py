"""The effect player (src/sound/sound.c) plays every effect of the GBFX and GBFX2 packs to
its end, in the sound test ROM (obj/soundtest.gb, built by "make soundtest")."""

import sys
from pathlib import Path

import pytest

from gamelib import boot

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import fxhammer2c  # noqa: E402

PACKS = [ROOT / "sounds" / "GBFX.sav", ROOT / "sounds" / "GBFX2.sav"]


def test_every_effect_holds_its_channels_for_its_length(rom_dir):
    rom = rom_dir / "soundtest.gb"
    if not rom.exists():
        pytest.skip("obj/soundtest.gb is missing: run 'make soundtest' or 'make test'")
    pyboy = boot(rom, "dmg")
    pyboy.tick(30, False)
    selected = pyboy.symbol_lookup("_selected")[1]
    taken_channels = pyboy.symbol_lookup("_hUGE_mute_mask")[1]
    effects = [effect for pack in PACKS for effect in fxhammer2c.read_whole_pack(pack)]
    for number, effect in enumerate(effects):
        pyboy.memory[selected] = number
        pyboy.button("a")
        pyboy.tick(2, False)                            # the press, then the first step
        assert pyboy.memory[taken_channels] == effect.channels, effect.name
        frames = 2
        while pyboy.memory[taken_channels] and frames < 1000:
            pyboy.tick(1, False)
            frames += 1
        # One step per frame from the VBlank after the press, and one more call to end it.
        assert abs(frames - sum(step[0] for step in effect.steps) - 2) <= 1, effect.name
    pyboy.stop(save=False)

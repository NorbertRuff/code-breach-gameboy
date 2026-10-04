"""Golden screenshots (roadmap M4): key screens must match the PNGs in tests/golden/
pixel for pixel, in DMG and GBC mode. After a change that should alter them (new art,
a layout change), look at the new pictures and rewrite them with "make goldens".

A mismatch saves the new picture and a difference image to tests/out/.
"""

from pathlib import Path

import pytest
from PIL import Image, ImageChops

from gamelib import COREGAMELOOP, LEVEL_1, Pilot

GOLDEN = Path(__file__).resolve().parent / "golden"


def level_1_start(game):
    game.load_level(LEVEL_1)
    game.frames(10)


def level_1_rack_stair(game):
    # Pilot moves from tests/test_level_1.py: up the steps to the top of the rack stair.
    game.load_level(LEVEL_1)
    pilot = Pilot(game)
    pilot.run_to(50); pilot.jump(1); pilot.jump(8)
    pilot.run_to(124); pilot.jump(20); pilot.jump(20)
    pilot.stop()


def greybox_paused(game):
    game.start_on_open_ground()
    game.press("start")
    game.frames(3)


def greybox_invincible(game):
    # 10 frames after the fade-in: on DMG the blinking player is shown in that frame.
    game.start_on_open_ground()
    game.lose_life()
    game.wait_for_state(COREGAMELOOP)
    game.frames(10)


SCREENS = {
    "level_1_start": level_1_start,
    "level_1_rack_stair": level_1_rack_stair,
    "greybox_paused": greybox_paused,
    "greybox_invincible": greybox_invincible,
}


def render(game) -> Image.Image:
    """The next frame as an RGB image. PyBoy's first rendered frame after unrendered ones
    starts the window a line early, so the second one is used."""
    game.tick(render=True)
    game.tick(render=True)
    return game.pyboy.screen.image.convert("RGB")


@pytest.mark.parametrize("screen", SCREENS)
def test_screen_matches_its_golden_picture(game, screen, request):
    SCREENS[screen](game)
    picture = render(game)
    golden = GOLDEN / f"{screen}_{game.mode}.png"
    if request.config.getoption("--update-golden"):
        GOLDEN.mkdir(exist_ok=True)
        picture.save(golden)
        return
    assert golden.exists(), f"no golden picture {golden.name}: run 'make goldens'"
    red, green, blue = ImageChops.difference(picture, Image.open(golden).convert("RGB")).split()
    changed_mask = ImageChops.lighter(ImageChops.lighter(red, green), blue).point(lambda v: 255 if v else 0)
    changed = changed_mask.histogram()[255]
    if changed:
        out = game.out_dir
        out.mkdir(parents=True, exist_ok=True)
        picture.save(out / f"golden_{screen}_{game.mode}_new.png")
        changed_mask.save(out / f"golden_{screen}_{game.mode}_diff.png")
    assert not changed, f"{changed} pixels differ from {golden.name} (new picture and diff in tests/out/)"

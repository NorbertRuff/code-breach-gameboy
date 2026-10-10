"""The screen's line splits (roadmap M3, src/engine/screen.c): the parallax band in screen
rows 0-2 scrolls at half the camera's speed, the level below with the camera, and no
sprite is drawn over the HUD."""

import pytest

from gamelib import palette_lumas

pytestmark = pytest.mark.both_modes   # the mode changes the result: run both in the quick tier too

BAND_LAST_LINE, HUD_FIRST_LINE = 23, 128        # src/engine/screen.h
BAND_TILE_BASE, BAND_TILE_COUNT = 8, 8          # greybox level: after the 8 tileset tiles (level_manager.c draw_band)


def scroll_per_line(game) -> list:
    """SCX for each of the 144 screen lines of a rendered frame."""
    game.tick(render=True)
    return [line[0] for line in game.pyboy.screen.tilemap_position_list]


def band_tiles(game) -> list:
    """The background map's rows 0-2, all 32 columns."""
    return [game.bkg_tile(col, row) for row in range(3) for col in range(32)]


def test_band_scrolls_at_half_the_camera_speed(game):
    game.start_on_open_ground()
    game.hold("right", 20)
    game.frames(60)                                 # the camera settles
    camera = game.camera_x()
    assert camera > 0
    scroll = scroll_per_line(game)
    assert scroll[:BAND_LAST_LINE + 1] == [(camera >> 1) & 0xFF] * (BAND_LAST_LINE + 1)
    assert scroll[BAND_LAST_LINE + 1:] == [camera & 0xFF] * (144 - BAND_LAST_LINE - 1)


def test_band_palette_is_light_to_dark(game):
    # DMG shows color index n as shade n: index 0 lightest, 3 darkest (ART_PIPELINE.md).
    lumas = palette_lumas(game, "_greybox_band_palettes")
    assert lumas == sorted(lumas, reverse=True), f"palette not light to dark: {lumas}"


def test_band_is_drawn_once_and_streaming_leaves_it(game):
    game.start_on_open_ground()
    before = band_tiles(game)
    assert set(before) <= set(range(BAND_TILE_BASE, BAND_TILE_BASE + BAND_TILE_COUNT))
    assert len(set(before)) > 1, "the band has a pattern"
    game.place_player(480, 103)                     # the far end: every column redrawn
    assert band_tiles(game) == before


def test_no_sprite_is_drawn_over_the_hud(game):
    # Fall into the pit at columns 35-38: on the way down the player's sprite reaches into
    # the HUD's lines, which must look the same as with nothing there.
    game.start_on_open_ground()
    game.tick(render=True)                          # PyBoy's first rendered frame after
    game.tick(render=True)                          # unrendered ones starts the window a line early
    hud = game.pyboy.screen.ndarray[HUD_FIRST_LINE:].copy()
    game.place_player(296, 119)                     # over the pit, past gate A
    reached_hud = False
    lives = game.lives()
    for _ in range(20):
        game.tick(render=True)
        player = game.player()
        if game.lives() != lives:                   # fell out: the HUD's lives digit changes
            break
        if player.py >= HUD_FIRST_LINE:
            reached_hud = True
            assert (game.pyboy.screen.ndarray[HUD_FIRST_LINE:] == hud).all(), f"sprite over the HUD at feet {player.py}"
    assert reached_hud, "the player never fell into the HUD's lines"

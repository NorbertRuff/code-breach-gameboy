"""Boot and basic gameplay, in DMG and GBC mode."""

from gamelib import COREGAMELOOP, PLAYER_SIZE

FOOT_ON_LAND = 0x01   # src/player/player.h


def test_hardware_detected(game):
    # GBDK sets _cpu from the CPU's A register at power-on: 0x11 on GBC, 0x01 on DMG.
    game.frames(5)
    expected_cpu = 0x11 if game.mode == "cgb" else 0x01
    assert game.u8("__cpu") == expected_cpu
    assert game.u8("_is_cgb") == (1 if game.mode == "cgb" else 0)


def test_player_struct_size_matches(game):
    # If this fails, update PLAYER_FIELDS in tests/gamelib.py to match player.h.
    assert game.u8("_player_struct_size") == PLAYER_SIZE


def test_reaches_gameplay(game):
    # SKIP_INTRO_AND_START is on, so the level starts right after boot.
    game.wait_for_state(COREGAMELOOP, timeout=300)
    assert game.lives() == 3


def test_screen_is_drawn(game):
    # On GBC, palettes that were never loaded show as one flat color.
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.frames(30)
    game.screenshot("level_start")
    assert game.screen_colors() >= 3


def test_hud_window_at_bottom_after_boot(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.frames(2)
    assert game.reg("WY") == 128


def test_player_walks_right(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.frames(10)
    start_x = game.player().x
    game.hold("right", 10)
    assert game.player().x > start_x


def test_player_jumps_and_lands(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.frames(10)
    ground_y = game.player().y
    game.hold("a", 10)
    assert game.player().y < ground_y, "player did not rise"
    game.frames(90)
    after = game.player()
    assert after.underfoot_state == FOOT_ON_LAND, "player is still in the air"
    assert after.y == ground_y, "player did not land on the same ground"

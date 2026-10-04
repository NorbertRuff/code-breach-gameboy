"""The pause (START) and the level-select cheat (SELECT while paused)."""

from gamelib import (COREGAMELOOP, GAMEPLAY_START, LEVEL_1, LEVEL_2, LEVEL_GREYBOX_TEST,
                     LEVEL_TEST_JUMPS, PAUSED)

PAUSE_TILES = {44, 46, 48, 50, 52}      # P, A, U, S, E (src/game_state/pause.c)


def pause_label_shown(game) -> bool:
    return PAUSE_TILES <= {tile for _, _, tile in game.visible_sprites()}


def test_start_pauses_and_everything_stops(game):
    game.start_on_open_ground()
    game.press("start")
    game.frames(1)
    assert game.state() == PAUSED
    assert pause_label_shown(game)
    player, bots = game.player(), game.actors()
    game.pyboy.button_press("right")
    game.frames(30)
    game.pyboy.button_release("right")
    assert (game.player().x, game.player().y) == (player.x, player.y)
    assert [(a.x, a.velocity_x) for a in game.actors()] == [(a.x, a.velocity_x) for a in bots]


def test_start_again_resumes(game):
    game.start_on_open_ground()
    game.press("start")
    game.frames(5)
    game.press("start")
    game.frames(2)
    assert game.state() == COREGAMELOOP
    assert not pause_label_shown(game)
    x = game.player().x
    game.hold("right", 10)
    assert game.player().x > x


def test_holding_start_pauses_once(game):
    game.start_on_open_ground()
    game.hold("start", 20)
    game.frames(1)
    assert game.state() == PAUSED


def test_select_while_paused_loads_the_next_level(game):
    # The game fixture starts in the greybox level; the levels go round in index order.
    game.start_on_open_ground()
    for expected in (LEVEL_TEST_JUMPS, LEVEL_1, LEVEL_2, LEVEL_GREYBOX_TEST):
        game.press("start")
        game.frames(2)
        game.press("select")
        game.wait_for_state(GAMEPLAY_START, timeout=60)
        game.wait_for_state(COREGAMELOOP, timeout=120)
        assert game.u8("_level_index") == expected
        assert game.level().section == 0 and game.lives() == 3
        game.frames(2)


def test_select_does_nothing_while_playing(game):
    game.start_on_open_ground()
    game.press("select")
    game.frames(30)
    assert game.state() == COREGAMELOOP and game.u8("_level_index") == LEVEL_GREYBOX_TEST

"""The pause (START), the level-select cheat (A + SELECT while paused) and the no-death
cheat (A + B while paused)."""

from gamelib import (COREGAMELOOP, GAMEPLAY_START, LEVEL_1, LEVEL_2, LEVEL_3, LEVEL_4, LEVEL_5, LEVEL_GREYBOX_TEST,
                     LEVEL_TEST_JUMPS, LEVEL_TEST_MOVERS, PAUSED)

PAUSE_TILES = {98, 100, 102, 104, 106}  # P, A, U, S, E (src/game_state/pause.c)


def pause_label_shown(game) -> bool:
    return PAUSE_TILES <= {tile for _, _, tile in game.visible_sprites()}


def press_with_a(game, button: str) -> None:
    """Press a button for one frame with A held, the way the cheats are entered."""
    game.pyboy.button_press("a")
    game.press(button)
    game.pyboy.button_release("a")


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


def test_a_select_while_paused_loads_the_next_level(game):
    # The game fixture starts in the greybox level; the levels go round in index order.
    game.start_on_open_ground()
    for expected in (LEVEL_TEST_JUMPS, LEVEL_TEST_MOVERS, LEVEL_1, LEVEL_2, LEVEL_3, LEVEL_4, LEVEL_5,
                     LEVEL_GREYBOX_TEST):
        game.press("start")
        game.frames(2)
        press_with_a(game, "select")
        game.wait_for_state(GAMEPLAY_START, timeout=60)
        game.wait_for_state(COREGAMELOOP, timeout=400)      # the boss level types its log first
        assert game.u8("_level_index") == expected
        assert game.level().section == 0 and game.lives() == 3
        game.frames(2)


def test_select_does_nothing_while_playing(game):
    game.start_on_open_ground()
    game.press("select")
    game.frames(30)
    assert game.state() == COREGAMELOOP and game.u8("_level_index") == LEVEL_GREYBOX_TEST


def turn_on_no_death(game):
    game.press("start")
    game.frames(2)
    press_with_a(game, "b")
    game.frames(2)
    game.press("start")
    game.frames(2)
    assert game.u8("_no_death_cheat") == 1


def test_a_b_while_paused_turns_the_no_death_cheat_on_and_off(game):
    game.start_on_open_ground()
    turn_on_no_death(game)
    game.press("start")
    game.frames(2)
    press_with_a(game, "b")
    game.frames(2)
    assert game.u8("_no_death_cheat") == 0


def test_select_or_b_alone_while_paused_does_nothing(game):
    game.start_on_open_ground()
    game.press("start")
    game.frames(2)
    for button in ("select", "b"):
        game.press(button)
        game.frames(30)
    assert game.state() == PAUSED and game.u8("_level_index") == LEVEL_GREYBOX_TEST
    assert game.u8("_no_death_cheat") == 0


def test_with_no_death_a_bot_passes_through_and_a_fall_keeps_the_lives(game):
    game.start_on_open_ground()
    turn_on_no_death(game)
    game.place_player(80, 71)                   # on platform B, in the bot's way (test_actors.py)
    game.frames(60)
    assert game.state() == COREGAMELOOP and game.lives() == 3
    game.set_player_y(170)                      # below the level: back to the section start
    for _ in range(300):
        game.frames(1)
        if game.player().py < 170:
            break
    assert game.player().py < 170 and game.lives() == 3

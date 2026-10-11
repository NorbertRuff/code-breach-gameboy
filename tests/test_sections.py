"""Sections, gates, checkpoints and the exit (roadmap M3, GAME_DESIGN.md, "Rules"),
in levels/greybox_test.tmj: gate A at column 33 and gate B at column 50, both with a
quota of 1 and their bottom tile on row 14; the exit at column 62."""

from gamelib import (COLLISION_NONE, COLLISION_SOLID, COREGAMELOOP, HUD_DIGIT_TILE_BASE,
                     HUD_SLASH_TILE, LEVEL_COMPLETE, LEVEL_SIZE, SPRITE_RIGHT_W)
from test_streaming import MAP_TOP_ROW, MARKER_TILE

GATE_A, GATE_BOTTOM_ROW = 33, 14
GATE_A_X = GATE_A * 8
CHECKPOINT_A = (GATE_A_X + 8 + 8, 119)     # body just past the gate, feet on its bottom row
GATE_ROWS = range(MAP_TOP_ROW, GATE_BOTTOM_ROW + 1)


def hud_count(game) -> str:
    """The HUD's fragment count, like '01/03'."""
    tiles = [game.window_tile(col, 0) for col in range(15, 20)]
    return "".join("/" if t == HUD_SLASH_TILE else str(t - HUD_DIGIT_TILE_BASE) for t in tiles)


def pick_up_and_run_into_gate_a(game) -> None:
    """From the level start: pick up the fragment at column 26 (section A's quota), then
    run right into gate A and through it as it opens."""
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.place_player(212, 119)
    game.pyboy.button_press("right")
    for _ in range(90):
        game.frames(1)
        if game.player().px >= CHECKPOINT_A[0] - 4:     # through, short of the pit
            break
    game.pyboy.button_release("right")
    game.frames(30)                                     # the gate finishes opening


def test_level_struct_size_matches(game):
    # If this fails, update LEVEL_FIELDS in tests/gamelib.py to match level_manager.h.
    assert game.u8("_level_struct_size") == LEVEL_SIZE


def test_gate_is_solid_up_to_the_top_of_the_map(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.place_player(240, 119)                         # gate A in view
    assert [game.collision(GATE_A, row) for row in GATE_ROWS] == [COLLISION_SOLID] * len(GATE_ROWS)
    assert {game.bkg_tile(GATE_A, row) for row in GATE_ROWS} == {MARKER_TILE["gate"]}


def test_gate_stays_closed_without_its_quota(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.place_player(240, 119)                         # no fragment of section A picked up
    game.hold("right", 40)
    assert game.player().px + SPRITE_RIGHT_W == GATE_A_X - 1, "stopped against the gate"
    level = game.level()
    assert level.section == 0 and level.gates[0][3] == 0
    assert hud_count(game) == "00/01"


def test_touching_the_gate_with_its_quota_opens_it(game):
    pick_up_and_run_into_gate_a(game)
    level = game.level()
    assert level.section == 1, "section B has started"
    assert [game.collision(GATE_A, row) for row in GATE_ROWS] == [COLLISION_NONE] * len(GATE_ROWS)
    assert game.player().px >= CHECKPOINT_A[0] - 4, "the player went through"
    assert hud_count(game) == "00/01", "the HUD counts section B against its quota"


def test_opened_gate_is_the_checkpoint(game):
    pick_up_and_run_into_gate_a(game)
    game.lose_life()
    game.wait_for_state(COREGAMELOOP)
    game.frames(1)
    player = game.player()
    assert (player.px, player.py) == CHECKPOINT_A
    assert game.camera_x() == GATE_A_X + 8, "the camera stops just past the gate"
    assert game.level().section == 1


def test_camera_and_player_stay_past_an_opened_gate(game):
    pick_up_and_run_into_gate_a(game)
    game.lose_life()                                    # at the checkpoint the lock is in force
    game.wait_for_state(COREGAMELOOP)
    game.hold("left", 60)
    assert game.camera_x() == GATE_A_X + 8
    assert game.player().px == CHECKPOINT_A[0]


def test_losing_a_life_keeps_the_fragments(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.place_player(212, 119)                         # picks up the fragment at column 26
    game.lose_life()
    game.wait_for_state(COREGAMELOOP)
    assert game.level().section_collected[0] == 1
    assert hud_count(game) == "01/01"


def test_exit_completes_the_level(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.place_player(490, 103)                         # on the top step, in the exit
    assert game.state() == LEVEL_COMPLETE


def test_closed_gate_buzzes_once_per_touch(game):
    # Walking into a gate without its quota plays the "denied" sound once, not every frame.
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.place_player(240, 119)
    effects = game.track_effects()
    game.hold("right", 40)
    assert effects.count("denied") == 1
    game.hold("left", 10)
    game.hold("right", 20)
    assert effects.count("denied") == 2


def test_game_over_shows_a_message_then_restarts_the_section(game):
    # Game over in section B after picking up one of its fragments: "GAME OVER" in the play
    # area (the band and the HUD stay), then CONTINUE restarts section B at its checkpoint,
    # with full lives and only section B's fragments back.
    pick_up_and_run_into_gate_a(game)
    game.place_player(330, 87)                          # on the one-way shelf: column 41's fragment
    assert game.level().section_collected[:2] == [1, 1]
    band = [game.bkg_tile(col, row) for row in range(3) for col in range(32)]
    game.set_lives(1)
    game.lose_life()
    game.frames(200)                                    # the flashes, then the message
    assert game.text(5, 5, 9) == "GAME OVER"
    assert game.window_tile(1, 0) == HUD_DIGIT_TILE_BASE    # the HUD shows 0 lives
    assert [game.bkg_tile(col, row) for row in range(3) for col in range(32)] == band
    game.answer_game_over()
    game.wait_for_state(COREGAMELOOP, timeout=200)
    level, player = game.level(), game.player()
    assert level.section == 1 and level.section_collected[:2] == [1, 0], "only section B's fragments reset"
    assert (player.px, player.py) == CHECKPOINT_A
    game.frames(30)
    assert game.lives() == 3 and game.window_tile(1, 0) == HUD_DIGIT_TILE_BASE + 3
    assert game.collision(41, 9) == 4 and game.bkg_tile(41, 9) == MARKER_TILE["fragment"],         "column 41's fragment is back"

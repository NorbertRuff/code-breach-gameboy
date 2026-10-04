"""Player physics and tile collision (roadmap M2). Values from src/player/player.h.

Places in levels/greybox_test.tmj used here (columns x 8 px, screen rows y 8 px; the
floor is row 15): platform C is row 6, x 112-143; platform D is row 10, x 168-199;
x = 156 is open floor with nothing above it; a one-way shelf is row 11, x 320-351 (feet
y 87 on it); spikes sit on the floor at x 368-383.
"""

from gamelib import (COREGAMELOOP, CORNER_CORRECTION, COYOTE_FRAMES, JUMP_BUFFER_FRAMES,
                     SUBPIXELS)


def jumped_within(game, frames: int) -> bool:
    """Does the player move upward at some point in the next frames?"""
    for _ in range(frames):
        game.frames(1)
        if game.player().velocity_y < 0:
            return True
    return False


def test_coyote_time(game):
    # Walk right off platform C, then press A a few frames after the feet left it.
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.place_player(136, 47)
    assert game.player().on_ground
    game.pyboy.button_press("right")
    for _ in range(60):
        game.frames(1)
        if not game.player().on_ground:
            break
    left_ledge = game.save()
    for wait in (1, COYOTE_FRAMES - 2):
        game.load(left_ledge)
        game.frames(wait)
        game.press("a")
        assert jumped_within(game, 4), f"no jump {wait} frames after leaving the ledge"
    game.load(left_ledge)
    game.frames(COYOTE_FRAMES + 2)
    game.press("a")
    assert not jumped_within(game, 4), "jumped long after leaving the ledge"


def test_jump_buffer(game):
    # Drop onto open ground, pressing A a few frames before the landing.
    game.start_on_open_ground()
    game.place_player(156, 60)
    game.frames(1)
    falling = game.save()
    fall_frames = 0
    while not game.player().on_ground:
        game.frames(1)
        fall_frames += 1
    for early in (1, JUMP_BUFFER_FRAMES - 2):
        game.load(falling)
        game.frames(fall_frames - early)
        game.press("a")
        assert jumped_within(game, early + 4), f"A pressed {early} frames before landing was lost"
    early = JUMP_BUFFER_FRAMES + 3
    game.load(falling)
    game.frames(fall_frames - early)
    game.press("a")
    assert not jumped_within(game, early + 4), f"A pressed {early} frames before landing still jumped"


def test_head_slides_past_a_ceiling_corner(game):
    # Under platform D's left corner. The box overlaps it by CORNER_CORRECTION px: the
    # jump slides left past it. Overlapping by more, the head bumps.
    game.start_on_open_ground()
    start = game.save()
    corner_x = 168
    game.place_player(corner_x + CORNER_CORRECTION - 8, 119)
    game.jump_rise(60)
    assert game.player().px == corner_x - 8, "did not slide clear of the corner"
    assert game.player().on_ground

    game.load(start)
    game.place_player(corner_x + CORNER_CORRECTION + 2 - 8, 119)
    rise = game.jump_rise(60) / SUBPIXELS
    assert rise == 0, f"rose {rise} px into a ceiling it overlaps by 6 px"


def test_one_way_platform_lets_the_player_up_and_holds_them(game):
    # Standing under the shelf (it doesn't block the body): jump up through it, land on top.
    game.start_on_open_ground()
    game.place_player(336, 119)
    game.jump_rise(60)
    player = game.player()
    assert player.on_ground and player.py == 11 * 8 - 1, f"not standing on the one-way row: y = {player.py}"


def stand_on_the_shelf(game) -> None:
    game.start_on_open_ground()
    game.place_player(336, 87)
    game.frames(2)
    assert game.player().on_ground


def test_down_and_a_drops_through_a_one_way_platform(game):
    stand_on_the_shelf(game)
    game.pyboy.button_press("down")
    game.press("a")
    game.frames(40)
    player = game.player()
    assert player.on_ground and player.py == 119, f"did not drop to the floor: y = {player.py}"


def test_down_alone_or_a_alone_stays_on_the_shelf(game):
    stand_on_the_shelf(game)
    game.hold("down", 30)
    assert game.player().on_ground and game.player().py == 87, "Down alone left the shelf"
    game.press("a")
    assert jumped_within(game, 4), "A alone on the shelf did not jump"
    game.frames(60)
    assert game.player().on_ground and game.player().py == 87, "a jump from the shelf did not land on it"


def test_down_and_a_on_solid_ground_jumps(game):
    game.start_on_open_ground()
    game.pyboy.button_press("down")
    game.press("a")
    assert jumped_within(game, 4), "Down + A on solid ground did not jump"


def test_hazard_tile_costs_a_life(game):
    game.start_on_open_ground()
    game.place_player(336, 119)
    lives = game.lives()
    game.hold("right", 30)
    assert game.lives() == lives - 1


"""Level 3's movers (M8). Lifts: a one-way platform that moves up and down its travel and
carries the player standing on it (src/actor/lift.c, src/player/player.c). Chasers: stand
until they see the player near on their floor, then walk toward them inside their range
(src/actor/chaser.c). levels/test_movers.tmj: one lift, center x 132, its top from y 48 to
y 104 (world pixels); a chaser on the floor (feet row 119) with centers 216-296, starting
at 296; a one-way shelf over the chaser's range, feet row 95."""

import pytest

from gamelib import COREGAMELOOP, LEVEL_TEST_MOVERS, PLAYER_OFFSETS

pytestmark = pytest.mark.level(LEVEL_TEST_MOVERS)

LIFT_X = 132
LIFT_HIGHEST, LIFT_LOWEST = 48, 104
FLOOR_FEET = 119
PAUSE_FRAMES = 30                                  # at each end
CYCLE_FRAMES = 2 * (LIFT_LOWEST - LIFT_HIGHEST + PAUSE_FRAMES)   # 1 px per frame, up and down


CHASER_MIN_X, CHASER_MAX_X = 216, 296
SHELF_FEET = 95
SIGHT_X = 48                                       # src/actor/chaser.c


def the_lift(game):
    return next(actor for actor in game.actors() if actor.lift)


def record_updates(game) -> list:
    """From now on, add (player feet row, on_ground, lift top) at the end of each update."""
    rows = []
    game.at_update_end(lambda _: rows.append((game.player().py, game.player().on_ground, the_lift(game).y)))
    return rows


def test_a_lift_travels_one_pixel_per_frame_and_waits_at_the_ends(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    rows = record_updates(game)
    game.frames(CYCLE_FRAMES + 10)
    tops = [lift_top for _, _, lift_top in rows]
    assert min(tops) == LIFT_HIGHEST and max(tops) == LIFT_LOWEST
    assert all(abs(b - a) <= 1 for a, b in zip(tops, tops[1:]))
    # Each stop lasts the pause: the top stays put for PAUSE_FRAMES + 1 updates in a row.
    stops = [run for run in runs_of_equal(tops) if run > 1]
    assert stops and all(run == PAUSE_FRAMES + 1 for run in stops[:-1])


def runs_of_equal(values: list) -> list:
    """Lengths of the runs of equal neighbours, in order."""
    runs = [1]
    for a, b in zip(values, values[1:]):
        if a == b:
            runs[-1] += 1
        else:
            runs.append(1)
    return runs


def test_the_player_lands_on_a_lift_and_rides_it_both_ways(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.place_player(LIFT_X, the_lift(game).y - 20)    # drops onto it
    game.frames(20)
    rows = record_updates(game)
    game.frames(CYCLE_FRAMES + 10)                       # past both turnarounds
    assert all(on_ground and feet == lift_top - 1 for feet, on_ground, lift_top in rows), rows


def test_a_lift_passes_through_a_body_below_it(game):
    # The lift starts at its lowest place, inside the body of a player on the floor under
    # it, and rises: one-way, so it never catches the player.
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.place_player(LIFT_X, FLOOR_FEET)
    rows = record_updates(game)
    game.frames(CYCLE_FRAMES)
    assert all(on_ground and feet == FLOOR_FEET for feet, on_ground, _ in rows)


def test_the_player_jumps_off_and_drops_through_a_lift(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.place_player(LIFT_X, the_lift(game).y - 20)
    rows = record_updates(game)
    game.frames(20)
    feet_on_lift = rows[-1][0]
    game.press("a")
    game.frames(8)
    assert rows[-1][0] < feet_on_lift - 8, "A on a lift didn't jump"
    game.frames(60)                                     # lands on it again
    feet, on_ground, lift_top = rows[-1]
    assert on_ground and feet == lift_top - 1
    game.pyboy.button_press("down")
    game.press("a")
    game.pyboy.button_release("down")
    game.frames(90)
    assert game.player().py == FLOOR_FEET, "Down + A didn't drop through the lift"


def the_chaser(game):
    return next(actor for actor in game.actors() if actor.chaser)


def make_invincible(game):
    """The player can't be hurt for the next 255 frames (the game counts it down)."""
    _, base = game.pyboy.symbol_lookup("_player")
    game.pyboy.memory[base + PLAYER_OFFSETS["invincibility_frames"]] = 255


def test_a_chaser_stands_until_it_sees_the_player(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.place_player(LIFT_X, FLOOR_FEET)               # the chaser is in play, the player far
    game.frames(60)
    assert the_chaser(game).x == CHASER_MAX_X
    game.place_player(CHASER_MAX_X - SIGHT_X + 4, FLOOR_FEET)
    x_before = the_chaser(game).x
    game.frames(10)
    assert the_chaser(game).x == x_before - 10, "it didn't walk 1 px per frame toward the player"


def test_a_chaser_doesnt_see_the_player_on_a_shelf_above_it(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.place_player(CHASER_MAX_X - 16, SHELF_FEET)
    game.frames(60)
    assert game.player().on_ground and the_chaser(game).x == CHASER_MAX_X


def test_a_chaser_stays_inside_its_range(game):
    # Lured step by step to the left end of its range by an invincible player, who ends
    # just past it.
    game.wait_for_state(COREGAMELOOP, timeout=300)
    make_invincible(game)
    for x in (CHASER_MAX_X - 36, CHASER_MAX_X - 72, CHASER_MIN_X - 10):
        game.place_player(x, FLOOR_FEET)
        game.frames(40)
    assert the_chaser(game).x == CHASER_MIN_X

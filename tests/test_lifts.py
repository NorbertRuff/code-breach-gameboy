"""Lifts (level 3's mechanic, M8): a one-way platform that moves up and down its travel
and carries the player standing on it (src/actor/lift.c, src/player/player.c).
levels/test_lifts.tmj: one lift, center x 132, its top from y 48 to y 104 (world pixels),
over the floor, whose feet row is 119."""

import pytest

from gamelib import COREGAMELOOP, LEVEL_TEST_LIFTS

pytestmark = pytest.mark.level(LEVEL_TEST_LIFTS)

LIFT_X = 132
LIFT_HIGHEST, LIFT_LOWEST = 48, 104
FLOOR_FEET = 119
PAUSE_FRAMES = 30                                  # at each end
CYCLE_FRAMES = 2 * (LIFT_LOWEST - LIFT_HIGHEST + PAUSE_FRAMES)   # 1 px per frame, up and down


def the_lift(game):
    return next(actor for actor in game.actors() if not actor.bot)


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

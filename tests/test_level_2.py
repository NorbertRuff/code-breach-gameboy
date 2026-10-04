"""Level 2, the Code Testing Lab (levels/level_2.tmj): a replay plays it from the start to
the exit elevator without losing a life, within the frame budget and the sprites-per-line
limit. Level 1's test (test_level_1.py) explains the Pilot moves and the x values.
"""

import pytest

from gamelib import (COREGAMELOOP, LEVEL_1, LEVEL_2, LEVEL_COMPLETE, LEVEL_GREYBOX_TEST,
                     LINES_PER_FRAME, Pilot)
from test_frame_budget import FRAME_LINE_LIMIT
from test_level_1 import SPRITES_PER_LINE


def section_a(pilot: Pilot) -> None:
    """"Test bench": all 5 fragments, through gate A."""
    pilot.run_to(64); pilot.stop()
    pilot.jump(30, steer=1, direction="")         # up through the first shelf, fragment 1
    pilot.run_to(84); pilot.jump(8)               # over to the bookcase's lowest shelf
    pilot.stop()
    pilot.jump(30, steer=1, direction="")         # up the bookcase,
    pilot.jump(30, steer=1, direction="")         # fragment 2 on top
    pilot.run_to(200); pilot.stop()               # off its top, onto the room's lid
    pilot.drop_through()                          # into the room
    pilot.run_to(196); pilot.stop()               # fragment 3
    pilot.jump(30, steer=1, direction="")         # back up through the lid
    pilot.run_to(236); pilot.stop()
    pilot.jump(20, steer=26)                      # onto the shelf over the pit
    pilot.run_to(292); pilot.stop()
    pilot.jump(60, steer=1, direction="")         # bonus fragment 4 above it
    pilot.run_to(300); pilot.stop()
    pilot.jump(20, steer=26)                      # over the rest of the pit
    pilot.wait_for_bot(360, 380, 420)             # the bot walks away under the long shelf
    pilot.run_to(352); pilot.jump(30, steer=10)   # onto the shelf
    pilot.run_to(440); pilot.stop()               # fragment 5 on the way
    pilot.wait(lambda: pilot.bot(360).velocity_x < 0 and pilot.bot(360).x < 400, "the bot to pass beneath")
    pilot.open_gate(1)


def zig_zag(pilot: Pilot) -> None:
    """The first two shelves of section B's zig-zag climb: a fragment."""
    pilot.run_to(536); pilot.stop()
    pilot.jump(30, steer=1, direction="")         # up onto the first shelf
    pilot.jump(30, steer=12)                      # right and up, fragment


def section_b_high_route(pilot: Pilot) -> None:
    """"Unit tests", the high route of one-way shelves: 2 more fragments."""
    zig_zag(pilot)
    pilot.stop(); pilot.jump(30, steer=12, direction="left")   # left and up, the top
    pilot.stop(); pilot.run_to(544); pilot.stop()
    pilot.jump(30); pilot.stop()                  # Gap-6 to the high route, fragment
    pilot.run_to(624); pilot.stop()
    pilot.jump(20); pilot.stop()                  # Gap-4
    pilot.run_to(688); pilot.stop()
    pilot.jump(30); pilot.stop()                  # Gap-4, fragment
    pilot.run_to(752); pilot.stop()
    pilot.jump(12, steer=30); pilot.stop()        # down to the bridge


def section_b_low_route(pilot: Pilot) -> None:
    """The low route on the floor: 2 fast bots, 1 more fragment."""
    zig_zag(pilot)
    pilot.stop(); pilot.drop_through()            # back down to the floor
    pilot.pass_bot(592, hold=30)
    pilot.run_to(688); pilot.stop()               # fragment between the bots
    pilot.pass_bot(704, hold=30)
    pilot.run_to(800); pilot.stop()
    pilot.jump(30, steer=1, direction="")         # up onto the bridge


def section_b_end(pilot: Pilot) -> None:
    """The bridge over the pit, with a bonus fragment above its second hole, through gate B."""
    pilot.jump(12, steer=24); pilot.stop()        # over the first hole
    pilot.run_to(856); pilot.stop()
    pilot.jump(60, steer=20); pilot.stop()        # over the second hole, bonus fragment
    pilot.run_to(904); pilot.stop()
    pilot.jump(12, steer=24); pilot.stop()        # over the third hole
    pilot.drop_off()
    pilot.open_gate(2)


def section_c(pilot: Pilot) -> None:
    """"Regression suite": up the tiers past the fast bots, the Gap-6, the exit."""
    pilot.run_to(1008); pilot.stop()
    pilot.jump(60, steer=10); pilot.stop()        # up onto tier 2, before the floor bot
    pilot.wait(lambda: pilot.bot(1072).velocity_x > 0 and pilot.bot(1072).x > 1110, "tier 3's bot to run off")
    pilot.jump(60, steer=14); pilot.stop()        # up onto tier 3
    pilot.pass_bot(1072, hold=30, distance=40)
    pilot.run_to(1214); pilot.stop()
    pilot.jump(60)                                # Gap-6, the longest jump, onto the exit block
    pilot.reach_exit()


def play_level_2(pilot: Pilot, low_route: bool = False) -> None:
    """The whole level, through section B on the high or the low route."""
    section_a(pilot)
    if low_route:
        section_b_low_route(pilot)
    else:
        section_b_high_route(pilot)
    section_b_end(pilot)
    section_c(pilot)


@pytest.mark.level(LEVEL_2)
@pytest.mark.parametrize("start_delay", [0, 5])
@pytest.mark.parametrize("route", ["high", "low"])
def test_level_2_can_be_finished(game, route, start_delay):
    # Two start times put the bots in other places: the route must not depend on luck.
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.frames(start_delay)
    lines = game.track_frame_lines()
    sprites_per_line = game.track_sprites_per_line()
    play_level_2(Pilot(game), low_route=(route == "low"))
    assert game.state() == LEVEL_COMPLETE and game.lives() == 3
    assert game.level().section_collected == [5, 4 if route == "high" else 3, 0]
    worst = max(lines)
    assert game.u8("_lag_frames") == 0, f"lag frames; the worst update ended at line {worst}"
    assert worst <= FRAME_LINE_LIMIT, f"the worst update ended at line {worst} of {LINES_PER_FRAME}"
    assert max(sprites_per_line) <= SPRITES_PER_LINE, f"{max(sprites_per_line)} sprites on one screen line"


@pytest.mark.parametrize("level, next_level", [(LEVEL_1, LEVEL_2), (LEVEL_2, LEVEL_1),
                                               (LEVEL_GREYBOX_TEST, LEVEL_GREYBOX_TEST)])
def test_level_complete_goes_on_to_the_next_level_with_full_lives(game, level, next_level):
    # After the last level comes level 1 again (no ending yet); a test room starts over.
    game.load_level(level)
    game.set_lives(1)
    game.set_state(LEVEL_COMPLETE)
    game.frames(200)                                    # the flashes, then the summary
    game.press("start")
    game.wait_for_state(COREGAMELOOP, timeout=200)
    assert game.u8("_level_index") == next_level and game.lives() == 3

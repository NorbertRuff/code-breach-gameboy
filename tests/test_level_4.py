"""Level 4, the Network Hub (levels/level_4.tmj): a replay plays it from the start to the
exit elevator without losing a life, collecting all 13 fragments, within the frame budget
and the sprites-per-line limit. Level 1's test (test_level_1.py) explains the Pilot moves
and the x values; a lift is named by its center x (Tiled x + 12)."""

import pytest

from gamelib import COREGAMELOOP, LEVEL_4, LEVEL_COMPLETE, LINES_PER_FRAME, Pilot
from test_frame_budget import FRAME_LINE_LIMIT
from test_level_1 import SPRITES_PER_LINE

LIFT = 1284   # the lift over the spike pit in section C


def section_a(pilot: Pilot) -> None:
    """"Patch panel": spike strips, the step, the spike pit's shelf, the bot, 5 fragments."""
    pilot.run_to(60); pilot.jump(20, steer=36); pilot.stop()          # over the first strip, fragment 1
    pilot.run_to(128); pilot.stop(); pilot.jump(30, steer=40); pilot.stop()   # over the 3-wide strip
    pilot.jump(20, steer=20); pilot.stop()                            # onto the step, fragment 2
    pilot.run_to(236); pilot.stop(); pilot.jump(20, steer=30); pilot.stop()   # off it, over the spikes
    pilot.jump(30, steer=28); pilot.stop()                            # onto the pit's shelf, fragment 3
    pilot.jump(40, direction=""); pilot.stop()                        # bonus fragment 4 above it
    pilot.run_to(346); pilot.stop(); pilot.jump(20, steer=24); pilot.stop()   # off to the far side
    pilot.pass_bot(408)                                               # fragment 5 over the bot
    pilot.open_gate(1)


def section_b(pilot: Pilot) -> None:
    """"Router farm": a low hop between the hanging hazards, the packet bridge, the fast bot."""
    pilot.run_to(544); pilot.jump(10, steer=30); pilot.stop()         # fragment 6 over the strip
    pilot.run_to(624); pilot.stop(); pilot.jump(30, steer=24); pilot.stop()   # bridge shelf 1
    pilot.run_to(668); pilot.stop(); pilot.jump(30, steer=30); pilot.stop()   # shelf 2
    pilot.run_to(724); pilot.stop(); pilot.jump(40, direction=""); pilot.stop()   # fragments 7 and 8
    pilot.run_to(732); pilot.stop(); pilot.jump(14, steer=30); pilot.stop()   # shelf 3
    pilot.run_to(796); pilot.stop(); pilot.jump(20, steer=30); pilot.stop()   # shelf 4, fragment 9
    pilot.run_to(880); pilot.stop()
    pilot.pass_bot(912)                                               # fragment 10 over the fast bot
    pilot.open_gate(2)


def section_c(pilot: Pilot) -> None:
    """"Backbone": onto the shelf over the chaser, over it and the spikes, the lift, the exit."""
    pilot.run_to(1080); pilot.stop(); pilot.jump(30, steer=14); pilot.stop()  # the shelf, fragment 11
    pilot.run_to(1100); pilot.stop(); pilot.run_to(1128); pilot.jump(36)       # over the chaser and spikes
    pilot.run_to(1224); pilot.stop()
    pilot.wait_for_lift(LIFT, at_top=False, frames_left=24)
    pilot.jump(16, steer=34); pilot.stop()
    assert pilot.on_lift(LIFT), pilot.where()
    pilot.ride_to_top(LIFT)                                           # fragment 12 at its highest place
    pilot.jump(20, steer=48); pilot.stop()                            # onto the top block
    pilot.run_to(1360); pilot.stop(); pilot.jump(20)                  # fragment 13 over the last strip
    pilot.reach_exit()


def play_level_4(pilot: Pilot) -> None:
    section_a(pilot)
    section_b(pilot)
    section_c(pilot)


@pytest.mark.level(LEVEL_4)
@pytest.mark.parametrize("start_delay", [0, pytest.param(5, marks=pytest.mark.full)])
def test_level_4_can_be_finished(game, start_delay):
    # Two start times put the bots and the lift in other places: the route must not depend
    # on luck.
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.frames(start_delay)
    frame_lines = game.track_frame_lines()
    sprites_per_line = game.track_sprites_per_line()
    play_level_4(Pilot(game))
    assert game.state() == LEVEL_COMPLETE and game.lives() == 3
    assert game.level().section_collected[:3] == [5, 5, 3], "not every fragment collected"
    worst = max(frame_lines)
    assert game.u8("_lag_frames") == 0, f"lag frames; the worst update ended at line {worst}"
    assert worst <= FRAME_LINE_LIMIT, f"the worst update ended at line {worst} of {LINES_PER_FRAME}"
    assert max(sprites_per_line) <= SPRITES_PER_LINE, f"{max(sprites_per_line)} sprites on one screen line"

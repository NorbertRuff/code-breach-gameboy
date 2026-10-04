"""Level 3, the AI Research Wing (levels/level_3.tmj): a replay plays it from the start to
the exit elevator without losing a life, collecting all 13 fragments, within the frame
budget and the sprites-per-line limit. Level 1's test (test_level_1.py) explains the
Pilot moves and the x values; a lift is named by its center x (Tiled x + 12)."""

import pytest

from gamelib import COREGAMELOOP, LEVEL_3, LEVEL_COMPLETE, LINES_PER_FRAME, Pilot
from test_frame_budget import FRAME_LINE_LIMIT
from test_level_1 import SPRITES_PER_LINE

L1, L2, L3, L4, L5, L6, L7, L8 = 76, 188, 252, 380, 716, 972, 1076, 1164   # lift centers


def section_a(pilot: Pilot) -> None:
    """"Elevator lobby": lifts up to the shelves over bot 1, the lift in the pit, 5 fragments."""
    pilot.board_lift(L1)
    pilot.ride_to_top(L1)
    pilot.run_to(116)                             # fragment 1 on the shelf
    pilot.run_to(156); pilot.stop()               # off the shelf, fragment 2 on the floor
    pilot.board_lift(L2)
    pilot.ride_to_top(L2)
    pilot.run_to(222); pilot.stop()               # onto the shelf between L2 and L3
    pilot.wait_for_lift(L3, at_top=False, frames_left=29)
    pilot.run_to(L3); pilot.stop()                # down onto L3, waiting at its bottom
    pilot.ride_to_top(L3)
    pilot.run_to(292)                             # fragment 3 on the high shelf
    pilot.run_to(326); pilot.stop()               # off its end, to the pit's edge
    pilot.wait_for_lift(L4, at_top=False, frames_left=24)
    pilot.jump(14, steer=22); pilot.stop()        # onto L4 in the pit
    assert pilot.on_lift(L4), pilot.where()
    pilot.ride_to_top(L4)                         # fragment 4 over its highest place
    pilot.jump(30)                                # off to the far side
    pilot.run_to(444); pilot.stop()               # fragment 5
    pilot.open_gate(1)


def section_b(pilot: Pilot) -> None:
    """"Neural lab": over chaser 1 from its shelf, up L5 over chaser 2, 5 fragments."""
    pilot.run_to(548); pilot.stop()
    pilot.jump(30, steer=10)                      # onto the shelf, chaser 1 stays below
    pilot.run_to(580)                             # fragment 6
    pilot.run_to(596)
    pilot.jump(30)                                # over chaser 1, past its range
    pilot.board_lift(L5)
    pilot.ride_to_top(L5)                         # fragment 7 at its highest place
    pilot.run_to(764)                             # fragment 8 on the high shelf
    pilot.drop_off()                              # past chaser 2's range
    pilot.run_to(836); pilot.run_to(868)          # fragments 9 and 10
    pilot.open_gate(2)


def section_c(pilot: Pilot) -> None:
    """"Training core": lifts over chaser 3, up the shaft to the top block, over bot 2 to the exit."""
    pilot.board_lift(L6)
    pilot.ride_to_top(L6)
    pilot.run_to(1020)                            # fragment 11 on the shelf over chaser 3
    pilot.run_to(1040); pilot.stop()
    pilot.wait_for_lift(L7, at_top=False, frames_left=29)
    pilot.run_to(L7); pilot.stop()                # off the shelf onto L7
    pilot.ride_to_top(L7)
    pilot.run_to(1130); pilot.drop_off()          # off the high shelf, past chaser 3's range
    pilot.board_lift(L8)
    pilot.ride_to_top(L8)                         # fragment 12 at its highest place
    pilot.jump(20); pilot.stop()                  # onto the top block, fragment 13
    pilot.pass_bot(1240)                          # over bot 2
    pilot.reach_exit()


def play_level_3(pilot: Pilot) -> None:
    section_a(pilot)
    section_b(pilot)
    section_c(pilot)


@pytest.mark.level(LEVEL_3)
@pytest.mark.parametrize("start_delay", [0, pytest.param(5, marks=pytest.mark.full)])
def test_level_3_can_be_finished(game, start_delay):
    # Two start times put the lifts in other places: the route must not depend on luck.
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.frames(start_delay)
    frame_lines = game.track_frame_lines()
    sprites_per_line = game.track_sprites_per_line()
    play_level_3(Pilot(game))
    assert game.state() == LEVEL_COMPLETE and game.lives() == 3
    assert game.level().section_collected[:3] == [5, 5, 3], "not every fragment collected"
    worst = max(frame_lines)
    assert game.u8("_lag_frames") == 0, f"lag frames; the worst update ended at line {worst}"
    assert worst <= FRAME_LINE_LIMIT, f"the worst update ended at line {worst} of {LINES_PER_FRAME}"
    assert max(sprites_per_line) <= SPRITES_PER_LINE, f"{max(sprites_per_line)} sprites on one screen line"

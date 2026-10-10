"""Level 1, the Server Room (levels/level_1.tmj): a replay plays it from the start to the
exit elevator without losing a life, within the frame budget and the sprites-per-line limit,
and the elevator opens.

The route is a list of Pilot moves (gamelib.py) that watch the player, so it survives
small layout changes. x values are world pixels: Tiled's x, or column * 8. Bots are
named by the left edge of their patrol rectangle in Tiled.
"""

import sys
from pathlib import Path

import pytest

from gamelib import COREGAMELOOP, LEVEL_1, LEVEL_COMPLETE, LINES_PER_FRAME, Pilot
from test_frame_budget import FRAME_LINE_LIMIT

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import tiled2c  # noqa: E402

SPRITES_PER_LINE = 10   # the hardware draws at most 10 sprites on one screen line


def section_a(pilot: Pilot) -> None:
    """"Boot sequence": all 4 fragments, through gate A."""
    pilot.run_to(50); pilot.jump(1)          # 1-tile step: a tap
    pilot.jump(8)                            # 2-tile step, fragment 1
    pilot.run_to(124); pilot.jump(20)        # rack stair, two steps at once
    pilot.jump(20)                           # its top
    pilot.run_to(196); pilot.stop()          # back for fragment 2
    pilot.jump(30, steer=20)                 # alcove, fragment 3
    pilot.stop(); pilot.drop_off()
    pilot.jump(20, steer=14)                 # Gap-3 cable trench
    pilot.run_to(296)
    pilot.pass_bot(312)                      # patrol lane
    pilot.open_gate(1)                       # fragment 4 on the way


def section_b_high_route(pilot: Pilot) -> None:
    """"Rack aisles", the high route over the rack tops: 2 fragments."""
    pilot.run_to(420); pilot.jump(20)        # up both steps onto the plateau
    pilot.stop()
    pilot.jump(30)                           # to the first rack top
    pilot.jump(30)                           # fragment 5
    pilot.jump(30, steer=24)                 # the next rack top
    pilot.stop(); pilot.run_to(600)          # back for fragment 6
    pilot.jump(20)                           # down to where the routes merge


def section_b_low_route(pilot: Pilot) -> None:
    """The low route on the floor under the rack tops: 2 bots, 2 fragments."""
    pilot.run_to(420); pilot.jump(20)        # up both steps onto the plateau
    pilot.stop()
    pilot.drop_off()                         # down to the floor lane
    pilot.pass_bot(536)                      # under a high rack top: room to jump it
    pilot.pass_bot(592)                      # fragment on the way
    pilot.jump(10, steer=12)                 # onto the stair where the routes merge, fragment
    pilot.jump(12, steer=12)


def section_b_end(pilot: Pilot) -> None:
    """The middle shelf (a fragment) and the bot's shelf, through gate B."""
    pilot.run_to(680); pilot.stop()
    pilot.jump(30, steer=26)                 # Gap-3 onto the middle shelf, fragment
    pilot.run_to(724)                        # to its right end
    pilot.wait_for_bot(768, 776, 788)        # the next shelf's bot walks away
    pilot.jump(12)                           # Gap-3 onto its shelf, behind it
    pilot.pass_bot(768)
    pilot.drop_off()
    pilot.open_gate(2)


def section_c(pilot: Pilot) -> None:
    """"Core cooling": the bot stair, the longest jump and the final run, 4 fragments."""
    pilot.run_to(920)
    pilot.wait_for_bot(960, 968, 976)        # the first step's bot walks away
    pilot.jump(12, steer=14)                 # onto the step's safe end
    pilot.pass_bot(960, hold=20)             # over it to the second step, fragment
    pilot.run_to(1020)
    pilot.wait_for_bot(1056, 1064, 1072)     # the third step's bot walks away
    pilot.jump(20, steer=14)                 # onto the step's safe end
    pilot.pass_bot(1056, hold=12)
    pilot.jump(30)                           # Gap-5, the longest jump, fragment
    pilot.run_to(1196)                       # down to the final run
    pilot.pass_bot(1208)                     # onto crate 1, fragment
    pilot.pass_bot(1272)                     # onto crate 2
    pilot.stop(); pilot.jump(30, steer=20)   # the last bonus fragment
    pilot.reach_exit()


def play_level_1(pilot: Pilot, low_route: bool = False) -> None:
    """The whole level, through section B on the high or the low route. Collects 4, 3
    and 4 fragments either way."""
    section_a(pilot)
    if low_route:
        section_b_low_route(pilot)
    else:
        section_b_high_route(pilot)
    section_b_end(pilot)
    section_c(pilot)


@pytest.mark.level(LEVEL_1)
@pytest.mark.parametrize("start_delay", [0, pytest.param(5, marks=pytest.mark.full)])
@pytest.mark.parametrize("route", ["high", "low"])
def test_level_1_can_be_finished(game, route, start_delay):
    # Two start times put the bots in other places: the route must not depend on luck.
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.frames(start_delay)
    lines = game.track_frame_lines()
    sprites_per_line = game.track_sprites_per_line()
    play_level_1(Pilot(game), low_route=(route == "low"))
    assert game.state() == LEVEL_COMPLETE and game.lives() == 3
    assert game.level().section_collected == [4, 3, 4]
    worst = max(lines)
    assert game.u8("_lag_frames") == 0, f"lag frames; the worst update ended at line {worst}"
    assert worst <= FRAME_LINE_LIMIT, f"the worst update ended at line {worst} of {LINES_PER_FRAME}"
    assert max(sprites_per_line) <= SPRITES_PER_LINE, f"{max(sprites_per_line)} sprites on one screen line"
    assert_elevator_opens(game)


def assert_elevator_opens(game) -> None:
    """At level complete the closed elevator around the exit (its bottom-left tile) turns
    into the tileset's open one, with its palettes on GBC."""
    level = tiled2c.load_level(ROOT / "levels" / "level_1.tmj")
    exit_tile = next(e for e in level.entities if e.kind == "exit")
    column, row = exit_tile.x // 8, exit_tile.y // 8
    cells = [(column + i % 2, row - 3 + i // 2) for i in range(tiled2c.ELEVATOR_TILES)]
    game.frames(2)
    shown = [game.bkg_tile(x, y) for x, y in cells]
    assert shown == level.tileset.named("exit_open"), f"elevator tiles {shown}"
    if game.mode == "cgb":
        attributes = [game.bkg_attribute(x, y) for x, y in cells]
        assert attributes == [level.tileset.palette[t] for t in shown]


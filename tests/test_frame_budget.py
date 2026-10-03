"""The frame budget (roadmap M3): every gameplay update must end before the next VBlank.

An update that runs past VBlank leaves a frame without one: a lag frame. The ROM counts
them in lag_frames (src/main.c). The route test also measures how many screen lines each
update takes, to keep a margin for what M3 adds (entities, parallax) and M6 (music).
"""

from gamelib import COREGAMELOOP, LINES_PER_FRAME

# The worst update may end this many lines after VBlank starts, of LINES_PER_FRAME. The
# rest is a margin: PyBoy's timing is close to the hardware's, but not exact.
FRAME_LINE_LIMIT = 140

# Through levels/greybox_test.tmj from the start to the exit without losing a life, as
# (frames, buttons held). Fragment pickups, the head bump and the columns drawn on both
# sides are the expensive updates. The wait times the jump over the floor bot.
GREYBOX_ROUTE = [
    (24, "a right"),     # onto platform A, two fragments
    (40, "right"),       # off it, a fragment on the floor
    (80, "right"),       # along the floor, two more, to under platform D
    (12, "a right"),     # head bump under D
    (30, "right"),
    (40, "left"),        # back left: columns are drawn on the left
    (66, "right"),
    (26, "a right"),     # over the pit onto the one-way shelf, a fragment
    (18, "right"),
    (6, "a right"),      # hop off the shelf over the spikes, a fragment
    (24, "right"),
    (52, ""),            # wait for the floor bot to turn
    (20, "a right"),     # over the bot
    (16, "right"),
    (12, "a right"),     # up the two steps
    (12, "right"),
    (12, "a right"),
    (30, "right"),       # onto the exit
]
EXIT_X = 62 * 8          # the exit marker's column


def test_greybox_route_stays_within_the_frame_budget(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    lines = game.track_frame_lines()
    game.play_route(GREYBOX_ROUTE)
    assert game.lives() == 3 and game.player().px >= EXIT_X, "the route no longer reaches the exit"
    worst = max(lines)
    assert game.u8("_lag_frames") == 0, f"lag frames; the worst update ended at line {worst}"
    assert worst <= FRAME_LINE_LIMIT, f"the worst update ended at line {worst} of {LINES_PER_FRAME}"


def test_lag_counter_counts_the_frames_without_an_update(game):
    # A far move makes the camera jump, and drawing every column in one update takes
    # several frames. The ROM's count must match the frames PyBoy ran without an update.
    game.start_on_open_ground()
    updates = []
    game.pyboy.hook_register(None, "_main_game_loop", lambda calls: calls.append(1), updates)
    before = game.u8("_lag_frames")
    game.place_player(490, 103)
    lag = game.u8("_lag_frames") - before
    assert lag >= 3
    assert lag == game.PLACE_FRAMES - len(updates)

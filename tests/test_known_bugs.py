"""Known bugs from docs/PROJECT_STATE.md, written as tests that fail today.

Mark each test xfail with strict=True: while the bug exists the run stays green, and
once the bug is fixed the test passes, which fails the run (see pytest.ini). The fix
then moves the test to test_regressions.py without the marker.
"""

import pytest

from gamelib import COREGAMELOOP
from test_frame_budget import GREYBOX_ROUTE


# Bug 19 ---------------------------------------------------------------------------

@pytest.mark.xfail(strict=True, reason="bug 19: a pickup in an update that draws a column is a lag frame")
def test_pickup_while_a_column_is_drawn_is_no_lag_frame(game):
    # Running right along the floor from the start, the fragment at column 12 is picked
    # up in the same update that draws column 23.
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.play_route(GREYBOX_ROUTE[:3])
    assert game.u8("_lag_frames") == 0

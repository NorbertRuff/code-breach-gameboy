"""Known bugs from docs/PROJECT_STATE.md, written as tests that fail today.

Mark each test xfail with strict=True: while the bug exists the run stays green, and
once the bug is fixed the test passes, which fails the run (see pytest.ini). The fix
then moves the test to test_regressions.py without the marker.

No open bugs have a test right now: bug 19 was fixed in M3.
"""

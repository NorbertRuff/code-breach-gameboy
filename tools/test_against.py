"""Run the current tests against the ROM built from an older commit.

Use it to prove a regression test really catches its bug: the test must fail on the
commit without the fix and pass on the working tree.

    .venv/Scripts/python.exe tools/test_against.py HEAD -k test_name
    .venv/Scripts/python.exe tools/test_against.py <commit> tests/test_regressions.py

The commit is exported with "git archive" (read-only: no checkout, no branch change)
into a temporary folder, built there with this repo's gbdk/, and the tests in the
working tree run against that build (APEXLAB_OBJ). Exit code: pytest's (non-zero means
some test failed there).
"""

from __future__ import annotations

import io
import os
import shutil
import subprocess
import sys
import tarfile
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def main() -> int:
    if len(sys.argv) < 2 or sys.argv[1] in ("-h", "--help"):
        print(__doc__)
        return 2
    rev, pytest_args = sys.argv[1], sys.argv[2:]
    keep = "--keep" in pytest_args
    pytest_args = [a for a in pytest_args if a != "--keep"]

    folder = Path(tempfile.mkdtemp(prefix="apexlab_rev_"))
    try:
        archive = subprocess.run(["git", "archive", "--format=tar", rev], cwd=ROOT,
                                 check=True, capture_output=True).stdout
        with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
            tar.extractall(folder, filter="data")
        print(f"exported {rev} to {folder}")

        gbdk = (ROOT / "gbdk").as_posix() + "/"
        build = subprocess.run(["make", f"GBDK_HOME={gbdk}", "all", "savetest"],
                               cwd=folder, capture_output=True, text=True)
        if build.returncode != 0:
            print(build.stdout[-2000:], build.stderr[-2000:])
            print(f"build of {rev} failed")
            return 3

        env = dict(os.environ, APEXLAB_OBJ=str(folder / "obj"))
        print(f"running tests against {rev}: pytest {' '.join(pytest_args)}")
        return subprocess.run([sys.executable, "-m", "pytest", "-p", "no:cacheprovider",
                               *pytest_args], cwd=ROOT, env=env).returncode
    finally:
        if keep:
            print(f"kept {folder}")
        else:
            shutil.rmtree(folder, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())

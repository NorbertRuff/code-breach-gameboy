"""Compare two folders of screenshots pixel by pixel, e.g. before and after a change.

    .venv/Scripts/python.exe tools/shots.py --out tests/out/shots_before   (before the change)
    make shots                                                             (after the change)
    .venv/Scripts/python.exe tools/compare_shots.py tests/out/shots_before tests/out/shots

Compares every PNG found under the first folder (including subfolders such as dmg/ and
cgb/) with the same path under the second. For each difference it writes
diff_<name>.png next to the "after" picture: the after picture with changed pixels in red.
Exit code 1 if anything differs, 0 if all pictures are identical.
"""

from __future__ import annotations

import sys
from pathlib import Path

from PIL import Image, ImageChops


def main() -> int:
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    before_root, after_root = Path(sys.argv[1]), Path(sys.argv[2])
    changed = 0
    for before in sorted(before_root.rglob("*.png")):
        if before.name.startswith("diff_"):
            continue
        rel = before.relative_to(before_root)
        after = after_root / rel
        if not after.exists():
            print(f"{rel}: missing in {after_root}")
            changed += 1
            continue
        a = Image.open(before).convert("RGB")
        b = Image.open(after).convert("RGB")
        if a.size != b.size:
            print(f"{rel}: size {a.size} -> {b.size}")
            changed += 1
            continue
        box = ImageChops.difference(a, b).getbbox()
        if box is None:
            print(f"{rel}: identical")
            continue
        changed += 1
        mask = ImageChops.difference(a, b).convert("L").point(lambda v: 255 if v else 0)
        marked = b.copy()
        marked.paste((255, 0, 0), mask=mask)
        marked.save(after.parent / f"diff_{after.name}")
        print(f"{rel}: DIFFERENT in {box} -> {after.parent / ('diff_' + after.name)}")
    print(f"{changed} picture(s) differ")
    return 1 if changed else 0


if __name__ == "__main__":
    sys.exit(main())

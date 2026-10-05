"""Passwords of ApexLab, the same as src/game_state/progress.c (DECISIONS.md "Saves").

    python tools/password.py           the password of each level, plain and with every mark before it
    python tools/password.py BCDF      what a password holds

A password is 4 letters for 2 bytes: the progress (bits 0-2 the furthest level, 0 for
level 1; bits 3-6 the 100% marks of levels 1-4; bit 7 always 0) and a check byte mixed
from it. The letters hold the check's high half, the progress's low half, the check's low
half, the progress's high half.
"""

import sys

LETTERS = "BCDFGHJKLMNPRTWX"
LEVELS = 5
MARKED_LEVELS = 4    # the boss level has no 100% mark
UNUSED_BIT = 0x80


def check_byte(progress: int) -> int:
    mixed = (progress * 0x9D + 0x4B) & 0xFF
    return ((mixed << 3) | (mixed >> 5)) & 0xFF


def make(level: int, perfect: int = 0) -> str:
    """The password for the furthest level (0 = level 1) and the marks (bit n = level n)."""
    progress = level | (perfect << 3)
    check = check_byte(progress)
    return "".join(LETTERS[n] for n in (check >> 4, progress & 0x0F, check & 0x0F, progress >> 4))


def read(text: str):
    """(level, perfect) of a password, or None if the game refuses it."""
    if len(text) != 4 or any(c not in LETTERS for c in text.upper()):
        return None
    values = [LETTERS.index(c) for c in text.upper()]
    check, progress = (values[0] << 4) | values[2], (values[3] << 4) | values[1]
    level, perfect = progress & 0x07, (progress >> 3) & 0x0F
    if check != check_byte(progress) or progress & UNUSED_BIT or level >= LEVELS or perfect >> level:
        return None
    return level, perfect


def main() -> None:
    if len(sys.argv) > 1:
        held = read(sys.argv[1])
        if held is None:
            print("not a password")
            return
        level, perfect = held
        marks = [n + 1 for n in range(MARKED_LEVELS) if perfect >> n & 1]
        print(f"level {level + 1}, 100% marks {marks or 'none'}")
        return
    for level in range(LEVELS):
        line = f"level {level + 1}: {make(level)}"
        if level:
            line += f"   with the marks of levels 1-{min(level, MARKED_LEVELS)}: {make(level, (1 << min(level, MARKED_LEVELS)) - 1)}"
        print(line)


if __name__ == "__main__":
    main()

"""Where the frame time goes. Plays the frame budget route (tests/test_frame_budget.py) and
prints, for each part of a gameplay update, the screen line where it starts and how many
lines it takes, counted from the start of VBlank (154 lines per frame). Then the worst
update, part by part.

Run with "make budget" (DMG mode, greybox route), or:
    tools/frame_budget.py --mode cgb --route level1 --rom <file.gb>
The level1 to level4 routes are the replays that finish those levels
(tests/test_level_1.py to tests/test_level_4.py).

Each part runs until the next part in PARTS starts. A part that an update skips (the HUD
count without a pickup) adds its time to the part before it.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tests"))

from gamelib import COREGAMELOOP, LEVEL_1, LEVEL_2, LEVEL_3, LEVEL_4, LEVEL_GREYBOX_TEST, LINES_PER_FRAME, Game, Pilot, boot  # noqa: E402
from test_frame_budget import FRAME_LINE_LIMIT, GREYBOX_ROUTE  # noqa: E402
from test_level_1 import play_level_1  # noqa: E402
from test_level_2 import play_level_2  # noqa: E402
from test_level_3 import play_level_3  # noqa: E402
from test_level_4 import play_level_4  # noqa: E402

# Route name: the level it plays and how.
ROUTES = {
    "greybox": (LEVEL_GREYBOX_TEST, lambda game: game.play_route(GREYBOX_ROUTE)),
    "level1": (LEVEL_1, lambda game: play_level_1(Pilot(game))),
    "level2": (LEVEL_2, lambda game: play_level_2(Pilot(game))),
    "level3": (LEVEL_3, lambda game: play_level_3(Pilot(game))),
    "level4": (LEVEL_4, lambda game: play_level_4(Pilot(game))),
}

# C functions in the order an update calls them, and a label for each. The first starts
# the update; the last is the game waiting for the next VBlank.
PARTS = [
    ("main_game_loop", "input"),
    ("update_run_speed", "player: run speed"),
    ("try_jump", "player: jump"),
    ("apply_gravity", "player: gravity"),
    ("move_x", "player: move x"),
    ("move_y", "player: move y"),
    ("update_ground", "player: ground"),
    ("check_touched_tiles", "player: pickups, hazards"),
    ("update_animation", "player: animation"),
    ("update_view", "camera, streaming"),
    ("actors_spawn", "actors: spawn"),
    ("actors_update", "actors: update"),
    ("hud_update_count", "HUD count"),
    ("player_touches_actor", "actors: hit test"),
    ("draw_sprites", "sprite list"),
    ("wait_vbl_done", "end of update"),
]


def function_addresses(sym_file: Path) -> dict:
    """(bank, address) of every C function in the .sym file, by its C name. Global
    functions are listed as _name, static ones as F<file>$name$0$0."""
    found = {}
    for line in sym_file.read_text().splitlines():
        place, _, symbol = line.partition(" ")
        if symbol.startswith("_"):
            name = symbol[1:]
        elif symbol.startswith("F") and symbol.endswith("$0$0"):
            name = symbol.split("$")[1]
        else:
            continue
        bank, address = place.split(":")
        found.setdefault(name, (int(bank, 16), int(address, 16)))
    return found


def record_updates(rom: Path, mode: str, route: str) -> tuple[list, int]:
    """Plays the route. Returns, per update, the (label, line) of each part it reached
    (the first call only), and the lag frames the ROM counted."""
    level, play = ROUTES[route]
    pyboy = boot(rom, mode)
    game = Game(pyboy, mode, ROOT / "tests" / "out", "frame_budget", level)
    game.wait_for_state(COREGAMELOOP, timeout=300)
    updates, current = [], {}

    def at_part(label):
        if label == PARTS[0][1]:
            current.clear()
            current["vblanks"] = game.u8("_sys_time")
            current["parts"] = []
        if "parts" not in current or label in dict(current["parts"]):
            return
        current["parts"].append((label, game.lines_since_vblank(current["vblanks"])))
        if label == PARTS[-1][1]:
            updates.append(current.pop("parts"))

    addresses = function_addresses(rom.with_suffix(".sym"))
    for function, label in PARTS:
        if function not in addresses:
            print(f"(no function {function} in the ROM: part '{label}' left out)")
            continue
        pyboy.hook_register(*addresses[function], at_part, label)
    play(game)
    lag_frames = game.u8("_lag_frames")
    pyboy.stop(save=False)
    return updates, lag_frames


def print_report(updates: list, lag_frames: int, mode: str, route: str) -> None:
    """The table of parts, then the worst update part by part."""
    starts, takes = {}, {}
    for parts in updates:
        for (label, line), (_, next_line) in zip(parts, parts[1:] + [parts[-1]]):
            starts.setdefault(label, []).append(line)
            takes.setdefault(label, []).append(next_line - line)

    print(f"{mode} mode, {len(updates)} updates on the {route} route.")
    print(f"Screen lines from the start of VBlank: a frame has {LINES_PER_FRAME}, the tests allow {FRAME_LINE_LIMIT}.\n")
    print(f"{'part':26s} {'starts at':>14s} {'takes':>14s}   updates")
    print(f"{'':26s} {'avg':>7s}{'worst':>7s} {'avg':>7s}{'worst':>7s}")
    for _, label in PARTS:
        if label in starts:
            s, t = starts[label], takes[label]
            print(f"{label:26s} {sum(s) / len(s):7.1f}{max(s):7d} {sum(t) / len(t):7.1f}{max(t):7d}   {len(s)}")
    print(f"\nlag frames: {lag_frames}")

    number, worst = max(enumerate(updates), key=lambda u: u[1][-1][1])
    print(f"\nworst update: number {number}, ends at line {worst[-1][1]}")
    for (label, line), (_, next_line) in zip(worst, worst[1:] + [worst[-1]]):
        print(f"  {label:26s} {line:5d} {next_line - line:5d}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--mode", choices=("dmg", "cgb"), default="dmg")
    parser.add_argument("--route", choices=ROUTES, default="greybox")
    parser.add_argument("--rom", type=Path, default=ROOT / "obj" / "ApexLab.gb")
    args = parser.parse_args()
    print_report(*record_updates(args.rom, args.mode, args.route), args.mode, args.route)


if __name__ == "__main__":
    main()

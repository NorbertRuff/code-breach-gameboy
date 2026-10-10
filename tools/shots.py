"""Play fixed scripts through the greybox test level and the game's levels, and save
screenshots for visual review.

Run with "make shots". Writes, per mode (dmg, cgb):
  tests/out/shots/<mode>/NN_<name>.png   one picture per step in the greybox level
  tests/out/shots/<mode>_contact.png     all steps on one sheet, 2x, with labels
  tests/out/shots/<mode>_level_map.png   the whole 32x18 background map from VRAM, 2x
  tests/out/shots/<mode>_level_N/ and <mode>_level_N_contact.png: level N (1-4) played by
                                         its replay (tests/test_level_N.py), high route

The script uses the same helpers as the tests (tests/gamelib.py). Long ways are skipped
by moving the player (place_player), lives are lost with lose_life, and game over comes
from setting the lives to 1 first, as the tests do.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tests"))

from gamelib import COREGAMELOOP, LEVEL_1, LEVEL_2, LEVEL_3, LEVEL_4, Game, Pilot, boot  # noqa: E402
from test_level_1 import play_level_1  # noqa: E402
from test_level_2 import play_level_2  # noqa: E402
from test_level_3 import play_level_3  # noqa: E402
from test_level_4 import play_level_4  # noqa: E402

# The game's levels by number: the level index and the replay that plays it.
LEVELS = {1: (LEVEL_1, play_level_1), 2: (LEVEL_2, play_level_2), 3: (LEVEL_3, play_level_3),
          4: (LEVEL_4, play_level_4)}

OUT = ROOT / "tests" / "out" / "shots"
SHADES = [(232, 232, 232), (160, 160, 160), (88, 88, 88), (16, 16, 16)]
MAP_W, MAP_H = 32, 18     # background map tiles shown: the 32 that wrap, 18 screen rows


class Recorder:
    def __init__(self, game: Game, folder: Path):
        self.game = game
        self.folder = folder
        self.shots: list[tuple[str, Path]] = []
        folder.mkdir(parents=True, exist_ok=True)
        for old in folder.glob("*.png"):        # shots from an earlier run
            old.unlink()

    def shot(self, name: str) -> None:
        self.game.pyboy.tick(1, True)
        path = self.folder / f"{len(self.shots):02d}_{name}.png"
        self.game.pyboy.screen.image.convert("RGB").save(path)
        self.shots.append((name, path))


def decode_bg_tile(memory, index: int, bgp: int) -> Image.Image:
    """One 8x8 background tile from VRAM bank 0, in 0x8800 addressing (GBDK default)."""
    base = 0x9000 + index * 16 if index < 128 else 0x8800 + (index - 128) * 16
    data = memory[base:base + 16]
    img = Image.new("RGB", (8, 8))
    for row in range(8):
        lo, hi = data[row * 2], data[row * 2 + 1]
        for col in range(8):
            bit = 7 - col
            color = ((hi >> bit) & 1) << 1 | ((lo >> bit) & 1)
            img.putpixel((col, row), SHADES[(bgp >> (color * 2)) & 3])
    return img


def level_map(game: Game) -> Image.Image:
    """The background map (0x9800) as an image, MAP_W x MAP_H tiles. Levels wider than
    32 tiles stream through it, so it shows the columns drawn last."""
    memory = game.pyboy.memory
    bgp = game.reg("BGP") if game.mode == "dmg" else 0xE4
    cache: dict[int, Image.Image] = {}
    img = Image.new("RGB", (MAP_W * 8, MAP_H * 8))
    for ty in range(MAP_H):
        for tx in range(MAP_W):
            index = memory[0x9800 + ty * 32 + tx]
            if index not in cache:
                cache[index] = decode_bg_tile(memory, index, bgp)
            img.paste(cache[index], (tx * 8, ty * 8))
    draw = ImageDraw.Draw(img)
    for tx in range(0, MAP_W, 5):           # column numbers every 5 tiles
        draw.text((tx * 8 + 1, 1), str(tx), fill=(255, 0, 0))
    return img.resize((img.width * 2, img.height * 2), Image.NEAREST)


def contact_sheet(shots: list[tuple[str, Path]], columns: int = 4) -> Image.Image:
    cell_w, cell_h, label_h = 320, 288, 14
    rows = (len(shots) + columns - 1) // columns
    sheet = Image.new("RGB", (columns * cell_w, rows * (cell_h + label_h)), (40, 40, 40))
    draw = ImageDraw.Draw(sheet)
    for i, (name, path) in enumerate(shots):
        x, y = (i % columns) * cell_w, (i // columns) * (cell_h + label_h)
        draw.text((x + 4, y + 1), f"{i:02d} {name}", fill=(255, 255, 255))
        img = Image.open(path).convert("RGB").resize((cell_w, cell_h), Image.NEAREST)
        sheet.paste(img, (x, y + label_h))
    return sheet


def run_right_until(game: Game, x_px: int, jump_at: int = 0) -> None:
    """Run right until the player reaches x_px, jumping once at jump_at (0: no jump)."""
    game.pyboy.button_press("right")
    for _ in range(300):
        player = game.player()
        if player.px >= x_px:
            break
        if jump_at and jump_at <= player.px < jump_at + 4:
            game.pyboy.button_press("a")
        game.frames(1)
    game.pyboy.button_release("right")
    game.pyboy.button_release("a")


def run(mode: str, rom: Path) -> None:
    pyboy = boot(rom, mode)
    game = Game(pyboy, mode, OUT, mode)
    rec = Recorder(game, OUT / mode)

    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.frames(30)
    rec.shot("level_start")
    level_map(game).save(OUT / f"{mode}_level_map.png")

    # Platform A blocks the way at body height: jump onto it.
    game.pyboy.button_press("right")
    game.pyboy.button_press("a")
    game.frames(6)
    rec.shot("jump_rising")
    game.frames(14)
    rec.shot("jump_top")
    game.pyboy.button_release("a")
    game.frames(10)
    game.pyboy.button_release("right")
    game.frames(20)
    rec.shot("landed_on_platform")

    game.hold("right", 8)
    rec.shot("walking_off_platform")
    game.frames(20)
    rec.shot("standing_after_walk")

    game.hold("left", 10)
    game.frames(3)
    rec.shot("walking_left_into_wall")

    # Across the level: gate A (open with a fragment of section A), past the first 256 px,
    # over the pit, the one-way shelf, spikes. Only teleport forward: behind an opened gate
    # the camera stops, and columns behind it are not drawn.
    game.place_player(230, 119)
    rec.shot("gate_a_closed")
    run_right_until(game, 262)
    rec.shot("gate_a_opening")
    run_right_until(game, 290, jump_at=262)
    rec.shot("jump_over_pit")
    run_right_until(game, 336)
    game.frames(20)
    rec.shot("under_one_way_shelf")
    game.hold("a", 40)
    game.frames(20)
    rec.shot("on_one_way_shelf")
    game.place_player(350, 119)
    rec.shot("spikes_ahead")
    game.place_player(480, 103)
    game.frames(20)
    rec.shot("far_end_exit")

    # Fall out: lose a life, fade, restart at gate A, the last one opened.
    game.lose_life()
    rec.shot("lost_a_life")
    game.wait_for_state(COREGAMELOOP)
    rec.shot("respawn_at_gate_a")
    game.frames(2)
    rec.shot("respawn_invincible")

    # Into the exit elevator.
    game.place_player(480, 103)
    game.hold("right", 10)
    rec.shot("level_complete_flash")
    game.wait_for_state(COREGAMELOOP, timeout=600)
    game.frames(20)
    rec.shot("after_level_complete")

    game.set_lives(1)
    game.lose_life()
    game.frames(5)
    rec.shot("game_over_flash")
    game.frames(120)
    rec.shot("game_over_wait")
    game.answer_game_over()
    game.wait_for_state(COREGAMELOOP, timeout=600)
    game.frames(20)
    rec.shot("after_game_over")

    contact_sheet(rec.shots).save(OUT / f"{mode}_contact.png")
    pyboy.stop(save=False)
    print(f"{mode}: {len(rec.shots)} shots, contact sheet {OUT / f'{mode}_contact.png'}")


def run_level(mode: str, rom: Path, number: int) -> None:
    """A level played by its replay: a shot at the start, after every third landing, at
    each gate, and at the exit (the level-complete flash)."""
    index, play = LEVELS[number]
    pyboy = boot(rom, mode)
    game = Game(pyboy, mode, OUT, mode, index)
    rec = Recorder(game, OUT / f"{mode}_level_{number}")
    game.wait_for_state(COREGAMELOOP, timeout=300)
    rec.shot("start")
    landings = []

    def on_move(line: str) -> None:
        if line.startswith("gate"):
            rec.shot("gate_opened")
        elif line.startswith("landed"):
            landings.append(line)
            if len(landings) % 3 == 0:
                rec.shot(f"landing_{len(landings)}")

    play(Pilot(game, on_move))
    rec.shot("level_complete_flash")
    sheet = OUT / f"{mode}_level_{number}_contact.png"
    contact_sheet(rec.shots).save(sheet)
    pyboy.stop(save=False)
    print(f"{mode}: {len(rec.shots)} level {number} shots, contact sheet {sheet}")


def main() -> None:
    global OUT
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("rom", nargs="?", default=str(ROOT / "obj" / "ApexLab.gb"))
    parser.add_argument("--out", help="output folder (default tests/out/shots), e.g. "
                                      "tests/out/shots_before for a before/after comparison")
    args = parser.parse_args()
    if args.out:
        OUT = Path(args.out)
    for mode in ("dmg", "cgb"):
        run(mode, Path(args.rom))
        for number in LEVELS:
            run_level(mode, Path(args.rom), number)


if __name__ == "__main__":
    main()

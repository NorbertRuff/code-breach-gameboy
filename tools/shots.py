"""Play a fixed script through level 1 and save screenshots for visual review.

Run with "make shots". Writes, per mode (dmg, cgb):
  tests/out/shots/<mode>/NN_<name>.png   one picture per step
  tests/out/shots/<mode>_contact.png     all steps on one sheet, 2x, with labels
  tests/out/shots/<mode>_level_map.png   the whole 32x18 background map from VRAM, 2x

The script uses the same helpers as the tests (tests/gamelib.py). Steps that can't be
reached by input in level 1 (level complete, game over) are forced by writing the
game state, as the regression tests do.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tests"))

from gamelib import (COREGAMELOOP, GAMEPLAY_END, LEVEL_COMPLETE,  # noqa: E402
                     LEVEL_HEIGHT, LEVEL_WIDTH, Game, boot)

OUT = ROOT / "tests" / "out" / "shots"
SHADES = [(232, 232, 232), (160, 160, 160), (88, 88, 88), (16, 16, 16)]


class Recorder:
    def __init__(self, game: Game, folder: Path):
        self.game = game
        self.folder = folder
        self.shots: list[tuple[str, Path]] = []
        folder.mkdir(parents=True, exist_ok=True)

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
    """The background map (0x9800) as an image, LEVEL_WIDTH x LEVEL_HEIGHT tiles."""
    memory = game.pyboy.memory
    bgp = game.reg("BGP") if game.mode == "dmg" else 0xE4
    cache: dict[int, Image.Image] = {}
    img = Image.new("RGB", (LEVEL_WIDTH * 8, LEVEL_HEIGHT * 8))
    for ty in range(LEVEL_HEIGHT):
        for tx in range(LEVEL_WIDTH):
            index = memory[0x9800 + ty * 32 + tx]
            if index not in cache:
                cache[index] = decode_bg_tile(memory, index, bgp)
            img.paste(cache[index], (tx * 8, ty * 8))
    draw = ImageDraw.Draw(img)
    for tx in range(0, LEVEL_WIDTH, 5):           # column numbers every 5 tiles
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


def walk_until_hit(game: Game, rec: Recorder) -> None:
    lives = game.lives()
    game.pyboy.button_press("right")
    try:
        for _ in range(300):
            game.frames(1)
            if game.lives() != lives:
                break
    finally:
        game.pyboy.button_release("right")


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

    walk_until_hit(game, rec)
    rec.shot("hit_by_enemy")
    game.frames(4)
    rec.shot("respawn_invincible_a")
    game.frames(2)
    rec.shot("respawn_invincible_b")

    # The far end of the level, once the invincibility blink is over: put the player
    # past the ground bot, then walk.
    game.frames(90)
    game.place_player(200, 119)
    game.hold("right", 40)
    rec.shot("far_right")
    game.frames(30)
    rec.shot("far_right_standing")

    game.set_state(LEVEL_COMPLETE)
    game.frames(5)
    rec.shot("level_complete_flash")
    game.wait_for_state(COREGAMELOOP, timeout=600)
    game.frames(20)
    rec.shot("after_level_complete")

    game.set_lives(1)
    game.set_state(GAMEPLAY_END)
    game.frames(5)
    rec.shot("game_over_flash")
    game.frames(120)
    rec.shot("game_over_wait")
    game.wait_for_state(COREGAMELOOP, timeout=600)
    game.frames(20)
    rec.shot("after_game_over")

    contact_sheet(rec.shots).save(OUT / f"{mode}_contact.png")
    pyboy.stop(save=False)
    print(f"{mode}: {len(rec.shots)} shots, contact sheet {OUT / f'{mode}_contact.png'}")


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


if __name__ == "__main__":
    main()

"""tools/gbconvert.py: the art converter (docs/ART_PIPELINE.md section 4), on small
generated pictures."""

import sys
from pathlib import Path

import pytest
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import gbconvert  # noqa: E402
from gbconvert import ConvertError, convert, load_palettes, to_888  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
WHITE, LIGHT, DARK, BLACK = (31, 31, 31), (21, 21, 21), (10, 10, 10), (0, 0, 0)
RED, PINK = (31, 0, 0), (31, 20, 20)
BG = [[WHITE, LIGHT, DARK, BLACK], [PINK, RED, DARK, BLACK]]
SPRITE = [[WHITE, LIGHT, BLACK]]


def picture(rows: list[str], legend: dict, scale: int = 1) -> Image.Image:
    """A picture from text, one character per pixel ('.' transparent), at a scale."""
    image = Image.new("RGBA", (len(rows[0]), len(rows)))
    for y, row in enumerate(rows):
        for x, key in enumerate(row):
            image.putpixel((x, y), (0, 0, 0, 0) if key == "." else to_888(legend[key]) + (255,))
    return image.resize((image.width * scale, image.height * scale), Image.NEAREST)


def tiles(*keys: str) -> list[str]:
    """8 rows of 8x8 tiles side by side, each filled with one character."""
    return ["".join(key * 8 for key in keys)] * 8


def test_shrinks_an_upscaled_export_and_indexes_by_palette():
    result = convert(picture(tiles("w", "r"), {"w": WHITE, "r": RED}, scale=4), BG, sprite=False, budget=10)
    assert result.scale == 4 and result.image.size == (16, 8)
    assert result.tile_palettes == [0, 1]
    assert result.image.getpixel((0, 0)) == 0          # palette 0, color 0
    assert result.image.getpixel((8, 0)) == 4 + 1      # palette 1, color 1 (red)
    assert result.image.getpalette()[:12] == [v for c in BG[0] for v in to_888(c)]


def test_the_given_size_settles_an_ambiguous_scale():
    # 2 px features at 4x also fit 8x: only the size says which one the art is.
    rows = ["wwbb" * 4] * 16
    art = picture(rows, {"w": WHITE, "b": BLACK}, scale=4)
    assert convert(art, BG, sprite=False, budget=10).image.size == (8, 8)
    assert convert(art, BG, sprite=False, budget=10, size=(16, 16)).image.size == (16, 16)
    with pytest.raises(ConvertError, match="not a whole-number upscale of 24x24"):
        convert(art, BG, sprite=False, budget=10, size=(24, 24))


def test_a_few_off_colors_are_snapped():
    rows = tiles("w")
    rows[0] = "x" + rows[0][1:]                        # 1 of 64 pixels slightly off white
    result = convert(picture(rows, {"w": WHITE, "x": (30, 31, 31)}), BG, sprite=False, budget=10)
    assert 0 < result.snapped <= gbconvert.SNAP_LIMIT
    assert result.image.getpixel((0, 0)) == 0


SPRITE_TILE = ["." * 8] + ["..ll..dd"] * 7             # a tile and its mirror below it


def test_sprite_tiles_reuse_flipped_copies_and_keep_transparency():
    rows = SPRITE_TILE + [row[::-1] for row in SPRITE_TILE]
    result = convert(picture(rows, {"l": LIGHT, "d": BLACK}), SPRITE, sprite=True, budget=10)
    assert result.unique_tiles == 1
    assert result.image.getpixel((0, 0)) == 0          # transparent
    assert result.image.getpixel((2, 1)) == 2          # light: sprite colors start at 1


def test_one_pattern_in_two_palettes_is_one_tile():
    # The palette goes in the attribute map, so VRAM holds the pattern once (png2asset too).
    rows = ["wwllddbb" + "pprrddbb"] * 8
    legend = {"w": WHITE, "l": LIGHT, "d": DARK, "b": BLACK, "p": PINK, "r": RED}
    result = convert(picture(rows, legend), BG, sprite=False, budget=10)
    assert result.tile_palettes == [0, 1] and result.unique_tiles == 1


def test_background_tiles_may_not_reuse_flips():
    rows = ["wwwwbbbb"] * 8
    rows = [a + a[::-1] for a in rows]
    result = convert(picture(rows, {"w": WHITE, "b": BLACK}), BG, sprite=False, budget=10)
    assert result.unique_tiles == 2


RULE_BREAKS = {
    "not whole tiles": (["w" * 9] * 8, {"w": WHITE}, "whole number of 8x8 tiles"),
    "foreign colors": (tiles("g"), {"g": (0, 31, 0)}, "snap"),
    "five colors": (["wldbrwldbr"[:8]] * 8, {"w": WHITE, "l": LIGHT, "d": DARK, "b": BLACK, "r": RED},
                    "5 colors, at most 4"),
    "mixed palettes": (["wwwwrrrr"] * 8, {"w": WHITE, "r": RED}, "mixes palettes"),
}


@pytest.mark.parametrize("rows, legend, message", RULE_BREAKS.values(), ids=RULE_BREAKS.keys())
def test_rule_breaks_name_the_problem(rows, legend, message):
    with pytest.raises(ConvertError, match=message):
        convert(picture(rows, legend), BG, sprite=False, budget=10)


def test_tile_budget():
    with pytest.raises(ConvertError, match="3 unique tiles, the budget is 2"):
        convert(picture(tiles("w", "l", "d"), {"w": WHITE, "l": LIGHT, "d": DARK}), BG, sprite=False, budget=2)


def test_palettes_must_be_light_to_dark(tmp_path):
    path = tmp_path / "palettes.json"
    path.write_text('{"s": {"bg": [["#000000", "#555555", "#AAAAAA", "#FFFFFF"]]}}', encoding="utf-8")
    with pytest.raises(ConvertError, match="not ordered light to dark"):
        load_palettes(path, "s", "bg")


def test_project_palettes_load():
    path = ROOT / "graphics" / "palettes.json"
    assert len(load_palettes(path, "greybox", "bg")[0]) == 4
    assert len(load_palettes(path, "greybox", "sprite")[0]) == 3


def test_previews_show_gbc_colors_and_dmg_grays():
    result = convert(picture(tiles("r"), {"r": RED}), BG, sprite=False, budget=10)
    gbc, dmg = gbconvert.previews(result, sprite=False)
    assert gbc.size == (8 * gbconvert.PREVIEW_SCALE, 8 * gbconvert.PREVIEW_SCALE)
    assert gbc.getpixel((1, 1)) == to_888(RED)
    assert dmg.getpixel((1, 1)) == gbconvert.DMG_GRAYS[1]

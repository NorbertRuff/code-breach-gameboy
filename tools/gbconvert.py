"""Make an exported PNG fit the Game Boy, ready for png2asset (docs/ART_PIPELINE.md).

    python tools/gbconvert.py <asset name> <exported.png>

The asset's entry in graphics/assets.json says its type (bg or sprite), its palette set in
graphics/palettes.json, where the result goes, its tile budget, and its size in Game Boy
pixels ("size": [w, h], optional but safer). The script:
1. shrinks a whole-number upscale (4x exports) back to 1 pixel per Game Boy pixel: to the
   given size, or else at the largest scale that fits,
2. snaps every pixel to the nearest style-guide color (fails if too many change),
3. checks each 8x8 tile: at most 4 colors (sprites: 3 + transparent), all from one palette,
4. counts the unique tiles against the budget (sprites may reuse flipped tiles),
5. writes an indexed PNG: palettes in groups of 4, light to dark, for
   png2asset -keep_palette_order,
6. writes GBC and DMG previews at 4x with a tile grid to tests/out/previews/.
Errors name the tile and the colors at fault.
"""

from __future__ import annotations

import argparse
import json
import sys
from dataclasses import dataclass
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent.parent
TILE = 8
SNAP_LIMIT = 0.02                  # share of pixels the snap may change
PREVIEW_SCALE = 4
DMG_GRAYS = [(255, 255, 255), (170, 170, 170), (85, 85, 85), (0, 0, 0)]
TRANSPARENT_FILL = (255, 0, 255)   # palette entry 0 of a sprite palette, never drawn
PREVIEW_BACKDROP = (200, 230, 255)


class ConvertError(Exception):
    """The art breaks a hardware rule; the message says where."""


Color = tuple[int, int, int]       # 5 bits per channel, as the GBC stores it


@dataclass
class Result:
    image: Image.Image             # the indexed PNG for png2asset
    scale: int
    snapped: float                 # share of pixels the snap changed
    tile_palettes: list[int]       # palette per tile, row by row
    unique_tiles: int


def to_555(rgb) -> Color:
    return (rgb[0] >> 3, rgb[1] >> 3, rgb[2] >> 3)


def to_888(color: Color) -> tuple[int, int, int]:
    return tuple((c << 3) | (c >> 2) for c in color)


def luma(color: Color) -> float:
    return 0.299 * color[0] + 0.587 * color[1] + 0.114 * color[2]


def hex_color(color: Color) -> str:
    return "#{:02X}{:02X}{:02X}".format(*to_888(color))


def load_palettes(path: Path, set_name: str, kind: str) -> list[list[Color]]:
    """A palette set's background or sprite palettes from palettes.json, as GBC colors.
    Each palette is listed light to dark: background 4 colors, sprite 3 (0 is transparent)."""
    sets = json.loads(path.read_text(encoding="utf-8"))
    if set_name not in sets or kind not in sets[set_name]:
        raise ConvertError(f"{path.name} has no {kind} palettes in set {set_name!r}")
    size = 4 if kind == "bg" else 3
    palettes = []
    for number, hexes in enumerate(sets[set_name][kind]):
        palette = [to_555(bytes.fromhex(h.lstrip("#"))) for h in hexes]
        if len(palette) != size:
            raise ConvertError(f"{set_name} {kind} palette {number} has {len(palette)} colors, not {size}")
        if any(luma(a) < luma(b) for a, b in zip(palette, palette[1:])):
            raise ConvertError(f"{set_name} {kind} palette {number} is not ordered light to dark")
        palettes.append(palette)
    if len(palettes) > 8:
        raise ConvertError(f"{set_name} has {len(palettes)} {kind} palettes; the GBC has 8")
    return palettes


def downscale(image: Image.Image, scale: int):
    """The image at 1/scale, if every scale x scale block is one color; else None."""
    small = image.resize((image.width // scale, image.height // scale), Image.NEAREST)
    if scale == 1 or small.resize(image.size, Image.NEAREST).tobytes() == image.tobytes():
        return small
    return None


def shrink(image: Image.Image, size=None) -> tuple[Image.Image, int]:
    """Undo a whole-number upscale: to the given (width, height), or else the largest
    scale whose pixel blocks are all one color and that leaves whole 8x8 tiles. Art drawn
    with 2 px features fits half its size too, so give the size when you know it."""
    width, height = image.size
    if size:
        scale = width // size[0]
        if scale < 1 or (width, height) != (size[0] * scale, size[1] * scale) or downscale(image, scale) is None:
            raise ConvertError(f"{width}x{height} px is not a whole-number upscale of {size[0]}x{size[1]}")
        if size[0] % TILE or size[1] % TILE:
            raise ConvertError(f"{size[0]}x{size[1]} px is not a whole number of 8x8 tiles")
        return downscale(image, scale), scale
    for scale in range(min(width, height) // TILE, 0, -1):
        if width % (scale * TILE) == 0 and height % (scale * TILE) == 0:
            small = downscale(image, scale)
            if small is not None:
                return small, scale
    raise ConvertError(f"{width}x{height} px is not a whole number of 8x8 tiles at any whole scale")


def snap(image: Image.Image, palettes: list[list[Color]], sprite: bool):
    """Each pixel as its nearest palette color (None: transparent sprite pixel), and the
    share of pixels that changed."""
    colors = {c for palette in palettes for c in palette}
    nearest = {}
    grid, changed = [], 0
    for y in range(image.height):
        row = []
        for x in range(image.width):
            r, g, b, a = image.getpixel((x, y))
            if sprite and a < 128:
                row.append(None)
                continue
            color = to_555((r, g, b))
            if color not in nearest:
                nearest[color] = min(colors, key=lambda c: sum((p - q) ** 2 for p, q in zip(c, color)))
            changed += nearest[color] != color
            row.append(nearest[color])
        grid.append(row)
    return grid, changed / (image.width * image.height)


def tile_palettes(grid, palettes: list[list[Color]], sprite: bool) -> list[int]:
    """The palette of each 8x8 tile, row by row: the first one that has all its colors."""
    found = []
    for ty in range(len(grid) // TILE):
        for tx in range(len(grid[0]) // TILE):
            colors = {grid[ty * TILE + y][tx * TILE + x] for y in range(TILE) for x in range(TILE)} - {None}
            limit = 3 if sprite else 4
            where = f"tile at column {tx}, row {ty} (pixel {tx * TILE}, {ty * TILE})"
            if len(colors) > limit:
                raise ConvertError(f"{where} has {len(colors)} colors, at most {limit}: "
                                   f"{', '.join(sorted(map(hex_color, colors)))}")
            fits = [i for i, palette in enumerate(palettes) if colors <= set(palette)]
            if not fits:
                raise ConvertError(f"{where} mixes palettes: no palette has all of "
                                   f"{', '.join(sorted(map(hex_color, colors)))}")
            found.append(fits[0])
    return found


def indices(grid, palettes, tile_palette: list[int], sprite: bool) -> list[list[int]]:
    """The indexed PNG's pixels: palette * 4 + the color's place in it (sprites: 1-3,
    0 for transparent)."""
    columns = len(grid[0]) // TILE
    out = []
    for y, row in enumerate(grid):
        out_row = []
        for x, color in enumerate(row):
            p = tile_palette[(y // TILE) * columns + x // TILE]
            if color is None:
                out_row.append(p * 4)
            else:
                out_row.append(p * 4 + palettes[p].index(color) + (1 if sprite else 0))
        out.append(out_row)
    return out


def count_unique_tiles(pixels: list[list[int]], sprite: bool) -> int:
    """The tiles VRAM must hold: the palette is set per map cell or sprite, so tiles with
    the same pattern in different palettes are one tile. For sprites a flipped copy of a
    tile counts as the same tile too."""
    seen = set()
    for ty in range(len(pixels) // TILE):
        for tx in range(len(pixels[0]) // TILE):
            tile = tuple(tuple(i & 3 for i in pixels[ty * TILE + y][tx * TILE:(tx + 1) * TILE]) for y in range(TILE))
            versions = [tile]
            if sprite:
                mirrored = tuple(row[::-1] for row in tile)
                versions += [mirrored, tile[::-1], mirrored[::-1]]
            seen.add(min(versions))
    return len(seen)


def indexed_image(pixels: list[list[int]], palettes, sprite: bool) -> Image.Image:
    """A paletted PNG with the palettes in groups of 4, light to dark."""
    image = Image.new("P", (len(pixels[0]), len(pixels)))
    entries = []
    for palette in palettes:
        group = ([TRANSPARENT_FILL] if sprite else []) + [to_888(c) for c in palette]
        entries += [value for color in group for value in color]
    image.putpalette(entries)
    image.putdata([index for row in pixels for index in row])
    return image


def convert(image: Image.Image, palettes: list[list[Color]], sprite: bool, budget: int,
            size=None) -> Result:
    """Steps 1 to 5 of the module docstring. size: the art's (width, height) in Game Boy pixels."""
    small, scale = shrink(image.convert("RGBA"), size)
    grid, snapped = snap(small, palettes, sprite)
    if snapped > SNAP_LIMIT:
        raise ConvertError(f"the snap to the style-guide colors changed {snapped:.1%} of the pixels "
                           f"(at most {SNAP_LIMIT:.0%}): the export uses other colors")
    tile_palette = tile_palettes(grid, palettes, sprite)
    pixels = indices(grid, palettes, tile_palette, sprite)
    unique = count_unique_tiles(pixels, sprite)
    if unique > budget:
        raise ConvertError(f"{unique} unique tiles, the budget is {budget}")
    return Result(indexed_image(pixels, palettes, sprite), scale, snapped, tile_palette, unique)


def previews(result: Result, sprite: bool) -> tuple[Image.Image, Image.Image]:
    """GBC colors and DMG grays at PREVIEW_SCALE with a tile grid."""
    indexed = result.image
    palette = indexed.getpalette()
    gbc, dmg = Image.new("RGB", indexed.size), Image.new("RGB", indexed.size)
    for y in range(indexed.height):
        for x in range(indexed.width):
            index = indexed.getpixel((x, y))
            if sprite and index % 4 == 0:
                gbc.putpixel((x, y), PREVIEW_BACKDROP)
                dmg.putpixel((x, y), PREVIEW_BACKDROP)
                continue
            gbc.putpixel((x, y), tuple(palette[index * 3:index * 3 + 3]))
            dmg.putpixel((x, y), DMG_GRAYS[index % 4])
    scaled = []
    for picture in (gbc, dmg):
        big = picture.resize((picture.width * PREVIEW_SCALE, picture.height * PREVIEW_SCALE), Image.NEAREST)
        pen = ImageDraw.Draw(big)
        step = TILE * PREVIEW_SCALE
        for x in range(step, big.width, step):
            pen.line([(x, 0), (x, big.height)], fill=(255, 64, 64))
        for y in range(step, big.height, step):
            pen.line([(0, y), (big.width, y)], fill=(255, 64, 64))
        scaled.append(big)
    return scaled[0], scaled[1]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("asset", help="name in graphics/assets.json")
    parser.add_argument("png", type=Path, help="the exported PNG")
    args = parser.parse_args()
    try:
        assets = json.loads((ROOT / "graphics" / "assets.json").read_text(encoding="utf-8"))
        if args.asset not in assets:
            raise ConvertError(f"graphics/assets.json has no asset {args.asset!r}")
        entry = assets[args.asset]
        sprite = entry["type"] == "sprite"
        palettes = load_palettes(ROOT / "graphics" / "palettes.json", entry["palettes"], entry["type"])
        result = convert(Image.open(args.png), palettes, sprite, entry["budget"], entry.get("size"))
    except ConvertError as error:
        print(f"error: {args.asset}: {error}", file=sys.stderr)
        return 1

    out = ROOT / entry["out"]
    out.parent.mkdir(parents=True, exist_ok=True)
    if sprite:
        result.image.save(out, transparency=0)    # palette entry 0 is see-through
    else:
        result.image.save(out)
    preview_dir = ROOT / "tests" / "out" / "previews"
    preview_dir.mkdir(parents=True, exist_ok=True)
    gbc, dmg = previews(result, sprite)
    gbc.save(preview_dir / f"{args.asset}_cgb.png")
    dmg.save(preview_dir / f"{args.asset}_dmg.png")
    used = sorted(set(result.tile_palettes))
    print(f"{args.asset}: {result.image.width}x{result.image.height} px (export at {result.scale}x), "
          f"{result.snapped:.1%} of pixels snapped, {result.unique_tiles} unique tiles of {entry['budget']}, "
          f"palettes {used} -> {entry['out']}, previews in {preview_dir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

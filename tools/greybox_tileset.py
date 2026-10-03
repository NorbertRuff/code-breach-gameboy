"""Draw the greybox tileset: plain blocks for building levels before the art exists.

Writes graphics/tilesets/greybox.png (one row of 8x8 tiles, 4 shades, light to dark)
and graphics/tilesets/greybox.tsx, the Tiled tileset with each tile's class and
collision property. "make assets" runs this, then png2asset.

Each tile has its own shape, so it reads on DMG without color (ART_PIPELINE.md).
"""

from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "graphics" / "tilesets"

# Shades 0-3, light to dark (DMG order).
PALETTE = [(255, 255, 255), (170, 170, 170), (85, 85, 85), (0, 0, 0)]

# name, class (entity markers, used for Tiled tile objects), collision, pixels
TILES = [
    ("empty", "", "", [
        "10000000",
        "00000000",
        "00000000",
        "00000000",
        "00000000",
        "00000000",
        "00000000",
        "00000000",
    ]),
    ("solid", "", "solid", [
        "33333333",
        "30000013",
        "30111123",
        "30111123",
        "30111123",
        "30111123",
        "31222223",
        "33333333",
    ]),
    ("one_way", "", "one_way", [
        "33333333",
        "22222222",
        "03000030",
        "00300300",
        "00033000",
        "00000000",
        "00000000",
        "00000000",
    ]),
    ("hazard", "", "hazard", [
        "00000000",
        "00300030",
        "00300030",
        "03230323",
        "03230323",
        "32223222",
        "32223222",
        "33333333",
    ]),
    ("gate", "gate", "", [
        "33333333",
        "32033203",
        "32033203",
        "33333333",
        "32033203",
        "32033203",
        "32033203",
        "33333333",
    ]),
    ("exit", "exit", "", [
        "03333330",
        "31111113",
        "31222213",
        "31222213",
        "31222233",
        "31222213",
        "31222213",
        "33333333",
    ]),
    ("fragment", "fragment", "", [
        "00033000",
        "00311300",
        "03122130",
        "31222213",
        "31222213",
        "03122130",
        "00311300",
        "00033000",
    ]),
    ("player_start", "player_start", "", [
        "00333000",
        "03111300",
        "03111300",
        "00333000",
        "03222300",
        "30222030",
        "00303000",
        "03000300",
    ]),
]


def draw_png(path: Path) -> None:
    image = Image.new("P", (8 * len(TILES), 8))
    image.putpalette([c for rgb in PALETTE for c in rgb])
    for index, (_, _, _, rows) in enumerate(TILES):
        for y, row in enumerate(rows):
            for x, shade in enumerate(row):
                image.putpixel((index * 8 + x, y), int(shade))
    image.save(path)


def write_tsx(path: Path) -> None:
    lines = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        f'<tileset version="1.10" tiledversion="1.12.2" name="greybox" tilewidth="8" '
        f'tileheight="8" tilecount="{len(TILES)}" columns="{len(TILES)}">',
        f' <image source="greybox.png" width="{8 * len(TILES)}" height="8"/>',
    ]
    for index, (name, tile_class, collision, _) in enumerate(TILES):
        type_attr = f' type="{tile_class}"' if tile_class else ""
        lines.append(f' <tile id="{index}"{type_attr}>')
        lines.append("  <properties>")
        lines.append(f'   <property name="name" value="{name}"/>')
        if collision:
            lines.append(f'   <property name="collision" value="{collision}"/>')
        lines.append("  </properties>")
        lines.append(" </tile>")
    lines.append("</tileset>")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    draw_png(OUT / "greybox.png")
    write_tsx(OUT / "greybox.tsx")
    print(f"wrote {OUT / 'greybox.png'} and greybox.tsx ({len(TILES)} tiles)")


if __name__ == "__main__":
    main()

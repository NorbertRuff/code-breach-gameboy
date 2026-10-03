"""Draw the greybox parallax band: a placeholder for the 3-row band above the playfield
until the Server Room art (M5).

Writes graphics/bands/greybox_band.png: 256x24 px (32x3 tiles, it wraps around the
background map), 4 shades light to dark. Distant server racks in the light shades, so
the band reads as background and its slower scroll is easy to see. "make assets" runs
this, then png2asset.
"""

from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "graphics" / "bands" / "greybox_band.png"

WIDTH, HEIGHT = 256, 24
PALETTE = [(255, 255, 255), (170, 170, 170), (85, 85, 85), (0, 0, 0)]   # light to dark
BACK, BODY, EDGE, LED = 0, 1, 2, 3

# One 128 px stretch, drawn twice: (left x, rack height in px). Racks are 16 px wide.
RACKS = [(0, 24), (24, 16), (40, 24), (72, 16), (96, 24)]


def draw_rack(image: Image.Image, left: int, height: int) -> None:
    """A rack standing on the band's bottom row: an outline, slots every 4 px, and an
    LED in every other slot."""
    top = HEIGHT - height
    for y in range(top, HEIGHT):
        for x in range(left, left + 16):
            edge = y == top or x in (left, left + 15)
            slot = (y - top) % 4 == 3 and left + 2 <= x <= left + 13
            image.putpixel((x, y), EDGE if edge or slot else BODY)
    for y in range(top + 2, HEIGHT - 1, 8):
        image.putpixel((left + 3, y), LED)


def main() -> None:
    image = Image.new("P", (WIDTH, HEIGHT), BACK)
    image.putpalette([c for rgb in PALETTE for c in rgb])
    for repeat in (0, 128):
        for left, height in RACKS:
            draw_rack(image, repeat + left, height)
    for x in range(WIDTH):                      # the far room's floor
        image.putpixel((x, HEIGHT - 1), EDGE)
    OUT.parent.mkdir(parents=True, exist_ok=True)
    image.save(OUT)
    print(f"wrote {OUT}")


if __name__ == "__main__":
    main()

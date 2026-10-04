"""The palette module (src/engine/palette.c): the only code that writes palettes, with
fades and flashes on DMG and GBC (docs/CGB_PLAN.md sections 3 and 5)."""

import re
from pathlib import Path

from gamelib import COREGAMELOOP, read_symbol

ROOT = Path(__file__).resolve().parent.parent
PALETTE_WRITES = re.compile(r"\b(BGP_REG|OBP0_REG|OBP1_REG|BCPS_REG|BCPD_REG|OCPS_REG|OCPD_REG"
                            r"|set_bkg_palette|set_sprite_palette)\b")


def test_only_the_palette_module_writes_palettes():
    writers = [f"{path.relative_to(ROOT)}:{number}"
               for path in sorted((ROOT / "src").rglob("*.[ch]"))
               if path.name not in ("palette.c", "palette.h")
               for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1)
               if PALETTE_WRITES.search(line.split("//")[0])]
    assert not writers, f"palette writes outside src/engine/palette.c: {writers}"


def background_palette_state(game):
    """BGP on DMG; the first background palette's colors on GBC."""
    return game.reg("BGP") if game.mode == "dmg" else tuple(game.gbc_colors()[:4])


def test_losing_a_life_fades_out_and_back_in_step_by_step(game):
    # DMG: BGP E4 -> F9 -> FE -> FF and back. GBC: every color at 5/8, 1/4, black and back.
    game.start_on_open_ground()
    colors_before = game.gbc_colors() if game.mode == "cgb" else None
    states = [background_palette_state(game)]
    game.lose_life()
    for _ in range(60):
        game.frames(1)
        if background_palette_state(game) != states[-1]:
            states.append(background_palette_state(game))
    assert len(states) == 7, f"fade states: {states}"
    assert states[:3] == states[:-4:-1] and states[0] == states[-1], "the fade in doesn't mirror the fade out"
    if game.mode == "dmg":
        assert states[:4] == [0xE4, 0xF9, 0xFE, 0xFF]
    else:
        white = [state[0] for state in states[:4]]
        assert white == [0x7FFF, 0x4A52, 0x1CE7, 0], f"white during the fade: {[hex(c) for c in white]}"
        assert game.gbc_colors() == colors_before, "the palettes came back changed"


def player_oam(game) -> list:
    """(shown, GBC palette) for the player's 4 hardware sprites, OAM 0-3."""
    oam = game.pyboy.memory[0xFE00:0xFE10]
    return [(oam[i] != 0, oam[i + 3] & 0x07) for i in range(0, 16, 4)]


def test_invincible_player_is_pale_on_gbc_and_blinks_on_dmg(game):
    game.start_on_open_ground()
    game.lose_life()
    game.wait_for_state(COREGAMELOOP)
    seen = []
    while game.player().invincibility_frames > 1:
        game.frames(1)
        seen.append(player_oam(game))
    if game.mode == "dmg":
        assert any(not shown for frame in seen for shown, _ in frame), "the player never blinked"
    else:
        assert all(frame == [(True, 1)] * 4 for frame in seen), "not drawn pale every frame"
        colors = game.gbc_colors(sprites=True)
        for normal, pale in zip(colors[1:4], colors[5:8]):     # index 0 is transparent
            for shift in (0, 5, 10):
                assert (pale >> shift) & 0x1F >= (normal >> shift) & 0x1F
    game.frames(3)
    assert player_oam(game) == [(True, 0)] * 4, "not back to normal after invincibility"


def rom_palette(game, symbol: str) -> list:
    """A png2asset palette's 4 colors (RGB555) from the ROM, like _hud_palettes."""
    raw = read_symbol(game.pyboy, symbol, 8)
    return [raw[i] | raw[i + 1] << 8 for i in range(0, 8, 2)]


def test_band_and_hud_use_their_own_gbc_palettes(game):
    # GBC background palette 0: the level, 1: the band (rows 0-2), 2: the HUD window.
    if game.mode != "cgb":
        return
    game.start_on_open_ground()
    colors = game.gbc_colors()
    assert colors[4:8] == rom_palette(game, "_greybox_band_palettes")
    assert colors[8:12] == rom_palette(game, "_hud_palettes")
    memory = game.pyboy.memory
    band = [memory[1, 0x9800 + row * 32 + col] for row in range(3) for col in range(32)]
    level_row = [memory[1, 0x9800 + 3 * 32 + col] for col in range(32)]
    window_base = 0x9C00 if game.reg("LCDC") & 0x40 else 0x9800
    hud = [memory[1, window_base + row * 32 + col] for row in range(2) for col in range(20)]
    assert set(band) == {1} and set(level_row) == {0} and set(hud) == {2}

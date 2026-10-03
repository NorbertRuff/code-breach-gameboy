"""One test per fixed bug. Bug numbers refer to docs/PROJECT_STATE.md."""

import hashlib

import pytest

from gamelib import (COLLISION_SOLID, COREGAMELOOP, GAMEPLAY_END, HUD_DIGIT_TILE_BASE,
                     HUD_SLASH_TILE, LEVEL_COMPLETE, read_symbol)

dmg_only = pytest.mark.parametrize("mode", ["dmg"])  # BGP_REG effects are DMG-only until M4


def vram_maps_hash(game) -> str:
    """Hash of both background maps (0x9800-0x9FFF): changes when tiles are drawn."""
    return hashlib.md5(bytes(game.pyboy.memory[0x9800:0xA000])).hexdigest()


# Bug 1 ---------------------------------------------------------------------------

def test_hud_window_stays_at_bottom_after_game_over(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    for _ in range(2):
        game.lose_life()
        game.wait_for_state(COREGAMELOOP)
    game.lose_life()                      # third hit: game over, then the level reloads
    for _ in range(1200):                 # game over flash, fade, reload
        game.frames(1)
        if game.lives() == 3 and game.state() == COREGAMELOOP:
            break
    else:
        raise AssertionError("the game never restarted after game over")
    game.frames(5)
    game.screenshot("after_restart")
    assert game.reg("WY") == 128
    assert game.reg("WX") == 7


# Bug 2 ---------------------------------------------------------------------------

def test_hud_counter_shows_two_digits(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.frames(2)
    digits = [game.window_tile(col, 0) for col in range(15, 20)]
    d = HUD_DIGIT_TILE_BASE
    assert digits == [d + 0, d + 0, HUD_SLASH_TILE, d + 1, d + 5], "expected '00/15'"


# Bug 3 ---------------------------------------------------------------------------

@dmg_only
def test_level_loads_while_screen_is_dark(game):
    # From power-on to gameplay: whenever the tile maps change during a frame, the
    # background palette was all black (BGP = 0xFF) when that frame started. The fade-in
    # may begin in the same frame the last tiles are written.
    previous = vram_maps_hash(game)
    bgp_at_start = game.reg("BGP")
    for _ in range(300):
        game.frames(1)
        current = vram_maps_hash(game)
        if current != previous:
            assert bgp_at_start == 0xFF, f"tiles changed while visible (BGP={bgp_at_start:#04x})"
        previous = current
        bgp_at_start = game.reg("BGP")
        if game.state() == COREGAMELOOP:
            break
    assert game.reg("BGP") == 0xE4, "the screen did not fade back in"


# Bug 4 ---------------------------------------------------------------------------

def test_unused_background_tiles_stay_empty(game):
    # Tiles 78-127 (after the level and HUD tiles) are unused. Copying 256 tiles from
    # the 64-tile background array used to fill them with whatever followed in ROM.
    game.wait_for_state(COREGAMELOOP, timeout=300)
    first, last = 0x9000 + 78 * 16, 0x9000 + 128 * 16
    assert not any(game.pyboy.memory[first:last])


# Bug 5 ---------------------------------------------------------------------------

def test_falling_never_moves_up(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.frames(5)
    game.set_player_y(40)                 # in the air, above open floor
    game.frames(1)
    ys = [game.player().y]
    for _ in range(90):
        game.frames(1)
        ys.append(game.player().y)
    rises = [(i, a, b) for i, (a, b) in enumerate(zip(ys, ys[1:])) if b < a]
    assert not rises, f"the player moved up while falling: {rises[:5]}"


# Bug 6 ---------------------------------------------------------------------------

def test_releasing_a_cuts_the_jump_short(game):
    # Compares raw y within one build, so it also runs against builds from before the
    # sub-pixel Player struct (tools/test_against.py).
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.frames(10)
    start = game.save()
    tap = game.jump_rise(1)
    game.load(start)
    held = game.jump_rise(30)
    assert held >= 2 * tap, f"a tapped jump rises {tap}, a held one only {held}"


# Bug 7 ---------------------------------------------------------------------------

def test_player_never_overlaps_solid_tiles(game):
    # Walks right from the start into the side of platform A, which is at body height.
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.frames(5)
    game.pyboy.button_press("right")
    try:
        for _ in range(40):
            game.frames(1)
            hits = game.solid_tiles_overlapped()
            assert not hits, f"player box {game.player_box()} overlaps {hits}"
    finally:
        game.pyboy.button_release("right")


# Bug 11 --------------------------------------------------------------------------

def test_falling_below_the_level_costs_a_life(game):
    # Level 1 has no pits yet, so the player is put below the floor directly.
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.frames(5)
    game.set_player_y(170)
    for _ in range(60):
        game.frames(1)
        if game.lives() == 2:
            break
    assert game.lives() == 2


# Bug 13 --------------------------------------------------------------------------

def test_no_slide_to_the_right_after_releasing_left(game):
    game.start_on_open_ground()
    game.hold("left", 8)
    xs = [game.player().x]
    for _ in range(6):
        game.frames(1)
        xs.append(game.player().x)
    assert all(b <= a for a, b in zip(xs, xs[1:])), f"x moved right after releasing left: {xs}"


# Review L1: the level sits above the HUD ----------------------------------------

def test_level_and_player_are_above_the_hud(game):
    # The HUD window starts at y = 128 (row 16). The floor is row 15 on the whole first
    # screen, rows 0-2 (the parallax band) are open, and the player's feet are above the HUD.
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.frames(5)
    assert all(game.collision(x, 15) == COLLISION_SOLID for x in range(20))
    assert not any(game.collision(x, y) for x in range(20) for y in range(3))
    assert game.player().py < 128 == game.reg("WY")


# Review G1: sprite palettes are ordered light to dark ----------------------------

def rgb555_luma(color: int) -> float:
    r, g, b = color & 0x1F, (color >> 5) & 0x1F, (color >> 10) & 0x1F
    return 0.299 * r + 0.587 * g + 0.114 * b


def test_player_palette_is_light_to_dark(game):
    # DMG shows color index n as shade n, so index 1 must be the lightest visible color
    # and index 3 the darkest (index 0 is transparent). docs/ART_PIPELINE.md.
    raw = read_symbol(game.pyboy, "_programmer_palettes", 8)
    colors = [int.from_bytes(raw[i:i + 2], "little") for i in range(0, 8, 2)]
    lumas = [rgb555_luma(c) for c in colors[1:]]
    assert lumas == sorted(lumas, reverse=True), f"palette not light to dark: {[hex(c) for c in colors]}"


# Review P1: a jump starts only on a new press of A --------------------------------

def test_holding_a_jumps_only_once(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.frames(10)
    takeoffs = 0
    on_ground = True
    game.pyboy.button_press("a")
    try:
        for _ in range(150):              # long enough for three jumps if it auto-hops
            game.frames(1)
            grounded = game.player().on_ground
            if on_ground and not grounded:
                takeoffs += 1
            on_ground = grounded
    finally:
        game.pyboy.button_release("a")
    assert takeoffs == 1


# Bugs 9 and 14 -------------------------------------------------------------------

def flashes_per_second(bgp_by_frame) -> int:
    """Most changes into BGP = 0xFF (black) within any 60-frame window."""
    onsets = [i for i in range(1, len(bgp_by_frame))
              if bgp_by_frame[i] == 0xFF and bgp_by_frame[i - 1] != 0xFF]
    return max((sum(1 for t in onsets if start <= t < start + 60) for start in range(len(bgp_by_frame))),
               default=0)


@dmg_only
@pytest.mark.parametrize("event", ["level_complete", "game_over"])
def test_flashes_are_slow_and_restore_palettes(game, event):
    # Forcing the state saves collecting 12 fragments or losing 3 lives by hand.
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.frames(5)
    obp0_before = game.reg("OBP0")
    if event == "level_complete":
        game.set_state(LEVEL_COMPLETE)
    else:
        game.set_lives(1)
        game.set_state(GAMEPLAY_END)
    bgp = []
    for _ in range(600):
        game.frames(1)
        bgp.append(game.reg("BGP"))
        if game.state() == COREGAMELOOP and game.reg("BGP") == 0xE4:
            break
    else:
        raise AssertionError("the level never restarted")
    assert flashes_per_second(bgp) <= 3
    assert game.reg("OBP0") == obp0_before, "sprite palette not restored after the flash"


# Bug 17 --------------------------------------------------------------------------

def player_sprite_area(game) -> tuple:
    """Screen rectangle covered by the player's 4 hardware sprites (8x16 each, OAM 0-3)."""
    oam = game.pyboy.memory[0xFE00:0xFE10]
    xs = [oam[i + 1] - 8 for i in range(0, 16, 4)]
    ys = [oam[i] - 16 for i in range(0, 16, 4)]
    return min(xs), min(ys), max(xs) + 7, max(ys) + 15


def test_player_sprite_covers_its_collision_box(game):
    # Level 1 starts the player at x = 8, y = 119 with the camera at 0: the box is
    # x 0-15, y 88-119. Holding left turns the player around without moving (level edge).
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.frames(5)
    assert player_sprite_area(game) == (0, 88, 15, 119), "facing right"
    game.hold("left", 5)
    game.frames(2)
    assert player_sprite_area(game) == (0, 88, 15, 119), "facing left"

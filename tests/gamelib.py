"""Helpers for driving the ApexLab ROM in PyBoy. See docs/TESTING.md."""

from __future__ import annotations

import io
import struct
from pathlib import Path
from types import SimpleNamespace

import pyboy as pyboy_package
from pyboy import PyBoy

# Game states, from src/utils.h.
INTRO_CREDITS_STATE = 0
TITLE_SCREEN_STATE = 1
GAMEPLAY_START = 2
COREGAMELOOP = 3
GAMEPLAY_END = 4
LEVEL_COMPLETE = 5

# Hardware registers by name (Pan Docs).
REGISTERS = {
    "LCDC": 0xFF40, "STAT": 0xFF41, "SCY": 0xFF42, "SCX": 0xFF43, "LY": 0xFF44,
    "BGP": 0xFF47, "OBP0": 0xFF48, "OBP1": 0xFF49, "WY": 0xFF4A, "WX": 0xFF4B,
}

# Mirror of the Player struct in src/player/player.h, in field order. SDCC does not pad
# structs on the Game Boy CPU, and multi-byte values are little endian.
# test_player_struct_size_matches fails when the C struct changes without this list.
# x and y are in 1/32 px; player() adds px and py in whole pixels.
PLAYER_FIELDS = [
    ("x", "H"), ("y", "H"), ("velocity_x", "b"), ("velocity_y", "b"), ("input_x", "b"),
    ("jump_held", "B"), ("jump_buffer_frames", "B"), ("coyote_frames", "B"),
    ("on_ground", "B"), ("touched_hazard", "B"), ("direction", "B"), ("draw_frame", "B"),
    ("animation_frame_count", "B"), ("invincibility_frames", "B"),
]
PLAYER_FORMAT = "<" + "".join(fmt for _, fmt in PLAYER_FIELDS)
PLAYER_SIZE = struct.calcsize(PLAYER_FORMAT)
PLAYER_OFFSETS = {}
_offset = 0
for _name, _fmt in PLAYER_FIELDS:
    PLAYER_OFFSETS[_name] = _offset
    _offset += struct.calcsize("<" + _fmt)

# Level data, from src/level/level_manager.h. collision_map comes after level_id,
# state and two 2-byte pointers in Level, which is the first member of LevelManager.
LEVEL_WIDTH = 32
LEVEL_HEIGHT = 18
TILE_SIZE = 8
COLLISION_MAP_OFFSET = 6
COLLISION_NONE = 0
COLLISION_SOLID = 1
COLLISION_ONE_WAY = 2
COLLISION_HAZARD = 3

# HUD tiles, from src/level/level_manager.h.
HUD_DIGIT_TILE_BASE = 133
HUD_SLASH_TILE = 143

# Player body box around its pivot (x = center, y = lowest row), from src/player/player.h.
SUBPIXELS = 32
SPRITE_LEFT_W = 8
SPRITE_RIGHT_W = 7
SPRITE_HEAD_OFFSET = 31
COYOTE_FRAMES = 6
JUMP_BUFFER_FRAMES = 5
CORNER_CORRECTION = 4


PYBOY_DMG_BOOTROM = Path(pyboy_package.__file__).parent / "core" / "bootrom_dmg.bin"
CARTRIDGE_ENTRY = 0x100


def boot(rom: Path, mode: str, ram_file=None) -> PyBoy:
    """Start a headless PyBoy at full speed and run it to the game's first instruction.

    mode is "dmg" or "cgb". PyBoy picks its boot ROM from the cartridge header, and our
    header says "GBC-enhanced", so it would run GBC hardware even with cgb=False. Passing
    PyBoy's own DMG boot ROM forces real DMG mode. Its boot ROMs take about 65 frames;
    after that GBDK has set _cpu (0x01 DMG, 0x11 GBC) from register A.
    """
    extra = {"bootrom": str(PYBOY_DMG_BOOTROM)} if mode == "dmg" else {}
    pyboy = PyBoy(str(rom), window="null", cgb=(mode == "cgb"),
                  sound_emulated=False, ram_file=ram_file, **extra)
    pyboy.set_emulation_speed(0)

    reached = []
    pyboy.hook_register(0, CARTRIDGE_ENTRY, lambda _: reached.append(True), None)
    for _ in range(300):
        if reached:
            break
        pyboy.tick(1, False)
    pyboy.hook_deregister(0, CARTRIDGE_ENTRY)
    if not reached:
        raise RuntimeError("the boot ROM never handed over to the cartridge")
    return pyboy


def read_symbol(pyboy: PyBoy, symbol: str, size: int = 1) -> bytes:
    """Read size bytes at a symbol from the .sym file. C globals start with "_"."""
    bank, addr = pyboy.symbol_lookup(symbol)
    if 0x4000 <= addr < 0x8000:
        return bytes(pyboy.memory[bank, addr:addr + size])
    return bytes(pyboy.memory[addr:addr + size])


class Game:
    """One running ROM plus the test's output folder for screenshots."""

    def __init__(self, pyboy: PyBoy, mode: str, out_dir: Path, test_name: str):
        self.pyboy = pyboy
        self.mode = mode
        self.out_dir = out_dir
        self.test_name = test_name
        # PyBoy's frames end a few lines into VBlank, in the middle of the game's update.
        # So player() reads a copy taken when the game starts drawing the player, after
        # that frame's update is complete. The copy is dropped at every new frame, so a
        # frame without a draw (like a respawn) reads memory directly (docs/TESTING.md).
        self._drawn_player = None
        pyboy.hook_register(None, "_player_draw", self._copy_player, None)

    def _copy_player(self, _context) -> None:
        self._drawn_player = read_symbol(self.pyboy, "_player", PLAYER_SIZE)

    # --- time and input ---------------------------------------------------------

    def tick(self, render: bool = False) -> None:
        """Step one frame."""
        self._drawn_player = None
        self.pyboy.tick(1, render)

    def frames(self, n: int) -> None:
        """Step n frames without rendering (fast)."""
        for _ in range(n):
            self.tick()

    def hold(self, button: str, n: int) -> None:
        """Hold a button for n frames, then release it."""
        self.pyboy.button_press(button)
        self.frames(n)
        self.pyboy.button_release(button)

    def press(self, button: str) -> None:
        """Press a button for one frame."""
        self.hold(button, 1)

    def wait_for_state(self, state: int, timeout: int = 600) -> int:
        """Step until the state machine reaches state. Returns the frames it took."""
        for frame in range(timeout):
            if self.state() == state:
                return frame
            self.frames(1)
        raise AssertionError(f"state {state} not reached in {timeout} frames (now {self.state()})")

    def save(self) -> io.BytesIO:
        """Snapshot of the whole machine, to replay from the same frame with load()."""
        snapshot = io.BytesIO()
        self.pyboy.save_state(snapshot)
        return snapshot

    def load(self, snapshot: io.BytesIO) -> None:
        snapshot.seek(0)
        self.pyboy.load_state(snapshot)
        self._drawn_player = None

    # --- reading the game -------------------------------------------------------

    def u8(self, symbol: str) -> int:
        return read_symbol(self.pyboy, symbol)[0]

    def u16(self, symbol: str) -> int:
        return int.from_bytes(read_symbol(self.pyboy, symbol, 2), "little")

    def state(self) -> int:
        return self.u8("_current_game_state")

    def lives(self) -> int:
        return self.u8("_lives")

    def player(self) -> SimpleNamespace:
        """The Player struct as of the last completed frame (live, until the first draw)."""
        raw = self._drawn_player or read_symbol(self.pyboy, "_player", PLAYER_SIZE)
        values = struct.unpack(PLAYER_FORMAT, raw)
        p = SimpleNamespace(**{name: v for (name, _), v in zip(PLAYER_FIELDS, values)})
        p.px, p.py = p.x // SUBPIXELS, p.y // SUBPIXELS
        return p

    def player_box(self) -> tuple:
        """The player's body box in world pixels: left, top, right, bottom (inclusive)."""
        p = self.player()
        return (p.px - SPRITE_LEFT_W, p.py - SPRITE_HEAD_OFFSET, p.px + SPRITE_RIGHT_W, p.py)

    def solid_tiles_overlapped(self) -> list:
        """Solid level tiles that the player's body box overlaps right now."""
        left, top, right, bottom = self.player_box()
        return [(tx, ty)
                for ty in range(max(top, 0) // TILE_SIZE, bottom // TILE_SIZE + 1)
                for tx in range(max(left, 0) // TILE_SIZE, right // TILE_SIZE + 1)
                if ty < LEVEL_HEIGHT and tx < LEVEL_WIDTH and self.collision(tx, ty) == COLLISION_SOLID]

    def reg(self, name: str) -> int:
        return self.pyboy.memory[REGISTERS[name]]

    def window_tile(self, col: int, row: int) -> int:
        """Tile index in the window map (VRAM bank 0). LCDC bit 6 picks the map."""
        base = 0x9C00 if self.reg("LCDC") & 0x40 else 0x9800
        return self.pyboy.memory[base + row * 32 + col]

    def collision(self, tile_x: int, tile_y: int) -> int:
        """Collision type of a level tile, from the level's collision map in WRAM."""
        _, addr = self.pyboy.symbol_lookup("_level_manager")
        return self.pyboy.memory[addr + COLLISION_MAP_OFFSET + tile_y * LEVEL_WIDTH + tile_x]

    # --- changing the game (tests only) ----------------------------------------
    # Some situations can't be reached by input in level 1 (it has no pits) or would
    # take minutes of play. These write the game's variables directly. Keep them rare.

    def set_state(self, state: int) -> None:
        _, addr = self.pyboy.symbol_lookup("_current_game_state")
        self.pyboy.memory[addr] = state

    def set_lives(self, lives: int) -> None:
        _, addr = self.pyboy.symbol_lookup("_lives")
        self.pyboy.memory[addr] = lives

    def _set_player_u16(self, field: str, value: int) -> None:
        self._drawn_player = None
        _, addr = self.pyboy.symbol_lookup("_player")
        addr += PLAYER_OFFSETS[field]
        self.pyboy.memory[addr] = value & 0xFF
        self.pyboy.memory[addr + 1] = (value >> 8) & 0xFF

    def set_player_y(self, y_px: int) -> None:
        self._set_player_u16("y", y_px * SUBPIXELS)

    def place_player(self, x_px: int, y_px: int) -> None:
        """Put the player at a world position in pixels (x = center, y = lowest row)."""
        self._set_player_u16("x", x_px * SUBPIXELS)
        self._set_player_u16("y", y_px * SUBPIXELS)

    def set_collision(self, tile_x: int, tile_y: int, kind: int) -> None:
        """Change one tile's collision type (the screen keeps the old tile)."""
        _, addr = self.pyboy.symbol_lookup("_level_manager")
        self.pyboy.memory[addr + COLLISION_MAP_OFFSET + tile_y * LEVEL_WIDTH + tile_x] = kind

    # --- common sequences -------------------------------------------------------

    def start_on_open_ground(self) -> None:
        """Wait for gameplay, then put the player on the floor at x = 156, with open air above,
        and patrol bots. At the level start, platform A blocks the way at body height."""
        self.wait_for_state(COREGAMELOOP, timeout=300)
        self.frames(5)
        self.place_player(156, 119)
        self.frames(2)

    def jump_rise(self, frames_held: int) -> int:
        """Jump with A held for frames_held; returns the highest rise in units of Player.y."""
        ground_y = top = self.player().y
        self.pyboy.button_press("a")
        for frame in range(90):
            if frame == frames_held:
                self.pyboy.button_release("a")
            self.frames(1)
            top = min(top, self.player().y)
        return ground_y - top

    def lose_life(self, timeout: int = 400) -> None:
        """Wait until invincibility is over, then jump over platform A and run right into
        the first patrol bot."""
        lives = self.lives()
        for _ in range(200):
            if self.player().invincibility_frames == 0:
                break
            self.frames(1)
        self.pyboy.button_press("right")
        self.pyboy.button_press("a")
        try:
            for frame in range(timeout):
                if frame == 30:
                    self.pyboy.button_release("a")
                self.frames(1)
                if self.lives() != lives:
                    return
        finally:
            self.pyboy.button_release("right")
            self.pyboy.button_release("a")
        raise AssertionError("running right never hit an enemy")

    # --- pictures ---------------------------------------------------------------

    def screenshot(self, name: str) -> Path:
        """Render one frame and save it as tests/out/<test>_<name>.png."""
        self.tick(render=True)
        self.out_dir.mkdir(parents=True, exist_ok=True)
        safe = self.test_name.replace("[", "_").replace("]", "").replace("/", "_")
        path = self.out_dir / f"{safe}_{name}.png"
        self.pyboy.screen.image.save(path)
        return path

    def screen_colors(self) -> int:
        """Number of distinct colors in the current frame (renders one frame)."""
        self.tick(render=True)
        pixels = self.pyboy.screen.ndarray[:, :, :3].reshape(-1, 3)
        return len({tuple(p) for p in pixels})

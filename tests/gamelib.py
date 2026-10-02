"""Helpers for driving the ApexLab ROM in PyBoy. See docs/TESTING.md."""

from __future__ import annotations

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
PLAYER_FIELDS = [
    ("x", "H"), ("y", "H"), ("draw_x", "H"), ("draw_y", "H"),
    ("draw_frame", "B"), ("direction", "B"), ("animation_frame_count", "B"),
    ("movement_frame_count", "B"), ("last_movement_x", "B"),
    ("movement_force_x", "b"), ("movement_force_y", "b"), ("velocity_x", "b"), ("slowing_x", "B"),
    ("is_jumping", "B"), ("jump_frames", "B"), ("velocity_y", "b"), ("jump_frame_count", "B"),
    ("underfoot_state", "B"), ("invincibility_frames", "B"),
]
PLAYER_FORMAT = "<" + "".join(fmt for _, fmt in PLAYER_FIELDS)
PLAYER_SIZE = struct.calcsize(PLAYER_FORMAT)


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

    # --- time and input ---------------------------------------------------------

    def frames(self, n: int) -> None:
        """Step n frames without rendering (fast)."""
        if n > 0:
            self.pyboy.tick(n, False)

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
        raw = read_symbol(self.pyboy, "_player", PLAYER_SIZE)
        values = struct.unpack(PLAYER_FORMAT, raw)
        return SimpleNamespace(**{name: v for (name, _), v in zip(PLAYER_FIELDS, values)})

    def reg(self, name: str) -> int:
        return self.pyboy.memory[REGISTERS[name]]

    # --- pictures ---------------------------------------------------------------

    def screenshot(self, name: str) -> Path:
        """Render one frame and save it as tests/out/<test>_<name>.png."""
        self.pyboy.tick(1, True)
        self.out_dir.mkdir(parents=True, exist_ok=True)
        safe = self.test_name.replace("[", "_").replace("]", "").replace("/", "_")
        path = self.out_dir / f"{safe}_{name}.png"
        self.pyboy.screen.image.save(path)
        return path

    def screen_colors(self) -> int:
        """Number of distinct colors in the current frame (renders one frame)."""
        self.pyboy.tick(1, True)
        pixels = self.pyboy.screen.ndarray[:, :, :3].reshape(-1, 3)
        return len({tuple(p) for p in pixels})

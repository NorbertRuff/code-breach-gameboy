"""Helpers for driving the ApexLab ROM in PyBoy. See docs/TESTING.md."""

from __future__ import annotations

import io
import re
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
PAUSED = 6

# Sound effect numbers, from src/sound/effects.h (made from sounds/effects.txt).
EFFECTS = {name.lower(): int(number) for name, number in re.findall(
    r"#define EFFECT_(\w+) (\d+)", (Path(__file__).parent.parent / "src/sound/effects.h").read_text())}

# Hardware registers by name (Pan Docs).
REGISTERS = {
    "LCDC": 0xFF40, "STAT": 0xFF41, "SCY": 0xFF42, "SCX": 0xFF43, "LY": 0xFF44,
    "BGP": 0xFF47, "OBP0": 0xFF48, "OBP1": 0xFF49, "WY": 0xFF4A, "WX": 0xFF4B,
}

# Screen timing (Pan Docs): VBlank starts at line 144, and a frame has 154 lines.
VBLANK_LINE = 144
LINES_PER_FRAME = 154

# Mirror of the Player struct in src/player/player.h, in field order. SDCC does not pad
# structs on the Game Boy CPU, and multi-byte values are little endian.
# test_player_struct_size_matches fails when the C struct changes without this list.
# x and y are in 1/32 px; player() adds px and py in whole pixels.
PLAYER_FIELDS = [
    ("x", "H"), ("y", "H"), ("velocity_x", "b"), ("velocity_y", "b"), ("input_x", "b"),
    ("down_held", "B"), ("jump_held", "B"), ("jump_buffer_frames", "B"), ("coyote_frames", "B"),
    ("on_ground", "B"), ("touched_hazard", "B"), ("touched_exit", "B"), ("direction", "B"), ("draw_frame", "B"),
    ("animation_frame_count", "B"), ("invincibility_frames", "B"),
]
PLAYER_FORMAT = "<" + "".join(fmt for _, fmt in PLAYER_FIELDS)

# Mirror of the Actor struct in src/actor/actor.h; type is the address of its ActorType.
# test_actor_struct_size_matches fails when the C struct changes without this list.
ACTOR_FIELDS = [("type", "H"), ("x", "H"), ("y", "B"), ("velocity_x", "b"), ("min_x", "H"), ("max_x", "H"),
                ("velocity_y", "b"), ("step_y", "b"), ("wait_frames", "B"), ("min_y", "B"), ("max_y", "B")]
ACTOR_FORMAT = "<" + "".join(fmt for _, fmt in ACTOR_FIELDS)
ACTOR_SIZE = struct.calcsize(ACTOR_FORMAT)
MAX_ACTORS = 8

# Mirror of the Level struct in src/level/level_manager.h (gates: 2 of 4 bytes each).
# test_level_struct_size_matches fails when the C struct changes without this list.
LEVEL_FIELDS = [
    ("bank", "B"), ("width", "B"), ("width_px", "H"), ("player_start_x", "H"),
    ("player_start_y", "B"), ("fragment_count", "B"), ("entity_count", "B"),
    ("section_start", "3s"), ("map", "H"), ("entities", "H"), ("gate_count", "B"),
    ("gates", "8s"), ("exit_column", "B"), ("exit_row", "B"), ("section", "B"),
    ("section_fragments", "3s"), ("section_collected", "3s"),
]
LEVEL_FORMAT = "<" + "".join(fmt for _, fmt in LEVEL_FIELDS)
LEVEL_SIZE = struct.calcsize(LEVEL_FORMAT)
PLAYER_SIZE = struct.calcsize(PLAYER_FORMAT)
PLAYER_OFFSETS = {}
_offset = 0
for _name, _fmt in PLAYER_FIELDS:
    PLAYER_OFFSETS[_name] = _offset
    _offset += struct.calcsize("<" + _fmt)

# Levels and the collision ring, from src/level/level_manager.h and level_data.h.
LEVEL_1 = 0
LEVEL_2 = 1
LEVEL_3 = 2
LEVEL_GREYBOX_TEST = 3
LEVEL_TEST_JUMPS = 4
LEVEL_TEST_MOVERS = 5
RING_COLUMNS = 32
RING_ROWS = 16
TILE_SIZE = 8
COLLISION_NONE = 0
COLLISION_SOLID = 1
COLLISION_ONE_WAY = 2
COLLISION_HAZARD = 3

# The patrol bot's two 8x16 sprites use these sprite tiles (src/actor/bot.c).
BOT_TILES = (40, 42)

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


def palette_lumas(game, symbol: str) -> list:
    """Brightness of the 4 colors of a png2asset palette (RGB555) in the ROM, like
    _programmer_palettes. A palette ordered light to dark gives falling values."""
    raw = read_symbol(game.pyboy, symbol, 8)
    colors = [int.from_bytes(raw[i:i + 2], "little") for i in range(0, 8, 2)]
    return [0.299 * (c & 0x1F) + 0.587 * ((c >> 5) & 0x1F) + 0.114 * ((c >> 10) & 0x1F) for c in colors]


def read_symbol(pyboy: PyBoy, symbol: str, size: int = 1) -> bytes:
    """Read size bytes at a symbol from the .sym file. C globals start with "_"."""
    bank, addr = pyboy.symbol_lookup(symbol)
    if 0x4000 <= addr < 0x8000:
        return bytes(pyboy.memory[bank, addr:addr + size])
    return bytes(pyboy.memory[addr:addr + size])


class Game:
    """One running ROM plus the test's output folder for screenshots."""

    def __init__(self, pyboy: PyBoy, mode: str, out_dir: Path, test_name: str,
                 level: int = LEVEL_GREYBOX_TEST):
        self.pyboy = pyboy
        self.mode = mode
        self.out_dir = out_dir
        self.test_name = test_name
        # PyBoy's frames end a few lines into VBlank, in the middle of the game's update.
        # So player() reads a copy taken when the game starts drawing the player, after
        # that frame's update is complete. The copy is dropped at every new frame, so a
        # frame without a draw (like a respawn) reads memory directly (docs/TESTING.md).
        # Writes to the player wait for the same point, so they never land mid-update.
        self._drawn_player = None
        self._player_writes = {}
        self._update_end_callbacks = []
        pyboy.hook_register(None, "_player_draw", self._at_player_draw, None)
        # The game starts in level 1; most tests play the greybox test level. main() runs
        # once, after the C globals are set, so the first GAMEPLAY_START loads `level`.
        # Builds from before level 1 (tools/test_against.py) number the levels otherwise
        # and start in the greybox level anyway: leave those alone.
        if self.has_symbol("_level_1"):
            pyboy.hook_register(None, "_main", self._set_level_index, level)

    def has_symbol(self, symbol: str) -> bool:
        try:
            self.pyboy.symbol_lookup(symbol)
            return True
        except ValueError:
            return False

    def rom_level_index(self, level: int) -> int:
        """A LEVEL_* index in this build's numbering: builds before level 3 had the test
        rooms one index lower, builds before level 2 two."""
        missing = sum(not self.has_symbol(f"_level_{n}") for n in (2, 3))
        return level - missing if level >= LEVEL_GREYBOX_TEST else level

    def _set_level_index(self, level: int) -> None:
        _, addr = self.pyboy.symbol_lookup("_level_index")
        self.pyboy.memory[addr] = self.rom_level_index(level)

    def _at_player_draw(self, _context) -> None:
        """The frame's update is done: apply waiting writes, then copy the struct."""
        _, base = self.pyboy.symbol_lookup("_player")
        for field, value in self._player_writes.items():
            self.pyboy.memory[base + PLAYER_OFFSETS[field]] = value & 0xFF
            self.pyboy.memory[base + PLAYER_OFFSETS[field] + 1] = (value >> 8) & 0xFF
        self._player_writes.clear()
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

    def play_route(self, route: list) -> None:
        """Play (frames, buttons) steps: each step holds exactly the buttons named in its
        space-separated string ("a right", or "" for none) for that many frames. A button
        held in two steps in a row stays down; a new press needs a step without it between."""
        held = set()
        for frames, buttons in route:
            wanted = set(buttons.split())
            for button in held - wanted:
                self.pyboy.button_release(button)
            for button in wanted - held:
                self.pyboy.button_press(button)
            held = wanted
            self.frames(frames)
        for button in held:
            self.pyboy.button_release(button)

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
        """Go back to a snapshot. Buttons are not part of it: press held ones again."""
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

    def actors(self) -> list:
        """The actors in play, in array order. Each has the Actor fields, plus bot, chaser and
        lift: is it one."""
        count = self.u8("_actor_count")
        if not count:
            return []                           # PyBoy refuses an empty memory slice
        raw = read_symbol(self.pyboy, "_actors", ACTOR_SIZE * count)
        types = {name: self.pyboy.symbol_lookup(f"_{name}_type")[1] if self.has_symbol(f"_{name}_type") else None
                 for name in ("bot", "chaser", "lift")}
        actors = []
        for i in range(count):
            values = struct.unpack_from(ACTOR_FORMAT, raw, i * ACTOR_SIZE)
            actor = SimpleNamespace(**{name: v for (name, _), v in zip(ACTOR_FIELDS, values)})
            actor.bot, actor.chaser, actor.lift = (actor.type == types[name] for name in ("bot", "chaser", "lift"))
            actors.append(actor)
        return actors

    def level(self) -> SimpleNamespace:
        """The loaded level's state. gates is a list of (column, bottom_row, quota,
        open_rows); section_fragments and section_collected are lists per section."""
        values = struct.unpack(LEVEL_FORMAT, read_symbol(self.pyboy, "_current_level", LEVEL_SIZE))
        level = SimpleNamespace(**{name: v for (name, _), v in zip(LEVEL_FIELDS, values)})
        level.gates = [tuple(level.gates[i:i + 4]) for i in range(0, 4 * level.gate_count, 4)]
        level.section_fragments = list(level.section_fragments)
        level.section_collected = list(level.section_collected)
        return level

    def visible_sprites(self) -> list:
        """The hardware sprites in OAM that are switched on (y not 0): (x, y, tile) each,
        in screen pixels for the sprite's top-left corner."""
        oam = self.pyboy.memory[0xFE00:0xFEA0]
        return [(oam[i + 1] - 8, oam[i] - 16, oam[i + 2]) for i in range(0, 160, 4) if oam[i]]

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
                if ty < RING_ROWS and self.collision(tx, ty) == COLLISION_SOLID]

    def reg(self, name: str) -> int:
        return self.pyboy.memory[REGISTERS[name]]

    def gbc_colors(self, sprites: bool = False) -> list:
        """GBC only: the 32 colors (RGB555) of the 8 background or sprite palettes. Reads
        palette memory through its index register, which is put back afterwards."""
        index_reg, data_reg = (0xFF6A, 0xFF6B) if sprites else (0xFF68, 0xFF69)
        memory = self.pyboy.memory
        saved = memory[index_reg]
        raw = []
        for i in range(64):
            memory[index_reg] = i
            raw.append(memory[data_reg])
        memory[index_reg] = saved
        return [raw[i] | raw[i + 1] << 8 for i in range(0, 64, 2)]

    def background_is_black(self) -> bool:
        """Is the background (with the HUD) all black? DMG: BGP is 0xFF. GBC: every
        background color is 0."""
        if self.mode == "dmg":
            return self.reg("BGP") == 0xFF
        return not any(self.gbc_colors())

    def sprite_palettes(self):
        """What the sprites are drawn with: OBP0 on DMG, the sprite colors on GBC."""
        return self.reg("OBP0") if self.mode == "dmg" else self.gbc_colors(sprites=True)

    def window_tile(self, col: int, row: int) -> int:
        """Tile index in the window map (VRAM bank 0). LCDC bit 6 picks the map."""
        base = 0x9C00 if self.reg("LCDC") & 0x40 else 0x9800
        return self.pyboy.memory[base + row * 32 + col]

    def collision(self, tile_x: int, tile_y: int) -> int:
        """Collision type of a tile (tile_y in screen rows), from the collision ring.
        Only the columns drawn around the camera are in the ring."""
        _, addr = self.pyboy.symbol_lookup("_collision_ring")
        return self.pyboy.memory[addr + ((tile_x % RING_COLUMNS) << 4) + tile_y]

    def bkg_tile(self, tile_x: int, tile_y: int) -> int:
        """Tile index in the background map at a level column (it wraps every 32)."""
        base = 0x9C00 if self.reg("LCDC") & 0x08 else 0x9800
        return self.pyboy.memory[base + tile_y * 32 + tile_x % 32]

    def camera_x(self) -> int:
        return int.from_bytes(read_symbol(self.pyboy, "_camera_x", 2), "little", signed=True)

    def lines_since_vblank(self, vblanks_at_start: int) -> int:
        """Screen lines since the start of the VBlank where sys_time was vblanks_at_start:
        LY counted from VBlank, plus LINES_PER_FRAME for every VBlank since."""
        vblanks = (self.u8("_sys_time") - vblanks_at_start) & 0xFF
        return (self.reg("LY") - VBLANK_LINE) % LINES_PER_FRAME + vblanks * LINES_PER_FRAME

    def track_frame_lines(self) -> list:
        """From now on, add to the returned list, for every gameplay update, how many screen
        lines passed from the start of VBlank to the end of the update (when the game
        starts waiting for the next VBlank). A frame has LINES_PER_FRAME lines: an update
        that ends later caused a lag frame."""
        lines = []
        start = {}

        def at_update_start(_):
            start["vblanks"] = self.u8("_sys_time")

        def at_update_end(_):
            if "vblanks" in start:
                lines.append(self.lines_since_vblank(start.pop("vblanks")))

        self.pyboy.hook_register(None, "_main_game_loop", at_update_start, None)
        self.at_update_end(at_update_end)
        return lines

    def at_update_end(self, callback) -> None:
        """Call callback(None) whenever the game starts waiting for VBlank. PyBoy allows one
        hook per address, so the trackers share this one."""
        if not self._update_end_callbacks:
            self.pyboy.hook_register(None, "_wait_vbl_done",
                                     lambda _: [c(None) for c in self._update_end_callbacks], None)
        self._update_end_callbacks.append(callback)

    def at_screen_ready(self, callback) -> None:
        """Call callback(None) whenever a gameplay update reads the input. By then it has
        written the column the last update drew (level_stream_flush), so the background
        map and the collision ring match camera_x."""
        self.pyboy.hook_register(None, "_input_read_player", callback, None)

    def track_effects(self) -> list:
        """From now on, add each sound effect the game asks for to the returned list (by name)."""
        names = {number: name for name, number in EFFECTS.items()}
        played = []
        # sound_play gets its uint8_t argument in A (GBDK's calling convention).
        self.pyboy.hook_register(None, "_sound_play",
                                 lambda _: played.append(names[self.pyboy.register_file.A]), None)
        return played

    def track_sprites_per_line(self) -> list:
        """From now on, add to the returned list, for every gameplay update, the most
        hardware sprites on one screen line (8x16 sprites; the hardware draws at most 10
        per line). Counted by y only: the hardware counts sprites off screen to the side
        too. Reads OAM when an update ends, so each entry is the frame being shown."""
        most = []
        self.at_update_end(lambda _: most.append(self.sprites_on_busiest_line()))
        return most

    def sprites_on_busiest_line(self) -> int:
        """The most hardware sprites in OAM on one screen line (8x16 sprites; the hardware
        draws at most 10 per line). By y only: sprites off screen to the side count too."""
        oam = self.pyboy.memory[0xFE00:0xFEA0]
        per_line = [0] * 144
        for i in range(0, 160, 4):
            top = oam[i] - 16
            for line in range(max(top, 0), min(top + 16, 144)):
                per_line[line] += 1
        return max(per_line)

    # --- changing the game (tests only) ----------------------------------------
    # Some situations would take minutes of play to reach. These write the game's
    # variables directly. Keep them rare.

    def load_level(self, index: int) -> None:
        """Wait for gameplay, then restart in another level (LEVEL_*)."""
        self.wait_for_state(COREGAMELOOP, timeout=300)
        _, addr = self.pyboy.symbol_lookup("_level_index")
        self.pyboy.memory[addr] = self.rom_level_index(index)
        self.set_state(GAMEPLAY_START)
        self.frames(1)
        self.wait_for_state(COREGAMELOOP)

    def set_state(self, state: int) -> None:
        _, addr = self.pyboy.symbol_lookup("_current_game_state")
        self.pyboy.memory[addr] = state

    def set_lives(self, lives: int) -> None:
        _, addr = self.pyboy.symbol_lookup("_lives")
        self.pyboy.memory[addr] = lives

    def set_player_y(self, y_px: int) -> None:
        """Move the player to a feet row in pixels, at the end of the next frame's update."""
        self._player_writes["y"] = y_px * SUBPIXELS

    PLACE_FRAMES = 10   # one frame to take the move, then the camera catches up

    def place_player(self, x_px: int, y_px: int) -> None:
        """Put the player at a world position in pixels (x = center, y = lowest row), at
        the end of the next frame's update, then run PLACE_FRAMES frames: a far move makes
        the camera jump, and redrawing a whole screen of columns takes about 6 frames."""
        self._player_writes["x"] = x_px * SUBPIXELS
        self._player_writes["y"] = y_px * SUBPIXELS
        self.frames(self.PLACE_FRAMES)

    # --- common sequences -------------------------------------------------------

    def start_on_open_ground(self) -> None:
        """Wait for gameplay, then put the player on the floor at x = 156, with open air
        above and no patrol bot near. At the level start, platform A blocks the way."""
        self.wait_for_state(COREGAMELOOP, timeout=300)
        self.frames(5)
        self.place_player(156, 119)

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

    FALL_OUT_Y = 144 + SPRITE_HEAD_OFFSET + 8   # feet below this: fell out (FALL_DEATH_Y, main.c)

    def lose_life(self, timeout: int = 30) -> None:
        """Drop the player below the screen: falling out costs a life, even while
        invincible, and the game restarts the section the player is in. A write, not
        input: with the gates closed, not every section has a pit within reach."""
        lives = self.lives()
        self.set_player_y(self.FALL_OUT_Y)
        for _ in range(timeout):
            self.frames(1)
            if self.lives() != lives:
                return
        raise AssertionError("falling out never cost a life")

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


class Pilot:
    """Plays a level with moves that watch the player every frame: run to an x, jump, drop
    off a ledge, get past a bot, open a gate. Positions are world pixels, like Tiled's.
    Any move that costs a life fails, with the place. log gets a line per move."""

    FOLLOW_GAP = 40     # px between the body centers while following a bot
    TIMEOUT = 600       # frames a move may take

    def __init__(self, game: Game, log=None):
        self.game = game
        self.log = log or (lambda _: None)
        self.held = set()
        self.lives = game.lives()

    def where(self) -> str:
        p = self.game.player()
        return f"x {p.px}, y {p.py}, fragments {self.game.level().section_collected}"

    def buttons(self, wanted: str) -> None:
        """Hold exactly the named buttons ("a right", or "" for none)."""
        wanted = set(wanted.split())
        for button in self.held - wanted:
            self.game.pyboy.button_release(button)
        for button in wanted - self.held:
            self.game.pyboy.button_press(button)
        self.held = wanted

    def frame(self) -> None:
        """Step one frame; fail if it cost a life."""
        self.game.frames(1)
        if self.game.lives() != self.lives:
            raise AssertionError(f"lost a life at {self.where()}")

    def wait(self, condition, what: str) -> None:
        for _ in range(self.TIMEOUT):
            if condition():
                return
            self.frame()
        raise AssertionError(f"waited too long for {what} at {self.where()}")

    def run_to(self, x: int) -> None:
        """Run until the body center reaches x."""
        right = x > self.game.player().px
        self.buttons("right" if right else "left")
        self.wait(lambda: self.game.player().px >= x if right else self.game.player().px <= x, f"x {x}")

    def stop(self) -> None:
        """Let go of everything and wait until standing still on the ground."""
        self.buttons("")
        self.wait(lambda: self.game.player().on_ground and self.game.player().velocity_x == 0, "a stop")

    def jump(self, hold: int, steer: int = 0, direction: str = "right") -> None:
        """Jump holding A for hold frames. steer: frames to hold the direction ("right",
        "left", or "" for straight up), or 0 to hold it until the landing and keep running."""
        self.wait(lambda: self.game.player().on_ground, "ground to jump from")
        for frame in range(self.TIMEOUT):
            self.buttons((direction if not steer or frame < steer else "") + (" a" if frame < hold else ""))
            self.frame()
            if frame and self.game.player().on_ground:
                break
        self.buttons("" if steer else direction)
        self.log(f"landed at {self.where()}")

    def drop_off(self) -> None:
        """Walk right off the ledge and fall straight down."""
        self.buttons("right")
        self.wait(lambda: not self.game.player().on_ground, "the ledge's edge")
        self.buttons("")
        self.wait(lambda: self.game.player().on_ground, "a landing")
        self.log(f"dropped to {self.where()}")

    def drop_through(self) -> None:
        """Down + A on a one-way top: fall through it to the next floor."""
        self.wait(lambda: self.game.player().on_ground, "ground to drop from")
        self.buttons("down")
        self.frame()
        self.buttons("down a")
        self.wait(lambda: not self.game.player().on_ground, "the drop")
        self.buttons("")
        self.wait(lambda: self.game.player().on_ground, "a landing")
        self.log(f"dropped through to {self.where()}")

    def bot(self, patrol_left: int):
        """The bot whose patrol starts at x patrol_left (its Tiled rectangle's left edge)."""
        for actor in self.game.actors():
            if actor.bot and actor.min_x == patrol_left + 8:
                return actor
        raise AssertionError(f"no bot patrols from x {patrol_left} at {self.where()}")

    def wait_for_bot(self, patrol_left: int, x_from: int, x_to: int) -> None:
        """Stand still until the bot walks right, away from the player, between two x."""
        self.stop()
        self.wait(lambda: self.bot(patrol_left).velocity_x > 0 and x_from <= self.bot(patrol_left).x <= x_to,
                  f"the bot from x {patrol_left}")

    def pass_bot(self, patrol_left: int, hold: int = 30, distance: int = 48) -> None:
        """Wait until the bot walks away, follow it, and jump over it when it turns back
        and is distance px away."""
        self.stop()
        self.wait(lambda: self.bot(patrol_left).velocity_x > 0 and self.bot(patrol_left).x - self.game.player().px > 32,
                  f"the bot from x {patrol_left} to walk away")

        def turned_back() -> bool:
            gap = self.bot(patrol_left).x - self.game.player().px
            self.buttons("right" if gap > self.FOLLOW_GAP else "")
            return self.bot(patrol_left).velocity_x < 0 and gap <= distance

        self.wait(turned_back, f"the bot from x {patrol_left} to turn back")
        self.jump(hold)

    def lift(self, center_x: int):
        """The lift whose center is at x center_x (its Tiled rectangle's left edge + 12)."""
        for actor in self.game.actors():
            if actor.lift and actor.x == center_x:
                return actor
        raise AssertionError(f"no lift at x {center_x} at {self.where()}")

    def wait_for_lift(self, center_x: int, at_top: bool, frames_left: int = 20) -> None:
        """Stand still until the lift waits at its highest (or lowest) place, with at least
        frames_left frames of its pause to go."""
        def waiting() -> bool:
            lift = self.lift(center_x)
            return lift.y == (lift.min_y if at_top else lift.max_y) and lift.wait_frames >= frames_left
        self.wait(waiting, f"the lift at x {center_x} to wait at its {'top' if at_top else 'bottom'}")

    def on_lift(self, center_x: int) -> bool:
        """Standing on the lift? The player is read as of its last draw, the lift live, so
        a moving lift may be 1 px further."""
        player, lift = self.game.player(), self.lift(center_x)
        return player.on_ground and abs(player.py - (lift.y - 1)) <= 1 and abs(player.px - center_x) < 20

    def board_lift(self, center_x: int) -> None:
        """Stand under a lift and jump straight up onto it while it waits at its lowest place."""
        self.run_to(center_x); self.stop()
        self.wait_for_lift(center_x, at_top=False, frames_left=26)
        self.jump(12, direction="")
        assert self.on_lift(center_x), f"missed the lift at x {center_x}: {self.where()}"

    def ride_to_top(self, center_x: int) -> None:
        """Stand on the lift until it waits at its highest place."""
        self.buttons("")
        self.wait(lambda: self.on_lift(center_x) and self.lift(center_x).y == self.lift(center_x).min_y,
                  f"the lift at x {center_x} to reach its top")
        self.log(f"lift at its top: {self.where()}")

    def open_gate(self, section: int) -> None:
        """Run right into a gate until it opens and the section (1 = B) starts."""
        self.buttons("right")
        self.wait(lambda: self.game.level().section >= section, "the gate to open")
        self.log(f"gate opened at {self.where()}")

    def reach_exit(self) -> None:
        """Run right until the level is complete."""
        self.buttons("right")
        self.wait(lambda: self.game.state() == LEVEL_COMPLETE, "the exit")
        self.buttons("")
        self.log(f"level complete, {self.where()}")

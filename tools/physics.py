"""A frame-exact copy of the player physics in src/player/player.c, for tools that plan
levels (tools/levelpreview.py).

tests/test_levelpreview.py runs the same jumps here and in the ROM and compares every
frame. Change this file together with player.c and player.h.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Callable

# src/player/player.h
SUBPIXELS = 32
SPRITE_LEFT_W, SPRITE_RIGHT_W, SPRITE_HEAD_OFFSET = 8, 7, 31
RUN_SPEED_MAX, RUN_ACCEL, RUN_DECEL, AIR_ACCEL, AIR_DECEL = 48, 8, 6, 5, 4
JUMP_SPEED, GRAVITY_RISE, GRAVITY_FALL, APEX_SPEED, FALL_SPEED_MAX = 96, 4, 12, 16, 112
COYOTE_FRAMES, JUMP_BUFFER_FRAMES, CORNER_CORRECTION = 6, 5, 4

# src/level/level_data.h and level_manager.h
NONE, SOLID, ONE_WAY, HAZARD = 0, 1, 2, 3
RING_ROWS = 16
BLOCKS_FEET = (1 << SOLID) | (1 << ONE_WAY)
BLOCKS_BODY = 1 << SOLID

# src/main.c
FALL_DEATH_Y = 144 + SPRITE_HEAD_OFFSET


@dataclass
class Room:
    """What the physics collides with: the level's width and its tile collision types
    (tile_y in screen rows). min_x is the leftmost body center in 1/32 px."""
    width_px: int
    collision: Callable[[int, int], int]
    min_x: int = SPRITE_LEFT_W * SUBPIXELS

    def at(self, tile_x: int, tile_y: int) -> int:
        if tile_x >= self.width_px // 8 or tile_y >= RING_ROWS:
            return NONE
        return self.collision(tile_x, tile_y)


@dataclass
class Body:
    """The physics fields of the Player struct."""
    x: int                      # 1/32 px
    y: int
    velocity_x: int = 0
    velocity_y: int = 0
    jump_held: bool = False
    jump_buffer_frames: int = 0
    coyote_frames: int = 0
    on_ground: bool = False
    touched_hazard: bool = False

    @property
    def px(self) -> int:
        return self.x >> 5

    @property
    def py(self) -> int:
        return self.y >> 5

    @property
    def dead(self) -> bool:
        """main.c ends the life: fell below the screen, or touched a hazard."""
        return self.py > FALL_DEATH_Y or self.touched_hazard


def tile_of(px: int) -> int:
    return 0 if px < 0 else px >> 3


def area_has(room: Room, left: int, top: int, right: int, bottom: int, types: int) -> bool:
    if bottom < 0:
        return False
    return any(types & (1 << room.at(tile_x, tile_y))
               for tile_y in range(tile_of(top), tile_of(bottom) + 1)
               for tile_x in range(tile_of(left), tile_of(right) + 1))


def row_has(room: Room, y: int, left: int, right: int, types: int) -> bool:
    return area_has(room, left, y, right, y, types)


def approach(speed: int, target: int, step: int) -> int:
    if speed < target:
        return speed + step if target - speed > step else target
    if speed > target:
        return speed - step if speed - target > step else target
    return speed


def update_run_speed(p: Body, input_x: int) -> None:
    if input_x:
        p.velocity_x = approach(p.velocity_x, input_x * RUN_SPEED_MAX, RUN_ACCEL if p.on_ground else AIR_ACCEL)
    else:
        p.velocity_x = approach(p.velocity_x, 0, RUN_DECEL if p.on_ground else AIR_DECEL)


def try_jump(p: Body) -> None:
    if p.jump_buffer_frames and (p.on_ground or p.coyote_frames):
        p.velocity_y = -JUMP_SPEED
        p.jump_buffer_frames = p.coyote_frames = 0
        p.on_ground = False
        return
    if p.jump_buffer_frames:
        p.jump_buffer_frames -= 1
    if p.coyote_frames and not p.on_ground:
        p.coyote_frames -= 1


def choose_gravity(p: Body) -> int:
    gravity = GRAVITY_RISE if p.jump_held and p.velocity_y < 0 else GRAVITY_FALL
    if p.jump_held and -APEX_SPEED < p.velocity_y < APEX_SPEED:
        gravity >>= 1
    return gravity


def apply_gravity(p: Body) -> None:
    if not p.on_ground:
        p.velocity_y = min(p.velocity_y + choose_gravity(p), FALL_SPEED_MAX)


def move_x(p: Body, room: Room) -> None:
    if not p.velocity_x:
        return
    moving_right = p.velocity_x > 0
    max_x = (room.width_px - 1 - SPRITE_RIGHT_W) * SUBPIXELS
    x = p.x + p.velocity_x
    if x < room.min_x or x > max_x:
        x = room.min_x if x < room.min_x else max_x
        p.velocity_x = 0
    feet = p.py
    edge = (x >> 5) + SPRITE_RIGHT_W if moving_right else (x >> 5) - SPRITE_LEFT_W
    if area_has(room, edge, feet - SPRITE_HEAD_OFFSET, edge, feet, BLOCKS_BODY):
        center = (edge & ~7) - 1 - SPRITE_RIGHT_W if moving_right else (edge | 7) + 1 + SPRITE_LEFT_W
        x = center * SUBPIXELS
        p.velocity_x = 0
    p.x = x


def slide_past_corner(p: Body, room: Room, head: int) -> bool:
    left, right = p.px - SPRITE_LEFT_W, p.px + SPRITE_RIGHT_W
    row, first_col, last_col = tile_of(head), left >> 3, right >> 3
    first_solid = room.at(first_col, row) == SOLID
    last_solid = room.at(last_col, row) == SOLID
    if first_solid and not last_solid:
        shift = ((first_col + 1) << 3) - left
    elif last_solid and not first_solid:
        shift = (last_col << 3) - 1 - right
    else:
        return False
    if abs(shift) > CORNER_CORRECTION:
        return False
    if area_has(room, left + shift, head, right + shift, head + SPRITE_HEAD_OFFSET, BLOCKS_BODY):
        return False
    p.x += shift * SUBPIXELS
    return True


def move_y(p: Body, room: Room) -> None:
    if not p.velocity_y:
        return
    y = max(p.y + p.velocity_y, 0)          # never above the top of the map
    feet = y >> 5
    left, right = p.px - SPRITE_LEFT_W, p.px + SPRITE_RIGHT_W
    if p.velocity_y > 0:
        if feet >> 3 != p.py >> 3 and row_has(room, feet, left, right, BLOCKS_FEET):
            y = ((feet & ~7) - 1) * SUBPIXELS
            p.velocity_y = 0
    else:
        head = feet - SPRITE_HEAD_OFFSET
        if row_has(room, head, left, right, BLOCKS_BODY) and not slide_past_corner(p, room, head):
            y = ((head | 7) + 1 + SPRITE_HEAD_OFFSET) * SUBPIXELS
            p.velocity_y = 0
    p.y = y


def update_ground(p: Body, room: Room) -> None:
    below = p.py + 1
    p.on_ground = (p.velocity_y >= 0 and below & 7 == 0 and
                   row_has(room, below, p.px - SPRITE_LEFT_W, p.px + SPRITE_RIGHT_W, BLOCKS_FEET))
    if p.on_ground:
        p.velocity_y = 0
        p.coyote_frames = COYOTE_FRAMES


def body_tiles(p: Body) -> list[tuple[int, int]]:
    """The tiles the body overlaps (check_touched_tiles in player.c)."""
    head = p.py - SPRITE_HEAD_OFFSET
    return [(tile_x, tile_y)
            for tile_y in range(tile_of(head), (p.py >> 3) + 1)
            for tile_x in range((p.px - SPRITE_LEFT_W) >> 3, ((p.px + SPRITE_RIGHT_W) >> 3) + 1)]


def step(p: Body, room: Room, input_x: int, a_down: bool, a_was_down: bool) -> None:
    """One frame: input_read_player() and player_update()."""
    p.jump_held = a_down
    if a_down and not a_was_down:
        p.jump_buffer_frames = JUMP_BUFFER_FRAMES
    update_run_speed(p, input_x)
    try_jump(p)
    apply_gravity(p)
    move_x(p, room)
    move_y(p, room)
    update_ground(p, room)
    p.touched_hazard = any(room.at(x, y) == HAZARD for x, y in body_tiles(p))

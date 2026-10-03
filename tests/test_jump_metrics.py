"""The jump metrics in docs/LEVEL_DESIGN.md section 4, measured in an empty room.

If one of these fails after a physics change, measure again and update the table: the
level layouts are built on these numbers.
"""

from gamelib import (COLLISION_NONE, COLLISION_SOLID, COREGAMELOOP, LEVEL_WIDTH,
                     SPRITE_LEFT_W, SPRITE_RIGHT_W, TILE_SIZE)

FLOOR_ROW = 15
FLOOR_Y = FLOOR_ROW * TILE_SIZE - 1     # feet row of a player standing on the floor
LEDGE_END_COL = 17                      # pits start right of this column
LEDGE_END = LEDGE_END_COL * TILE_SIZE + TILE_SIZE - 1   # last pixel of the ledge (143)
RUN_START_X = 24                        # running jumps start here, about 15 tiles back

# docs/LEVEL_DESIGN.md section 4, in tiles.
MAX_CLIMB_HELD = 4
MAX_CLIMB_TAPPED = 1
MAX_GAP_STANDING = 7
MAX_GAP_RUNNING = 9


def empty_room(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.frames(5)
    game.empty_room()
    return game.save()


def jump(game, frames_held: int):
    """Press A, hold it for frames_held, and wait until the player lands again.
    Returns (landed, takeoff_x): landed is False after a fall out of the level, and
    takeoff_x is the body center on the first frame of upward speed (None: no jump)."""
    takeoff_x = None
    airborne = False
    game.pyboy.button_press("a")
    try:
        for frame in range(150):
            if frame == frames_held:
                game.pyboy.button_release("a")
            game.frames(1)
            if game.state() != COREGAMELOOP:
                return False, takeoff_x
            player = game.player()
            if takeoff_x is None and player.velocity_y < 0:
                takeoff_x = player.px
            if airborne and player.on_ground:
                return True, takeoff_x
            airborne |= not player.on_ground
        return False, takeoff_x
    finally:
        game.pyboy.button_release("a")


def climbs_wall(game, height_tiles: int, frames_held: int) -> bool:
    """Standing against a wall height_tiles tall, jump while holding right: on top?"""
    wall_col = 20
    for row in range(FLOOR_ROW - height_tiles, FLOOR_ROW):
        for col in range(wall_col, LEVEL_WIDTH):
            game.set_collision(col, row, COLLISION_SOLID)
    game.place_player(wall_col * TILE_SIZE - 1 - SPRITE_RIGHT_W, FLOOR_Y)
    game.frames(2)
    game.pyboy.button_press("right")
    try:
        landed, _ = jump(game, frames_held)
    finally:
        game.pyboy.button_release("right")
    return landed and game.player().py == (FLOOR_ROW - height_tiles) * TILE_SIZE - 1


def dig_pit(game, gap_tiles: int) -> int:
    """Remove gap_tiles floor tiles right of the ledge. Returns the far side's first pixel."""
    for col in range(LEDGE_END_COL + 1, LEDGE_END_COL + 1 + gap_tiles):
        game.set_collision(col, FLOOR_ROW, COLLISION_NONE)
    return (LEDGE_END_COL + 1 + gap_tiles) * TILE_SIZE


def landed_across(game, far_side: int) -> bool:
    player = game.player()
    return player.py == FLOOR_Y and player.px + SPRITE_RIGHT_W >= far_side


def clears_gap_standing(game, gap_tiles: int) -> bool:
    """Standing at the edge, with the front of the body over the ledge's last pixel,
    press A and right together and hold both."""
    far_side = dig_pit(game, gap_tiles)
    game.place_player(LEDGE_END - SPRITE_RIGHT_W, FLOOR_Y)
    game.frames(2)
    game.pyboy.button_press("right")
    try:
        landed, _ = jump(game, 150)
    finally:
        game.pyboy.button_release("right")
    return landed and landed_across(game, far_side)


def clears_gap_running(game, gap_tiles: int) -> bool:
    """Running at full speed, press A on one of the last frames the feet are still on the
    ledge (coyote time not counted). True if any of those jumps gets across."""
    far_side = dig_pit(game, gap_tiles)
    game.place_player(RUN_START_X, FLOOR_Y)
    game.frames(2)
    run_start = game.save()
    game.pyboy.button_press("right")
    try:
        frames_to_edge = 0
        while game.player().px - SPRITE_LEFT_W <= LEDGE_END:
            game.frames(1)
            frames_to_edge += 1
        for press_at in range(frames_to_edge - 8, frames_to_edge + 1):
            game.load(run_start)
            game.pyboy.button_press("right")
            game.frames(press_at)
            landed, takeoff_x = jump(game, 150)
            from_ledge = takeoff_x is not None and takeoff_x - SPRITE_LEFT_W <= LEDGE_END
            if from_ledge and landed and landed_across(game, far_side):
                return True
        return False
    finally:
        game.pyboy.button_release("right")


def test_held_jump_climbs_4_tiles(game):
    room = empty_room(game)
    assert climbs_wall(game, MAX_CLIMB_HELD, frames_held=150)
    game.load(room)
    assert not climbs_wall(game, MAX_CLIMB_HELD + 1, frames_held=150)


def test_tapped_jump_climbs_1_tile(game):
    room = empty_room(game)
    assert climbs_wall(game, MAX_CLIMB_TAPPED, frames_held=1)
    game.load(room)
    assert not climbs_wall(game, MAX_CLIMB_TAPPED + 1, frames_held=1)


def test_standing_jump_clears_7_tiles(game):
    room = empty_room(game)
    assert clears_gap_standing(game, MAX_GAP_STANDING)
    game.load(room)
    assert not clears_gap_standing(game, MAX_GAP_STANDING + 1)


def test_running_jump_clears_9_tiles(game):
    room = empty_room(game)
    assert clears_gap_running(game, MAX_GAP_RUNNING)
    game.load(room)
    assert not clears_gap_running(game, MAX_GAP_RUNNING + 1)

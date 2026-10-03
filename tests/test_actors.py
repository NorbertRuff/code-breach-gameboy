"""Actors (roadmap M3): spawned from the level's entity list as the camera reaches them,
updated and drawn through their type, removed a screen behind the camera. In
levels/greybox_test.tmj: bot B patrols platform B (centers 72-88), bot D platform D
(176-192), the floor bot the floor past gate A (416-448)."""

from gamelib import ACTOR_SIZE, BOT_TILES, COREGAMELOOP

BOT_B, BOT_D, FLOOR_BOT = 72, 176, 416      # min_x of each bot's patrol
SPAWN_REACH = 160 + 16                      # screen width + SPAWN_AHEAD (src/actor/actor.c)


def patrol_starts(game) -> list:
    return sorted(a.min_x for a in game.actors())


def expected_sprite_count(game) -> int:
    """The player's 4 hardware sprites unless it flashes, and 2 per actor whose 16 px body
    is at least partly on screen."""
    flashing = game.player().invincibility_frames & 2
    camera = game.camera_x()
    return (0 if flashing else 4) + 2 * sum(-8 < a.x - camera < 168 for a in game.actors())


def test_actor_struct_size_matches(game):
    # If this fails, update ACTOR_FIELDS in tests/gamelib.py to match actor.h.
    assert game.u8("_actor_struct_size") == ACTOR_SIZE


def test_bots_in_reach_spawn_at_the_level_start(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    actors = game.actors()
    assert all(a.bot for a in actors)
    assert sorted((a.min_x, a.max_x, a.y) for a in actors) == [(72, 88, 71), (176, 192, 79)]


def test_a_bot_spawns_when_the_camera_comes_near(game):
    game.start_on_open_ground()
    assert game.camera_x() + SPAWN_REACH <= FLOOR_BOT - 8
    assert FLOOR_BOT not in patrol_starts(game)
    game.place_player(330, 87)                  # on the one-way shelf: the camera jumps right
    bot = next(a for a in game.actors() if a.min_x == FLOOR_BOT)
    assert bot.direction == 1 and FLOOR_BOT < bot.x <= FLOOR_BOT + game.PLACE_FRAMES, "starts at the left end"


def test_bot_patrols_between_its_ends(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    seen = []
    for _ in range(40):
        game.frames(1)
        bot = next(a for a in game.actors() if a.min_x == BOT_B)
        seen.append((bot.x, bot.direction))
    xs = [x for x, _ in seen]
    assert min(xs) == 72 and max(xs) == 88
    assert {d for _, d in seen} == {1, -1}


def test_bots_a_screen_behind_are_removed_until_a_restart(game):
    game.start_on_open_ground()
    game.place_player(480, 103)                 # far end, left of the exit: the camera is at 352
    game.frames(32)                             # bot D's patrol dips below 352 - 160
    assert patrol_starts(game) == [FLOOR_BOT]
    game.place_player(156, 119)                 # back: bots B and D stay gone
    assert patrol_starts(game) == [FLOOR_BOT]
    game.lose_life()
    game.wait_for_state(COREGAMELOOP)
    assert patrol_starts(game) == [BOT_B, BOT_D]


def test_touching_a_bot_costs_a_life_unless_invincible(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.place_player(80, 71)                   # on platform B, in the bot's way
    assert game.lives() == 2
    game.wait_for_state(COREGAMELOOP)
    game.place_player(80, 71)                   # again, while invincible after the respawn
    game.frames(30)
    assert game.lives() == 2


def test_sprite_list_has_the_player_and_the_bots_on_screen(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.frames(2)
    sprites = game.visible_sprites()
    bot = next(a for a in game.actors() if a.min_x == BOT_B)
    left = bot.x - 8 - game.camera_x()
    assert len(sprites) == 6, "the player's 4 hardware sprites and bot B's 2, the rest hidden"
    assert {(x, y) for x, y, _ in sprites[4:]} == {(left, bot.y - 15), (left + 8, bot.y - 15)}
    assert {tile for _, _, tile in sprites[4:]} == set(BOT_TILES)


def test_sprite_list_is_rebuilt_every_frame(game):
    # Running right, bots come into view and leave it; after a respawn the player flashes.
    # Every frame shows exactly the sprites in play, with nothing left over.
    game.start_on_open_ground()
    game.pyboy.button_press("right")
    for _ in range(60):
        game.frames(1)
        assert len(game.visible_sprites()) == expected_sprite_count(game)
    game.pyboy.button_release("right")
    game.lose_life()
    game.wait_for_state(COREGAMELOOP)
    for _ in range(20):
        game.frames(1)
        assert len(game.visible_sprites()) == expected_sprite_count(game)

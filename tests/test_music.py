"""Music (roadmap M6): hUGEDriver plays the level's song one step per frame, from the line
interrupt below the parallax band, and sound effects take their channels from the music
until they end (src/sound/music.c, src/sound/sound.c)."""

import sys
from pathlib import Path

from gamelib import COREGAMELOOP, LEVEL_1, LEVEL_2, LEVEL_3, LEVEL_4, LEVEL_COMPLETE, PAUSED, TITLE_SCREEN_STATE

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import fxhammer2c  # noqa: E402

BAND_SPLIT_LINE = 23        # src/engine/screen.h: BAND_LAST_LINE


def count_music_steps(game) -> list:
    """From now on, add the screen line to the returned list at every music step."""
    lines = []
    game.pyboy.hook_register(None, "_hUGE_dosound", lambda _: lines.append(game.reg("LY")), None)
    return lines


def test_one_music_step_per_frame_below_the_band(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    steps = count_music_steps(game)
    game.frames(120)
    assert 119 <= len(steps) <= 121
    assert all(BAND_SPLIT_LINE <= line <= BAND_SPLIT_LINE + 2 for line in steps), sorted(set(steps))


def test_the_music_keeps_playing_while_paused(game):
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.press("start")
    game.frames(1)
    assert game.state() == PAUSED
    steps = count_music_steps(game)
    game.frames(60)
    assert len(steps) >= 59


def track_songs(game) -> list:
    """From now on, add the name of each song the game starts to the returned list."""
    names = {game.pyboy.symbol_lookup(symbol)[1]: symbol[len("_song_"):]
             for symbol in ("_song_tech", "_song_neurotic_robonaut", "_song_observing_jupiter", "_song_decampment",
                            "_song_gone_missing", "_song_delight", "_song_whirlwind")}
    registers = game.pyboy.register_file
    started = []
    # hUGE_init gets the song's address in DE (GBDK's calling convention).
    game.pyboy.hook_register(None, "_hUGE_init",
                             lambda _: started.append(names[registers.D << 8 | registers.E]), None)
    return started


def press_start_on_screen(game, presses: int = 1) -> None:
    """Wait on a title or message screen, then START back to the game; START on each of
    `presses` screens in turn."""
    for _ in range(presses):
        game.frames(200)
        game.hold("start", 3)
    game.wait_for_state(COREGAMELOOP, timeout=300)


def test_each_level_and_screen_starts_its_own_song(game):
    # After the title, game over and level complete, a level's song starts again.
    game.wait_for_state(COREGAMELOOP, timeout=300)
    songs = track_songs(game)
    for level in (LEVEL_1, LEVEL_2, LEVEL_3, LEVEL_4):
        game.load_level(level)
    game.set_state(TITLE_SCREEN_STATE)
    press_start_on_screen(game, presses=3)              # PRESS START, NEW GAME, the boot screen
    game.set_lives(1)
    game.lose_life()
    game.answer_game_over()
    game.wait_for_state(COREGAMELOOP, timeout=300)
    game.set_state(LEVEL_COMPLETE)
    game.wait_for_text(4, 14, "PRESS START")
    game.tap("start")
    game.wait_for_text(0, 9, "NOT FOUND IN RACKS.")     # the elevator ride's log
    game.tap("start")
    game.wait_for_state(COREGAMELOOP, timeout=300)
    assert songs == ["tech", "neurotic_robonaut", "observing_jupiter", "whirlwind", "decampment", "tech",
                     "gone_missing", "tech", "delight", "neurotic_robonaut"]


def test_a_sound_effect_takes_its_channels_until_it_ends(game):
    jump = next(e for e in fxhammer2c.read_pick_list(ROOT / "sounds/effects.txt") if e.name == "jump")
    jump_frames = sum(step[0] for step in jump.steps)
    game.start_on_open_ground()
    game.frames(30)                                     # the pickup sound of the fragment there ends
    assert not game.u8("_hUGE_mute_mask") & jump.channels
    game.press("a")
    game.frames(1)
    assert game.u8("_hUGE_mute_mask") & jump.channels == jump.channels, "the jump didn't take its channels"
    game.frames(jump_frames + 1)
    assert not game.u8("_hUGE_mute_mask") & jump.channels, "the channels never went back to the music"

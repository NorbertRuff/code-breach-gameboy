"""Music (roadmap M6): hUGEDriver plays the level's song one step per frame, from the line
interrupt below the parallax band, and sound effects take their channels from the music
until they end (src/sound/music.c, src/sound/sound.c)."""

import sys
from pathlib import Path

from gamelib import COREGAMELOOP, LEVEL_1, LEVEL_2, PAUSED

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


def test_each_game_level_starts_its_own_song(game):
    # hUGE_init gets the song's address in DE (GBDK's calling convention).
    started = []
    registers = game.pyboy.register_file
    game.pyboy.hook_register(None, "_hUGE_init", lambda _: started.append(registers.D << 8 | registers.E), None)
    for level in (LEVEL_1, LEVEL_2):
        game.load_level(level)
    songs = [game.pyboy.symbol_lookup(name)[1] for name in ("_song_tech", "_song_neurotic_robonaut")]
    assert started[-2:] == songs


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

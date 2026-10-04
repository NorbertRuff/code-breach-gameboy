"""Music (roadmap M6): hUGEDriver plays the level's song one step per frame, from the line
interrupt below the parallax band, and sound effects take their channel from the music
until they have faded (src/sound/music.c, src/sound/sound.c)."""

from gamelib import COREGAMELOOP, LEVEL_1, LEVEL_2, PAUSED

BAND_SPLIT_LINE = 23        # src/engine/screen.h: BAND_LAST_LINE
HT_CH1 = 1                  # hUGE_mute_mask bit of channel 1
JUMP_EFFECT_FRAMES = 4      # sound_play_jump in sound.c


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


def test_a_sound_effect_takes_its_channel_until_it_fades(game):
    game.start_on_open_ground()
    assert not game.u8("_hUGE_mute_mask") & HT_CH1
    game.press("a")                                     # the jump sound is on CH1
    game.frames(1)
    assert game.u8("_hUGE_mute_mask") & HT_CH1, "the jump didn't take CH1 from the music"
    game.frames(JUMP_EFFECT_FRAMES + 1)
    assert not game.u8("_hUGE_mute_mask") & HT_CH1, "CH1 never went back to the music"

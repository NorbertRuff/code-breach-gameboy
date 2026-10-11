"""The committed generated files match their sources: a level, a song or the effect list
changed without its make target fails here."""

from pathlib import Path

import fxhammer2c
import tiled2c
import uge2c

ROOT = Path(__file__).resolve().parent.parent


def assert_same_files(generated_dir: Path, committed_dir: Path, make_target: str) -> None:
    for generated in generated_dir.iterdir():
        committed = committed_dir / generated.name
        assert committed.read_text(encoding="utf-8") == generated.read_text(encoding="utf-8"), \
            f"{committed.name} is out of date: run make {make_target}"


def test_level_files_are_up_to_date(tmp_path):
    for tmj in sorted((ROOT / "levels").glob("*.tmj")):
        tiled2c.write_outputs(tiled2c.load_level(tmj), tmj.relative_to(ROOT), tmp_path)
    assert_same_files(tmp_path, ROOT / "src" / "levels", "levels")


def test_song_files_are_up_to_date(tmp_path):
    for song in sorted((ROOT / "music").glob("*.uge")):
        uge2c.write_outputs(song.relative_to(ROOT), tmp_path)
    assert_same_files(tmp_path, ROOT / "src" / "music", "music")


def test_effect_files_are_up_to_date(tmp_path):
    fxhammer2c.write_outputs(ROOT / "sounds" / "effects.txt", tmp_path)
    assert_same_files(tmp_path, ROOT / "src" / "sound", "sounds")

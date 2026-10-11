"""Record a short gameplay video: the title screen, then a few seconds of each level's
demo, in GBC mode. Writes a GIF for the README and, when ffmpeg is on PATH, an MP4 with
the music.

    python tools/record_video.py [rom] [--gif screenshots/gameplay.gif] [--mp4 release/code-breach.mp4]

"make video" runs it on obj/ApexLab.gb. The levels play their demo clips
(src/game_state/demo_clips.h), so "make demo" changes the video too.
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import tempfile
import wave
from pathlib import Path

import numpy as np
from PIL import Image
from pyboy import PyBoy

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tests"))

from gamelib import COREGAMELOOP, LEVEL_1, TITLE_SCREEN_STATE, Game  # noqa: E402

GB_FPS = 59.7275               # frames per second of the Game Boy screen
SAMPLE_RATE = 48000
TITLE_WAIT_FRAMES = 30         # the title's blank screen before it fades in
TITLE_FRAMES = 180             # 3 s of the title screen
# Per level: the frames of its demo shown, counted from the demo's start (the fade in
# takes the first second; KERNEL's log the first six).
SEGMENTS = [(60, 480), (60, 420), (240, 600), (540, 900), (420, 900)]
GIF_FRAME_STEP = 3             # 20 frames per second
GIF_SCALE = 2
MP4_SCALE = 4


class Recorder:
    """Steps the game and keeps the picture and sound of the frames asked for."""

    def __init__(self, rom: Path):
        self.pyboy = PyBoy(str(rom), window="null", cgb=True, sound_emulated=True,
                           sound_sample_rate=SAMPLE_RATE)
        self.pyboy.set_emulation_speed(0)
        self.game = Game(self.pyboy, "cgb", ROOT / "tests" / "out", "record_video", LEVEL_1)
        self.frames: list[Image.Image] = []
        self.sound: list[np.ndarray] = []

    def skip(self, frames: int) -> None:
        for _ in range(frames):
            self.pyboy.tick(1, False)

    def capture(self, frames: int) -> None:
        for _ in range(frames):
            self.pyboy.tick(1, True)
            self.frames.append(self.pyboy.screen.image.convert("RGB"))
            self.sound.append(self.pyboy.sound.ndarray.copy())

    def skip_while(self, condition) -> None:
        while condition():
            self.pyboy.tick(1, False)


def record(rom: Path) -> Recorder:
    """The title screen, then each level's demo segment in turn."""
    recorder = Recorder(rom)
    demo_playing = lambda: recorder.game.u8("_demo_playing")
    recorder.game.wait_for_state(COREGAMELOOP, timeout=600)
    recorder.game.set_state(TITLE_SCREEN_STATE)
    recorder.skip(TITLE_WAIT_FRAMES)
    recorder.capture(TITLE_FRAMES)
    for start, end in SEGMENTS:
        recorder.skip_while(lambda: not demo_playing())
        recorder.skip(start)
        recorder.capture(end - start)
        recorder.skip_while(demo_playing)
    recorder.pyboy.stop(save=False)
    return recorder


def write_gif(frames: list[Image.Image], path: Path) -> None:
    shown = [f.resize((f.width * GIF_SCALE, f.height * GIF_SCALE), Image.NEAREST)
             for f in frames[::GIF_FRAME_STEP]]
    path.parent.mkdir(parents=True, exist_ok=True)
    shown[0].save(path, save_all=True, append_images=shown[1:], loop=0, optimize=True,
                  duration=round(1000 * GIF_FRAME_STEP / GB_FPS))


def write_mp4(frames: list[Image.Image], sound: list[np.ndarray], path: Path) -> None:
    """Every frame at the Game Boy's rate, scaled up without blur, with the sound."""
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as folder:
        audio = Path(folder) / "sound.wav"
        with wave.open(str(audio), "wb") as wav:
            wav.setnchannels(2)
            wav.setsampwidth(2)
            wav.setframerate(SAMPLE_RATE)
            wav.writeframes((np.concatenate(sound).astype(np.int16) * 256).tobytes())
        width, height = frames[0].size
        ffmpeg = subprocess.Popen(
            ["ffmpeg", "-y", "-loglevel", "error", "-f", "rawvideo", "-pix_fmt", "rgb24",
             "-s", f"{width}x{height}", "-r", str(GB_FPS), "-i", "-", "-i", str(audio),
             "-vf", f"scale={width * MP4_SCALE}:{height * MP4_SCALE}:flags=neighbor",
             "-c:v", "libx264", "-pix_fmt", "yuv420p", "-crf", "18", "-c:a", "aac",
             "-b:a", "160k", "-shortest", str(path)], stdin=subprocess.PIPE)
        for frame in frames:
            ffmpeg.stdin.write(frame.tobytes())
        ffmpeg.stdin.close()
        if ffmpeg.wait():
            raise SystemExit("ffmpeg failed")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("rom", nargs="?", default=str(ROOT / "obj" / "ApexLab.gb"))
    parser.add_argument("--gif", default=str(ROOT / "screenshots" / "gameplay.gif"))
    parser.add_argument("--mp4", default=str(ROOT / "release" / "code-breach.mp4"))
    args = parser.parse_args()
    recorder = record(Path(args.rom))
    write_gif(recorder.frames, Path(args.gif))
    print(f"{args.gif}: {len(recorder.frames[::GIF_FRAME_STEP])} frames, "
          f"{Path(args.gif).stat().st_size // 1024} KB")
    if shutil.which("ffmpeg"):
        write_mp4(recorder.frames, recorder.sound, Path(args.mp4))
        print(f"{args.mp4}: {len(recorder.frames) / GB_FPS:.0f} s, {Path(args.mp4).stat().st_size // 1024} KB")
    else:
        print("ffmpeg isn't on PATH: no MP4")


if __name__ == "__main__":
    main()

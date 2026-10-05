"""Level 5, KERNEL (src/game_state/kernel.c, docs/LEVEL_DESIGN.md section 7): a replay
beats it without losing a life, within the frame budget and the sprites-per-line limit, and
goes on through the ending and the credits to the title; packets warn before they fall; an
upload needs Up held; a lost life keeps the uploads, a game over clears them."""

import pytest

from gamelib import (BOSS_HIT, COREGAMELOOP, ENDING, LEVEL_5, LINES_PER_FRAME, TITLE_SCREEN_STATE,
                     Pilot)
from test_frame_budget import FRAME_LINE_LIMIT
from test_level_1 import SPRITES_PER_LINE

PHASE_FRAMES = "Fkernel$phase_frames$0_0$0"     # kernel.c's statics, in the .sym file
VOLLEY_FRAMES = "Fkernel$volley_frames$0_0$0"
UPLOAD_FRAMES = "Fkernel$upload_frames$0_0$0"
TERMINAL_LIGHT_FRAMES = 150
FLOOR_Y, FLOOR_ROW, SOLID = 119, 15, 1
TERMINALS = (12, 146, 76)                       # body x on each terminal, in order
FLOOR, SHELF = (12, 150), (130, 152)            # body x ranges to dodge in
LIFT = 116                                      # the lift's center x


class KernelPilot(Pilot):
    """Pilot moves for the arena: dodge packets and corrupted floor, go to a terminal and
    hold Up on it. A body at x spans x - 8 to x + 7; a packet's hit box min_x - 7 to + 6."""

    TIMEOUT = 1500      # frames a move may take: the lift and the volleys must line up

    def packets(self):
        return [a for a in self.game.actors() if a.packet]

    def uploads(self) -> int:
        return self.game.level().section_collected[0]

    def under_packet(self, x: int, margin: int = 0, soon: bool = False) -> bool:
        """Is x in a packet's lane? soon: only packets about to fall, or falling."""
        return any(x - 8 - margin <= a.min_x + 6 and x + 7 + margin >= a.min_x - 7
                   for a in self.packets() if not soon or a.wait_frames < 12)

    def floor_ok(self, x: int) -> bool:
        """Off the floor, or over floor tiles that are solid (no corrupted segment)."""
        columns = [c for c in ((x - 8) // 8, (x + 7) // 8) if 0 <= c < 20]
        return self.game.player().py != FLOOR_Y or all(self.game.collision(c, FLOOR_ROW) == SOLID for c in columns)

    def safe_spot(self, x_range: tuple) -> int:
        x = self.game.player().px
        for distance in range(0, 160, 4):
            for spot in (x - distance, x + distance):
                if x_range[0] <= spot <= x_range[1] and not self.under_packet(spot, 2) and self.floor_ok(spot):
                    return spot
        raise AssertionError(f"nowhere safe at {self.where()}")

    def step(self, x_range: tuple, target=None, hold: str = "") -> None:
        """One frame: out of a packet's lane or off a corrupted segment if needed, else
        toward target (waiting while the way is under a packet), else hold buttons."""
        x = self.game.player().px
        if self.under_packet(x) or not self.floor_ok(x):
            target, hold = self.safe_spot(x_range), ""
        if target is not None and abs(x - target) > 5:
            ahead = x + (12 if target > x else -12)
            blocked = self.under_packet(ahead, soon=True) or not self.floor_ok(ahead)
            self.buttons("" if blocked else "right" if target > x else "left")
        else:
            self.buttons(hold)
        self.frame()

    def step_until(self, done, x_range: tuple, target=lambda: None, hold: str = "", what: str = "") -> None:
        """step() every frame until done()."""
        for _ in range(self.TIMEOUT):
            if done():
                return
            self.step(x_range, target(), hold)
        raise AssertionError(f"waited too long for {what} at {self.where()}")

    def lit(self) -> bool:
        return self.game.u8(PHASE_FRAMES) >= TERMINAL_LIGHT_FRAMES

    def upload(self, terminal_x: int, x_range: tuple) -> None:
        """Dodge until the phase's terminal lights, then hold Up on it until the upload is done."""
        done = self.uploads() + 1
        self.step_until(lambda: self.uploads() == done, x_range, lambda: terminal_x if self.lit() else None,
                        "up", f"upload {done}")
        self.buttons("")
        self.game.wait_for_state(ENDING if done == 3 else COREGAMELOOP, timeout=300)

    def board_lift(self) -> None:
        """Stand under the lift and jump onto it while it waits at the bottom, with no packet
        over it and the next volley more than 40 frames away."""
        def ready() -> bool:
            lift, player = next(a for a in self.game.actors() if a.lift), self.game.player()
            return (lift.y == lift.max_y and lift.wait_frames >= 16 and not self.under_packet(LIFT, 8)
                    and abs(player.px - LIFT) <= 5 and player.on_ground and player.velocity_x == 0
                    and self.game.u8(VOLLEY_FRAMES) > 40)
        self.step_until(ready, FLOOR, lambda: LIFT, what="the lift")
        self.jump(12, direction="")
        assert self.on_lift(LIFT), self.where()

    def stand_on_shelf_until_lit(self) -> None:
        self.step_until(self.lit, SHELF, what="the terminal to light")


def beat_kernel(pilot: KernelPilot) -> None:
    pilot.upload(TERMINALS[0], FLOOR)                 # phase 1: terminal 1, on the floor
    pilot.board_lift()                                # phase 2: up the lift to the shelf
    pilot.ride_to_top(LIFT)
    pilot.upload(TERMINALS[1], SHELF)                # off the lift onto the shelf, dodging
    pilot.stand_on_shelf_until_lit()                  # phase 3: down to terminal 3 when it lights
    pilot.upload(TERMINALS[2], FLOOR)


@pytest.mark.level(LEVEL_5)
@pytest.mark.parametrize("start_delay", [0, pytest.param(7, marks=pytest.mark.full)])
def test_kernel_can_be_beaten_then_the_ending_goes_to_the_title(game, start_delay):
    game.wait_for_state(COREGAMELOOP, timeout=600)
    game.frames(start_delay)
    frame_lines = game.track_frame_lines()
    sprites_per_line = game.track_sprites_per_line()
    beat_kernel(KernelPilot(game))
    worst = max(frame_lines)
    assert game.u8("_lag_frames") == 0, f"lag frames; the worst update ended at line {worst}"
    assert worst <= FRAME_LINE_LIMIT, f"the worst update ended at line {worst} of {LINES_PER_FRAME}"
    assert max(sprites_per_line) <= SPRITES_PER_LINE, f"{max(sprites_per_line)} sprites on one screen line"
    game.wait_for_text(2, 8, "LOCKDOWN LIFTED")
    game.tap("start")
    game.wait_for_text(0, 14, "THANKS FOR PLAYING!")
    game.tap("start")
    game.wait_for_state(TITLE_SCREEN_STATE, timeout=300)


@pytest.mark.level(LEVEL_5)
def test_packets_warn_then_fall_and_lives_keep_the_uploads(game):
    # The first volley drops a packet in the player's lane: it blinks over the band for at
    # least 24 frames (the telegraphing rule), then falls. Standing still costs a life,
    # which keeps the upload done; a game over clears it.
    game.wait_for_state(COREGAMELOOP, timeout=600)
    pilot = KernelPilot(game)
    pilot.upload(TERMINALS[0], FLOOR)
    assert pilot.uploads() == 1
    pilot.wait(lambda: pilot.packets(), "a volley")
    packet = next(a for a in pilot.packets() if pilot.under_packet(game.player().px))
    assert packet.wait_frames >= 24 and packet.y < 24
    pilot.buttons("")
    game.frames(100)                                  # it falls on the player
    game.wait_for_state(COREGAMELOOP, timeout=200)
    assert game.lives() == 2 and pilot.uploads() == 1
    assert any(a.lift for a in game.actors()), "phase 2 restarts with its lift"
    game.set_lives(1)
    game.lose_life()
    game.answer_game_over()
    game.wait_for_state(COREGAMELOOP, timeout=300)
    assert pilot.uploads() == 0 and not any(a.lift for a in game.actors())


@pytest.mark.level(LEVEL_5)
def test_letting_go_of_up_starts_the_upload_over(game):
    game.wait_for_state(COREGAMELOOP, timeout=600)
    pilot = KernelPilot(game)
    pilot.step_until(lambda: pilot.lit() and not pilot.packets() and abs(game.player().px - TERMINALS[0]) <= 5,
                     FLOOR, lambda: TERMINALS[0], what="the first terminal")
    pilot.stop()
    pilot.buttons("up")
    for _ in range(20):
        pilot.frame()
    assert game.u8(UPLOAD_FRAMES) >= 18
    pilot.buttons("")
    pilot.frame(); pilot.frame()
    assert game.u8(UPLOAD_FRAMES) == 0 and pilot.uploads() == 0

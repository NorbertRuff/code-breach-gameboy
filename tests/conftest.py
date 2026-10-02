import shutil
from pathlib import Path

import pytest

from gamelib import Game, boot

ROOT = Path(__file__).resolve().parent.parent
OBJ = ROOT / "obj"
OUT = ROOT / "tests" / "out"


def pytest_sessionstart(session):
    # Screenshots from an earlier run would be mistaken for this run's results.
    if OUT.is_dir():
        for old in OUT.glob("*.png"):
            old.unlink()


@pytest.fixture(scope="session")
def rom_dir(tmp_path_factory) -> Path:
    """Copy the built ROMs and their .sym files into a fresh temp folder.

    PyBoy loads <rom>.ram if it exists and writes it when a battery cartridge stops.
    Running from a copy keeps tests independent of each other and of Emulicious saves.
    """
    if not (OBJ / "ApexLab.gb").exists():
        pytest.exit("obj/ApexLab.gb is missing: run 'make test' (or 'make' first)")
    folder = tmp_path_factory.mktemp("roms")
    for name in ("ApexLab", "savetest"):
        for ext in (".gb", ".sym", ".map"):
            src = OBJ / f"{name}{ext}"
            if src.exists():
                shutil.copy(src, folder / src.name)
    return folder


@pytest.fixture(scope="session")
def rom_bytes(rom_dir) -> bytes:
    return (rom_dir / "ApexLab.gb").read_bytes()


@pytest.fixture(params=["dmg", "cgb"])
def mode(request) -> str:
    """Every test that uses this runs twice: original Game Boy and Game Boy Color."""
    return request.param


@pytest.fixture
def game(request, rom_dir, mode):
    pyboy = boot(rom_dir / "ApexLab.gb", mode)
    g = Game(pyboy, mode, OUT, request.node.name)
    yield g
    report = getattr(request.node, "rep_call", None)
    if report is not None and report.failed:
        g.screenshot("failure")
    pyboy.stop(save=False)


@pytest.hookimpl(tryfirst=True, hookwrapper=True)
def pytest_runtest_makereport(item, call):
    # Store each phase's report on the test item, so fixtures can see if the test failed.
    outcome = yield
    report = outcome.get_result()
    setattr(item, "rep_" + report.when, report)

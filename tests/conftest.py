import os
import shutil
from pathlib import Path

import pytest

from gamelib import Game, boot

ROOT = Path(__file__).resolve().parent.parent
# APEXLAB_OBJ points the tests at another build folder, for example an older commit's
# build, to check that a regression test really fails without its fix.
OBJ = Path(os.environ.get("APEXLAB_OBJ", ROOT / "obj"))
OUT = ROOT / "tests" / "out"


def pytest_addoption(parser):
    parser.addoption("--update-golden", action="store_true",
                     help="rewrite the golden screenshots in tests/golden/ (make goldens)")
    parser.addoption("--full", action="store_true",
                     help="every ROM test in DMG and GBC mode, plus the tests marked full (make test-full)")


def pytest_generate_tests(metafunc):
    # The quick tier runs ROM tests in DMG mode only, unless the mode changes the result.
    # @pytest.mark.mode("cgb") pins a test to one system in both tiers.
    if "mode" in metafunc.fixturenames:
        pinned = metafunc.definition.get_closest_marker("mode")
        both = metafunc.config.getoption("--full") or metafunc.definition.get_closest_marker("both_modes")
        metafunc.parametrize("mode", list(pinned.args) if pinned else ["dmg", "cgb"] if both else ["dmg"])


def pytest_collection_modifyitems(config, items):
    # Tests marked full only run in the full tier.
    if config.getoption("--full"):
        return
    skipped = [item for item in items if item.get_closest_marker("full")]
    if skipped:
        config.hook.pytest_deselected(items=skipped)
        items[:] = [item for item in items if not item.get_closest_marker("full")]


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
    for name in ("ApexLab", "savetest", "savetest_mbc1", "savetest_mbc3", "soundtest"):
        for ext in (".gb", ".sym", ".map"):
            src = OBJ / f"{name}{ext}"
            if src.exists():
                shutil.copy(src, folder / src.name)
    return folder


@pytest.fixture(scope="session")
def rom_bytes(rom_dir) -> bytes:
    return (rom_dir / "ApexLab.gb").read_bytes()


@pytest.fixture
def game(request, rom_dir, mode):
    """The ROM booted in mode (see pytest_generate_tests), starting in the greybox test level, or in the level a
    @pytest.mark.level(LEVEL_...) marker names."""
    pyboy = boot(rom_dir / "ApexLab.gb", mode)
    marker = request.node.get_closest_marker("level")
    g = Game(pyboy, mode, OUT, request.node.name, *(marker.args if marker else ()))
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

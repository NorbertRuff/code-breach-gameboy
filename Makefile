#
# ApexLab: Code Breach — build
#
#   make            build obj/ApexLab.gb (incremental; headers are tracked)
#   make rebuild    clean, then build
#   make run        build, then open the ROM in Emulicious
#   make test       build, then the quick tests in parallel (DMG; GBC only where the mode matters)
#   make test-full  build, then every test in DMG and GBC mode (before a commit, and in CI)
#   make shots      build, then save review screenshots to tests/out/shots/
#   make goldens    build, then rewrite the golden screenshots in tests/golden/
#   make budget     build, then print where the frame time goes (tools/frame_budget.py)
#   make venv       create .venv with the test tools from tools/requirements.txt
#   make savetest   build obj/savetest.gb, the cartridge RAM persistence test, and its
#                   MBC1 and MBC3 variants for the flashcart (savetest_mbc1/3.gb)
#   make usage      show ROM and RAM use per bank
#   make assets     regenerate src/graphics/*.c from the PNGs in graphics/
#   make levels     regenerate src/levels/*.c from the Tiled maps in levels/
#   make music      regenerate src/music/*.c from the hUGETracker songs in music/
#   make sounds     regenerate src/sound/effects.c from the effect list sounds/effects.txt
#   make soundtest  build obj/soundtest.gb, every GBFX/GBFX2 effect and every song of the
#                   music packs, and render each to a WAV in tests/out/sounds/
#   make preview    draw every level to tests/out/levels/ and check it can be finished
#   make clean      delete obj/
#
# RELEASE=1 turns on slower, size-optimizing compilation (about 4x compile time).
# OBJDIR=<folder> builds into another folder, e.g. for a variant with EXTRA_CFLAGS.
#

PROJECTNAME := ApexLab
GBDK_HOME   ?= ./gbdk/

OBJDIR := obj
SRCDIR := src
ROM    := $(OBJDIR)/$(PROJECTNAME).gb
MAP    := $(OBJDIR)/$(PROJECTNAME).map

# On Windows, always run recipes with cmd.exe. Otherwise make picks sh.exe when Git Bash
# is on PATH and cmd.exe when it isn't, and the same recipe can't serve both.
ifeq ($(OS),Windows_NT)
SHELL   := cmd.exe
fixpath  = $(subst /,\,$1)
MKDIR    = if not exist $(call fixpath,$1) mkdir $(call fixpath,$1)
RMDIR    = if exist $(call fixpath,$1) rmdir /s /q $(call fixpath,$1)
VENV_PY := .venv/Scripts/python.exe
else
fixpath  = $1
MKDIR    = mkdir -p $1
RMDIR    = rm -rf $1
VENV_PY := .venv/bin/python
endif

LCC       := $(call fixpath,$(GBDK_HOME)bin/lcc)
PNG2ASSET := $(call fixpath,$(GBDK_HOME)bin/png2asset)
ROMUSAGE  := $(call fixpath,$(GBDK_HOME)bin/romusage)
EMULICIOUS := emulicious/Emulicious.jar

# Tests use the project venv when it exists, otherwise python from PATH (as in CI).
PYTHON ?= $(call fixpath,$(if $(wildcard $(VENV_PY)),$(VENV_PY),python))

# Cartridge (docs/CGB_PLAN.md section 1): MBC5 without RAM (progress is kept in
# passwords, docs/DECISIONS.md "Saves"), ROM size picked automatically, GBC-enhanced
# header that still runs on DMG, cartridge title.
CART_TYPE  := 0x19
CART_FLAGS := -Wl-yt$(CART_TYPE) -Wm-yoA -Wm-yc -Wm-ynAPEXLAB

# Save test ROMs: MBC5 + RAM + battery with 1 RAM bank (8 KB), and the same with an MBC1
# or MBC3 mapper.
SAVETEST_TYPE  := 0x1B
SAVETEST_FLAGS := -Wl-yt$(SAVETEST_TYPE) -Wm-yoA -Wm-ya1 -Wm-yc -Wm-ynAPEXLAB
CART_TYPE_MBC1 := 0x03
CART_TYPE_MBC3 := 0x13

# -debug: .cdb/.noi/.map for Emulicious. -Wm-yS: .sym for PyBoy.
LCCFLAGS := -debug
# The music driver (lib/hUGEDriver/README.md): its header for the compiler, its library
# for the linker.
HUGEDRIVER := lib/hUGEDriver

# -MMD/-MP: write obj/*.d header dependency files.
# EXTRA_CFLAGS: one-off defines, e.g. make EXTRA_CFLAGS=-DSKIP_INTRO_AND_START=0
# (use with a separate OBJDIR, or run "make rebuild" afterwards).
CFLAGS   := -Wf-MMD -Wf-Wp-MP -I$(HUGEDRIVER) $(EXTRA_CFLAGS)
# -autobank: place data and code marked "#pragma bank 255" into ROM banks.
LDFLAGS  := -autobank -Wb-ext=.rel -Wm-yS -Wl-l$(HUGEDRIVER)/hUGEDriver.lib $(CART_FLAGS)

ifdef RELEASE
CFLAGS += -Wf--max-allocs-per-node50000
endif

# All .c files under src/, sorted so autobank places them the same way on every build.
rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))
CSOURCES  := $(sort $(call rwildcard,$(SRCDIR),*.c))

# Object files go flat into obj/, so two sources must not share a file name.
OBJNAMES := $(notdir $(CSOURCES:.c=.o))
ifneq ($(words $(OBJNAMES)),$(words $(sort $(OBJNAMES))))
$(error Two .c files under $(SRCDIR)/ share a file name; their object files would collide)
endif
OBJS := $(addprefix $(OBJDIR)/,$(OBJNAMES))
vpath %.c $(sort $(dir $(CSOURCES)))

SAVETEST := $(OBJDIR)/savetest.gb
SOUNDTEST := $(OBJDIR)/soundtest.gb
SOUND_PACKS := sounds/GBFX.sav sounds/GBFX2.sav
MUSIC_PACKS := music/packs/game_boy_music_pack_1.zip music/packs/game_boy_music_pack_2.zip
SAVETEST_VARIANTS := $(OBJDIR)/savetest_mbc1.gb $(OBJDIR)/savetest_mbc3.gb

.PHONY: all rebuild run test test-full shots goldens budget venv savetest soundtest soundtest-link usage assets levels music sounds preview clean

all: $(ROM)

$(ROM): $(OBJS)
	$(LCC) $(LCCFLAGS) $(LDFLAGS) -o $@ $(OBJS)
	$(ROMUSAGE) $(MAP) -g -R

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(LCC) $(LCCFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJDIR):
	@$(call MKDIR,$@)

-include $(OBJS:.o=.d)

rebuild:
	$(MAKE) clean
	$(MAKE) all

run: $(ROM)
	java -jar $(EMULICIOUS) $(ROM)

test: $(ROM) $(SAVETEST) $(SAVETEST_VARIANTS) $(SOUNDTEST)
	$(PYTHON) -m pytest tests -n auto

test-full: $(ROM) $(SAVETEST) $(SAVETEST_VARIANTS) $(SOUNDTEST)
	$(PYTHON) -m pytest tests -n auto --full

# Scripted playthroughs of the greybox level and level 1: screenshots, contact sheets
# and the level map in tests/out/shots/, for visual review.
shots: $(ROM)
	$(PYTHON) tools/shots.py

# Rewrites the golden screenshots (tests/test_golden.py). Look at them before committing.
goldens: $(ROM)
	$(PYTHON) -m pytest tests/test_golden.py --update-golden

# Screen lines per part of a gameplay update, over the frame budget test's route.
budget: $(ROM)
	$(PYTHON) tools/frame_budget.py

venv:
	python -m venv .venv
	$(call fixpath,$(VENV_PY)) -m pip install -r tools/requirements.txt

# Standalone ROM with an MBC5 + RAM + battery cartridge. It counts boots in cartridge
# RAM, to check that saves survive a power cycle on a flashcart.
savetest: $(SAVETEST) $(SAVETEST_VARIANTS)

$(SAVETEST): tools/savetest/savetest.c | $(OBJDIR)
	$(LCC) $(LCCFLAGS) -Wm-yS $(SAVETEST_FLAGS) -o $@ $<

# MBC1 and MBC3 headers, to find out whether the flashcart keeps saves for another
# mapper (roadmap M7). The screen names the mapper.
$(OBJDIR)/savetest_mbc%.gb: tools/savetest/savetest.c | $(OBJDIR)
	$(LCC) $(LCCFLAGS) -Wm-yS $(subst -Wl-yt$(SAVETEST_TYPE),-Wl-yt$(CART_TYPE_MBC$*),$(SAVETEST_FLAGS)) -DMBC=$* -o $@ $<

# Every effect of the sound packs and every song of the music packs, played by the
# game's players, to pick them by ear (on hardware, or as WAVs from tools/render_sounds.py).
soundtest: $(SOUNDTEST)
	$(PYTHON) tools/render_sounds.py

# The songs' C files only exist after uge2c, so a second make links them (its wildcard
# sees them).
$(SOUNDTEST): tools/soundtest/soundtest.c tools/soundtest/song_table.h src/sound/sound.c src/sound/sound.h tools/fxhammer2c.py tools/uge2c.py $(SOUND_PACKS) $(MUSIC_PACKS) | $(OBJDIR)
	$(PYTHON) tools/fxhammer2c.py --all $(SOUND_PACKS) -o $(OBJDIR)/soundtest
	$(PYTHON) tools/uge2c.py $(MUSIC_PACKS) -o $(OBJDIR)/soundtest --table $(OBJDIR)/soundtest/song_table.c
	$(MAKE) --no-print-directory soundtest-link

soundtest-link:
	$(LCC) $(LCCFLAGS) -I$(HUGEDRIVER) -Isrc/sound -Itools/soundtest $(LDFLAGS) -o $(SOUNDTEST) tools/soundtest/soundtest.c src/sound/sound.c $(wildcard $(OBJDIR)/soundtest/*.c)

usage: $(ROM)
	$(ROMUSAGE) $(MAP) -a -g

# png2asset output is committed. Run this after changing a PNG, and read the output:
# png2asset only warns about unknown options and still succeeds.
# -b 255 puts the data in a ROM bank picked by autobank, so code must switch to
# BANK(name) before reading it (see src/engine/banking.h).
assets:
	$(PYTHON) tools/greybox_tileset.py
	$(PNG2ASSET) graphics/tilesets/greybox.png -map -noflip -keep_duplicate_tiles -keep_palette_order -tiles_only -b 255 -o src/graphics/greybox.c
	$(PYTHON) tools/greybox_band.py
	$(PNG2ASSET) graphics/bands/greybox_band.png -map -noflip -keep_palette_order -b 255 -o src/graphics/greybox_band.c
	$(PNG2ASSET) graphics/player/programmer.png -sw 16 -sh 32 -spr8x16 -keep_palette_order -b 255 -o src/graphics/programmer.c
	$(PNG2ASSET) graphics/hud/hud_a.png -map -noflip -b 255 -o src/graphics/hud.c
	$(PNG2ASSET) graphics/background/background.png -map -noflip -b 255 -o src/graphics/background.c
	$(PNG2ASSET) graphics/enemies/Mushroom.png -map -noflip -b 255 -o src/graphics/mushroom.c

# tools/tiled2c.py output is committed too. Run this after saving a map in Tiled.
# A level that breaks a rule (docs/LEVEL_PIPELINE.md) stops with a message.
levels:
	$(PYTHON) tools/tiled2c.py $(wildcard levels/*.tmj) -o src/levels

# tools/uge2c.py output is committed too. Run this after changing a song in music/.
music:
	$(PYTHON) tools/uge2c.py $(wildcard music/*.uge) -o src/music

# tools/fxhammer2c.py output is committed too. Run this after changing sounds/effects.txt.
sounds:
	$(PYTHON) tools/fxhammer2c.py sounds/effects.txt -o src/sound

# Full-level PNGs with column numbers, entities and jump arcs, plus the reachability
# check (tools/levelpreview.py). Fails when something in a level can't be reached.
preview:
	$(PYTHON) tools/levelpreview.py $(wildcard levels/*.tmj)

clean:
	@$(call RMDIR,$(OBJDIR))

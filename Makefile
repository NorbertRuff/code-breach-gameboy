#
# ApexLab: Code Breach — build
#
#   make            build obj/ApexLab.gb (incremental; headers are tracked)
#   make rebuild    clean, then build
#   make run        build, then open the ROM in Emulicious
#   make test       build, then run the PyBoy tests (DMG and GBC mode)
#   make shots      build, then save review screenshots to tests/out/shots/
#   make budget     build, then print where the frame time goes (tools/frame_budget.py)
#   make venv       create .venv with the test tools from tools/requirements.txt
#   make savetest   build obj/savetest.gb, the cartridge RAM persistence test
#   make usage      show ROM and RAM use per bank
#   make assets     regenerate src/graphics/*.c from the PNGs in graphics/
#   make levels     regenerate src/levels/*.c from the Tiled maps in levels/
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

# Cartridge (docs/CGB_PLAN.md section 1):
#   MBC5 + RAM + battery, 1 RAM bank (8 KB), ROM size picked automatically,
#   GBC-enhanced header that still runs on DMG, cartridge title.
CART_FLAGS := -Wl-yt0x1B -Wm-yoA -Wm-ya1 -Wm-yc -Wm-ynAPEXLAB

# -debug: .cdb/.noi/.map for Emulicious. -Wm-yS: .sym for PyBoy.
LCCFLAGS := -debug
# -MMD/-MP: write obj/*.d header dependency files.
# EXTRA_CFLAGS: one-off defines, e.g. make EXTRA_CFLAGS=-DSKIP_INTRO_AND_START=0
# (use with a separate OBJDIR, or run "make rebuild" afterwards).
CFLAGS   := -Wf-MMD -Wf-Wp-MP $(EXTRA_CFLAGS)
# -autobank: place data and code marked "#pragma bank 255" into ROM banks.
LDFLAGS  := -autobank -Wb-ext=.rel -Wm-yS $(CART_FLAGS)

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

.PHONY: all rebuild run test shots budget venv savetest usage assets levels preview clean

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

test: $(ROM) $(SAVETEST)
	$(PYTHON) -m pytest tests

# Scripted playthrough of level 1: screenshots, contact sheets and the level map
# in tests/out/shots/, for visual review.
shots: $(ROM)
	$(PYTHON) tools/shots.py

# Screen lines per part of a gameplay update, over the frame budget test's route.
budget: $(ROM)
	$(PYTHON) tools/frame_budget.py

venv:
	python -m venv .venv
	$(call fixpath,$(VENV_PY)) -m pip install -r tools/requirements.txt

# Standalone ROM with the same cartridge type as the game. It counts boots in
# cartridge RAM, to check that saves survive a power cycle on the flashcart.
savetest: $(SAVETEST)

$(SAVETEST): tools/savetest/savetest.c | $(OBJDIR)
	$(LCC) $(LCCFLAGS) -Wm-yS $(CART_FLAGS) -o $@ $<

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

# Full-level PNGs with column numbers, entities and jump arcs, plus the reachability
# check (tools/levelpreview.py). Fails when something in a level can't be reached.
preview:
	$(PYTHON) tools/levelpreview.py $(wildcard levels/*.tmj)

clean:
	@$(call RMDIR,$(OBJDIR))

#
# A Makefile that compiles all .c and .s files in "src" and "res" 
# subdirectories and places the output in a "obj" subdirectory
#

# OS Detection
ifeq ($(OS),Windows_NT)
    detected_OS := Windows
else
    detected_OS := $(shell uname -s)
endif

# Platform-specific settings
ifeq ($(detected_OS),Windows)
    # Windows commands and paths
    MKDIR = if not exist $(1) mkdir $(1)
    RMDIR = if exist $(1) rmdir /s /q $(1)
    PATH_SEP = \\
    JAVA_PATH_SEP = .\\
    SED_PATH_CONVERT = sed y/\\//\\\\/
    SED_MKDIR_CONVERT = sed s/mkdir\ -p\/mkdir\/
else
    # Linux/Unix commands and paths
    MKDIR = mkdir -p $(1)
    RMDIR = rm -rf $(1)
    PATH_SEP = /
    JAVA_PATH_SEP = ./
    SED_PATH_CONVERT = cat
    SED_MKDIR_CONVERT = cat
endif

GBDK_HOME = ./gbdk/

LCC = $(GBDK_HOME)bin/lcc 
PNG2ASSET = $(GBDK_HOME)bin/png2asset

LCCFLAGS += -debug # Uncomment to enable debug output
#LCCFLAGS += -v     # Uncomment for lcc verbose output

PROJECTNAME    = ApexLab

# Directories
SRCDIR      = src
GAMESTATES  = src/game_state
EMUDIR      = emulicious
GRAPHICS    = graphics
OBJDIR      = obj
RESDIR      = res
GRAPHICSDIR = src/graphics
PLAYERDIR   = src/player
INPUTDIR    = src/input

BINS	    = $(OBJDIR)/$(PROJECTNAME).gb
CSOURCES    = $(foreach dir,$(SRCDIR),$(notdir $(wildcard $(dir)/*.c))) $(foreach dir,$(RESDIR),$(notdir $(wildcard $(dir)/*.c))) $(foreach dir,$(GAMESTATES),$(notdir $(wildcard $(dir)/*.c))) $(foreach dir,$(GRAPHICSDIR),$(notdir $(wildcard $(dir)/*.c))) $(foreach dir,$(PLAYERDIR),$(notdir $(wildcard $(dir)/*.c))) $(foreach dir,$(INPUTDIR),$(notdir $(wildcard $(dir)/*.c)))
ASMSOURCES  = $(foreach dir,$(SRCDIR),$(notdir $(wildcard $(dir)/*.s))) $(foreach dir,$(GAMESTATES),$(notdir $(wildcard $(dir)/*.s)))
OBJS       = $(CSOURCES:%.c=$(OBJDIR)/%.o) $(ASMSOURCES:%.s=$(OBJDIR)/%.o)

all:	clean prepare $(BINS)

run:
	java -jar $(JAVA_PATH_SEP)$(EMUDIR)$(PATH_SEP)Emulicious.jar $(BINS)

compile: Makefile
	@echo "-----> REM Automatically generated from Makefile <-----" > compile.bat
	@make -sn | $(SED_PATH_CONVERT) | $(SED_MKDIR_CONVERT) | grep -v make >> compile.bat
	@echo "-----> REM End of automatically generated section <-----" >> compile.bat
	@echo "-----> REM You can now run compile.bat to compile your project <-----" >> compile.bat

# Compile .c files in "src/" to .o object files
$(OBJDIR)/%.o:	$(SRCDIR)/%.c
	$(LCC) $(LCCFLAGS) -c -o $@ $<

# Compile .c files in "GAMESTATES/" to .o object files
$(OBJDIR)/%.o:	$(GAMESTATES)/%.c
	$(LCC) $(LCCFLAGS) -c -o $@ $<

# Compile .c files in "res/" to .o object files
$(OBJDIR)/%.o:	$(RESDIR)/%.c
	$(LCC) $(LCCFLAGS) -c -o $@ $<

# Compile .c files in "src/graphics/" to .o object files
$(OBJDIR)/%.o:	$(GRAPHICSDIR)/%.c
	$(LCC) $(LCCFLAGS) -c -o $@ $<

# Compile .c files in "src/player/" to .o object files
$(OBJDIR)/%.o:	$(PLAYERDIR)/%.c
	$(LCC) $(LCCFLAGS) -c -o $@ $<

# Compile .c files in "src/input/" to .o object files
$(OBJDIR)/%.o:	$(INPUTDIR)/%.c
	$(LCC) $(LCCFLAGS) -c -o $@ $<

# Compile .s assembly files in "src/" to .o object files
$(OBJDIR)/%.o:	$(SRCDIR)/%.s
	$(LCC) $(LCCFLAGS) -c -o $@ $<

# Compile .s assembly files in "src/" to .o object files
$(OBJDIR)/%.o:	$(GAMESTATES)/%.s
	$(LCC) $(LCCFLAGS) -c -o $@ $<


# If needed, compile .c files in "src/" to .s assembly files
# (not required if .c is compiled directly to .o)
$(OBJDIR)/%.s:	$(SRCDIR)/%.c
	$(LCC) $(LCCFLAGS) -S -o $@ $<

# If needed, compile .c files in "src/" to .s assembly files
# (not required if .c is compiled directly to .o)
$(OBJDIR)/%.s:	$(GAMESTATES)/%.c
	$(LCC) $(LCCFLAGS) -S -o $@ $<

# Link the compiled object files into a .gb ROM file
$(BINS):	$(OBJS)
	$(LCC) $(LCCFLAGS) -o $(BINS) $(OBJS)

# Create the obj directory if it doesn't exist
prepare:
	echo "-----> Creating obj directory <-----"
	$(call MKDIR,$(OBJDIR))

# Create assets from png files
#usage: png2asset    <file>.png [options]
#-o                  ouput file (default: <png file>.c)
#-c                  deprecated, same as -o
#-sw <width>         metasprites width size (default: png width)
#-sh <height>        metasprites height size (default: png height)
#-sp <props>         change default for sprite OAM property bytes (in hex) (default: 0x00)
#-px <x coord>       metasprites pivot x coordinate (default: metasprites width / 2)
#-py <y coord>       metasprites pivot y coordinate (default: metasprites height / 2)
#-pw <width>         metasprites collision rect width (default: metasprites width)
#-ph <height>        metasprites collision rect height (default: metasprites height)
#-spr8x8             use SPRITES_8x8
#-spr8x16            use SPRITES_8x16 (this is the default)
#-spr16x16msx        use SPRITES_16x16
#-b <bank>           bank (default: fixed bank)
#-keep_palette_order use png palette
#-repair_indexed_pal try to repair indexed tile palettes (implies "-keep_palette_order")
#-noflip             disable tile flip
#-map                Export as map (tileset + bg)
#-use_map_attributes Use CGB BG Map attributes
#-use_nes_attributes Use NES BG Map attributes
#-use_nes_colors     Convert RGB color values to NES PPU colors
#-use_structs        Group the exported info into structs (default: false) (used by ZGB Game Engine)
#-bpp                bits per pixel: 1, 2, 4 (default: 2)
#-max_palettes       max number of palettes allowed (default: 8)
#                    (note: max colors = max_palettes x num colors per palette)
#-pack_mode          gb, nes, sgb, sms, 1bpp (default: gb)
#-tile_origin        tile index offset for maps (default: 0)
#-tiles_only         export tile data only
#-maps_only          export map tilemap only
#-metasprites_only   export metasprite descriptors only
#-source_tileset     use source tileset (image with common tiles)
#-keep_duplicate_tiles   do not remove duplicate tiles (default: not enabled)
#-no_palettes        do not export palette data
#-bin                export to binary format
#-transposed         export transposed (column-by-column instead of row-by-row)

assets:
	echo "-----> Creating assets from png files <-----"
	$(PNG2ASSET) $(GRAPHICS)/player/programmer.png -sw 16 -sh 32 -spr8x16 -keep_palette_order -name programmer -o $(SRCDIR)/graphics/programmer.c
	$(PNG2ASSET) $(GRAPHICS)/start_screen/apexLogo.png -bank 1 -org 0x8000 -noflip -map -name apexLogo -o $(SRCDIR)/graphics/apexLogo.c
	$(PNG2ASSET) $(GRAPHICS)/start_screen/press_start.png -bank 1 -org 0x8000 -noflip -map -name press_start -o $(SRCDIR)/graphics/press_start.c
	$(PNG2ASSET) $(GRAPHICS)/hud/hud_a.png -bank 1 -org 0x8000 -noflip -map -name hud -o $(SRCDIR)/graphics/hud.c
	$(PNG2ASSET) $(GRAPHICS)/background/background.png -bank 1 -org 0x8000 -noflip -map -name background -o $(SRCDIR)/graphics/background.c

# Remove all compiled files
clean:
	echo "-----> Cleaning up <-----"
	$(call RMDIR,$(OBJDIR))
	$(call RMDIR,./-p/)

# create necessary directories after Makefile is parsed but before build
# info prevents the command from being pasted into the makefile
# $(info $(shell mkdir -p $(MKDIRS)))


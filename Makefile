#
# A Makefile that compiles all .c and .s files in "src" and "res" 
# subdirectories and places the output in a "obj" subdirectory
#

# If you move this project you can change the directory 
# to match your GBDK root directory (ex: GBDK_HOME = "C:/GBDK/"
GBDK_HOME = ./gbdk/

LCC = $(GBDK_HOME)bin/lcc 
PNG2ASSET = $(GBDK_HOME)bin/png2asset

# You can set flags for LCC here
# For example, you can uncomment the line below to turn on debug output
LCCFLAGS += -debug # Uncomment to enable debug output
#LCCFLAGS += -v     # Uncomment for lcc verbose output


# You can set the name of the .gb ROM file here
PROJECTNAME    = ApexLab

SRCDIR      = src
GAMESTATES  = src/game_state
EMUDIR      = emulicious
IMGDIR      = img
OBJDIR      = obj
RESDIR      = res
BINS	    = $(OBJDIR)/$(PROJECTNAME).gb
CSOURCES    = $(foreach dir,$(SRCDIR),$(notdir $(wildcard $(dir)/*.c))) $(foreach dir,$(RESDIR),$(notdir $(wildcard $(dir)/*.c))) $(foreach dir,$(GAMESTATES),$(notdir $(wildcard $(dir)/*.c)))
ASMSOURCES  = $(foreach dir,$(SRCDIR),$(notdir $(wildcard $(dir)/*.s))) $(foreach dir,$(GAMESTATES),$(notdir $(wildcard $(dir)/*.s)))
OBJS       = $(CSOURCES:%.c=$(OBJDIR)/%.o) $(ASMSOURCES:%.s=$(OBJDIR)/%.o)

all:	clean prepare $(BINS) runEmulator

runEmulator:
	java -jar .\$(EMUDIR)\Emulicious.jar $(BINS)

compile: Makefile
	@echo "-----> REM Automatically generated from Makefile <-----" > compile.bat
	@make -sn | sed y/\\//\\\\/ | sed s/mkdir\ -p\/mkdir\/ | grep -v make >> compile.bat
	@echo "-----> REM End of automatically generated section <-----" >> compile.bat
	@echo "-----> REM You can now run compile.bat to compile your project <-----" >> compile.bat

# Compile .c files in "src/" to .o object files
$(OBJDIR)/%.o:	$(SRCDIR)/%.c
	echo "-----> Compiling src/c files <-----"
	$(LCC) $(LCCFLAGS) -c -o $@ $<

# Compile .c files in "GAMESTATES/" to .o object files
$(OBJDIR)/%.o:	$(GAMESTATES)/%.c
	echo "-----> Compiling res/c files <-----"
	$(LCC) $(LCCFLAGS) -c -o $@ $<

# Compile .c files in "res/" to .o object files
$(OBJDIR)/%.o:	$(RESDIR)/%.c
	echo "-----> Compiling res/c files <-----"
	$(LCC) $(LCCFLAGS) -c -o $@ $<

# Compile .s assembly files in "src/" to .o object files
$(OBJDIR)/%.o:	$(SRCDIR)/%.s
	echo "-----> Compiling src/s files <-----"
	$(LCC) $(LCCFLAGS) -c -o $@ $<

# Compile .s assembly files in "src/" to .o object files
$(OBJDIR)/%.o:	$(GAMESTATES)/%.s
	echo "-----> Compiling src/s files <-----"
	$(LCC) $(LCCFLAGS) -c -o $@ $<


# If needed, compile .c files in "src/" to .s assembly files
# (not required if .c is compiled directly to .o)
$(OBJDIR)/%.s:	$(SRCDIR)/%.c
	echo "-----> Compiling src/c files to src/s files <-----"
	$(LCC) $(LCCFLAGS) -S -o $@ $<

# If needed, compile .c files in "src/" to .s assembly files
# (not required if .c is compiled directly to .o)
$(OBJDIR)/%.s:	$(GAMESTATES)/%.c
	echo "-----> Compiling src/c files to src/s files <-----"
	$(LCC) $(LCCFLAGS) -S -o $@ $<

# Link the compiled object files into a .gb ROM file
$(BINS):	$(OBJS)
	echo "-----> Linking object files into a .gb ROM file <-----"
	$(LCC) $(LCCFLAGS) -o $(BINS) $(OBJS)

# Create the obj directory if it doesn't exist
prepare:
	echo "-----> Creating obj directory <-----"
	if not exist $(OBJDIR) mkdir $(OBJDIR)

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
	$(PNG2ASSET) $(IMGDIR)/apexLogo.png -bank 1 -org 0x8000 -noflip -map -name apexLogo -out $(SRCDIR)/apexLogo.c
	$(PNG2ASSET) $(IMGDIR)/press_start.png -bank 1 -org 0x8000 -noflip -map -name press_start -out $(SRCDIR)/press_start.c
	$(PNG2ASSET) $(IMGDIR)/apexLogo3.png -bank 1 -org 0x8000 -noflip -map -name apexLogo3 -out $(SRCDIR)/apexLogo3.c
	$(PNG2ASSET) $(IMGDIR)/hud_a.png -bank 1 -org 0x8000 -noflip -map -name hud -out $(SRCDIR)/hud.c

# Remove all compiled files
clean:
	echo "-----> Cleaning up <-----"
	if exist $(OBJDIR) rmdir /s /q $(OBJDIR)
	if exist .\-p\ rmdir /s /q .\-p\

# create necessary directories after Makefile is parsed but before build
# info prevents the command from being pasted into the makefile
# $(info $(shell mkdir -p $(MKDIRS)))


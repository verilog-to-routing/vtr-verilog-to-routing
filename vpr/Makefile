################################   MAKEFILE OPTIONS     ####################################

# Modify the following parameters to suit the user's needs.
# Only this section needs to be changed.

# The graphics are currently only compatible with X11, therefore X11 must be installed.
# If VPR makes when graphics are enabled without prompting that L/usr/lib/X11 is not found,
# then the following comment can be ignored.

# On Ubuntu, the Terminal command to install X11 is:
# sudo apt-get install libx11-dev
# Please search online for installing X11 on other platforms.

# Please note that a Mac can run the graphics if the X11 library is installed. 
# Set the following options appropriately to do so.

ENABLE_GRAPHICS = true # can be true or false
BUILD_TYPE = release   # can be debug or release
MAC_OS = false         # can be true or false

#############################################################################################

CC = gcc
LIB_DIR = -L../libvpr
LIB = -lm -lvpr_6
SRC_DIR = SRC
OBJ_DIR = OBJ
OTHER_DIR = -ISRC/util -ISRC/timing -ISRC/pack -ISRC/place -ISRC/base -ISRC/route


WARN_FLAGS = -Wall -Wpointer-arith -Wcast-qual -Wstrict-prototypes -D__USE_FIXED_PROTOTYPES__ -ansi -pedantic -Wmissing-prototypes -Wshadow -Wcast-align -D_POSIX_SOURCE
DEBUG_FLAGS = -g
OPT_FLAGS = -O3
INC_FLAGS = -I../libvpr/include

FLAGS = $(INC_FLAGS) $(WARN_FLAGS) -D EZXML_NOMMAP -D_POSIX_C_SOURCE

ifeq ($(BUILD_TYPE),release)
  FLAGS := $(FLAGS) $(OPT_FLAGS)
else                              # DEBUG build
  FLAGS := $(FLAGS) $(DEBUG_FLAGS)
endif

ifeq ($(ENABLE_GRAPHICS),true)
  # The following block defines the X11 directories. If X11 library
  # is located elsewhere, one can change it here.
  ifeq ($(MAC_OS),true)
    LIB_DIR := $(LIB_DIR) -L/usr/X11
  else  
    LIB_DIR := $(LIB_DIR) -L/usr/lib/X11
  endif
  LIB := $(LIB) -lX11
else
  FLAGS := $(FLAGS) -DNO_GRAPHICS
endif

EXE = vpr

OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o,$(wildcard $(SRC_DIR)/*.c $(SRC_DIR)/*/*.c))
OBJ_DIRS=$(sort $(dir $(OBJ)))
DEP := $(OBJ:.o=.d)

$(EXE): $(OBJ) Makefile ../libvpr/libvpr_6.a
	# checks if the package libx11-dev is installed, and installs it if not
	if ! dpkg -l | grep libx11-dev -c >>/dev/null; then sudo apt-get install libx11-dev; fi
	$(CC) $(FLAGS) $(OBJ) -o $(EXE) $(LIB_DIR) $(LIB)
	
../libvpr/libvpr_6.a:
	cd ../libvpr && make

# Enable a second round of expansion so that we may include
# the target directory as a prerequisite of the object file.
.SECONDEXPANSION:

# The directory follows a "|" to use an existence check instead of the usual
# timestamp check.  Every write to the directory updates the timestamp thus
# without this, all but the last file written to a directory would appear
# to be out of date.
$(OBJ): OBJ/%.o:$(SRC_DIR)/%.c | $$(dir $$@D)
	$(CC) $(FLAGS) -MD -MP -I$(OTHER_DIR) -ISRC/util -c $< -o $@

# Silently create target directories as need
$(OBJ_DIRS):
	@ mkdir -p $@

-include $(DEP)

clean:
	rm -f $(EXE) $(OBJ) $(DEP)
	cd ../libvpr && make clean

clean_coverage: clean
	rm -rf ./usr
	find ./OBJ -regex ".*.\(gcda\|gcno\)" -exec rm -f {} \;
	rm -f *.html
	find ./SRC -iname "*.html" -exec rm -f {} \;


ctags:
	cd $(SRC_DIR) && ctags *.[ch]

# This is using Target-specific variable values. See: http://www.gnu.org/software/make/manual/make.html#Target_002dspecific
coverage: FLAGS = $(DEBUG_FLAGS) $(INC_FLAGS) -pedantic  -D EZXML_NOMMAP -fprofile-arcs -ftest-coverage -lgcov
coverage: $(EXE)
	./coverage_reset.sh

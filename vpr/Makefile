################################   MAKEFILE OPTIONS     ####################################

# If ENABLE_GRAPHICS is set to true, VPR requires X11 and Xft to compile.
# On Ubuntu, type 'make packages' or 
# 'sudo apt-get install libx11-dev libxft-dev' to install.
# Please look online for information on how to install X11 on other Linux distributions.

# Please note that a Mac can run the graphics if the X11 library is installed. 

ENABLE_GRAPHICS = false
# can be true or false

BUILD_TYPE = release
# can be debug or release - this option gets inherited by the library libarchfpga 
# by default, though libarchfpga's build type can be set independently in libarchfpga's Makefile.

COMPILER = g++

OPTIMIZATION_LEVEL_FOR_RELEASE_BUILD = -O3
# can be -O0 (no optimization) to -O3 (full optimization), or -Os (optimize space)

#############################################################################################

CC = $(COMPILER)
LIB_DIR = -L. -L../libsdcparse -L../libblifparse -L../liblog -L../libeasygl -L../libpugixml -L../libarchfpga -L../libvtrutil
LIB = -lm -lvpr -leasygl -lsdcparse -lblifparse -larchfpga -lpugixml -lvtrutil -llog
SRC_DIR = SRC
OBJ_DIR = OBJ
OTHER_DIR = -ISRC/util -ISRC/timing -ISRC/pack -ISRC/place -ISRC/base -ISRC/draw -ISRC/route -ISRC/power 
OTHER_DIR := $(OTHER_DIR) \
	-I../libsdcparse/src \
	-I../libblifparse/src \
	-I../liblog/src \
	-I../libeasygl/src \
	$(shell pkg-config --cflags freetype2)

WARN_FLAGS = -Wall  -Wpointer-arith -Wcast-qual -ansi -pedantic -Wshadow -Wcast-align -Wno-write-strings

DEBUG_FLAGS = -g -O0 
#Potentially expensive asserts that should only be used if you're really scared
ADDITIONAL_DEBUG_FLAGS = -DEXPENSIVE_ASSERTS
PROFILE_FLAGS = -DPROFILE
OPT_FLAGS = $(OPTIMIZATION_LEVEL_FOR_RELEASE_BUILD)
INC_FLAGS = -I../libarchfpga/include -I ../libpugixml/src -I ../libvtrutil/src
DEFINES := 

FLAGS = $(INC_FLAGS) $(WARN_FLAGS) $(DEFINES) -std=c++0x

UNAME := $(shell uname)
# determine build env
ifeq ($(UNAME), Darwin)
	MAC_OS = true
else
	MAC_OS = false
endif

ifneq (,$(findstring release, $(BUILD_TYPE)))
  FLAGS := $(FLAGS) $(OPT_FLAGS)
else ifneq (,$(findstring profile, $(BUILD_TYPE)))
  FLAGS := $(FLAGS) $(PROFILE_FLAGS) $(OPT_FLAGS) 
else                              # DEBUG build
  FLAGS := $(FLAGS) $(DEBUG_FLAGS)
endif



ifneq (,$(findstring true, $(ENABLE_GRAPHICS)))
  # The following block defines the graphics library directories. If X11 library
  # or fontconfig is located elsewhere, change it here.
  ifneq (,$(findstring true, $(MAC_OS)))
    LIB_DIR := $(LIB_DIR) -L/usr/X11/lib -L/opt/X11/lib
  else  
    LIB_DIR := $(LIB_DIR) -L/usr/lib/X11
  endif
  LIB := $(LIB) -lX11 -lXft -lfontconfig
endif

EXE = vpr

OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o,$(wildcard $(SRC_DIR)/*.c $(SRC_DIR)/*/*.c))
OBJ_DIRS=$(sort $(dir $(OBJ)))
DEP := $(OBJ:.o=.d)

.PHONY: clean libeasygl libsdcparse libarchfpga libpugixml libvtrutil liblog 


# notify is an order-only prerequisite - note the "|"
$(EXE): $(OBJ) Makefile libvpr.a | notify
	$(CC) $(FLAGS) OBJ/main.o -o $(EXE) $(LIB_DIR) $(LIB)


# if graphics enabled but a library is not installed, notify the user
notify:
	@ $(PACKAGENOTIFICATION) 

#if graphics enabled but a library is not installed, install it
packages: 
	@ $(PACKAGEINSTALL)

libarchfpga:
	@ cd ../libarchfpga && make libarchfpga.a

libsdcparse:
	@ cd ../libsdcparse && make VERBOSITY=0 libsdcparse.a

libblifparse:
	@ cd ../libblifparse && make VERBOSITY=0 libblifparse.a

libeasygl:
	@ cd ../libeasygl && make ENABLE_GRAPHICS=$(ENABLE_GRAPHICS) libeasygl.a

libvpr.a: $(OBJ) Makefile libsdcparse libblifparse libeasygl libarchfpga
	@ ar rcs $@ $(OBJ)
	@#Remove main
	@ ar d $@ main.o

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
	rm -f $(EXE) $(OBJ) $(DEP) libvpr.a
	cd ../libarchfpga && make clean
	cd ../libsdcparse && make clean
	cd ../libblifparse && make clean
	cd ../libeasygl && make clean

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

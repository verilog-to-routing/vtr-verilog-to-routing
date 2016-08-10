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

#Build information to expose to program
# We touch vpr_api.c to force a re-compile, ensuring the build version and date are always are up to date
BUILD_VERSION := $(shell git describe --always --long --dirty && touch SRC/base/vpr_api.c)
BUILD_DATE := $(shell date -R)

CC = $(COMPILER)
LIB_DIR = -L.
LIB = -lm -lvpr
SRC_DIR = SRC
OBJ_DIR = OBJ
OTHER_DIR = -ISRC/util -ISRC/timing -ISRC/pack -ISRC/place -ISRC/base -ISRC/draw -ISRC/route -ISRC/power 
OTHER_DIR := $(OTHER_DIR) \
	-I../libsdc_parse/src \
	 -I../liblog/src \
	$(shell pkg-config --cflags freetype2)

WARN_FLAGS = -Wall  -Wpointer-arith -Wcast-qual -D__USE_FIXED_PROTOTYPES__ -ansi -pedantic -Wshadow -Wcast-align -D_POSIX_SOURCE -Wno-write-strings

DEBUG_FLAGS = -g -O0 
#Potentially expensive asserts that should only be used if you're really scared
ADDITIONAL_DEBUG_FLAGS = -DEXPENSIVE_ASSERTS
PROFILE_FLAGS = -DPROFILE
OPT_FLAGS = $(OPTIMIZATION_LEVEL_FOR_RELEASE_BUILD)
INC_FLAGS = -I../libarchfpga/include -I ../libpugixml/src -I ../libvtrutil/src
DEFINES := -D BUILD_VERSION=\""$(BUILD_VERSION)"\" -D BUILD_DATE=\""$(BUILD_DATE)"\"

FLAGS = $(INC_FLAGS) $(WARN_FLAGS) $(DEFINES) -D EZXML_NOMMAP -D_POSIX_C_SOURCE -std=c++0x

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
    PACKAGEINSTALL := if cat /etc/issue | grep Ubuntu -c >>/dev/null; then if ! dpkg -l | grep libx11-dev -c >>/dev/null; then sudo apt-get install libx11-dev; fi; fi;
    PACKAGEINSTALL := if cat /etc/issue | grep Ubuntu -c >>/dev/null; then if ! dpkg -l | grep libxft-dev -c >>/dev/null; then sudo apt-get install libxft-dev; fi; fi;
    PACKAGENOTIFICATION :=                        if cat /etc/issue | grep Ubuntu -c >>/dev/null; then if ! dpkg -l | grep libx11-dev -c >>/dev/null; then printf "\n\n\n\n*****************************************************\n* VPR has detected that graphics are enabled,       *\n* but the required graphics package libx11-dev      *\n* is missing. Try:                                  *\n* a) Type 'make packages' to install libx11-dev     *\n*    automatically if not already installed.        *\n* b) Type 'sudo apt-get install libx11-dev' to      *\n*    install manually.                              *\n* c) If libx11-dev is installed, point the Makefile *\n*    to where your X11 libraries are installed.     *\n* d) If you wish to run VPR without graphics, set   *\n*    the flag ENABLE_GRAPHICS = false at the top    *\n*    of the Makefile in VPR's parent directory.     *\n*****************************************************\n\n\n\n"; fi; fi;
    PACKAGENOTIFICATION := $(PACKAGENOTIFICATION) if cat /etc/issue | grep Ubuntu -c >>/dev/null; then if ! dpkg -l | grep libxft-dev -c >>/dev/null; then printf "\n\n\n\n*****************************************************\n* VPR has detected that graphics are enabled,       *\n* but the required graphics package libxft-dev      *\n* is missing. Try:                                  *\n* a) Type 'make packages' to install libxft-dev     *\n*    automatically if not already installed.        *\n* b) Type 'sudo apt-get install libxft-dev' to      *\n*    install manually.                              *\n* c) If libxft-dev is installed, point the Makefile *\n*    to where your X11 libraries are installed.     *\n* d) If you wish to run VPR without graphics, set   *\n*    the flag ENABLE_GRAPHICS = false at the top    *\n*    of the Makefile in VPR's parent directory.     *\n*****************************************************\n\n\n\n"; fi; fi;
  endif
  LIB := $(LIB) -lX11 -lXft -lfontconfig
else
  FLAGS := $(FLAGS) -DNO_GRAPHICS
endif

EXE = vpr

OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o,$(wildcard $(SRC_DIR)/*.c $(SRC_DIR)/*/*.c))
OBJ_DIRS=$(sort $(dir $(OBJ)))
DEP := $(OBJ:.o=.d)


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
	@ cd ../libarchfpga && make

libpugixml:
	@ cd ../libpugixml && make

libvtrutil:
	@ cd ../libvtrutil && make

libsdc_parse:
	@ cd ../libsdc_parse && make VERBOSITY=0

liblog:
	@ cd ../liblog && make liblog.a

libvpr.a: $(OBJ) Makefile libarchfpga libsdc_parse liblog libpugixml libvtrutil
	@ cp ../libarchfpga/libarchfpga.a $@
	@ cp ../libsdc_parse/libsdc_parse.a .
	@#Extract libsdc_parse.a into OBJ/libsdc_parse and then have ar archive results
	@ mkdir -p  OBJ/libsdc_parse/
	@ cd OBJ/libsdc_parse && ar x ../../libsdc_parse.a 
	@ ar rcs $@ $(OBJ) OBJ/libsdc_parse/*.o
	@#Extract libpugixml.a into OBJ/libpugixml and then have ar archive results
	@ cp ../libpugixml/libpugixml.a .
	@ mkdir -p  OBJ/libpugixml/
	@ cd OBJ/libpugixml && ar x ../../libpugixml.a 
	@ ar rcs $@ $(OBJ) OBJ/libpugixml/*.o
	@#Extract libvtrutil.a into OBJ/libvtrutil and then have ar archive results
	@ cp ../libvtrutil/libvtrutil.a .
	@ mkdir -p  OBJ/libvtrutil/
	@ cd OBJ/libvtrutil && ar x ../../libvtrutil.a 
	@ ar rcs $@ $(OBJ) OBJ/libvtrutil/*.o
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
	rm -f $(EXE) $(OBJ) $(DEP)
	cd ../libarchfpga && make clean
	cd ../libpugixml && make clean
	cd ../libsdc_parse && make clean
	cd ../liblog && make clean
	rm -f $(OBJ_DIR)/libsdc_parse/*
	rm -f $(OBJ_DIR)/libpugixml/*

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

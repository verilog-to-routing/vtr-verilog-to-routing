################################   MAKEFILE OPTIONS     ####################################
# Build the pcre regular expression library

export BUILD_TYPE = release
# can be debug or release - this option gets inherited by the library libarchfpga 
# by default, though libarchfpga's build type can be set independently in libarchfpga's Makefile.

COMPILER = gcc

OPTIMIZATION_LEVEL_FOR_RELEASE_BUILD = -O3
# can be -O0 (no optimization) to -O3 (full optimization), or -Os (optimize space)

#############################################################################################

CC = $(COMPILER)
LIB_DIR = -L.
LIB = -lm -lpcre
SRC_DIR = SRC
OBJ_DIR = OBJ
OTHER_DIR = -ISRC/

WARN_FLAGS = -Wall -Wpointer-arith -Wcast-qual -D__USE_FIXED_PROTOTYPES__ -ansi -pedantic -Wshadow -Wcast-align -D_POSIX_SOURCE -Wno-write-strings

DEBUG_FLAGS = -g
OPT_FLAGS = $(OPTIMIZATION_LEVEL_FOR_RELEASE_BUILD)
INC_FLAGS = 

FLAGS = $(INC_FLAGS) $(WARN_FLAGS) -D EZXML_NOMMAP -D_POSIX_C_SOURCE

ifneq (,$(findstring release, $(BUILD_TYPE)))
  FLAGS := $(FLAGS) $(OPT_FLAGS)
else                              # DEBUG build
  FLAGS := $(FLAGS) $(DEBUG_FLAGS)
endif

EXE = pcredemo

OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o,$(wildcard $(SRC_DIR)/*.c))
OBJ_DIRS=$(sort $(dir $(OBJ)))
DEP := $(OBJ:.o=.d)

$(EXE): $(OBJ) Makefile libpcre.a
	$(CC) $(FLAGS) OBJ/main.o -o $(EXE) $(LIB_DIR) $(LIB)


libpcre.a: $(OBJ) Makefile
	ar rcs $@ $(OBJ)
	ar d $@ main.o


# Enable a second round of expansion so that we may include
# the target directory as a prerequisite of the object file.
.SECONDEXPANSION:

# The directory follows a "|" to use an existence check instead of the usual
# timestamp check.  Every write to the directory updates the timestamp thus
# without this, all but the last file written to a directory would appear
# to be out of date.
$(OBJ): OBJ/%.o:$(SRC_DIR)/%.c | $$(dir $$@D)
	$(CC) $(FLAGS) -MD -MP -I$(OTHER_DIR) -ISRC -c $< -o $@

# Silently create target directories as need
$(OBJ_DIRS):
	@ mkdir -p $@

-include $(DEP)

clean:
	rm -f $(EXE) $(OBJ) $(DEP) libpcre.a

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

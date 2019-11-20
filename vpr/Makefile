# For graphics, uncomment -lX11 flag and comment out -DNO_GRAPHICS flag

CC = gcc
LIB_DIR = -L/usr/lib/X11 -L../libvpr
#LIB = -lm -lX11 -lvpr_6
LIB = -lm -lvpr_6
SRC_DIR = SRC
OBJ_DIR = OBJ
OTHER_DIR = -ISRC/util -ISRC/timing -ISRC/pack -ISRC/place -ISRC/base -ISRC/route

WARN_FLAGS = -Wall -Wpointer-arith -Wcast-qual -Wstrict-prototypes -O -D__USE_FIXED_PROTOTYPES__ -ansi -pedantic -Wmissing-prototypes -Wshadow -Wcast-align -D_POSIX_SOURCE
DEBUG_FLAGS = -g 
OPT_FLAGS = -O3
INC_FLAGS = -I../libvpr/include

#FLAGS = $(WARN_FLAGS) -D EZXML_NOMMAP  -D_POSIX_C_SOURCE
FLAGS = $(OPT_FLAGS) $(INC_FLAGS) -D EZXML_NOMMAP -DNO_GRAPHICS -D_POSIX_C_SOURCE
#FLAGS = $(OPT_FLAGS) $(INC_FLAGS) -D EZXML_NOMMAP -D_POSIX_C_SOURCE
#FLAGS = $(DEBUG_FLAGS) $(INC_FLAGS) -pedantic  -D EZXML_NOMMAP  -D_POSIX_C_SOURCE
#FLAGS = $(DEBUG_FLAGS) -pedantic -D EZXML_NOMMAP -Wall -D_POSIX_C_SOURCE
#FLAGS = $(DEBUG_FLAGS) $(INC_FLAGS) -pedantic  -D EZXML_NOMMAP -DNO_GRAPHICS -D_POSIX_C_SOURCE

EXE = vpr

OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o,$(wildcard $(SRC_DIR)/*.c $(SRC_DIR)/*/*.c))
OBJ_DIRS=$(sort $(dir $(OBJ)))
DEP := $(OBJ:.o=.d)

$(EXE): $(OBJ) Makefile ../libvpr/libvpr_6.a
	$(CC) $(FLAGS) $(OBJ) -o $(EXE) $(LIB_DIR) $(LIB)

# Enable a second round of expansion so that we may include
# the target directory as a prerequisite of the object file.
.SECONDEXPANSION:

# The directory follows a "|" to use an existence check instead of the usual
# timestamp check.  Every write to the directory updates the timestamp thus
# without this, all but the last file written to a directory would appear
# to be out of date.
$(OBJ): OBJ/%.o:$(SRC_DIR)/%.c | $$(dir $$@D)
	$(CC) $(FLAGS) -MD -MP $(X11_INCLUDE) -I$(OTHER_DIR) -ISRC/util -c $< -o $@


# Silently create target directories as need
$(OBJ_DIRS):
	@ mkdir -p $@

-include $(DEP)

clean:
	rm -f $(EXE) $(OBJ) $(DEP)
	
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

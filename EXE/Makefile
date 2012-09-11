CC = g++

SRC_DIR = SRC
OBJ_DIR = OBJ
BASE_DIR = $(SRC_DIR)/base
INC_DIR = $(SRC_DIR)/include

LVQM_DIR = $(INC_DIR)/libvqm
LARCHFPGA_DIR = $(INC_DIR)/libarchfpga
LPCRE_DIR = $(INC_DIR)/pcre
LPRINTHANDLER_DIR = $(INC_DIR)/printhandler

#LIB_DIR = -L$(LARCHFPGA_DIR) -L$(LVQM_DIR)
LIB_DIR = -L$(LARCHFPGA_DIR) -L$(LVQM_DIR) -L$(LPCRE_DIR) -L$(LPRINTHANDLER_DIR)
#LIB = -lm -lvqm -larchfpga
LIB = -lm -lvqm -larchfpga -lpcre -lprinthandler

WARN_FLAGS = -Wall 

DEBUG_FLAGS = -g
OPT_FLAGS = -O2
INC_FLAGS = -Iinclude -ISRC/include/printhandler/SRC/TIO_InputOutputHandlers
LIB_FLAGS = rcs

FLAGS = $(DEBUG_FLAGS) $(WARN_FLAGS) $(INC_FLAGS)

EXE = vqm2blif.exe

OBJ = $(patsubst $(BASE_DIR)/%.cpp, $(OBJ_DIR)/%.o,$(wildcard $(BASE_DIR)/*.cpp))
OBJ_DIRS=$(sort $(dir $(OBJ)))

GEN = *.blif *.echo BLIF/*.blif

HEADERS = $(wildcard $(INC_DIR)/*.h)



#DEPENDANCIES
all: $(EXE) 

$(EXE): $(OBJ) Makefile $(HEADERS) $(LARCHFPGA_DIR)/libarchfpga.a $(LVQM_DIR)/libvqm.a $(LPCRE_DIR)/libpcre.a $(LPRINTHANDLER_DIR)/libprinthandler.a
	$(CC) $(FLAGS) $(OBJ) -o $(EXE) $(LIB_DIR) $(LIB)

$(LVQM_DIR)/libvqm.a:
	cd $(LVQM_DIR) && make

$(LARCHFPGA_DIR)/libarchfpga.a:
	cd $(LARCHFPGA_DIR) && make

$(LPCRE_DIR)/libpcre.a:
	cd $(LPCRE_DIR) && make

$(LPRINTHANDLER_DIR)/libprinthandler.a:
	cd $(LPRINTHANDLER_DIR) && make

# Enable a second round of expansion so that we may include
# the target directory as a prerequisite of the object file.
.SECONDEXPANSION:

# The directory follows a "|" to use an existence check instead of the usual
# timestamp check.  Every write to the directory updates the timestamp thus
# without this, all but the last file written to a directory would appear
# to be out of date.
$(OBJ): OBJ/%.o:$(BASE_DIR)/%.cpp $(HEADERS) | $$(dir $$@D)
	$(CC) $(FLAGS) -c $< -o $@


# Silently create target directories as need
$(OBJ_DIRS):
	@ mkdir -p $@

clean:
	rm -f $(EXE) $(OBJ) $(GEN)
	cd $(LVQM_DIR) && make clean
	cd $(LARCHFPGA_DIR) && make clean
	cd $(LPCRE_DIR) && make clean
	cd $(LPRINTHANDLER_DIR) && make clean

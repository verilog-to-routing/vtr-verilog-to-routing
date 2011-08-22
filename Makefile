CC = g++
SRC_DIR = SRC
OBJ_DIR = OBJ
INC_DIR = $(SRC_DIR)/include
LVPR_DIR = $(INC_DIR)/libvpr
LVQM_DIR = $(INC_DIR)/libvqm
BASE_DIR = $(SRC_DIR)/base
LIB_DIR = -L$(LVPR_DIR) -L$(LVQM_DIR)
LIB = -lm -lvqm -lvpr_6

WARN_FLAGS = -Wall 

DEBUG_FLAGS = -g
OPT_FLAGS = -O2
INC_FLAGS = -Iinclude
LIB_FLAGS = rcs

FLAGS = $(DEBUG_FLAGS) $(WARN_FLAGS) $(INC_FLAGS)

EXE = vqm2blif.exe

OBJ = $(patsubst $(BASE_DIR)/%.cpp, $(OBJ_DIR)/%.o,$(wildcard $(BASE_DIR)/*.cpp))
OBJ_DIRS=$(sort $(dir $(OBJ)))

GEN = *.blif *.echo

HEADERS = $(wildcard $(INC_DIR)/*.h)

all: $(EXE) $(LVQM_DIR)/libvqm.a $(LVPR_DIR)/libvpr_6.a

$(EXE): $(OBJ) Makefile $(HEADERS) $(LVPR_DIR)/libvpr_6.a $(LVQM_DIR)/libvqm.a
	$(CC) $(FLAGS) $(OBJ) -o $(EXE) $(LIB_DIR) $(LIB)

$(LVQM_DIR)/libvqm.a:
	cd $(LVQM_DIR) && make

$(LVPR_DIR)/libvpr_6.a:
	cd $(LVPR_DIR) && make

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
	cd $(LVPR_DIR) && make clean

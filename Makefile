#Tools
CXX = /project/trees/gcc_4.8_cilkplus/cilkplus-install/bin/g++ #g++-4.9
AR = ar
LEXER_GEN = flex
PARSER_GEN = bison

#Whether this is a debug (symbols, no opt) or
# release (no symbols, opt) build. May be 
# inherited from build environment. Can
# override by defining below.
#
# Can be 'debug' or 'release'
#BUILD_TYPE = release

# How verbose we want the make file
#  0: print nothing
#  1: print high-level message
#  2: print high-level message + detailed command
VERBOSITY = 1


#Verbosity Configuration
vecho_0 = true
vecho_1 = echo
vecho_2 = echo
vecho = $(vecho_$(VERBOSITY))
AT_0 := @
AT_1 := @
AT_2 := 
AT = $(AT_$(VERBOSITY))

#Final output files
EXE=sta
STATIC_LIB=libsta.a

#Directories
SRC_DIR = src
BUILD_DIR = build

DIRS = $(SRC_DIR)/timing_graph $(SRC_DIR)/parsers $(SRC_DIR)/base $(SRC_DIR)/timing_analyzer

#Flags
WARN_FLAGS = -Wall -Wpointer-arith -Wcast-qual -D__USE_FIXED_PROTOTYPES__ -ansi -pedantic -Wshadow -Wcast-align -D_POSIX_SOURCE -Wno-write-strings

DEP_FLAGS = -MMD -MP

DEBUG_FLAGS = -g -ggdb3 -g3 -O0 -fno-inline

OPT_FLAGS = -O3 -g

ifneq (,$(findstring debug, $(BUILD_TYPE)))
	DEBUG_OPT_FLAGS := $(DEBUG_FLAGS)
else
	DEBUG_OPT_FLAGS := $(OPT_FLAGS)
endif

ifneq (,$(findstring yes, $(PROFILE)))
	PROFILE_FLAGS := -pg
else
	PROFILE_FLAGS := 
endif

CFLAGS = $(DEP_FLAGS) $(WARN_FLAGS) $(DEBUG_OPT_FLAGS) $(PROFILE_FLAGS) $(INC_FLAGS) --std=c++11 -fopenmp -fcilkplus -lcilkrts

#Objects
MAIN_SRC = $(SRC_DIR)/main.cpp
MAIN_OBJ := $(foreach src, $(MAIN_SRC), $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(src)))

LIB_SRC = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
LIB_OBJ := $(foreach src, $(LIB_SRC), $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(src)))

LEXER_SRC = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.l))
LEXER_GEN_SRC = $(patsubst $(SRC_DIR)/%.l, $(BUILD_DIR)/%.lex.c, $(LEXER_SRC))
LEXER_GEN_OBJ = $(LEXER_GEN_SRC:.c=.o)

PARSER_SRC = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.y))
PARSER_GEN_SRC = $(patsubst $(SRC_DIR)/%.y, $(BUILD_DIR)/%.parse.c, $(PARSER_SRC))
PARSER_GEN_OBJ = $(PARSER_GEN_SRC:.c=.o)

OBJECTS_LIB = $(LIB_OBJ) $(LEXER_GEN_OBJ) $(PARSER_GEN_OBJ)
OBJECTS_EXE = $(MAIN_OBJ) $(OBJECTS_LIB)

SRC_INC_FLAGS = $(foreach inc_dir, $(DIRS), $(patsubst %, -I%, $(inc_dir)))

#Need to include obj dir since it includes any generated source/header files
INC_FLAGS = -I$(SRC_DIR) -I$(BUILD_DIR) $(SRC_INC_FLAGS)

#Dependancies
DEP = $(OBJECTS_EXE:.o=.d)

#
#Rules
#
.PHONY: clean

#Don't delete intermediate files
.SECONDARY:

all: $(EXE) $(STATIC_LIB)

#Dependancies must be included after all so it is the default goal
-include $(DEP)

$(EXE): $(OBJECTS_EXE)
	@$(vecho) "Linking executable: $@"
	$(AT) $(CXX) $(CFLAGS) -o $@ $(OBJECTS_EXE)

$(STATIC_LIB): $(OBJECTS_LIB)
	@$(vecho) "Linking static library: $@"
	$(AT) $(AR) rcs $@ $(OBJECTS_LIB)

$(BUILD_DIR)/%.lex.c: $(SRC_DIR)/%.l
	@$(vecho) "Generating Lexer $< ..."
	@mkdir -p $(@D)
	$(AT) $(LEXER_GEN) -o $@ $<

$(BUILD_DIR)/%.parse.c $(BUILD_DIR)/%.parse.h: $(SRC_DIR)/%.y
	@$(vecho) "Generating Parser $< ..."
	@mkdir -p $(@D)
	$(AT) $(PARSER_GEN) -d $< -o $(BUILD_DIR)/$*.parse.c 

$(BUILD_DIR)/%.lex.o: $(BUILD_DIR)/%.lex.c $(BUILD_DIR)/%.parse.h
	@$(vecho) "Compiling Lexer $< ..."
	$(AT) $(CXX) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.parse.o: $(BUILD_DIR)/%.parse.c
	@$(vecho) "Compiling Parser $< ..."
	$(AT) $(CXX) $(CFLAGS) -c $< -o $@

#Note: % matches recursively between prefix and suffix
#      so $(SRC_DIR)/%.cpp would match both src/a/a.cpp
#      and src/b/b.cpp
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@$(vecho) "Compiling Source $< ..."
	@mkdir -p $(@D)
	$(AT) $(CXX) $(CFLAGS) -c $< -o $@

clean:
	@$(vecho) "Cleaning..."
	$(AT) rm -f $(LEXER_GEN_SRC)
	$(AT) rm -f $(PARSER_GEN_SRC)
	$(AT) rm -f $(DEP)
	$(AT) rm -rf $(BUILD_DIR)
	$(AT) rm -f $(EXE)
	$(AT) rm -f $(STATIC_LIB)

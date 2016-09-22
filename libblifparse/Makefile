#Tools
CXX = g++
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
EXE=blifparse_test
STATIC_LIB=libblifparse.a

#Flags
WARN_FLAGS = -Wall -Wpointer-arith -Wcast-qual -D__USE_FIXED_PROTOTYPES__ -ansi -pedantic -Wshadow -Wcast-align -D_POSIX_SOURCE -Wno-write-strings

DEP_FLAGS = -MMD -MP

DEBUG_FLAGS = -g -ggdb3 -g3 -O0 -fno-inline

OPT_FLAGS = -O3

ifneq (,$(findstring debug, $(BUILD_TYPE)))
	DEBUG_OPT_FLAGS := $(DEBUG_FLAGS)
else
	DEBUG_OPT_FLAGS := $(OPT_FLAGS)
endif

INC_FLAGS = -I$(SRC_DIR) -I$(OBJ_DIR)

CFLAGS = $(DEP_FLAGS) $(WARN_FLAGS) $(DEBUG_OPT_FLAGS) $(INC_FLAGS) -std=c++11 

#Objects
#.SUFFIXES: .o .c .y .l
SRC_DIR = src
OBJ_DIR = obj

main_src = $(SRC_DIR)/main.cpp
main_obj := $(patsubst %.cpp, %.o, $(main_src))
main_obj := $(foreach obj, $(main_obj), $(patsubst $(SRC_DIR)/%.o, $(OBJ_DIR)/%.o, $(obj)))

lib_src = $(wildcard $(SRC_DIR)/blif*.cpp)
lib_obj := $(patsubst %.cpp, %.o, $(lib_src))
lib_obj := $(foreach obj, $(lib_obj), $(patsubst $(SRC_DIR)/%.o, $(OBJ_DIR)/%.o, $(obj)))

LEXER_SRC = $(SRC_DIR)/blif_lexer.l
LEXER_GEN_SRC = $(SRC_DIR)/blif_lexer.gen.c
LEXER_GEN_HDR = $(SRC_DIR)/blif_lexer.gen.h
LEXER_GEN_OBJ := $(LEXER_GEN_SRC:.c=.o)
LEXER_GEN_OBJ := $(foreach obj, $(LEXER_GEN_OBJ), $(patsubst $(SRC_DIR)/%.o, $(OBJ_DIR)/%.o, $(obj)))

PARSER_SRC = $(SRC_DIR)/blif_parser.y
PARSER_GEN_SRC = $(SRC_DIR)/blif_parser.gen.c
PARSER_GEN_HDR = $(SRC_DIR)/blif_parser.gen.h
PARSER_GEN_OBJ := $(PARSER_GEN_SRC:.c=.o)
PARSER_GEN_OBJ := $(foreach obj, $(PARSER_GEN_OBJ), $(patsubst $(SRC_DIR)/%.o, $(OBJ_DIR)/%.o, $(obj)))

OBJECTS_LIB = $(lib_obj) $(LEXER_GEN_OBJ) $(PARSER_GEN_OBJ)
OBJECTS_EXE = $(main_obj) $(OBJECTS_LIB)

#Dependancies
DEP = $(OBJECTS_EXE:.o=.d)

#Rules
.PHONY: clean

all: $(OBJ_DIR) $(STATIC_LIB) $(EXE) 

-include $(DEP)

$(EXE): $(OBJECTS_EXE)
	@$(vecho) "Linking executable: $@"
	$(AT) $(CXX) $(CFLAGS) -o $@ $(OBJECTS_EXE)

$(LEXER_GEN_SRC) $(LEXER_GEN_HDR): $(LEXER_SRC)
	@$(vecho) "Generating Lexer $(LEXER_SRC)..."
	@#Avoid including "unistd.h" since it is unavailable on windows
	$(AT) $(LEXER_GEN) --nounistd --header-file=$(LEXER_GEN_HDR) -o $(LEXER_GEN_SRC) $(LEXER_SRC)

$(PARSER_GEN_SRC) $(PARSER_GEN_HDR): $(PARSER_SRC)
	@$(vecho) "Generating Parser $(PARSER_SRC)..."
	$(AT) $(PARSER_GEN) -d -o $(PARSER_GEN_SRC) $(PARSER_SRC)

$(STATIC_LIB): $(OBJECTS_LIB)
	@$(vecho) "Linking static library: $@"
	$(AT) $(AR) rcs $@ $(OBJECTS_LIB)

$(LEXER_GEN_OBJ): $(LEXER_GEN_SRC) $(PARSER_GEN_SRC)
	@$(vecho) "Compiling Lexer $<..."
	@#Suppress warning about unused function produced by flex
	$(AT) $(CXX) $(CFLAGS) -Wno-unused-function -c $< -o $@

$(PARSER_GEN_OBJ): $(PARSER_GEN_SRC)
	@$(vecho) "Compiling Parser $<..."
	$(AT) $(CXX) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(LEXER_GEN_HDR) $(PARSER_GEN_HDR) $(OBJ_DIR)
	@$(vecho) "Compiling Source $<..."
	$(AT) $(CXX) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	@ mkdir -p $@

clean:
	rm -f $(OBJECTS_EXE)
	rm -f $(DEP)
	rm -rf $(OBJ_DIR)
	rm -f $(EXE)
	rm -f $(STATIC_LIB)
	rm -f $(LEXER_GEN_SRC) $(LEXER_GEN_HDR)
	rm -f $(PARSER_GEN_SRC) $(PARSER_GEN_HDR) $(SRC_DIR)/stack.hh

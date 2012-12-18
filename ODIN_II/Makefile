CC = gcc

PARSER1 = flex 
PARSER2 = bison

WARN_FLAGS = -Wall -Wno-unused-function

SRC_DIR = SRC
OBJ_DIR = OBJ

EXEO = odin_II.exe

DEBUG_FLAGS = -g -DYYERROR_VERBOSE

#OPT_FLAGS = -O2 
# Use this value of OPT_FLAGS to simulate in parallel (requires GCC >= 4.2)
#OPT_FLAGS = -O2 -fopenmp 

INC_FLAGS =  -I../libarchfpga/include -I../printhandler/SRC/TIO_InputOutputHandlers
LIB = -L../libarchfpga -larchfpga -lm -ldl
C_DIRECTIVE = -DVPR6 -DUSING_BISON

FLAGS = $(DEBUG_FLAGS) $(WARN_FLAGS) $(OPT_FLAGS) $(INC_FLAGS)
C_FLAGS = $(C_DIRECTIVE)

YFILES = $(SRC_DIR)/verilog_bison.y
LFILES = $(SRC_DIR)/verilog_flex.l

SRC_PARSERS=$(YFILES:.y=.c) $(LFILES:.l=.c)

SRC = $(addprefix $(SRC_DIR)/,$(strip \
      netlist_visualizer.c \
      multipliers.c \
      partial_map.c \
      errors.c \
      node_creation_library.c \
      netlist_utils.c \
      netlist_stats.c \
      odin_util.c \
      string_cache.c \
      hard_blocks.c \
      memories.c \
      queue.c \
      hashtable.c \
      simulate_blif.c \
      print_netlist.c \
      implicit_memory.c \
      adders.c \
      subtractions.c \
))

SRCO = $(addprefix $(SRC_DIR)/,$(strip \
	read_xml_config_file.c \
	outputs.c \
	parse_making_ast.c \
	ast_util.c \
	high_level_data.c \
	ast_optimizations.c \
	netlist_create_from_ast.c \
	netlist_optimizations.c \
	output_blif.c \
	netlist_check.c \
	activity_estimation.c \
	read_netlist.c \
	read_blif.c \
	output_graphcrunch_format.c \
	verilog_preprocessor.c \
	odin_ii_func.c \
	odin_ii.c \
))

SRCV = SRC/read_blif.c \
SRC/verify_blif.c 

OBJ = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o, \
       $(SRC_PARSERS) $(SRC) $(SRCO))


DEP = $(OBJ:.o=.d)

all: CTAGS $(EXEO)

$(EXEO): $(OBJ)
	g++ $(C_FLAGS) $(FLAGS) $(OBJ) -o $(EXEO) $(LIB)

CTAGS :
	@ cd SRC && ctags *.[ch]

# Make sure the parsers are generated before compiling
# any c files
$(OBJ_DIR)/%.o:$(SRC_DIR)/%.c | $(OBJ_DIR) $(SRC_PARSERS)
	$(CC) $(C_FLAGS) $(FLAGS) -MD -MP -c $< -o $@ 

SRC/verilog_flex.c: %.c: %.l
	$(PARSER1) -o$@ $<

SRC/verilog_bison.c: %.c: %.y
	$(PARSER2) --verbose -d -t -o $@ $<

# Silently create the object directory as needed
$(OBJ_DIR):
	@ mkdir -p $@
	
clean :
	rm -f $(SRC_PARSERS)	
	rm -rf $(OBJ_DIR)
	rm -f SRC/verilog_bison.h
	rm -f SRC/verilog_bison.output
	rm -f SRC/tags
	rm -f *.exe
	rm -f SRC/*~
	

regression_clean :
	rm -f REGRESSION_TESTS/BASIC_TESTS/*.*

-include $(DEP)

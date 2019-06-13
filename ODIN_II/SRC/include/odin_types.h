/*

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/
#include "string_cache.h"
#include "odin_util.h"
#include "odin_error.h"
#include "read_xml_arch_file.h"
#include "simulate_blif.h"
#include "argparse_value.hpp"
#include "AtomicBuffer.hpp"
#include "config_t.h"
#include <mutex>
#include <atomic>

#include <stdlib.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef TYPES_H
#define TYPES_H

/**
 * to use short vs long string for output
 */
#define ODIN_LONG_STRING 0
#define ODIN_SHORT_STRING 1

#ifdef DEBUG_ODIN
	#define ODIN_STRING_TYPE ODIN_SHORT_STRING
#else
	#define ODIN_STRING_TYPE ODIN_LONG_STRING
#endif

#define ODIN_STD_BITWIDTH (sizeof(long)*8)

typedef struct global_args_t_t global_args_t;
/* new struct for the global arguments of verify_blif function */
typedef struct global_args_read_blif_t_t global_args_read_blif_t;

typedef struct ast_node_t_t ast_node_t;
typedef struct typ_t typ;

typedef struct sim_state_t_t sim_state_t;
typedef struct nnode_t_t nnode_t;
typedef struct ace_obj_info_t_t ace_obj_info_t;
typedef struct npin_t_t npin_t;
typedef struct nnet_t_t nnet_t;
typedef struct signal_list_t_t signal_list_t;
typedef struct char_list_t_t char_list_t;
typedef struct netlist_t_t netlist_t;
typedef struct netlist_stats_t_t netlist_stats_t;
typedef struct chain_information_t_t chain_information_t;

// to define type of adder in cmd line
typedef struct adder_def_t_t adder_def_t;

/* unique numbers to mark the nodes as we DFS traverse the netlist */
#define PARTIAL_MAP_TRAVERSE_VALUE 10
#define OUTPUT_TRAVERSE_VALUE 12
#define COUNT_NODES 14 /* NOTE that you can't call countnodes one after the other or the mark will be incorrect */
#define COMBO_LOOP 15
#define COMBO_LOOP_ERROR 16
#define GRAPH_CRUNCH 17
#define STATS 18
#define SEQUENTIAL_LEVELIZE 19

/* unique numbers for using void *data entries in some of the datastructures */
#define RESET -1
#define LEVELIZE 12
#define ACTIVATION 13

#define verify_i_o_availabilty(node, expected_input_size, expected_output_size) passed_verify_i_o_availabilty(node, expected_input_size, expected_output_size, __FILE__, __LINE__)

/* the global arguments of the software */
struct global_args_t_t
{
	std::string program_name;

    argparse::ArgValue<char*> config_file;
	argparse::ArgValue<std::vector<std::string>> verilog_files;
	argparse::ArgValue<char*> blif_file;
	argparse::ArgValue<char*> output_file;
	argparse::ArgValue<char*> arch_file; // Name of the FPGA architecture file

	argparse::ArgValue<char*> high_level_block; //Legacy option, no longer used

	argparse::ArgValue<char*> top_level_module_name; // force the name of the top level module desired

    argparse::ArgValue<bool> write_netlist_as_dot;
    argparse::ArgValue<bool> write_ast_as_dot;
    argparse::ArgValue<bool> all_warnings;
    argparse::ArgValue<bool> show_help;

	argparse::ArgValue<char*> adder_def; //carry skip adder skip size
    // defines if the first cin of an adder/subtractor is connected to a global gnd/vdd
    // or generated using a dummy adder with both inputs set to gnd/vdd
    argparse::ArgValue<bool> adder_cin_global;

	/////////////////////
	// For simulation.
	/////////////////////
	// Generate this number of random vectors.
	argparse::ArgValue<int> sim_num_test_vectors;
	// Input vectors to simulate instead of generating vectors.
	argparse::ArgValue<char*> sim_vector_input_file;
	// Existing output vectors to verify against.
	argparse::ArgValue<char*> sim_vector_output_file;
	// Simulation output Directory
	argparse::ArgValue<char*> sim_directory;
	// Tells the simulator whether or not to generate random vectors which include the unknown logic value.
	argparse::ArgValue<bool> sim_generate_three_valued_logic;
	// Output both falling and rising edges in the output_vectors file. (DEFAULT)
	argparse::ArgValue<bool> sim_output_both_edges;
	// Request to read mif file input
	argparse::ArgValue<bool> read_mif_input;
	// Additional pins, nets, and nodes to output.
	argparse::ArgValue<std::vector<std::string>> sim_additional_pins;
	// Comma-separated list of primary input pins to hold high for all cycles but the first.
	argparse::ArgValue<std::vector<std::string>> sim_hold_high;
	// Comma-separated list of primary input pins to hold low for all cycles but the first.
	argparse::ArgValue<std::vector<std::string>> sim_hold_low;
	// target coverage
	argparse::ArgValue<double> sim_min_coverage;
	// simulate until best coverage is achieved
	argparse::ArgValue<bool> sim_achieve_best;

	argparse::ArgValue<int> parralelized_simulation;
	argparse::ArgValue<bool> parralelized_simulation_in_batch;
	argparse::ArgValue<int> sim_initial_value;
	// The seed for creating random simulation vector
    argparse::ArgValue<int> sim_random_seed;

	argparse::ArgValue<bool> interactive_simulation;

};

#endif // TYPES_H

//-----------------------------------------------------------------------------------------------------

#ifndef AST_TYPES_H
#define AST_TYPES_H

/**
 * defined in enum_str.cpp
 */
extern const char *file_extension_supported_STR[];

extern const char *ZERO_GND_ZERO;
extern const char *ONE_VCC_CNS;
extern const char *ZERO_PAD_ZERO;

extern const char *SINGLE_PORT_RAM_string;
extern const char *DUAL_PORT_RAM_string;

extern const char *signedness_STR[];
extern const char *edge_type_e_STR[];
extern const char *operation_list_STR[][2];
extern const char *ids_STR [];

typedef enum
{
	VERILOG,
	file_extension_supported_END
} file_extension_supported;

typedef enum
{
	DEC = 10,
	HEX = 16,
	OCT = 8,
	BIN = 2,
	LONG = 0,
	bases_END
} bases;

typedef enum
{
	SIGNED,
	UNSIGNED,
	signedness_END
} signedness;

typedef enum
{
	UNDEFINED_SENSITIVITY,
	FALLING_EDGE_SENSITIVITY,
	RISING_EDGE_SENSITIVITY,
	ACTIVE_HIGH_SENSITIVITY,
	ACTIVE_LOW_SENSITIVITY,
	ASYNCHRONOUS_SENSITIVITY,
	edge_type_e_END
} edge_type_e;

typedef enum
{
	COMBINATIONAL,
	SEQUENTIAL,
	circuit_type_e_END
} circuit_type_e;

typedef enum
{
	NO_OP,
	MULTI_PORT_MUX, // port 1 = control, port 2+ = mux options
	FF_NODE,
	BUF_NODE,
	INPUT_NODE,
	OUTPUT_NODE,
	GND_NODE,
	VCC_NODE,
	CLOCK_NODE,
	ADD, // +
	MINUS, // -
	BITWISE_NOT, // ~
	BITWISE_AND, // &
	BITWISE_OR, // |
	BITWISE_NAND, // ~&
	BITWISE_NOR, // ~|
	BITWISE_XNOR, // ~^
	BITWISE_XOR, // ^
	LOGICAL_NOT, // !
	LOGICAL_OR, // ||
	LOGICAL_AND, // &&
	LOGICAL_NAND, // No Symbol
	LOGICAL_NOR, // No Symbol
	LOGICAL_XNOR, // No symbol
	LOGICAL_XOR, // No Symbol
	MULTIPLY, // *
	DIVIDE, // /
	MODULO, // %
	OP_POW, // **
	LT, // <
	GT, // >
	LOGICAL_EQUAL, // ==
	NOT_EQUAL, // !=
	LTE, // <=
	GTE, // >=
	SR, // >>
    ASR, // >>>
	SL, // <<
	CASE_EQUAL, // ===
	CASE_NOT_EQUAL, // !==
	ADDER_FUNC,
	CARRY_FUNC,
	MUX_2,
	BLIF_FUNCTION,
	NETLIST_FUNCTION,
	MEMORY,
	PAD_NODE,
	HARD_IP,
	GENERIC, /*added for the unknown node type */
	FULLADDER,
	CLOG2, // $clog2
	operation_list_END
} operation_list;

typedef enum
{
	NO_ID,
	/* top level things */
	FILE_ITEMS,
	MODULE,
	SPECIFY,
	/* VARIABLES */
	INPUT,
	OUTPUT,
	INOUT,
	WIRE,
	REG,
	INTEGER,
	GENVAR,
	PARAMETER,
	LOCALPARAM,
	INITIALS,
	PORT,
	/* OTHER MODULE ITEMS */
	MODULE_ITEMS,
	VAR_DECLARE,
	VAR_DECLARE_LIST,
	ASSIGN,
   	/* OTHER MODULE AND FUNCTION ITEMS */
	FUNCTION,
   	/* OTHER FUNCTION ITEMS */
  	FUNCTION_ITEMS,
	/* primitives */
	GATE,
	GATE_INSTANCE,
	ONE_GATE_INSTANCE,
	/* Module instances */
	MODULE_CONNECT_LIST,
	MODULE_CONNECT,
	MODULE_PARAMETER_LIST,
	MODULE_PARAMETER,
	MODULE_NAMED_INSTANCE,
	MODULE_INSTANCE,
	MODULE_MASTER_INSTANCE,
	ONE_MODULE_INSTANCE,
	/* Function instances*/
	FUNCTION_NAMED_INSTANCE,
	FUNCTION_INSTANCE,
	/* Specify Items */
	SPECIFY_ITEMS,
	SPECIFY_PARAMETER,
	SPECIFY_PAL_CONNECTION_STATEMENT,
	SPECIFY_PAL_CONNECT_LIST,
	/* statements */
	BLOCK,
	NON_BLOCKING_STATEMENT,
	BLOCKING_STATEMENT,
	ASSIGNING_LIST,
	CASE,
	CASE_LIST,
	CASE_ITEM,
	CASE_DEFAULT,
	ALWAYS,
	GENERATE,
	IF,
	IF_Q,
	FOR,
	WHILE,
	/* Delay Control */
	DELAY_CONTROL,
	POSEDGE,
	NEGEDGE,
	/* expressions */
	BINARY_OPERATION,
	UNARY_OPERATION,
	/* basic primitives */
	ARRAY_REF,
	RANGE_REF,
	CONCATENATE,
	/* basic identifiers */
	IDENTIFIERS,
	NUMBERS,
	/* Hard Blocks */
	HARD_BLOCK,
	HARD_BLOCK_NAMED_INSTANCE,
	HARD_BLOCK_CONNECT_LIST,
	HARD_BLOCK_CONNECT,
	// EDDIE: new enum value for ids to replace MEMORY from operation_t
	RAM,
	ids_END
} ids;

struct typ_t
{
	char *identifier;

	struct
	{
		short sign;
		short base;
		int size;
		int binary_size;
		char *binary_string;
		char *number;
		long value;
		short is_full; //'bx means all of the wire get 'x'(dont care)
	} number;
	struct
	{
		operation_list op;
	} operation;
	struct
	{
		short is_parameter;
		short is_localparam;
		short is_port;
		short is_input;
		short is_output;
		short is_inout;
		short is_wire;
		short is_reg;
		short is_integer;
		short is_initialized; // should the variable be initialized with some value?
		long initial_value;
	} variable;
	struct
	{
		short is_instantiated;
		ast_node_t **module_instantiations_instance;
		int size_module_instantiations;
		int index;
	} module;
	struct
	{
		short is_instantiated;
		ast_node_t **function_instantiations_instance;
		int size_function_instantiations;
		int index;
	} function;
	struct
	{
		int num_bit_strings;
		char **bit_strings;
	} concat;

};


struct ast_node_t_t
{
	long unique_count;
	int far_tag;
	int high_number;
	ids type;
	typ types;

	ast_node_t **children;
	long num_children;

	int line_number = -1;
	int file_number = -1;
	int related_module_id = -1;

	short shared_node;
	void *hb_port;
	void *net_node;
	short is_read_write;

};
#endif // AST_TYPES_H


//-----------------------------------------------------------------------------------------------------
#ifndef NETLIST_UTILS_H
#define NETLIST_UTILS_H
/* DEFINTIONS for carry chain*/
struct chain_information_t_t
{
	char *name;//unique name of the chain
	int count;//the number of hard blocks in this chain
	int num_bits;
};

/* DEFINTIONS for all the different types of nodes there are.  This is also used cross-referenced in utils.c so that I can get a string version
 * of these names, so if you add new tpyes in here, be sure to add those same types in utils.c */
struct nnode_t_t
{
	long unique_id;
	char *name; // unique name of a node
	operation_list type; // the type of node
	int bit_width; // Size of the operation (e.g. for adders/subtractors)

	ast_node_t *related_ast_node; // the abstract syntax node that made this node

	int line_number = -1;
	int file_number = -1;
	
	short traverse_visited; // a way to mark if we've visited yet

	npin_t **input_pins; // the input pins
	long num_input_pins;
	int *input_port_sizes; // info about the input ports
	int num_input_port_sizes;

	npin_t **output_pins; // the output pins
	long num_output_pins;
	int *output_port_sizes; // info if there is ports
	int num_output_port_sizes;

	short unique_node_data_id;
	void *node_data; // this is a point where you can add additional data for your optimization or technique

	int forward_level; // this is your logic level relative to PIs and FFs .. i.e farthest PI
	int backward_level; // this is your reverse logic level relative to POs and FFs .. i.e. farthest PO
	int sequential_level; // the associated sequential network that the node is in
	short sequential_terminator; // if this combinational node is a terminator for the sequential level (connects to flip-flop or Output pin

	netlist_t* internal_netlist; // this is a point of having a subgraph in a node

	std::vector<std::vector<signed char>> memory_data;
	std::map<int,std::map<long,std::vector<signed char>>> memory_directory;
	std::mutex memory_mtx;
	//(int cycle, int num_input_pins, npin_t *inputs, int num_output_pins, npin_t *outputs);
	void (*simulate_block_cycle)(int, int, int*, int, int*);

	short *associated_function;

	char** bit_map; /*storing the bit map */
	int bit_map_line_count;

	// For simulation
	int in_queue; // Flag used by the simulator to avoid double queueing.
	npin_t **undriven_pins; // These pins have been found by the simulator to have no driver.
	int num_undriven_pins;
	int ratio; //clock ratio for clock nodes
	signed char has_initial_value; // initial value assigned?
	signed char initial_value; // initial net value
	bool internal_clk_warn= false;
	edge_type_e edge_type; //
	bool covered = false;
	
	//Generic gate output
	unsigned char generic_output; //describes the output (1 or 0) of generic blocks
};


// Ace_Obj_Info_t; /* Activity info for each node */
struct ace_obj_info_t_t
{
	int value;
	int num_ones;
	int num_toggles;
	double static_prob;
	double switch_prob;
	double switch_act;
	double prob0to1;
	double prob1to0;
	int depth;
};

struct npin_t_t
{
	long unique_id;
	ids type;         // INPUT or OUTPUT
	char *name;
	nnet_t *net;      // related net
	int pin_net_idx;
	nnode_t *node;    // related node
	int pin_node_idx; // pin on the node where we're located
	char *mapping;    // name of mapped port from hard block

	edge_type_e sensitivity;

	////////////////////
	// For simulation
	std::shared_ptr<AtomicBuffer> values;

	bool delay_cycle;
	
	unsigned long coverage;
	bool is_default; // The pin is feeding a mux from logic representing an else or default.
	bool is_implied; // This signal is implied.

	////////////////////

	// For Activity Estimation
	ace_obj_info_t *ace_info;

};

struct nnet_t_t
{
	long unique_id;
	char *name; // name for the net
	short combined;

	npin_t *driver_pin; // the pin that drives the net

	npin_t **fanout_pins; // the pins pointed to by the net
	int num_fanout_pins; // the list size of pins

	short unique_net_data_id;
	void *net_data;

	/////////////////////
	// For simulation
	std::shared_ptr<AtomicBuffer> values;

	signed char has_initial_value; // initial value assigned?
	signed char initial_value; // initial net value
	//////////////////////
};

struct signal_list_t_t
{
	npin_t **pins;
	long count;

	char is_memory;
	char is_adder;
};

struct char_list_t_t
{
	char **strings;
	int num_strings;
};

struct netlist_t_t
{
	nnode_t *gnd_node;
	nnode_t *vcc_node;
	nnode_t *pad_node;
	nnet_t *zero_net;
	nnet_t *one_net;
	nnet_t *pad_net;
	nnode_t** top_input_nodes;
	int num_top_input_nodes;
	nnode_t** top_output_nodes;
	int num_top_output_nodes;
	nnode_t** ff_nodes;
	int num_ff_nodes;
	nnode_t** internal_nodes;
	int num_internal_nodes;
	nnode_t** clocks;
	int num_clocks;


	/* netlist levelized structures */
	nnode_t ***forward_levels;
	int num_forward_levels;
	int* num_at_forward_level;
	nnode_t ***backward_levels; // NOTE backward levels isn't neccessarily perfect.  Because of multiple output pins, the node can be put closer to POs than should be.  To fix, run a rebuild of the list afterwards since the marked "node->backward_level" is correct */
	int num_backward_levels;
	int* num_at_backward_level;

	nnode_t ***sequential_level_nodes;
	int num_sequential_levels;
	int* num_at_sequential_level;
	/* these structures store the last combinational node in a level before a flip-flop or output pin */
	nnode_t ***sequential_level_combinational_termination_node;
	int num_sequential_level_combinational_termination_nodes;
	int* num_at_sequential_level_combinational_termination_node;

	STRING_CACHE *nets_sc;
	STRING_CACHE *out_pins_sc;
	STRING_CACHE *nodes_sc;

	netlist_stats_t *stats;

	t_type_ptr type;

};

struct netlist_stats_t_t
{
	int num_inputs;
	int num_outputs;
	int num_ff_nodes;
	int num_logic_nodes;
	int num_nodes;

	float average_fanin; /* = to the fanin of all nodes: basic outs, combo and ffs */
	int *fanin_distribution;
	int num_fanin_distribution;

	long num_output_pins;
	float average_output_pins_per_node;
	float average_fanout; /* = to the fanout of all nodes: basic IOs, combo and ffs...no vcc, clocks, gnd */
	int *fanout_distribution;
	int num_fanout_distribution;

	/* expect these should be close but doubt they'll be equal */
	long num_edges_fo; /* without control signals like clocks and gnd and vcc...= to number of fanouts from output pins */
	long num_edges_fi; /* without control signals like clocks and gnd and vcc...= to number of fanins from output pins */

	int **combinational_shape;
	int *num_combinational_shape_for_sequential_level;
};

#endif // NET_TYPES_H

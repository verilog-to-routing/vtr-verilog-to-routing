#ifndef ODIN_TYPES_H
#define ODIN_TYPES_H
/*
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "string_cache.h"
#include "hierarchy_util.h"
#include "scope_util.h"
#include "odin_error.h"
#include "read_xml_arch_file.h"
#include "argparse_value.hpp"
#include "AtomicBuffer.hpp"
#include <mutex>
#include <atomic>
#include <string>
#include <stdbool.h>
#include <unordered_map>

#include <stdlib.h>

#include "rtl_int.hpp"

/**
 * to use short vs long string for output
 */
#define ODIN_LONG_STRING 0
#define ODIN_SHORT_STRING 1

#ifndef DEBUG_ODIN
#    define ODIN_STRING_TYPE ODIN_SHORT_STRING
#else
#    define ODIN_STRING_TYPE ODIN_LONG_STRING
#endif

#define ODIN_STD_BITWIDTH (sizeof(long) * 8)

/* buffer size for reading a directory path */
#define READ_BUFFER_SIZE 1048576 // 1MB

/* unique numbers to mark the nodes as we DFS traverse the netlist */
#define PARTIAL_MAP_TRAVERSE_VALUE 10
#define OUTPUT_TRAVERSE_VALUE 12
#define COUNT_NODES 14 /* NOTE that you can't call countnodes one after the other or the mark will be incorrect */
#define COMBO_LOOP 15
#define COMBO_LOOP_ERROR 16
#define GRAPH_CRUNCH 17
#define STATS 18
#define SEQUENTIAL_LEVELIZE 19
/* a unique mark for the BLIF elaboration DFS traversal */
#define BLIF_ELABORATE_TRAVERSE_VALUE 30

/* unique numbers for using void *data entries in some of the datastructures */
#define RESET -1
#define LEVELIZE 12
#define ACTIVATION 13

#define verify_i_o_availabilty(node, expected_input_size, expected_output_size) passed_verify_i_o_availabilty(node, expected_input_size, expected_output_size, __FILE__, __LINE__)

struct ast_node_t;
struct nnode_t;
struct npin_t;
struct nnet_t;
struct netlist_t;

/* the global arguments of the software */
struct global_args_t {
    std::string program_name;
    // Odin-II Root directory
    std::string program_root;
    // Current path Odin-II is running
    std::string current_path;

    argparse::ArgValue<std::string> config_file;
    argparse::ArgValue<std::vector<std::string>> verilog_files;
    argparse::ArgValue<std::string> blif_file;
    argparse::ArgValue<std::string> output_file;
    argparse::ArgValue<std::string> arch_file;   // Name of the FPGA architecture file
    argparse::ArgValue<std::string> tcl_file;    // TCL file to be run by yosys elaborator
    argparse::ArgValue<std::string> elaborator;  // Name of the external elaborator tool, currently Yosys is supported, default is Odin
    argparse::ArgValue<bool> permissive;         // turn possible_errors into warnings
    argparse::ArgValue<bool> print_parse_tokens; // print the tokens as they are parsed byt the parser

    argparse::ArgValue<std::string> high_level_block; //Legacy option, no longer used

    argparse::ArgValue<std::string> top_level_module_name; // force the name of the top level module desired

    argparse::ArgValue<bool> write_netlist_as_dot;
    argparse::ArgValue<bool> write_ast_as_dot;
    argparse::ArgValue<bool> all_warnings;
    argparse::ArgValue<bool> show_help;

    argparse::ArgValue<bool> fflegalize;     // makes flip-flops rising edge sensitive
    argparse::ArgValue<bool> coarsen;        // tells Odin-II that the input blif is coarse-grain
    argparse::ArgValue<bool> show_yosys_log; // Show Yosys output logs into the standard output stream

    argparse::ArgValue<std::string> adder_def; //DEPRECATED

    // defines if the first cin of an adder/subtractor is connected to a global gnd/vdd
    // or generated using a dummy adder with both inputs set to gnd/vdd
    argparse::ArgValue<bool> adder_cin_global;

    /////////////////////
    // For simulation.
    /////////////////////
    // Generate this number of random vectors.
    argparse::ArgValue<int> sim_num_test_vectors;
    // Input vectors to simulate instead of generating vectors.
    argparse::ArgValue<std::string> sim_vector_input_file;
    // Existing output vectors to verify against.
    argparse::ArgValue<std::string> sim_vector_output_file;
    // Simulation output Directory
    argparse::ArgValue<std::string> sim_directory;
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
    // deprecated since this should be defined when compiled
    argparse::ArgValue<int> sim_initial_value;
    // The seed for creating random simulation vector
    argparse::ArgValue<int> sim_random_seed;

    argparse::ArgValue<bool> interactive_simulation;

    // Arguments for mixing hard and soft logic
    argparse::ArgValue<int> exact_mults;
    argparse::ArgValue<float> mults_ratio;
};

/**
 * defined in enum_str.cpp
 */
extern const char* ieee_std_STR[];

extern const char* ZERO_GND_ZERO;
extern const char* ONE_VCC_CNS;
extern const char* ZERO_PAD_ZERO;

extern const char* SINGLE_PORT_RAM_string;
extern const char* DUAL_PORT_RAM_string;
extern const char* LUTRAM_string;

extern const char* edge_type_e_STR[];
extern const char* operation_list_STR[][2];
extern const char* ids_STR[];

template<typename T>
using strmap = std::unordered_map<std::string, T>;

enum file_type_e {
    _ILANG, /* not supported yet */
    _VERILOG,
    _VERILOG_HEADER,
    _BLIF,
    _EBLIF,     /* not supported yet */
    _UNDEFINED, /* EROOR */
    file_type_e_END
};

enum elaborator_e {
    _ODIN,
    _YOSYS,
    elaborator_e_END
};

enum ieee_std {
    ieee_1995,
    ieee_2001_noconfig,
    ieee_2001,
    ieee_2005
};

enum edge_type_e {
    UNDEFINED_SENSITIVITY,
    FALLING_EDGE_SENSITIVITY,
    RISING_EDGE_SENSITIVITY,
    ACTIVE_HIGH_SENSITIVITY,
    ACTIVE_LOW_SENSITIVITY,
    ASYNCHRONOUS_SENSITIVITY,
    edge_type_e_END
};

enum circuit_type_e {
    COMBINATIONAL,
    SEQUENTIAL,
    circuit_type_e_END
};

enum init_value_e {
    _0 = 0,
    _1 = 1,
    dont_care = 2,
    undefined = 3,
};

/**
 * list of Odin-II supported operation node
 * In the synthesis flow, most operations are resolved or mapped
 * to an operation mode that has instantiation procedure in the 
 * partial mapping. However, for techmap flow, nodes are elaborated
 * into the partial mapper supported operations in BLIF elaboration.
 * Technically, each Odin-II node should have one of the following
 * operation type. To add support for a new type you would need to
 * start from here to see how each operation mode is being resolved.
 */
enum operation_list {
    NO_OP,
    MULTI_PORT_MUX, // port 1 = control, port 2+ = mux options
    FF_NODE,
    BUF_NODE,
    INPUT_NODE,
    OUTPUT_NODE,
    GND_NODE,
    VCC_NODE,
    CLOCK_NODE,
    ADD,            // +
    MINUS,          // -
    BITWISE_NOT,    // ~
    BITWISE_AND,    // &
    BITWISE_OR,     // |
    BITWISE_NAND,   // ~&
    BITWISE_NOR,    // ~|
    BITWISE_XNOR,   // ~^
    BITWISE_XOR,    // ^
    LOGICAL_NOT,    // !
    LOGICAL_OR,     // ||
    LOGICAL_AND,    // &&
    LOGICAL_NAND,   // No Symbol
    LOGICAL_NOR,    // No Symbol
    LOGICAL_XNOR,   // No symbol
    LOGICAL_XOR,    // No Symbol
    MULTIPLY,       // *
    DIVIDE,         // /
    MODULO,         // %
    POWER,          // **
    LT,             // <
    GT,             // >
    LOGICAL_EQUAL,  // ==
    NOT_EQUAL,      // !=
    LTE,            // <=
    GTE,            // >=
    SR,             // >>
    ASR,            // >>>
    SL,             // <<
    ASL,            // <<<
    CASE_EQUAL,     // ===
    CASE_NOT_EQUAL, // !==
    ADDER_FUNC,
    CARRY_FUNC,
    MUX_2,
    SMUX_2, // MUX_2 with single bit selector (no need to add not selector as the second pin) => [SEL] [IN1, IN2] [OUT]
    BLIF_FUNCTION,
    NETLIST_FUNCTION,
    MEMORY,
    PAD_NODE,
    HARD_IP,
    GENERIC,             /*added for the unknown node type */
    CLOG2,               // $clog2
    UNSIGNED,            // $unsigned
    SIGNED,              // $signed
                         // [START] operations to cover yosys subckt
    MULTI_BIT_MUX_2,     // like MUX_2 but with n-bit input/output
    MULTIPORT_nBIT_SMUX, // n-bit input/output in multiple ports
    PMUX,                // Multiplexer with many inputs using one-hot select signal
    SDFF,                // data, S to reset value and output port
    DFFE,                // data, enable to output port
    SDFFE,               // data, synchronous reset value and enable to output port
    SDFFCE,              // data, synchronous reset value and enable to reset value and output port
    DFFSR,               // data, clear and set to output port
    DFFSRE,              // data, clear and set with enable to output port
    DLATCH,              // datato output port based on polarity without clk
    ADLATCH,             // datato output port based on polarity without clk
    SETCLR,              // set or clear an input pins
    SPRAM,               // representing primitive single port ram
    DPRAM,               // representing primitive dual port ram
    YMEM,                // $mem block memory generated by ysos, can have complete varaiable num of read and write plus clk for each port
    BRAM,                // Odin-II block memory, from techlib/bram_bb.v
    ROM,                 // Odin-II read-only memory, from techlib/rom_bb.v
                         // [END] operations to cover yosys subckt
    operation_list_END
};

enum ids {
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
    GENVAR,
    PARAMETER,
    LOCALPARAM,
    INITIAL,
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
    TASK,
    TASK_ITEMS,
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
    TASK_NAMED_INSTANCE,
    TASK_INSTANCE,
    /* Specify Items */
    SPECIFY_ITEMS,
    SPECIFY_PARAMETER,
    SPECIFY_PAL_CONNECTION_STATEMENT,
    SPECIFY_PAL_CONNECT_LIST,
    /* statements */
    STATEMENT,
    BLOCK,
    NON_BLOCKING_STATEMENT,
    BLOCKING_STATEMENT,
    ASSIGNING_LIST,
    CASE,
    CASE_LIST,
    CASE_ITEM,
    CASE_DEFAULT,
    ALWAYS,
    IF,
    FOR,
    WHILE,
    /* Delay Control */
    DELAY_CONTROL,
    POSEDGE,
    NEGEDGE,
    /* expressions */
    TERNARY_OPERATION,
    BINARY_OPERATION,
    UNARY_OPERATION,
    /* basic primitives */
    ARRAY_REF,
    RANGE_REF,
    CONCATENATE,
    REPLICATE,
    /* basic identifiers */
    IDENTIFIERS,
    NUMBERS,
    /* C functions */
    C_ARG_LIST,
    DISPLAY,
    FINISH,
    /* Hard Blocks */
    HARD_BLOCK,
    HARD_BLOCK_NAMED_INSTANCE,
    HARD_BLOCK_CONNECT_LIST,
    HARD_BLOCK_CONNECT,
    // EDDIE: new enum value for ids to replace MEMORY from operation_t
    RAM,
    ids_END
};

struct metric_t {
    double min_depth;
    double max_depth;
    double avg_depth;
    double avg_width;
};

struct stat_t {
    metric_t upward;
    metric_t downward;
};

struct typ {
    char* identifier;
    VNumber* vnumber = nullptr;
    sc_hierarchy* hierarchy;
    sc_scope* scope;
    struct
    {
        operation_list op;
    } operation;
    struct
    {
        short is_parameter;
        short is_string;
        short is_localparam;
        short is_defparam;
        short is_port;
        short is_input;
        short is_output;
        short is_inout;
        short is_wire;
        short is_reg;
        short is_genvar;
        short is_memory;
        operation_list signedness;
        VNumber* initial_value = nullptr;
    } variable;
    struct
    {
        short is_instantiated;
        ast_node_t** module_instantiations_instance;
        int size_module_instantiations;
    } module;
    struct
    {
        short is_instantiated;
        ast_node_t** function_instantiations_instance;
        int size_function_instantiations;
    } function;
    struct
    {
        short is_instantiated;
        ast_node_t** task_instantiations_instance;
        int size_task_instantiations;
    } task;
    struct
    {
        int num_bit_strings;
        char** bit_strings;
    } concat;
};

struct ast_node_t {
    loc_t loc;

    long unique_count;
    int far_tag;
    int high_number;
    ids type;
    typ types;

    ast_node_t* identifier_node;
    ast_node_t** children;
    long num_children;

    void* hb_port;
    void* net_node;
    long chunk_size;
};

//-----------------------------------------------------------------------------------------------------

/* DEFINTIONS for carry chain*/
struct chain_information_t {
    char* name; //unique name of the chain
    int count;  //the number of hard blocks in this chain
    int num_bits;
};

//-----------------------------------------------------------------------------------------------------

/**
 * DEFINTIONS netlist node attributes
 * the attr_t structure provides the control signals sensitivity
 * In the synthesis flow, the attribute structure is mostly used to 
 * specify the clock sensitivity. However, in the techmap flow, 
 * it is used in most sections, including DFFs, Block memories 
 * and arithmetic operation instantiation
 */
struct attr_t {
    edge_type_e clk_edge_type;   // clock edge sensitivity
    edge_type_e clr_polarity;    // clear (reset to GND) polarity
    edge_type_e set_polarity;    // set to VCC polarity
    edge_type_e enable_polarity; // enable polarity
    edge_type_e areset_polarity; // asynchronous reset polarity
    edge_type_e sreset_polarity; // synchronous reset polarity

    long areset_value; // asynchronous reset value
    long sreset_value; // synchronous reset value

    operation_list port_a_signed;
    operation_list port_b_signed;

    /* memory node attributes */
    long size;                   // memory size
    long offset;                 // ADDR offset
    char* memory_id;             // the id of memory in verilog file (different from name since for memory it is $mem~#)
    edge_type_e RD_CLK_ENABLE;   // read clock enable
    edge_type_e WR_CLK_ENABLE;   // write clock enable
    edge_type_e RD_CLK_POLARITY; // read clock polarity
    edge_type_e WR_CLK_POLARITY; // write clock polarity
    long RD_PORTS;               // Numof read ports
    long WR_PORTS;               // Num of Write ports
    long DBITS;                  // Data width
    long ABITS;                  // Addr width
};

/* DEFINTIONS for all the different types of nodes there are.  This is also used cross-referenced in utils.c so that I can get a string version
 * of these names, so if you add new tpyes in here, be sure to add those same types in utils.c */
struct nnode_t {
    loc_t loc;

    long unique_id;
    char* name;          // unique name of a node
    operation_list type; // the type of node
    int bit_width;       // Size of the operation (e.g. for adders/subtractors)

    ast_node_t* related_ast_node; // the abstract syntax node that made this node

    uintptr_t traverse_visited; // a way to mark if we've visited yet
    stat_t stat;

    npin_t** input_pins; // the input pins
    long num_input_pins;
    int* input_port_sizes; // info about the input ports
    int num_input_port_sizes;

    npin_t** output_pins; // the output pins
    long num_output_pins;
    int* output_port_sizes; // info if there is ports
    int num_output_port_sizes;

    short unique_node_data_id;
    void* node_data; // this is a point where you can add additional data for your optimization or technique

    int forward_level;           // this is your logic level relative to PIs and FFs .. i.e farthest PI
    int backward_level;          // this is your reverse logic level relative to POs and FFs .. i.e. farthest PO
    int sequential_level;        // the associated sequential network that the node is in
    short sequential_terminator; // if this combinational node is a terminator for the sequential level (connects to flip-flop or Output pin

    netlist_t* internal_netlist; // this is a point of having a subgraph in a node

    std::vector<std::vector<BitSpace::bit_value_t>> memory_data;
    //(int cycle, int num_input_pins, npin_t *inputs, int num_output_pins, npin_t *outputs);
    void (*simulate_block_cycle)(int, int, int*, int, int*);

    short* associated_function;

    char** bit_map; /*storing the bit map */
    int bit_map_line_count;

    // For simulation
    int in_queue;           // Flag used by the simulator to avoid double queueing.
    npin_t** undriven_pins; // These pins have been found by the simulator to have no driver.
    int num_undriven_pins;
    int ratio;                  //clock ratio for clock nodes
    init_value_e initial_value; // initial net value
    bool internal_clk_warn = false;

    attr_t* attributes;

    bool covered = false;

    // For mixing soft and hard logic optimizations
    // a field that is used for storing weights towards the
    // mixing optimization.
    //  value of -1 is reserved for hardened blocks
    long weight = 0;

    //Generic gate output
    unsigned char generic_output; //describes the output (1 or 0) of generic blocks
};

struct npin_t {
    long unique_id;
    ids type; // INPUT or OUTPUT
    char* name;
    nnet_t* net; // related net
    int pin_net_idx;
    nnode_t* node;    // related node
    int pin_node_idx; // pin on the node where we're located
    char* mapping;    // name of mapped port from hard block

    edge_type_e sensitivity;

    ////////////////////
    // For simulation
    std::shared_ptr<AtomicBuffer> values;

    bool delay_cycle;

    unsigned long coverage;
    bool is_default; // The pin is feeding a mux from logic representing an else or default.
    bool is_implied; // This signal is implied.
};

struct nnet_t {
    long unique_id;
    char* name; // name for the net
    short combined;

    int num_driver_pins;
    npin_t** driver_pins; // the pin that drives the net

    npin_t** fanout_pins; // the pins pointed to by the net
    int num_fanout_pins;  // the list size of pins

    short unique_net_data_id;
    void* net_data;

    uintptr_t traverse_visited;
    stat_t stat;
    /////////////////////
    // For simulation
    std::shared_ptr<AtomicBuffer> values;

    init_value_e initial_value; // initial net value
                                //////////////////////
};

struct signal_list_t {
    npin_t** pins;
    long count;

    char is_memory;
    char is_adder;
};

struct char_list_t {
    char** strings;
    int num_strings;
};

struct ast_t {
    ast_node_t** top_modules;
    int top_modules_count;
};

struct netlist_t {
    char* identifier;

    nnode_t* gnd_node;
    nnode_t* vcc_node;
    nnode_t* pad_node;
    nnet_t* zero_net;
    nnet_t* one_net;
    nnet_t* pad_net;
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
    nnode_t*** forward_levels;
    int num_forward_levels;
    int* num_at_forward_level;
    nnode_t*** backward_levels; // NOTE backward levels isn't neccessarily perfect.  Because of multiple output pins, the node can be put closer to POs than should be.  To fix, run a rebuild of the list afterwards since the marked "node->backward_level" is correct */
    int num_backward_levels;
    int* num_at_backward_level;

    nnode_t*** sequential_level_nodes;
    int num_sequential_levels;
    int* num_at_sequential_level;
    /* these structures store the last combinational node in a level before a flip-flop or output pin */
    nnode_t*** sequential_level_combinational_termination_node;
    int num_sequential_level_combinational_termination_nodes;
    int* num_at_sequential_level_combinational_termination_node;

    STRING_CACHE* nets_sc;
    STRING_CACHE* out_pins_sc;
    STRING_CACHE* nodes_sc;

    long long num_of_type[operation_list_END];
    long long num_of_node;
    long long num_logic_element;
    metric_t output_node_stat;

    t_logical_block_type_ptr type;
};

#endif

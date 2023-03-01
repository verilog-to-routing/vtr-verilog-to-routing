/*
 * Copyright 2023 CAS—Atlantic (University of New Brunswick, CASA)
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

#include "odin_types.h"

const char* ieee_std_STR[] = {
    "1364-1995",
    "1364-2001-noconfig",
    "1364-2001",
    "1364-2005",
};

const char* edge_type_e_STR[] = {
    "UNDEFINED_SENSITIVITY",
    "FALLING_EDGE_SENSITIVITY",
    "RISING_EDGE_SENSITIVITY",
    "ACTIVE_HIGH_SENSITIVITY",
    "ACTIVE_LOW_SENSITIVITY",
    "ASYNCHRONOUS_SENSITIVITY",
};

const char* _ZERO_GND_ZERO[] = {
    "ZERO_GND_ZERO", "ZGZ"};

const char* _ONE_VCC_CNS[] = {
    "ONE_VCC_CNS",
    "OVC",
};

const char* _ZERO_PAD_ZERO[] = {
    "ZERO_PAD_ZERO", "ZPZ"};

const char* ZERO_GND_ZERO = _ZERO_GND_ZERO[ODIN_STRING_TYPE];
const char* ONE_VCC_CNS = _ONE_VCC_CNS[ODIN_STRING_TYPE];
const char* ZERO_PAD_ZERO = _ZERO_PAD_ZERO[ODIN_STRING_TYPE];

const char* SINGLE_PORT_RAM_string = "single_port_ram";
const char* DUAL_PORT_RAM_string = "dual_port_ram";
const char* LUTRAM_string = "lutram_ram";

const char* operation_list_STR[][2] = {
    {"NO_OP", "nOP"},
    {"CLOCK_NODE", "CLK"},
    {"INPUT_NODE", "IN"},
    {"OUTPUT_NODE", "OUT"},
    {"MULTI_PORT_MUX", "nMUX"}, // port 1 = control, port 2+ = mux options
    {"FF_NODE", "FF"},
    {"BUF_NODE", "BUF"},
    {"GND_NODE", "GND"},
    {"VCC_NODE", "VCC"},
    {"ADD", "ADD"},             // +
    {"MINUS", "MIN"},           // -
    {"BITWISE_NOT", "bNOT"},    // ~
    {"BITWISE_AND", "bAND"},    // &
    {"BITWISE_OR", "bOR"},      // |
    {"BITWISE_NAND", "bNAND"},  // ~&
    {"BITWISE_NOR", "bNOR"},    // ~|
    {"BITWISE_XNOR", "bXNOR"},  // ~^
    {"BITWISE_XOR", "bXOR"},    // ^
    {"LOGICAL_NOT", "lNOT"},    // !
    {"LOGICAL_OR", "lOR"},      // ||
    {"LOGICAL_AND", "lAND"},    // &&
    {"LOGICAL_NAND", "lNAND"},  // No Symbol
    {"LOGICAL_NOR", "lNOR"},    // No Symbol
    {"LOGICAL_XNOR", "lXNOR"},  // No symbol
    {"LOGICAL_XOR", "lXOR"},    // No Symbol
    {"MULTIPLY", "MUL"},        // *
    {"DIVIDE", "DIV"},          // /
    {"MODULO", "MOD"},          // %
    {"POWER", "POW"},           // **
    {"LT", "LT"},               // <
    {"GT", "GT"},               // >
    {"LOGICAL_EQUAL", "lEQ"},   // ==
    {"NOT_EQUAL", "lNEQ"},      // !=
    {"LTE", "LTE"},             // <=
    {"GTE", "GTE"},             // >=
    {"SR", "SR"},               // >>
    {"ASR", "ASR"},             // >>>
    {"SL", "SL"},               // <<
    {"ASL", "ASL"},             // <<<
    {"CASE_EQUAL", "cEQ"},      // ===
    {"CASE_NOT_EQUAL", "cNEQ"}, // !==
    {"ADDER_FUNC", "ADDER"},
    {"CARRY_FUNC", "CARRY"},
    {"MUX_2", "MUX_2"},
    {"SMUX_2", "SMUX_2"}, // MUX_2 with single bit selector (no need to add not selector as the second pin)
    {"BLIF_FUNCTION", "BLIFf"},
    {"NETLIST_FUNCTION", "NETf"},
    {"MEMORY", "MEM"},
    {"PAD_NODE", "PAD"},
    {"HARD_IP", "HARD"},
    {"GENERIC", "GEN"},                /*added for the unknown node type */
    {"CLOG2", "CL2"},                  // $clog2
    {"UNSIGNED", "UNSG"},              // $unsigned
    {"SIGNED", "SG"},                  // $signed
                                       // [START] operations to cover yosys subckt
    {"MULTI_BIT_MUX_2", "nbMUX"},      // like MUX_2 but with n-bit input/output
    {"MULTIPORT_nBIT_SMUX", "npbMUX"}, // n-bit input/output in multi port mux
    {"PMUX", "pMUX"},                  // Multiplexer with many inputs using one-hot select signal
    {"SDFF", "sDFF"},                  // data, S to reset value and output port
    {"DFFE", "DFFe"},                  // data, enable to output port
    {"SDFFE", "sDFFe"},                // data, synchronous reset value and enable to output port
    {"SDFFCE", "sDFFce"},              // data, synchronous reset value and enable to reset value and output port
    {"DFFSR", "DFFsr"},                // data, clear and set to output port
    {"DFFSRE", "DFFsre"},              // data, clear and set with enable to output port
    {"DLATCH", "Dlatch"},              // datato output port based on polarity without clk
    {"ADLATCH", "aDlatch"},            // datato output port based on polarity without clk
    {"SETCLR", "setclr"},              // set or clear an input pins
    {"SPRAM", "spRAM"},                // representing primitive single port ram
    {"DPRAM", "dpRAM"},                // representing primitive dual port ram
    {"YMEM", "yRAM"},                  // representing primitive dual port ram
    {"YMEM2", "yRAM"},                 // representing primitive dual port ram
    {"BRAM", "bRAM"},                  // block of memry generated in yosys subcircuit formet blif file
    {"ROM", "ROM"},
    // [END] operations to cover yosys subckt
    {"ERROR OOB", "OOB"} // should not reach this
};

const char* ids_STR[] = {
    "NO_ID",
    /* top level things */
    "FILE_ITEMS",
    "MODULE",
    "SPECIFY",
    /* VARIABLES */
    "INPUT",
    "OUTPUT",
    "INOUT",
    "WIRE",
    "REG",
    "GENVAR",
    "PARAMETER",
    "LOCALPARAM",
    "INITIAL",
    "PORT",
    /* OTHER MODULE ITEMS */
    "MODULE_ITEMS",
    "VAR_DECLARE",
    "VAR_DECLARE_LIST",
    "ASSIGN",
    /* OTHER MODULE AND FUNCTION ITEMS */
    "FUNCTION",
    /* OTHER FUNCTION ITEMS */
    "FUNCTION_ITEMS",
    "TASK",
    "TASK_ITEMS",
    /* primitives */
    "GATE",
    "GATE_INSTANCE",
    "ONE_GATE_INSTANCE",
    /* Module instances */
    "MODULE_CONNECT_LIST",
    "MODULE_CONNECT",
    "MODULE_PARAMETER_LIST",
    "MODULE_PARAMETER",
    "MODULE_NAMED_INSTANCE",
    "MODULE_INSTANCE",
    "MODULE_MASTER_INSTANCE",
    "ONE_MODULE_INSTANCE",
    /* Function instances*/
    "FUNCTION_NAMED_INSTANCE",
    "FUNCTION_INSTANCE",

    "TASK_NAMED_INSTANCE",
    "TASK_INSTANCE",
    /* Specify Items */
    "SPECIFY_ITEMS",
    "SPECIFY_PARAMETER",
    "SPECIFY_PAL_CONNECTION_STATEMENT",
    "SPECIFY_PAL_CONNECT_LIST",
    /* statements */
    "STATEMENT",
    "BLOCK",
    "NON_BLOCKING_STATEMENT",
    "BLOCKING_STATEMENT",
    "ASSIGNING_LIST",
    "CASE",
    "CASE_LIST",
    "CASE_ITEM",
    "CASE_DEFAULT",
    "ALWAYS",
    "IF",
    "FOR",
    "WHILE",
    /* Delay Control */
    "DELAY_CONTROL",
    "POSEDGE",
    "NEGEDGE",
    /* expressions */
    "TERNARY_OPERATION",
    "BINARY_OPERATION",
    "UNARY_OPERATION",
    /* basic primitives */
    "ARRAY_REF",
    "RANGE_REF",
    "CONCATENATE",
    "REPLICATE",
    /* basic identifiers */
    "IDENTIFIERS",
    "NUMBERS",
    /* C functions */
    "C_ARG_LIST",
    "DISPLAY",
    "FINISH",
    /* Hard Blocks */
    "HARD_BLOCK",
    "HARD_BLOCK_NAMED_INSTANCE",
    "HARD_BLOCK_CONNECT_LIST",
    "HARD_BLOCK_CONNECT",
    // EDDIE: new enum value for ids to replace MEMORY from operation_t
    "RAM",
    "ids_END"};

/* supported input/output file extensions */
extern const strbimap<file_type_e> file_extension_strmap({{".ilang", file_type_e::ILANG},
                                                          {".v", file_type_e::VERILOG},
                                                          {".vh", file_type_e::VERILOG_HEADER},
                                                          {".sv", file_type_e::SYSTEM_VERILOG},
                                                          {".svh", file_type_e::SYSTEM_VERILOG_HEADER},
                                                          {".blif", file_type_e::BLIF},
                                                          {".eblif", file_type_e::EBLIF}});

/* supported input/output file types */
extern const strbimap<file_type_e> file_type_strmap({{"ilang", file_type_e::ILANG},
                                                     {"verilog", file_type_e::VERILOG},
                                                     {"verilog_header", file_type_e::VERILOG_HEADER},
                                                     {"systemverilog", file_type_e::SYSTEM_VERILOG},
                                                     {"systemverilog_header", file_type_e::SYSTEM_VERILOG_HEADER},
                                                     {"uhdm", file_type_e::UHDM},
                                                     {"blif", file_type_e::BLIF},
                                                     {"eblif", file_type_e::EBLIF}});

/**
 * global hashmap of odin subckt types
 * Technically, Odin-II only outputs the following hard blocks
 * as (.subckt) record in its output BLIF file
 *
 *  FIRST_ELEMENT: model name showing in a blif file
 *  SECOND_ELEMENT: corresponding Odin-II cell type
 */
extern const strmap<operation_list> odin_subckt_strmap({{"multiply", MULTIPLY},
                                                        {"mult_", MULTIPLY},
                                                        {"adder", ADD},
                                                        {"sub", MINUS}});

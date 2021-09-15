#include "odin_types.h"

const char* ieee_std_STR[] = {
    "1364-1995",
    "1364-2001-noconfig",
    "1364-2001",
    "1364-2005",
};

const char* file_extension_supported_STR[] = {
    ".v",
    ".vh",
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
    {"MULTI_PORT_MUX", "nMUX"}, // port 1 = control, port 2+ = mux options
    {"FF_NODE", "FF"},
    {"BUF_NODE", "BUF"},
    {"INPUT_NODE", "IN"},
    {"OUTPUT_NODE", "OUT"},
    {"GND_NODE", "GND"},
    {"VCC_NODE", "VCC"},
    {"CLOCK_NODE", "CLK"},
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
strmap<file_type_e> file_type_strmap({{"ilang", file_type_e::_ILANG},
                                      {"verilog", file_type_e::_VERILOG},
                                      {"verilog_header", file_type_e::_VERILOG_HEADER},
                                      {"blif", file_type_e::_BLIF},
                                      {"eblif", file_type_e::_EBLIF},
                                      {"undef", file_type_e::_UNDEFINED}});

/* available elaborators for Odin-II techmap */
strmap<elaborator_e> elaborator_strmap({{"odin", elaborator_e::_ODIN},
                                        {"yosys", elaborator_e::_YOSYS}});

/**
 * global hashmap of odin subckt types
 * Technically, Odin-II only outputs the following hard blocks
 * as (.subckt) record in its output BLIF file
 *
 *  FIRST_ELEMENT: model name showing in a blif file
 *  SECOND_ELEMENT: corresponding Odin-II cell type
 */
strmap<operation_list> odin_subckt_strmap({{"multiply", MULTIPLY},
                                           {"mult_", MULTIPLY},
                                           {"adder", ADD},
                                           {"sub", MINUS}});

/**
 * global hashmap of yosys subckt types
 * Technically, Yosys only outputs the following hard blocks
 * as (.subckt) record in its output BLIF file
 *
 *  FIRST_ELEMENT: Yosys model names showing in a blif file
 *  SECOND_ELEMENT: corresponding Odin-II cell type
 *
 * NOTE: to add support for a new type, first you would find a 
 * corresponding Odin-II cell type or create a new one matching 
 * with the new type corresponding model (.subckt) in BLIF file, 
 * and add it to the following typemap. Then, you would need to 
 * specify the model input-output order in the Odin-II BLIF Reader.
 * At the end, a resolve_XXX_node function needs to be implemented
 * in the BLIF Elaboration phase to make the new node compatible
 * with the Odin-II partial mapper.
 */
strmap<operation_list> yosys_subckt_strmap({
    {"$_ANDNOT_", operation_list_END},
    {"$_AND_", operation_list_END},          // (A, B, Y)
    {"$_AOI3_", operation_list_END},         // (A, B, C, Y)
    {"$_AOI4_", operation_list_END},         // (A, B, C, Y)
    {"$_BUF_", BUF_NODE},                    // (A, Y)
    {"$_DFFE_NN0N_", operation_list_END},    // (D, C, R, E, Q)
    {"$_DFFE_NN0P_", operation_list_END},    // (D, C, R, E, Q)
    {"$_DFFE_NN1N_", operation_list_END},    // (D, C, R, E, Q)
    {"$_DFFE_NN1P_", operation_list_END},    // (D, C, R, E, Q)
    {"$_DFFE_NN_", operation_list_END},      // (D, C, E, Q)
    {"$_DFFE_NP0N_", operation_list_END},    // (D, C, R, E, Q)
    {"$_DFFE_NP0P_", operation_list_END},    // (D, C, R, E, Q)
    {"$_DFFE_NP1N_", operation_list_END},    // (D, C, R, E, Q)
    {"$_DFFE_NP1P_", operation_list_END},    // (D, C, R, E, Q)
    {"$_DFFE_NP_", operation_list_END},      // (D, C, E, Q)
    {"$_DFFE_PN0N_", operation_list_END},    // (D, C, R, E, Q)
    {"$_DFFE_PN0P_", operation_list_END},    // (D, C, R, E, Q)
    {"$_DFFE_PN1N_", operation_list_END},    // (D, C, R, E, Q)
    {"$_DFFE_PN1P_", operation_list_END},    // (D, C, R, E, Q)
    {"$_DFFE_PN_", operation_list_END},      // (D, C, E, Q)
    {"$_DFFE_PP0N_", operation_list_END},    // (D, C, R, E, Q)
    {"$_DFFE_PP0P_", operation_list_END},    // (D, C, R, E, Q)
    {"$_DFFE_PP1N_", operation_list_END},    // (D, C, R, E, Q)
    {"$_DFFE_PP1P_", operation_list_END},    // (D, C, R, E, Q)
    {"$_DFFE_PP_", operation_list_END},      // (D, C, E, Q)
    {"$_DFFSRE_NNNN_", operation_list_END},  // (C, S, R, E, D, Q)
    {"$_DFFSRE_NNNP_", operation_list_END},  // (C, S, R, E, D, Q)
    {"$_DFFSRE_NNPN_", operation_list_END},  // (C, S, R, E, D, Q)
    {"$_DFFSRE_NNPP_", operation_list_END},  // (C, S, R, E, D, Q)
    {"$_DFFSRE_NPNN_", operation_list_END},  // (C, S, R, E, D, Q)
    {"$_DFFSRE_NPNP_", operation_list_END},  // (C, S, R, E, D, Q)
    {"$_DFFSRE_NPPN_", operation_list_END},  // (C, S, R, E, D, Q)
    {"$_DFFSRE_NPPP_", operation_list_END},  // (C, S, R, E, D, Q)
    {"$_DFFSRE_PNNN_", operation_list_END},  // (C, S, R, E, D, Q)
    {"$_DFFSRE_PNNP_", operation_list_END},  // (C, S, R, E, D, Q)
    {"$_DFFSRE_PNPN_", operation_list_END},  // (C, S, R, E, D, Q)
    {"$_DFFSRE_PNPP_", operation_list_END},  // (C, S, R, E, D, Q)
    {"$_DFFSRE_PPNN_", operation_list_END},  // (C, S, R, E, D, Q)
    {"$_DFFSRE_PPNP_", operation_list_END},  // (C, S, R, E, D, Q)
    {"$_DFFSRE_PPPN_", operation_list_END},  // (C, S, R, E, D, Q)
    {"$_DFFSRE_PPPP_", operation_list_END},  // (C, S, R, E, D, Q)
    {"$_DFFSR_NNN_", operation_list_END},    // (C, S, R, D, Q)
    {"$_DFFSR_NNP_", operation_list_END},    // (C, S, R, D, Q)
    {"$_DFFSR_NPN_", operation_list_END},    // (C, S, R, D, Q)
    {"$_DFFSR_NPP_", operation_list_END},    // (C, S, R, D, Q)
    {"$_DFFSR_PNN_", operation_list_END},    // (C, S, R, D, Q)
    {"$_DFFSR_PNP_", operation_list_END},    // (C, S, R, D, Q)
    {"$_DFFSR_PPN_", operation_list_END},    // (C, S, R, D, Q)
    {"$_DFFSR_PPP_", operation_list_END},    // (C, S, R, D, Q)
    {"$_DFF_NN0_", operation_list_END},      // (D, C, R, Q)
    {"$_DFF_NN1_", operation_list_END},      // (D, C, R, Q)
    {"$_DFF_NP0_", operation_list_END},      // (D, C, R, Q)
    {"$_DFF_NP1_", operation_list_END},      // (D, C, R, Q)
    {"$_DFF_N_", operation_list_END},        // (D, C, Q)
    {"$_DFF_PN0_", operation_list_END},      // (D, C, R, Q)
    {"$_DFF_PN1_", operation_list_END},      // (D, C, R, Q)
    {"$_DFF_PP0_", operation_list_END},      // (D, C, R, Q)
    {"$_DFF_PP1_", operation_list_END},      // (D, C, R, Q)
    {"$_DFF_P_", operation_list_END},        // (D, C, Q)
    {"$_DLATCHSR_NNN_", operation_list_END}, // (E, S, R, D, Q)
    {"$_DLATCHSR_NNP_", operation_list_END}, // (E, S, R, D, Q)
    {"$_DLATCHSR_NPN_", operation_list_END}, // (E, S, R, D, Q)
    {"$_DLATCHSR_NPP_", operation_list_END}, // (E, S, R, D, Q)
    {"$_DLATCHSR_PNN_", operation_list_END}, // (E, S, R, D, Q)
    {"$_DLATCHSR_PNP_", operation_list_END}, // (E, S, R, D, Q)
    {"$_DLATCHSR_PPN_", operation_list_END}, // (E, S, R, D, Q)
    {"$_DLATCHSR_PPP_", operation_list_END}, // (E, S, R, D, Q)
    {"$_DLATCH_NN0_", operation_list_END},   // (E, R, D, Q)
    {"$_DLATCH_NN1_", operation_list_END},   // (E, R, D, Q)
    {"$_DLATCH_NP0_", operation_list_END},   // (E, R, D, Q)
    {"$_DLATCH_NP1_", operation_list_END},   // (E, R, D, Q)
    {"$_DLATCH_N_", operation_list_END},     // (E, D, Q)
    {"$_DLATCH_PN0_", operation_list_END},   // (E, R, D, Q)
    {"$_DLATCH_PN1_", operation_list_END},   // (E, R, D, Q)
    {"$_DLATCH_PP0_", operation_list_END},   // (E, R, D, Q)
    {"$_DLATCH_PP1_", operation_list_END},   // (E, R, D, Q)
    {"$_DLATCH_P_", operation_list_END},     // (E, D, Q)
    {"$_FF_", operation_list_END},           // (D, Q)
    {"$_MUX16_", operation_list_END},        // (A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, S, T, U, V, Y)
    {"$_MUX4_", operation_list_END},         // (A, B, C, D, S, T, Y)
    {"$_MUX8_", operation_list_END},         // (A, B, C, D, E, F, G, H, S, T, U, Y)
    {"$_MUX_", operation_list_END},          // (A, B, S, Y)
    {"$_NAND_", operation_list_END},         // (A, B, Y)
    {"$_NMUX_", operation_list_END},         // (A, B, S, Y)
    {"$_NOR_", operation_list_END},          // (A, B, Y)
    {"$_NOT_", operation_list_END},          // (A, Y)
    {"$_OAI3_", operation_list_END},         // (A, B, C, Y)
    {"$_OAI4_", operation_list_END},         // (A, B, C, Y)
    {"$_ORNOT_", operation_list_END},        // (A, B, Y)
    {"$_OR_", operation_list_END},           // (A, B, Y)
    {"$_SDFFCE_NN0N_", operation_list_END},  // (D, C, R, E, Q)
    {"$_SDFFCE_NN0P_", operation_list_END},  // (D, C, R, E, Q)
    {"$_SDFFCE_NN1N_", operation_list_END},  // (D, C, R, E, Q)
    {"$_SDFFCE_NN1P_", operation_list_END},  // (D, C, R, E, Q)
    {"$_SDFFCE_NP0N_", operation_list_END},  // (D, C, R, E, Q)
    {"$_SDFFCE_NP0P_", operation_list_END},  // (D, C, R, E, Q)
    {"$_SDFFCE_NP1N_", operation_list_END},  // (D, C, R, E, Q)
    {"$_SDFFCE_NP1P_", operation_list_END},  // (D, C, R, E, Q)
    {"$_SDFFCE_PN0N_", operation_list_END},  // (D, C, R, E, Q)
    {"$_SDFFCE_PN0P_", operation_list_END},  // (D, C, R, E, Q)
    {"$_SDFFCE_PN1N_", operation_list_END},  // (D, C, R, E, Q)
    {"$_SDFFCE_PN1P_", operation_list_END},  // (D, C, R, E, Q)
    {"$_SDFFCE_PP0N_", operation_list_END},  // (D, C, R, E, Q)
    {"$_SDFFCE_PP0P_", operation_list_END},  // (D, C, R, E, Q)
    {"$_SDFFCE_PP1N_", operation_list_END},  // (D, C, R, E, Q)
    {"$_SDFFCE_PP1P_", operation_list_END},  // (D, C, R, E, Q)
    {"$_SDFFE_NN0N_", operation_list_END},   // (D, C, R, E, Q)
    {"$_SDFFE_NN0P_", operation_list_END},   // (D, C, R, E, Q)
    {"$_SDFFE_NN1N_", operation_list_END},   // (D, C, R, E, Q)
    {"$_SDFFE_NN1P_", operation_list_END},   // (D, C, R, E, Q)
    {"$_SDFFE_NP0N_", operation_list_END},   // (D, C, R, E, Q)
    {"$_SDFFE_NP0P_", operation_list_END},   // (D, C, R, E, Q)
    {"$_SDFFE_NP1N_", operation_list_END},   // (D, C, R, E, Q)
    {"$_SDFFE_NP1P_", operation_list_END},   // (D, C, R, E, Q)
    {"$_SDFFE_PN0N_", operation_list_END},   // (D, C, R, E, Q)
    {"$_SDFFE_PN0P_", operation_list_END},   // (D, C, R, E, Q)
    {"$_SDFFE_PN1N_", operation_list_END},   // (D, C, R, E, Q)
    {"$_SDFFE_PN1P_", operation_list_END},   // (D, C, R, E, Q)
    {"$_SDFFE_PP0N_", operation_list_END},   // (D, C, R, E, Q)
    {"$_SDFFE_PP0P_", operation_list_END},   // (D, C, R, E, Q)
    {"$_SDFFE_PP1N_", operation_list_END},   // (D, C, R, E, Q)
    {"$_SDFFE_PP1P_", operation_list_END},   // (D, C, R, E, Q)
    {"$_SDFF_NN0_", operation_list_END},     // (D, C, R, Q)
    {"$_SDFF_NN1_", operation_list_END},     // (D, C, R, Q)
    {"$_SDFF_NP0_", operation_list_END},     // (D, C, R, Q)
    {"$_SDFF_NP1_", operation_list_END},     // (D, C, R, Q)
    {"$_SDFF_PN0_", operation_list_END},     // (D, C, R, Q)
    {"$_SDFF_PN1_", operation_list_END},     // (D, C, R, Q)
    {"$_SDFF_PP0_", operation_list_END},     // (D, C, R, Q)
    {"$_SDFF_PP1_", operation_list_END},     // (D, C, R, Q)
    {"$_SR_NN_", operation_list_END},        // (S, R, Q)
    {"$_SR_NP_", operation_list_END},        // (S, R, Q)
    {"$_SR_PN_", operation_list_END},        // (S, R, Q)
    {"$_SR_PP_", operation_list_END},        // (S, R, Q)
    {"$_TBUF_", operation_list_END},         // (A, E, Y)
    {"$_XNOR_", operation_list_END},         // (A, B, Y)
    {"$_XOR_", operation_list_END},          // (A, B, Y)
    {"$add", ADD},                           // (A, B, Y)
    {"$adff", operation_list_END},           // (CLK, ARST, D, Q)
    {"$adffe", operation_list_END},          // (CLK, ARST, EN, D, Q)
    {"$adlatch", ADLATCH},                   // (EN, ARST, D, Q)
    {"$allconst", operation_list_END},       // (Y)
    {"$allseq", operation_list_END},         // (Y)
    {"$alu", operation_list_END},            // (A, B, CI, BI, X, Y, CO)
    {"$and", BITWISE_AND},                   // (A, B, Y)
    {"$anyconst", operation_list_END},       // (Y)
    {"$anyseq", operation_list_END},         // (Y)
    {"$assert", operation_list_END},         // (A, EN)
    {"$assume", operation_list_END},         // (A, EN)
    {"$concat", operation_list_END},         // (A, B, Y)
    {"$cover", operation_list_END},          // (A, EN)
    {"$dff", FF_NODE},                       // (CLK, D, Q)
    {"$dffe", DFFE},                         // (CLK, EN, D, Q)
    {"$dffsr", DFFSR},                       // (CLK, SET, CLR, D, Q)
    {"$dffsre", DFFSRE},                     // (CLK, SET, CLR, EN, D, Q)
    {"$div", DIVIDE},                        // (A, B, Y)
    {"$divfloor", operation_list_END},       // (A, B, Y)
    {"$dlatch", DLATCH},                     // (EN, D, Q)
    {"$dlatchsr", operation_list_END},       // (EN, SET, CLR, D, Q)
    {"$eq", LOGICAL_EQUAL},                  // (A, B, Y)
    {"$equiv", operation_list_END},          // (A, B, Y)
    {"$eqx", CASE_EQUAL},                    // (A, B, Y)
    {"$fa", operation_list_END},             // (A, B, C, X, Y)
    {"$fair", operation_list_END},           // (A, EN)
    {"$ff", operation_list_END},             // (D, Q)
    {"$fsm", operation_list_END},            // (CLK, ARST, CTRL_IN, CTRL_OUT)
    {"$ge", GTE},                            // (A, B, Y)
    {"$gt", GT},                             // (A, B, Y)
    {"$initstate", operation_list_END},      // (Y)
    {"$lcu", operation_list_END},            // (P, G, CI, CO)
    {"$le", LTE},                            // (A, B, Y)
    {"$live", operation_list_END},           // (A, EN)
    {"$logic_and", LOGICAL_AND},             // (A, B, Y)
    {"$logic_not", LOGICAL_NOT},             // (A, Y)
    {"$logic_or", LOGICAL_OR},               // (A, B, Y)
    {"$lt", LT},                             // (A, B, Y)
    {"$lut", operation_list_END},            // (A, Y)
    {"$macc", operation_list_END},           // (A, B, Y)
    {"$mem", YMEM},                          // (RD_CLK, RD_EN, RD_ADDR, RD_DATA, WR_CLK, WR_EN, WR_ADDR, WR_DATA)
    {"$meminit", operation_list_END},        // (ADDR, DATA)
    {"$memrd", ROM},                         // (CLK, EN, ADDR, DATA)
    {"$memwr", operation_list_END},          // (CLK, EN, ADDR, DATA)
    {"$mod", MODULO},                        // (A, B, Y)
    {"$modfloor", operation_list_END},       // (A, B, Y)
    {"$mul", MULTIPLY},                      // (A, B, Y)
    {"$mux", MULTIPORT_nBIT_SMUX},           // (A, B, S, Y)
    {"$ne", NOT_EQUAL},                      // (A, B, Y)
    {"$neg", MINUS},                         // (A, Y)
    {"$nex", CASE_NOT_EQUAL},                // (A, B, Y)
    {"$not", BITWISE_NOT},                   // (A, Y)
    {"$or", BITWISE_OR},                     // (A, B, Y)
    {"$pmux", PMUX},                         // (A, B, S, Y)
    {"$pos", operation_list_END},            // (A, Y)
    {"$pow", POWER},                         // (A, B, Y)
    {"$reduce_and", BITWISE_AND},            // (A, Y)
    {"$reduce_bool", BITWISE_OR},            // (A, Y)
    {"$reduce_or", BITWISE_OR},              // (A, Y)
    {"$reduce_xnor", BITWISE_XNOR},          // (A, Y)
    {"$reduce_xor", BITWISE_XOR},            // (A, Y)
    {"$sdff", SDFF},                         // (CLK, SRST, D, Q)
    {"$sdffce", SDFFCE},                     // (CLK, SRST, EN, D, Q)
    {"$sdffe", SDFFE},                       // (CLK, SRST, EN, D, Q)
    {"$shift", operation_list_END},          // (A, B, Y)
    {"$shiftx", operation_list_END},         // (A, B, Y)
    {"$shl", SL},                            // (A, B, Y)
    {"$shr", SR},                            // (A, B, Y)
    {"$slice", operation_list_END},          // (A, Y)
    {"$sop", operation_list_END},            // (A, Y)
    {"$specify2", operation_list_END},       // (EN, SRC, DST)
    {"$specify3", operation_list_END},       // (EN, SRC, DST, DAT)
    {"$specrule", operation_list_END},       // (EN_SRC, EN_DST, SRC, DST)
    {"$sr", SETCLR},                         // (SET, CLR, Q)
    {"$sshl", ASL},                          // (A, B, Y)
    {"$sshr", ASR},                          // (A, B, Y)
    {"$sub", MINUS},                         // (A, B, Y)
    {"$tribuf", operation_list_END},         // (A, EN, Y)
    {"$xnor", LOGICAL_XNOR},                 // (A, B, Y)
    {"$xor", LOGICAL_XOR},                   // (A, B, Y)
    /***************** Odin techlib START **************/
    {"_$ROM", ROM},   // (in, out)
    {"_$BRAM", BRAM}, // (in, out)
    /*********** VTR Primitive modules START ***********/
    {"LUT_K", operation_list_END},             // (in, out)
    {"DFF", FF_NODE},                          // (clock, D, Q)
    {"fpga_interconnect", operation_list_END}, // (datain, dataout)
    {"mux", SMUX_2},                           // (select, x, y, z)
    {"adder", ADD},                            // (a, b, out)
    {"multiply", MULTIPLY},                    // (a, b, cin, cout, sumout)
    {"single_port_ram", SPRAM},                // (clock, addr, data, we, out)
    {"dual_port_ram", DPRAM}                   // (clock, addr1, addr2, data1, data2, we1, we2, out1, out2)
});

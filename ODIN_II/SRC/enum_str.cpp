#include "odin_types.h"

const char *file_extension_supported_STR[] =
{
	".v"
};

const char *signedness_STR[] =
{
	"SIGNED",
	"UNSIGNED"
};

const char *edge_type_e_STR[] =
{
	"UNDEFINED_SENSITIVITY",
	"FALLING_EDGE_SENSITIVITY",
	"RISING_EDGE_SENSITIVITY",
	"ACTIVE_HIGH_SENSITIVITY",
	"ACTIVE_LOW_SENSITIVITY",
	"ASYNCHRONOUS_SENSITIVITY",
};

const char *_ZERO_GND_ZERO[] = 
{
	"ZERO_GND_ZERO","ZGZ"
};

const char *_ONE_VCC_CNS[] = 
{
	"ONE_VCC_CNS","OVC",
};

const char *_ZERO_PAD_ZERO[] = 
{
	"ZERO_PAD_ZERO","ZPZ"
};

const char *_SINGLE_PORT_RAM_string[] = 
{
	"single_port_ram","SPR"
};

const char *_DUAL_PORT_RAM_string[] = 
{
	"dual_port_ram","DPR"
};

const char *ZERO_GND_ZERO = _ZERO_GND_ZERO[ODIN_STRING_TYPE];
const char *ONE_VCC_CNS = _ONE_VCC_CNS[ODIN_STRING_TYPE];
const char *ZERO_PAD_ZERO = _ZERO_PAD_ZERO[ODIN_STRING_TYPE];

const char *SINGLE_PORT_RAM_string = _SINGLE_PORT_RAM_string[ODIN_STRING_TYPE];
const char *DUAL_PORT_RAM_string = _DUAL_PORT_RAM_string[ODIN_STRING_TYPE];

const char *operation_list_STR[][2] = 
{
	{"NO_OP",           "nOP"},
	{"MULTI_PORT_MUX",  "nMUX"}, // port 1 = control, port 2+ = mux options
	{"FF_NODE",         "FF"},
	{"BUF_NODE",        "BUF"},
	{"INPUT_NODE",      "IN"},
	{"OUTPUT_NODE",     "OUT"},
	{"GND_NODE",        "GND"},
	{"VCC_NODE",        "VCC"},
	{"CLOCK_NODE",      "CLK"},
	{"ADD",             "ADD"}, // +
	{"MINUS",           "MIN"}, // -
	{"BITWISE_NOT",     "bNOT"}, // ~
	{"BITWISE_AND",     "bAND"}, // &
	{"BITWISE_OR",      "bOR"}, // |
	{"BITWISE_NAND",    "bNAND"}, // ~&
	{"BITWISE_NOR",     "bNOR"}, // ~|
	{"BITWISE_XNOR",    "bXNOR"}, // ~^
	{"BITWISE_XOR",     "bXOR"}, // ^
	{"LOGICAL_NOT",     "lNOT"}, // !
	{"LOGICAL_OR",      "lOR"}, // ||
	{"LOGICAL_AND",     "lAND"}, // &&
	{"LOGICAL_NAND",    "lNAND"}, // No Symbol
	{"LOGICAL_NOR",     "lNOR"}, // No Symbol
	{"LOGICAL_XNOR",    "lXNOR"}, // No symbol
	{"LOGICAL_XOR",     "lXOR"}, // No Symbol
	{"MULTIPLY",        "MUL"}, // *
	{"DIVIDE",          "DIV"}, // /
	{"MODULO",          "MOD"}, // %
	{"OP_POW",          "POW"}, // **
	{"LT",              "LT"}, // <
	{"GT",              "GT"}, // >
	{"LOGICAL_EQUAL",   "lEQ"}, // ==
	{"NOT_EQUAL",       "lNEQ"}, // !=
	{"LTE",             "LTE"}, // <=
	{"GTE",             "GTE"}, // >=
	{"SR",              "SR"}, // >>
	{"ASR",             "ASR"}, // >>>
	{"SL",              "SL"}, // <<
	{"CASE_EQUAL",      "cEQ"}, // ===
	{"CASE_NOT_EQUAL",  "cNEQ"}, // !==
	{"ADDER_FUNC",      "ADDER"},
	{"CARRY_FUNC",      "CARRY"},
	{"MUX_2",           "MUX_2"},
	{"BLIF_FUNCTION",   "BLIFf"},
	{"NETLIST_FUNCTION","NETf"},
	{"MEMORY",          "MEM"},
	{"PAD_NODE",        "PAD"},
	{"HARD_IP",         "HARD"},
	{"GENERIC",         "GEN"}, /*added for the unknown node type */
	{"FULLADDER",       "FlADD"},
	{"CLOG2",			"CL2"}, // $clog2
	{"ERROR OOB",		"OOB"} // should not reach this
};

const char *ids_STR []= 
{
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
	"INTEGER",
	"GENVAR",
	"PARAMETER",
	"LOCALPARAM",
	"INITIALS",
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
	/* Specify Items */
	"SPECIFY_ITEMS",
	"SPECIFY_PARAMETER",
	"SPECIFY_PAL_CONNECTION_STATEMENT",
	"SPECIFY_PAL_CONNECT_LIST",
	/* statements */
	"BLOCK",
	"NON_BLOCKING_STATEMENT",
	"BLOCKING_STATEMENT",
	"ASSIGNING_LIST",
	"CASE",
	"CASE_LIST",
	"CASE_ITEM",
	"CASE_DEFAULT",
	"ALWAYS",
	"GENERATE",
	"IF",
	"IF_Q",
	"FOR",
	"WHILE",
	/* Delay Control */
	"DELAY_CONTROL",
	"POSEDGE",
	"NEGEDGE",
	/* expressions */
	"BINARY_OPERATION",
	"UNARY_OPERATION",
	/* basic primitives */
	"ARRAY_REF",
	"RANGE_REF",
	"CONCATENATE",
	/* basic identifiers */
	"IDENTIFIERS",
	"NUMBERS",
	/* Hard Blocks */
	"HARD_BLOCK",
	"HARD_BLOCK_NAMED_INSTANCE",
	"HARD_BLOCK_CONNECT_LIST",
	"HARD_BLOCK_CONNECT",
	// EDDIE: new enum value for ids to replace MEMORY from operation_t
	"RAM",
	"ids_END"
};
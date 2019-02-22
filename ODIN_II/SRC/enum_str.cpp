#include "types.h"

#define ODIN_SHORT_STRING

const char *signedness_STR[] =
{
	"SIGNED",
	"UNSIGNED"
};

const char *edge_type_e_STR[] =
{
	"FALLING_EDGE_SENSITIVITY",
	"RISING_EDGE_SENSITIVITY",
	"ACTIVE_HIGH_SENSITIVITY",
	"ACTIVE_LOW_SENSITIVITY",
	"ASYNCHRONOUS_SENSITIVITY",
	"UNDEFINED_SENSITIVITY"
};

#ifndef ODIN_SHORT_STRING

    const char *ZERO_GND_ZERO = "ZERO_GND_ZERO";
    const char *ONE_VCC_CNS = "ONE_VCC_CNS";
    const char *SINGLE_PORT_RAM_string = "single_port_ram";
    const char *DUAL_PORT_RAM_string = "dual_port_ram";

    const char *operation_list_STR[] = 
    {
        "NO_OP",
        "MULTI_PORT_MUX", // port 1 = control, port 2+ = mux options
        "FF_NODE",
        "BUF_NODE",
        "INPUT_NODE",
        "OUTPUT_NODE",
        "GND_NODE",
        "VCC_NODE",
        "CLOCK_NODE",
        "ADD", // +
        "MINUS", // -
        "BITWISE_NOT", // ~
        "BITWISE_AND", // &
        "BITWISE_OR", // |
        "BITWISE_NAND", // ~&
        "BITWISE_NOR", // ~|
        "BITWISE_XNOR", // ~^
        "BITWISE_XOR", // ^
        "LOGICAL_NOT", // !
        "LOGICAL_OR", // ||
        "LOGICAL_AND", // &&
        "LOGICAL_NAND", // No Symbol
        "LOGICAL_NOR", // No Symbol
        "LOGICAL_XNOR", // No symbol
        "LOGICAL_XOR", // No Symbol
        "MULTIPLY", // *
        "DIVIDE", // /
        "MODULO", // %
        "OP_POW", // **
        "LT", // <
        "GT", // >
        "LOGICAL_EQUAL", // ==
        "NOT_EQUAL", // !=
        "LTE", // <=
        "GTE", // >=
        "SR", // >>
        "ASR", // >>>
        "SL", // <<
        "CASE_EQUAL", // ===
        "CASE_NOT_EQUAL", // !==
        "ADDER_FUNC",
        "CARRY_FUNC",
        "MUX_2",
        "BLIF_FUNCTION",
        "NETLIST_FUNCTION",
        "MEMORY",
        "PAD_NODE",
        "HARD_IP",
        "GENERIC", /*added for the unknown node type */
        "FULLADDER"
    };

#else

    const char *ZERO_GND_ZERO = "ZGZ";
    const char *ONE_VCC_CNS = "OVC";
    const char *SINGLE_PORT_RAM_string = "SPR";
    const char *DUAL_PORT_RAM_string = "DPR";

    const char *operation_list_STR[] = 
    {
        "NO_OP",
        "MULTI_PORT_MUX", // port 1 = control, port 2+ = mux options
        "FF_NODE",
        "BUF_NODE",
        "INPUT_NODE",
        "OUTPUT_NODE",
        "GND_NODE",
        "VCC_NODE",
        "CLOCK_NODE",
        "ADD", // +
        "MINUS", // -
        "BITWISE_NOT", // ~
        "BITWISE_AND", // &
        "BITWISE_OR", // |
        "BITWISE_NAND", // ~&
        "BITWISE_NOR", // ~|
        "BITWISE_XNOR", // ~^
        "BITWISE_XOR", // ^
        "LOGICAL_NOT", // !
        "LOGICAL_OR", // ||
        "LOGICAL_AND", // &&
        "LOGICAL_NAND", // No Symbol
        "LOGICAL_NOR", // No Symbol
        "LOGICAL_XNOR", // No symbol
        "LOGICAL_XOR", // No Symbol
        "MULTIPLY", // *
        "DIVIDE", // /
        "MODULO", // %
        "OP_POW", // **
        "LT", // <
        "GT", // >
        "LOGICAL_EQUAL", // ==
        "NOT_EQUAL", // !=
        "LTE", // <=
        "GTE", // >=
        "SR", // >>
        "ASR", // >>>
        "SL", // <<
        "CASE_EQUAL", // ===
        "CASE_NOT_EQUAL", // !==
        "ADDER_FUNC",
        "CARRY_FUNC",
        "MUX_2",
        "BLIF_FUNCTION",
        "NETLIST_FUNCTION",
        "MEMORY",
        "PAD_NODE",
        "HARD_IP",
        "GENERIC", /*added for the unknown node type */
        "FULLADDER"
    };

#endif

const char *ids_STR []= 
{
	"NO_ID",
	/* top level things */
	"FILE_ITEMS",
	"MODULE",
	/* VARIABLES */
	"INPUT",
	"OUTPUT",
	"INOUT",
	"WIRE",
	"REG",
	"INTEGER",
	"PARAMETER",
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
	"RAM"
};
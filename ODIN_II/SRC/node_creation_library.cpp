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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "types.h"
#include "globals.h"
#include "netlist_utils.h"
#include "odin_util.h"
#include "node_creation_library.h"
#include "vtr_util.h"

long unique_node_name_id = 0;

/*-----------------------------------------------------------------------
 * (function: get_a_pad_pin)
 * 	this allows us to attach to the constant netlist driving hb_pad
 *---------------------------------------------------------------------*/
npin_t *get_pad_pin(netlist_t *netlist)
{
	npin_t *pad_fanout_pin = allocate_npin();
	pad_fanout_pin->name = vtr::strdup(pad_string);
	add_fanout_pin_to_net(netlist->pad_net, pad_fanout_pin);
	return pad_fanout_pin;
}

/*-----------------------------------------------------------------------
 * (function: get_a_zero_pin)
 * 	this allows us to attach to the constant netlist driving zero
 *---------------------------------------------------------------------*/
npin_t *get_zero_pin(netlist_t *netlist)
{
	npin_t *zero_fanout_pin = allocate_npin();
	zero_fanout_pin->name = vtr::strdup(zero_string);
	add_fanout_pin_to_net(netlist->zero_net, zero_fanout_pin);
	return zero_fanout_pin;
}

/*---------------------------------------------------------------------------------------------
 * (function: get_a_one_pin)
 * 	this allows us to attach to the constant netlist driving one
 *-------------------------------------------------------------------------------------------*/
npin_t *get_one_pin(netlist_t *netlist)
{
	npin_t *one_fanout_pin = allocate_npin();
	one_fanout_pin->name = vtr::strdup(one_string);
	add_fanout_pin_to_net(netlist->one_net, one_fanout_pin);
	return one_fanout_pin;
}


/*---------------------------------------------------------------------------------------------
 * (function: make_not_gate_with_input)
 * 	Creates a not gate and attaches it to the inputs
 *-------------------------------------------------------------------------------------------*/
nnode_t *make_not_gate_with_input(npin_t *input_pin, nnode_t *node, short mark)
{
	nnode_t *logic_node;

	logic_node = make_not_gate(node, mark);

	/* add the input ports as needed */
	add_input_pin_to_node(logic_node, input_pin, 0);

	return logic_node;

}

/*-------------------------------------------------------------------------
 * (function: make_not_gate)
 * 	Just make a not gate
 *-----------------------------------------------------------------------*/
nnode_t *make_not_gate(nnode_t *node, short mark)
{
	nnode_t *logic_node;

	logic_node = allocate_nnode();
	logic_node->traverse_visited = mark;
	logic_node->type = LOGICAL_NOT;
	logic_node->name = node_name(logic_node, node->name);
	logic_node->related_ast_node = node->related_ast_node;

	allocate_more_input_pins(logic_node, 1);
	allocate_more_output_pins(logic_node, 1);

	return logic_node;

}

/*---------------------------------------------------------------------------------------------
 * (function: make_1port_gate)
 * 	Make a 1 port gate with variable sizes
 *-------------------------------------------------------------------------------------------*/
nnode_t *make_1port_gate(operation_list type, int width_input, int width_output, nnode_t *node, short mark)
{
	nnode_t *logic_node;

	logic_node = allocate_nnode();
	logic_node->traverse_visited = mark;
	logic_node->type = type;
	logic_node->name = node_name(logic_node, node->name);
	logic_node->related_ast_node = node->related_ast_node;

	/* add the input ports as needed */
	allocate_more_input_pins(logic_node, width_input);
	add_input_port_information(logic_node, width_input);
	/* add output */
	allocate_more_output_pins(logic_node, width_output);
	add_output_port_information(logic_node, width_output);

	return logic_node;
}
/*---------------------------------------------------------------------------------------------
 * (function: make_1port_logic_gate)
 * 	Make a gate with variable sized inputs and 1 output
 *-------------------------------------------------------------------------------------------*/
nnode_t *make_1port_logic_gate(operation_list type, int width, nnode_t *node, short mark)
{
	nnode_t *logic_node = make_1port_gate(type, width, 1, node, mark);

	return logic_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: make_1port_logic_gate_with_inputs)
 * 	Make a gate with variable sized inputs, 1 output, and connect to the supplied inputs
 *-------------------------------------------------------------------------------------------*/
nnode_t *make_1port_logic_gate_with_inputs(operation_list type, int width, signal_list_t *pin_list, nnode_t *node, short mark)
{
	nnode_t *logic_node;
	int i;

	logic_node = make_1port_gate(type, width, 1, node, mark);

	/* hookup all the pins */
	for (i = 0; i < width; i++)
	{
		add_input_pin_to_node(logic_node, pin_list->pins[i], i);
	}

	return logic_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: make_3port_logic_gates)
 * 	Make a 3 port gate all variable port widths.
 *-------------------------------------------------------------------------------------------*/
nnode_t *make_3port_gate(operation_list type, int width_port1, int width_port2, int width_port3, int width_output, nnode_t *node, short mark)
{
	nnode_t *logic_node = allocate_nnode();
	logic_node->traverse_visited = mark;
	logic_node->type = type;
	logic_node->name = node_name(logic_node, node->name);
	logic_node->related_ast_node = node->related_ast_node;

	/* add the input ports as needed */
	allocate_more_input_pins(logic_node, width_port1);
	add_input_port_information(logic_node, width_port1);
	allocate_more_input_pins(logic_node, width_port2);
	add_input_port_information(logic_node, width_port2);
	allocate_more_input_pins(logic_node, width_port3);
	add_input_port_information(logic_node, width_port3);
	/* add output */
	allocate_more_output_pins(logic_node, width_output);
	add_output_port_information(logic_node, width_output);

	return logic_node;
}


/*---------------------------------------------------------------------------------------------
 * (function: make_2port_logic_gates)
 * 	Make a 2 port gate with variable sizes.  The first port will be input_pins index 0..width_port1.
 *-------------------------------------------------------------------------------------------*/
nnode_t *make_2port_gate(operation_list type, int width_port1, int width_port2, int width_output, nnode_t *node, short mark)
{
	nnode_t *logic_node = allocate_nnode();
	logic_node->traverse_visited = mark;
	logic_node->type = type;
	logic_node->name = node_name(logic_node, node->name);
	logic_node->related_ast_node = node->related_ast_node;

	/* add the input ports as needed */
	allocate_more_input_pins(logic_node, width_port1);
	add_input_port_information(logic_node, width_port1);
	allocate_more_input_pins(logic_node, width_port2);
	add_input_port_information(logic_node, width_port2);
	/* add output */
	allocate_more_output_pins(logic_node, width_output);
	add_output_port_information(logic_node, width_output);

	return logic_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: make_nport_logic_gates)
 * 	Make a n port gate with variable sizes.  The first port will be input_pins index 0..width_port1.
 *-------------------------------------------------------------------------------------------*/
nnode_t *make_nport_gate(operation_list type, int port_sizes, int width, int width_output, nnode_t *node, short mark)
{
    int i;
	nnode_t *logic_node = allocate_nnode();
	logic_node->traverse_visited = mark;
	logic_node->type = type;
	logic_node->name = node_name(logic_node, node->name);
	logic_node->related_ast_node = node->related_ast_node;

	/* add the input ports as needed */
    for(i = 0; i < port_sizes; i++) {
    	allocate_more_input_pins(logic_node, width);
	    add_input_port_information(logic_node, width);
    }
	//allocate_more_input_pins(logic_node, width_port2);
	//add_input_port_information(logic_node, width_port2);
	/* add output */
	allocate_more_output_pins(logic_node, width_output);
	add_output_port_information(logic_node, width_output);

	return logic_node;
}
/* string conversions */
const char *MULTI_PORT_MUX_string = "MULTI_PORT_MUX";
const char *FF_NODE_string = "FF_NODE";
const char *BUF_NODE_string = "BUF_NODE";
const char *INPUT_NODE_string = "INPUT_NODE";
const char *CLOCK_NODE_string = "CLOCK_NODE";
const char *OUTPUT_NODE_string = "OUTPUT_NODE";
const char *GND_NODE_string = "GND_NODE";
const char *VCC_NODE_string = "VCC_NODE";
const char *ADD_string = "ADD";
const char *MINUS_string = "MINUS";
const char *BITWISE_NOT_string = "BITWISE_NOT";
const char *BITWISE_AND_string = "BITWISE_AND";
const char *BITWISE_OR_string = "BITWISE_OR";
const char *BITWISE_NAND_string = "BITWISE_NAND";
const char *BITWISE_NOR_string = "BITWISE_NOR";
const char *BITWISE_XNOR_string = "BITWISE_XNOR";
const char *BITWISE_XOR_string = "BITWISE_XOR";
const char *LOGICAL_NOT_string = "LOGICAL_NOT";
const char *LOGICAL_OR_string = "LOGICAL_OR";
const char *LOGICAL_AND_string = "LOGICAL_AND";
const char *LOGICAL_NAND_string = "LOGICAL_NAND";
const char *LOGICAL_NOR_string = "LOGICAL_NOR";
const char *LOGICAL_XOR_string = "LOGICAL_XOR";
const char *LOGICAL_XNOR_string = "LOGICAL_XNOR";
const char *MULTIPLY_string = "MULTIPLY";
const char *DIVIDE_string = "DIVIDE";
const char *MODULO_string = "MODULO";
const char *LT_string = "LT";
const char *GT_string = "GT";
const char *LOGICAL_EQUAL_string = "LOGICAL_EQUAL";
const char *NOT_EQUAL_string = "NOT_EQUAL";
const char *LTE_string = "LTE";
const char *GTE_string = "GTE";
const char *SR_string = "SR";
const char *SL_string = "SL";
const char *CASE_EQUAL_string = "CASE_EQUAL";
const char *CASE_NOT_EQUAL_string = "CASE_NOT_EQUAL";
const char *ADDER_FUNC_string = "ADDER_FUNC";
const char *CARRY_FUNC_string = "CARRY_FUNC";
const char *MUX_2_string = "MUX_2";
const char *HARD_IP_string = "HARD_IP";
const char *MEMORY_string = "MEMORY";

/*---------------------------------------------------------------------------------------------
 * (function: node_name_based_on_op)
 * 	Get the string version of a node
 *-------------------------------------------------------------------------------------------*/
const char *node_name_based_on_op(nnode_t *node)
{
	const char *return_string;

	switch(node->type)
	{
		case MULTI_PORT_MUX:
			return_string = MULTI_PORT_MUX_string;
			break;
		case FF_NODE:
			return_string = FF_NODE_string;
			break;
		case BUF_NODE:
			return_string = BUF_NODE_string;
			break;
		case CLOCK_NODE:
			return_string = CLOCK_NODE_string;
			break;
		case INPUT_NODE:
			return_string = INPUT_NODE_string;
			break;
		case OUTPUT_NODE:
			return_string = OUTPUT_NODE_string;
			break;
		case GND_NODE:
			return_string = GND_NODE_string;
			break;
		case VCC_NODE:
			return_string = VCC_NODE_string;
			break;
		case ADD:
			return_string = ADD_string;
			break;
		case MINUS:
			return_string = MINUS_string;
			break;
		case BITWISE_NOT:
			return_string = BITWISE_NOT_string;
			break;
		case BITWISE_AND:
			return_string = BITWISE_AND_string;
			break;
		case BITWISE_OR:
			return_string = BITWISE_OR_string;
			break;
		case BITWISE_NAND:
			return_string = BITWISE_NAND_string;
			break;
		case BITWISE_NOR:
			return_string = BITWISE_NOR_string;
			break;
		case BITWISE_XNOR:
			return_string = BITWISE_XNOR_string;
			break;
		case BITWISE_XOR:
			return_string = BITWISE_XOR_string;
			break;
		case LOGICAL_NOT:
			return_string = LOGICAL_NOT_string;
			break;
		case LOGICAL_OR:
			return_string = LOGICAL_OR_string;
			break;
		case LOGICAL_AND:
			return_string = LOGICAL_AND_string;
			break;
		case LOGICAL_NOR:
			return_string = LOGICAL_NOR_string;
			break;
		case LOGICAL_NAND:
			return_string = LOGICAL_NAND_string;
			break;
		case LOGICAL_XOR:
			return_string = LOGICAL_XOR_string;
			break;
		case LOGICAL_XNOR:
			return_string = LOGICAL_XNOR_string;
			break;
		case MULTIPLY:
			return_string = MULTIPLY_string;
			break;
		case DIVIDE:
			return_string = DIVIDE_string;
			break;
		case MODULO:
			return_string = MODULO_string;
			break;
		case LT:
			return_string = LT_string;
			break;
		case GT:
			return_string = GT_string;
			break;
		case LOGICAL_EQUAL:
			return_string = LOGICAL_EQUAL_string;
			break;
		case NOT_EQUAL:
			return_string = NOT_EQUAL_string;
			break;
		case LTE:
			return_string = LTE_string;
			break;
		case GTE:
			return_string = GTE_string;
			break;
		case SR:
			return_string = SR_string;
			break;
		case SL:
			return_string = SL_string;
			break;
		case CASE_EQUAL:
			return_string = CASE_EQUAL_string;
			break;
		case CASE_NOT_EQUAL:
			return_string = CASE_NOT_EQUAL_string;
			break;
		case ADDER_FUNC:
			return_string = ADDER_FUNC_string;
			break;
		case CARRY_FUNC:
			return_string = CARRY_FUNC_string;
			break;
		case MUX_2:
			return_string = MUX_2_string;
			break;
		case MEMORY:
			return_string = MEMORY_string;
			break;
		case HARD_IP:
			return_string = HARD_IP_string;
			break;
		default:
			return_string = NULL;
			oassert(FALSE);
			break;
	}
	return return_string;
}

const char *edge_type_blif_str(nnode_t *node)
{

	if(node->type != FF_NODE)
		return NULL;

	switch(node->edge_type)
	{
		case FALLING_EDGE_SENSITIVITY:	return "fe";
		case RISING_EDGE_SENSITIVITY:	return "re";
		case ACTIVE_HIGH_SENSITIVITY:	return "ah";
		case ACTIVE_LOW_SENSITIVITY:	return "al";
		case ASYNCHRONOUS_SENSITIVITY:	return "as";
		default:	
			oassert(false &&
				"invalid sensitivity kind for flip flop");
			return NULL;
	}
}

edge_type_e edge_type_blif_enum(std::string edge_kind_str)
{

	if		(edge_kind_str == "fe")	return FALLING_EDGE_SENSITIVITY;
	else if	(edge_kind_str == "re")	return RISING_EDGE_SENSITIVITY;
	else if	(edge_kind_str == "ah")	return ACTIVE_HIGH_SENSITIVITY;
	else if	(edge_kind_str == "al")	return ACTIVE_LOW_SENSITIVITY;
	else if	(edge_kind_str == "as")	return ASYNCHRONOUS_SENSITIVITY;
	else
	{
		oassert(false &&
			"invalid sensitivity kind for flip flop");
		return UNDEFINED_SENSITIVITY;
	}
}

/*----------------------------------------------------------------------------
 * (function: hard_node_name)
 * 	This creates the unique node name for a hard block
 *--------------------------------------------------------------------------*/
char *hard_node_name(nnode_t * /*node*/, char *instance_name_prefix, char *hb_name, char *hb_inst)
{
	char *return_node_name;

	/* create the unique name for this node */
	return_node_name = make_full_ref_name(instance_name_prefix, hb_name, hb_inst, NULL, -1);

	unique_node_name_id ++;

	return return_node_name;
}

/*---------------------------------------------------------------------------------------------
 * (function: node_name)
 * 	This creates the unique node name
 *-------------------------------------------------------------------------------------------*/
char *node_name(nnode_t *node, char *instance_name_prefix)
{
	char *return_node_name;

	/* create the unique name for this node */
	return_node_name = make_full_ref_name(instance_name_prefix, NULL, NULL, node_name_based_on_op(node), unique_node_name_id);

	//oassert(unique_node_name_id != 199803);

	unique_node_name_id++;

	return return_node_name;
}

/*-------------------------------------------------------------------------
 * (function: make_mult_block)
 * 	Just make a multiplier hard block
 *-----------------------------------------------------------------------*/
nnode_t *make_mult_block(nnode_t *node, short mark)
{
	nnode_t *logic_node;

	logic_node = allocate_nnode();
	logic_node->traverse_visited = mark;
	logic_node->type = MULTIPLY;
	logic_node->name = node_name(logic_node, node->name);
	logic_node->related_ast_node = node->related_ast_node;
	logic_node->input_port_sizes = node->input_port_sizes;
	logic_node->num_input_port_sizes = node->num_input_port_sizes;
	logic_node->output_port_sizes = node->output_port_sizes;
	logic_node->num_output_port_sizes = node->num_output_port_sizes;

	allocate_more_input_pins(logic_node, node->num_input_pins);
	allocate_more_output_pins(logic_node, node->num_output_pins);

	return logic_node;
}


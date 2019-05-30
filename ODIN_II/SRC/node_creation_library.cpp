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
#include "odin_types.h"
#include "odin_globals.h"
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
			error_message(NETLIST_ERROR, node->line_number, node->file_number,
				"undefined sensitivity kind for flip flop %s", edge_type_e_STR[node->edge_type]);
				
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
		error_message(NETLIST_ERROR, -1, -1,
			"undefined sensitivity kind for flip flop %s", edge_kind_str.c_str());

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


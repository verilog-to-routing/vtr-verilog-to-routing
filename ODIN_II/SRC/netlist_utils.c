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
#include "errors.h"
#include "netlist_utils.h"
#include "node_creation_library.h"
#include "odin_util.h"


extern global_args_t global_args;
/*---------------------------------------------------------------------------------------------
 * (function: allocate_nnode)
 *-------------------------------------------------------------------------------------------*/
nnode_t* allocate_nnode() {
	nnode_t *new_node;

	new_node = (nnode_t *)my_malloc_struct(sizeof(nnode_t));
	
	new_node->name = NULL;
	new_node->type = NO_OP;
	new_node->bit_width = 0;
	new_node->related_ast_node = NULL;
	new_node->traverse_visited = -1;

	new_node->input_pins = NULL;
	new_node->num_input_pins = 0;
	new_node->output_pins = NULL;
	new_node->num_output_pins = 0;
	
	new_node->input_port_sizes = NULL;
	new_node->num_input_port_sizes = 0;
	new_node->output_port_sizes = NULL;
	new_node->num_output_port_sizes = 0;

	new_node->node_data = NULL;
	new_node->unique_node_data_id = -1;

	new_node->forward_level = -1;
	new_node->backward_level = -1;
	new_node->sequential_level = -1;
	new_node->sequential_terminator = FALSE;

	new_node->internal_netlist = NULL;

	new_node->associated_function = NULL;

	new_node->simulate_block_cycle = NULL;
	new_node->memory_data = NULL;
	
	new_node->bit_map= NULL;
	new_node->bit_map_line_count=0;

	new_node->in_queue = FALSE;

	new_node->undriven_pins = 0;
	new_node->num_undriven_pins = 0;

	new_node->ratio = 1;
	
	new_node->has_initial_value = FALSE;
	new_node->initial_value = 0;
	

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: free_nnode)
 *-------------------------------------------------------------------------------------------*/
void free_nnode(nnode_t *to_free) 
{
	int i;

	if (to_free != NULL)
	{
		/* need to free node_data */
		
		for (i = 0; i < to_free->num_input_pins; i++)
		{
			if (to_free->input_pins[i] != NULL)
			{
				free_npin(to_free->input_pins[i]);
				to_free->input_pins[i] = NULL;
			}
		}	
		if (to_free->input_pins != NULL)
		{
			free(to_free->input_pins);
			to_free->input_pins = NULL;
		}
	
		for (i = 0; i < to_free->num_output_pins; i++)
		{
			if (to_free->output_pins[i] != NULL)
			{
				free_npin(to_free->output_pins[i]);
				to_free->output_pins[i] = NULL;
			}
		}
		if (to_free->output_pins != NULL)
		{
			free(to_free->output_pins);
			to_free->output_pins = NULL;
		}

		if (to_free->input_port_sizes != NULL)
			free(to_free->input_port_sizes);
		if (to_free->output_port_sizes != NULL)
			free(to_free->output_port_sizes);

		if (to_free->undriven_pins)
			free(to_free->undriven_pins);

		/* now free the node */
		free(to_free);
	}	
}

/*-------------------------------------------------------------------------
 * (function: allocate_more_node_input_pins)
 * 	Makes more space in the node for pin connections ... 
 *-----------------------------------------------------------------------*/
void allocate_more_input_pins(nnode_t *node, int width)
{
	int i;

	if (width <= 0) 
	{
		warning_message(NETLIST_ERROR, -1, -1, "tried adding output pins for with <= 0 %s\n", node->name);
		return;
	}

	node->input_pins = (npin_t**)realloc(node->input_pins, sizeof(npin_t*)*(node->num_input_pins+width));
	for (i = 0; i < width; i++)
	{
		node->input_pins[node->num_input_pins+i] = NULL;
	}
	node->num_input_pins += width;
}

/*-------------------------------------------------------------------------
 * (function: allocate_more_node_output_pins)
 * 	Makes more space in the node for pin connections ... 
 *-----------------------------------------------------------------------*/
void allocate_more_output_pins(nnode_t *node, int width)
{
	int i;

	if (width <= 0)
	{
		warning_message(NETLIST_ERROR, -1, -1, "tried adding output pins for with <= 0 %s\n", node->name);
		return;
	}

	node->output_pins = (npin_t**)realloc(node->output_pins, sizeof(npin_t*)*(node->num_output_pins+width));
	for (i = 0; i < width; i++)
	{
		node->output_pins[node->num_output_pins+i] = NULL;
	}
	node->num_output_pins += width;
}

/*---------------------------------------------------------------------------------------------
 * (function: add_output_port_information)
 *-------------------------------------------------------------------------------------------*/
void add_output_port_information(nnode_t *node, int port_width)
{
	node->output_port_sizes = (int *)realloc(node->output_port_sizes, sizeof(int)*(node->num_output_port_sizes+1));
	node->output_port_sizes[node->num_output_port_sizes] = port_width;
	node->num_output_port_sizes++;
}

/*---------------------------------------------------------------------------------------------
 * (function: add_input_port_information)
 *-------------------------------------------------------------------------------------------*/
void add_input_port_information(nnode_t *node, int port_width)
{
	node->input_port_sizes = (int *)realloc(node->input_port_sizes, sizeof(int)*(node->num_input_port_sizes+1));
	node->input_port_sizes[node->num_input_port_sizes] = port_width;
	node->num_input_port_sizes++;
}

/*---------------------------------------------------------------------------------------------
 * (function: allocate_npin)
 *-------------------------------------------------------------------------------------------*/
npin_t* allocate_npin() {
	npin_t *new_pin;

	new_pin = (npin_t *)my_malloc_struct(sizeof(npin_t));
	
	new_pin->name = NULL;
	new_pin->type = NO_ID;
	new_pin->net = NULL;
	new_pin->pin_net_idx = -1;
	new_pin->node = NULL;
	new_pin->pin_node_idx = -1;
	new_pin->mapping = NULL;
	
	new_pin->cycle  = NULL;
	new_pin->values = NULL;

	new_pin->coverage = 0;

	new_pin->is_default = FALSE;
	new_pin->is_implied = FALSE;

	new_pin->ace_info = NULL;

	return new_pin;
}
/*-------------------------------------------------------------------------
 * (function: copy_output_npin)
 * 	Copies an output pin 
 *-----------------------------------------------------------------------*/
npin_t* copy_output_npin(npin_t* copy_pin)
{
	npin_t *new_pin = allocate_npin();
	oassert(copy_pin->type == OUTPUT);

	new_pin->name = copy_pin->name;
	new_pin->type = copy_pin->type;
	new_pin->net = copy_pin->net;
	new_pin->mapping = copy_pin->mapping;
	new_pin->is_default = copy_pin->is_default;
	return new_pin;
}

/*-------------------------------------------------------------------------
 * (function: copy_input_npin)
 * 	Copies an input pin and potentially adds to the net
 *-----------------------------------------------------------------------*/
npin_t* copy_input_npin(npin_t* copy_pin)
{
	npin_t *new_pin = allocate_npin();
	oassert(copy_pin->type == INPUT);

	new_pin->name = copy_pin->name?strdup(copy_pin->name):0;
	new_pin->type = copy_pin->type;
	new_pin->mapping = copy_pin->mapping?strdup(copy_pin->mapping):0;
	new_pin->is_default = copy_pin->is_default;
	if (copy_pin->net != NULL)
	{
		add_fanout_pin_to_net(copy_pin->net, new_pin);
	}

	return new_pin;
}

/*---------------------------------------------------------------------------------------------
 * (function: free_npin)
 * 	doesn't free any one else
 *-------------------------------------------------------------------------------------------*/
void free_npin(npin_t *to_free)
{
	if (to_free) {
		free(to_free);
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: allocate_nnet)
 *-------------------------------------------------------------------------------------------*/
nnet_t* allocate_nnet()
{
	nnet_t *new_net;

	new_net = (nnet_t*)my_malloc_struct(sizeof(nnet_t));
	
	new_net->name = NULL;
	new_net->driver_pin = NULL;
	new_net->fanout_pins = NULL;
	new_net->num_fanout_pins = 0;
	new_net->combined = FALSE;

	new_net->net_data = NULL;
	new_net->unique_net_data_id = -1;
	
	new_net->has_initial_value = FALSE;
	new_net->initial_value = 0;

	return new_net;
}

/*-------------------------------------------------------------------------
 * (function: free_nnet)
 * 	free nnet_t struct
 *-----------------------------------------------------------------------*/
void free_nnet(nnet_t *to_free)
{
	if (to_free != NULL)
	{
		free(to_free);
	}
}

/*---------------------------------------------------------------------------
 * (function: move_a_output_pin)
 *-------------------------------------------------------------------------*/
void move_output_pin(nnode_t *node, int old_idx, int new_idx)
{
	npin_t *pin;

	oassert(node != NULL);
	oassert(((old_idx >= 0) && (old_idx < node->num_output_pins)));
	oassert(((new_idx >= 0) && (new_idx < node->num_output_pins)));
	/* assumes the pin spots have been allocated and the pin */
	pin = node->output_pins[old_idx];
	node->output_pins[new_idx] = node->output_pins[old_idx];
	node->output_pins[old_idx] = NULL;
	/* record the node and pin spot in the pin */
	pin->type = OUTPUT;
	pin->node = node;
	pin->pin_node_idx = new_idx;
}

/*---------------------------------------------------------------------------
 * (function: move_a_input_pin)
 *-------------------------------------------------------------------------*/
void move_input_pin(nnode_t *node, int old_idx, int new_idx)
{
	npin_t *pin;

	oassert(node != NULL);
	oassert(((old_idx >= 0) && (old_idx < node->num_input_pins)));
	oassert(((new_idx >= 0) && (new_idx < node->num_input_pins)));
	/* assumes the pin spots have been allocated and the pin */
	node->input_pins[new_idx] = node->input_pins[old_idx];
	pin = node->input_pins[old_idx];
	node->input_pins[old_idx] = NULL;
	/* record the node and pin spot in the pin */
	pin->type = INPUT;
	pin->node = node;
	pin->pin_node_idx = new_idx;
}

/*---------------------------------------------------------------------------------------------
 * (function: add_a_input_pin_to_node_spot_idx)
 *-------------------------------------------------------------------------------------------*/
void add_input_pin_to_node(nnode_t *node, npin_t *pin, int pin_idx)
{
	oassert(node != NULL);
	oassert(pin != NULL);
	oassert(pin_idx < node->num_input_pins);
	/* assumes the pin spots have been allocated and the pin */
	node->input_pins[pin_idx] = pin;
	/* record the node and pin spot in the pin */
	pin->type = INPUT;
	pin->node = node;
	pin->pin_node_idx = pin_idx;
}

/*---------------------------------------------------------------------------------------------
 * (function: add_a_input_pin_to_spot_idx)
 *-------------------------------------------------------------------------------------------*/
void add_fanout_pin_to_net(nnet_t *net, npin_t *pin)
{
	oassert(net != NULL);
	oassert(pin != NULL);
	oassert(pin->type != OUTPUT);
	/* assumes the pin spots have been allocated and the pin */
	net->fanout_pins = (npin_t**)realloc(net->fanout_pins, sizeof(npin_t*)*(net->num_fanout_pins+1));
	net->fanout_pins[net->num_fanout_pins] = pin;
	net->num_fanout_pins++;
	/* record the node and pin spot in the pin */
	pin->net = net;
	pin->pin_net_idx = net->num_fanout_pins-1;
	pin->type = INPUT;
}

/*---------------------------------------------------------------------------------------------
 * (function: add_a_output_pin_to_node_spot_idx)
 *-------------------------------------------------------------------------------------------*/
void add_output_pin_to_node(nnode_t *node, npin_t *pin, int pin_idx)
{
	oassert(node != NULL);
	oassert(pin != NULL);
	oassert(pin_idx < node->num_output_pins);
	/* assumes the pin spots have been allocated and the pin */
	node->output_pins[pin_idx] = pin;
	/* record the node and pin spot in the pin */
	pin->type = OUTPUT;
	pin->node = node;
	pin->pin_node_idx = pin_idx;
}

/*---------------------------------------------------------------------------------------------
 * (function: add_a_output_pin_to_spot_idx)
 *-------------------------------------------------------------------------------------------*/
void add_driver_pin_to_net(nnet_t *net, npin_t *pin)
{
	oassert(net != NULL);
	oassert(pin != NULL);
	oassert(pin->type != INPUT);
	/* assumes the pin spots have been allocated and the pin */
	net->driver_pin = pin;
	/* record the node and pin spot in the pin */
	pin->net = net;
	pin->type = OUTPUT;
}

/*---------------------------------------------------------------------------------------------
 * (function: combine_nets)
 * 	// output net is a net with a driver
 * 	// input net is a net with all the fanouts
 * 	The lasting one is input, and output disappears
 *-------------------------------------------------------------------------------------------*/
void combine_nets(nnet_t *output_net, nnet_t* input_net, netlist_t *netlist)
{

	/* copy the driver over to the new_net */	
	if (output_net->driver_pin)
	{
		/* IF - there is a pin assigned to this net, then copy it */
		add_driver_pin_to_net(input_net, output_net->driver_pin);
	}
	/* in case there are any fanouts in output net (should only be zero and one nodes */
	join_nets(input_net, output_net);
	/* mark that this is combined */
	input_net->combined = TRUE;
	
	/* Need to keep the initial value data when we combine the nets */
	input_net->has_initial_value = output_net->has_initial_value;
	input_net->initial_value = output_net->initial_value;
	
	/* special cases for global nets */
	if (output_net == netlist->zero_net) 
	{
		netlist->zero_net = input_net;
	}
	else if (output_net == netlist->one_net)
	{
		netlist->one_net = input_net;
	}

	/* free the driver net */	
	free_nnet(output_net);
}

/*---------------------------------------------------------------------------------------------
 * (function: join_nets)
 * 	Copies the fanouts from input net into net
 *-------------------------------------------------------------------------------------------*/
void join_nets(nnet_t *join_to_net, nnet_t* other_net)
{
	if (join_to_net == other_net)
	{
		if ((join_to_net->driver_pin) && (join_to_net->driver_pin->node != NULL) && join_to_net->driver_pin->node->related_ast_node != NULL)
			error_message(NETLIST_ERROR, join_to_net->driver_pin->node->related_ast_node->line_number, join_to_net->driver_pin->node->related_ast_node->file_number, "This is a combinational loop\n");
		else
			error_message(NETLIST_ERROR, -1, -1, "This is a combinational loop\n");
		if ((join_to_net->fanout_pins[0] != NULL ) && (join_to_net->fanout_pins[0]->node != NULL) && join_to_net->fanout_pins[0]->node->related_ast_node != NULL)
			error_message(NETLIST_ERROR, join_to_net->fanout_pins[0]->node->related_ast_node->line_number, join_to_net->fanout_pins[0]->node->related_ast_node->file_number, "This is a combinational loop with more info\n");
		else
			error_message(NETLIST_ERROR, -1, -1, "Same error - This is a combinational loop\n");
	}

	/* copy the driver over to the new_net */
	int i;
	for (i = 0; i < other_net->num_fanout_pins; i++)
	{
		if (other_net->fanout_pins[i] )
		{
			add_fanout_pin_to_net(join_to_net, other_net->fanout_pins[i]);
		}
	}	
}

/*---------------------------------------------------------------------------------------------
 * (function: remap_pin_to_new_net)
 *-------------------------------------------------------------------------------------------*/
void remap_pin_to_new_net(npin_t *pin, nnet_t *new_net)
{
	if (pin->type == INPUT)
	{
		/* clean out the entry in the old net */
		pin->net->fanout_pins[pin->pin_net_idx] = NULL;
		/* do the new addition */
		add_fanout_pin_to_net(new_net, pin);
	}
	else if (pin->type == OUTPUT)
	{
		/* clean out the entry in the old net */
		pin->net->driver_pin = NULL;	
		/* do the new addition */
		add_driver_pin_to_net(new_net, pin);
	}
}

/*-------------------------------------------------------------------------
 * (function: remap_pin_to_new_node)
 *-----------------------------------------------------------------------*/
void remap_pin_to_new_node(npin_t *pin, nnode_t *new_node, int pin_idx)
{
	if (pin->type == INPUT) {
		/* clean out the entry in the old net */
		pin->node->input_pins[pin->pin_node_idx] = NULL;
		/* do the new addition */
		add_input_pin_to_node(new_node, pin, pin_idx);
	}
	else if (pin->type == OUTPUT) {
		/* clean out the entry in the old net */
		pin->node->output_pins[pin->pin_node_idx] = NULL;	
		/* do the new addition */
		add_output_pin_to_node(new_node, pin, pin_idx);
	}
}

/*------------------------------------------------------------------------
 * (function: connect_nodes)
 * 	Connect one output node to the inputs of the input node
 *----------------------------------------------------------------------*/
void connect_nodes(nnode_t *out_node, int out_idx, nnode_t *in_node, int in_idx)
{
	npin_t *new_in_pin;

	oassert(out_node->num_output_pins > out_idx);
	oassert(in_node->num_input_pins > in_idx);

	new_in_pin = allocate_npin();
	
	/* create the pin that hooks up to the input */
	add_input_pin_to_node(in_node, new_in_pin, in_idx);
	
	if (out_node->output_pins[out_idx] == NULL)
	{
		/* IF - this node has no output net or pin */
		npin_t *new_out_pin;
		nnet_t *new_net;
		new_net = allocate_nnet();
		new_out_pin = allocate_npin();
	
		/* create the pin that hooks up to the input */
		add_output_pin_to_node(out_node, new_out_pin, out_idx);
		/* hook up in pin out of the new net */
		add_fanout_pin_to_net(new_net, new_in_pin);
		/* hook up the new pin 2 to this new net */
		add_driver_pin_to_net(new_net, new_out_pin);
	}
	else
	{
		/* ELSE - there is a net so we just add a fanout */
		/* hook up in pin out of the new net */
		add_fanout_pin_to_net(out_node->output_pins[out_idx]->net, new_in_pin);
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: init_signal_list_structure)
 * 	Initializes the list structure which describes inputs and outputs of elements
 * 	as they coneect to other elements in the graph.
 *-------------------------------------------------------------------------------------------*/
signal_list_t *init_signal_list()
{
	signal_list_t *list;
	list = (signal_list_t*)malloc(sizeof(signal_list_t));

	list->count = 0;
	list->pins = NULL;
	list->is_memory = FALSE;
	list->is_adder = FALSE;

	return list;
}

/*---------------------------------------------------------------------------------------------
 * (function: add_inpin_to_signal_list)
 * 	Stores a pin in the signal list
 *-------------------------------------------------------------------------------------------*/
void add_pin_to_signal_list(signal_list_t *list, npin_t* pin)
{
	list->pins = (npin_t**)realloc(list->pins, sizeof(npin_t*)*(list->count+1));
	list->pins[list->count] = pin;
	list->count++;
}

/*---------------------------------------------------------------------------------------------
 * (function: combine_lists)
 *-------------------------------------------------------------------------------------------*/
signal_list_t *combine_lists(signal_list_t **signal_lists, int num_signal_lists)
{
	int i;
	for (i = 1; i < num_signal_lists; i++)
	{
		int j;
		for (j = 0; j < signal_lists[i]->count; j++)
			add_pin_to_signal_list(signal_lists[0], signal_lists[i]->pins[j]);

		free_signal_list(signal_lists[i]);
	}

	return signal_lists[0];
}

/*---------------------------------------------------------------------------------------------
 * (function: combine_lists_without_freeing_originals)
 *-------------------------------------------------------------------------------------------*/
signal_list_t *combine_lists_without_freeing_originals(signal_list_t **signal_lists, int num_signal_lists)
{
	signal_list_t *return_list = init_signal_list();

	int i;
	for (i = 0; i < num_signal_lists; i++)
	{
		int j;
		if (signal_lists[i])
			for (j = 0; j < signal_lists[i]->count; j++)
				add_pin_to_signal_list(return_list, signal_lists[i]->pins[j]);
	}

	return return_list;
}



signal_list_t *copy_input_signals(signal_list_t *signalsvar)
{
	signal_list_t *duplicate_signals = init_signal_list();
	int i;
	for (i = 0; i < signalsvar->count; i++)
	{
		npin_t *pin = signalsvar->pins[i];
		pin = copy_input_npin(pin);
		add_pin_to_signal_list(duplicate_signals, pin);
	}
	return duplicate_signals;
}


signal_list_t *copy_output_signals(signal_list_t *signalsvar)
{
	signal_list_t *duplicate_signals = init_signal_list();
	int i;
	for (i = 0; i < signalsvar->count; i++)
	{
		npin_t *pin = signalsvar->pins[i];
		add_pin_to_signal_list(duplicate_signals, copy_output_npin(pin));
	}
	return duplicate_signals;
}

/*---------------------------------------------------------------------------------------------
 * (function: sort_signal_list_alphabetically)
 * Bubble sort alphabetically
 * Andrew: changed to a quick sort because this function
 *         was consuming 99.6% of compile time for mcml.v
 *-------------------------------------------------------------------------------------------*/
static int compare_npin_t_names(const void *p1, const void *p2)
{
	npin_t *pin1 = *(npin_t * const *)p1;
	npin_t *pin2 = *(npin_t * const *)p2;
	return strcmp(pin1->name, pin2->name);
}

void sort_signal_list_alphabetically(signal_list_t *list)
{
	if(list)
		qsort(list->pins, list->count,  sizeof(npin_t *), compare_npin_t_names);
}

/*---------------------------------------------------------------------------------------------
 * (function: make_output_pins_for_existing_node)
 * 	Looks at a node and extracts the output pins into a signal list so they can be accessed
 * 	in this form
 *-------------------------------------------------------------------------------------------*/
signal_list_t *make_output_pins_for_existing_node(nnode_t* node, int width)
{
	signal_list_t *return_list = init_signal_list();
	int i; 

	oassert(node->num_output_pins == width);

	for (i = 0; i < width; i++)
	{
		npin_t *new_pin1;
		npin_t *new_pin2;
		nnet_t *new_net;
		new_pin1 = allocate_npin();
		new_pin2 = allocate_npin();
		new_net = allocate_nnet();
		new_net->name = node->name;
		/* hook the output pin into the node */
		add_output_pin_to_node(node, new_pin1, i);
		/* hook up new pin 1 into the new net */
		add_driver_pin_to_net(new_net, new_pin1);
		/* hook up the new pin 2 to this new net */
		add_fanout_pin_to_net(new_net, new_pin2);
		
		/* add the new_pin2 to the list of pins */
		add_pin_to_signal_list(return_list, new_pin2);
	}

	return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function: clean_signal_list_structure)
 *-------------------------------------------------------------------------------------------*/
void free_signal_list(signal_list_t *list)
{
	if (list == NULL)
		return;

	if (list->pins != NULL)
		free(list->pins);

	list->count = 0;

	free(list);
}

/*---------------------------------------------------------------------------
 * (function: hookup_input_pins_from_signal_list)
 * 	For each pin in this list hook it up to the inputs according to indexes and width
 *--------------------------------------------------------------------------*/
void hookup_input_pins_from_signal_list(nnode_t *node, int n_start_idx, signal_list_t* input_list, int il_start_idx, int width, netlist_t *netlist) 
{
	int i;

	for (i = 0; i < width; i++)
	{
		if (il_start_idx+i < input_list->count)
		{
			oassert(input_list->count > (il_start_idx+i));
			npin_t *pin = input_list->pins[il_start_idx+i];
			add_input_pin_to_node(node, pin, n_start_idx+i);

		}
		else
		{
			/* pad with 0's */
			add_input_pin_to_node(node, get_zero_pin(netlist), n_start_idx+i);

			if (global_args.all_warnings)
				warning_message(NETLIST_ERROR, -1, -1, "padding an input port with 0 for node %s\n", node->name);
		}
	}	
}

/*---------------------------------------------------------------------------
 * (function: hookup_hb_input_pins_from_signal_list)
 *   For each pin in this list hook it up to the inputs according to 
 *   indexes and width. Extra pins are tied to PAD for later resolution.
 *--------------------------------------------------------------------------*/
void hookup_hb_input_pins_from_signal_list(nnode_t *node, int n_start_idx, signal_list_t* input_list, int il_start_idx, int width, netlist_t *netlist) 
{
	int i;

	for (i = 0; i < width; i++)
	{
		if (il_start_idx+i < input_list->count)
		{
			oassert(input_list->count > (il_start_idx+i));
			add_input_pin_to_node(node, input_list->pins[il_start_idx+i], n_start_idx+i);
		}
		else
		{
			/* connect with "pad" signal for later resolution */
			add_input_pin_to_node(node, get_pad_pin(netlist), n_start_idx+i);

			if (global_args.all_warnings)
				warning_message(NETLIST_ERROR, -1, -1, "padding an input port with HB_PAD for node %s\n", node->name);
		}
	}	
}

/*---------------------------------------------------------------------------------------------
 * (function: hookup_output_pouts_from_signal_list)
 * 	hooks the pin into the output net, by checking for a driving net
 *-------------------------------------------------------------------------------------------*/
void hookup_output_pins_from_signal_list(nnode_t *node, int n_start_idx, signal_list_t* output_list, int ol_start_idx, int width) 
{
	int i;
	long sc_spot_output;

	for (i = 0; i < width; i++)
	{
		oassert(output_list->count > (ol_start_idx+i));
	
		/* hook outpin to the node */
		add_output_pin_to_node(node, output_list->pins[ol_start_idx+i], n_start_idx+i);

		if ((sc_spot_output = sc_lookup_string(output_nets_sc, output_list->pins[ol_start_idx+i]->name)) == -1)
		{
			/* this output pin does not have a net OR we couldn't find it */
			error_message(NETLIST_ERROR, -1, -1, "Net for driver (%s) doesn't exist for node %s\n", output_list->pins[ol_start_idx+i]->name, node->name);
		}
	
		/* hook the outpin into the net */
		add_driver_pin_to_net(((nnet_t*)output_nets_sc->data[sc_spot_output]), output_list->pins[ol_start_idx+i]);


	}	
}

void depth_traverse_count(nnode_t *node, int *count, int traverse_mark_number);
/*---------------------------------------------------------------------------------------------
 * (function: count_nodes_in_netlist)
 *-------------------------------------------------------------------------------------------*/
int count_nodes_in_netlist(netlist_t *netlist)
{
	int i;
	int count = 0;

	/* now traverse the ground and vcc pins */
	depth_traverse_count(netlist->gnd_node, &count, COUNT_NODES);
	depth_traverse_count(netlist->vcc_node, &count, COUNT_NODES);
	depth_traverse_count(netlist->pad_node, &count, COUNT_NODES);

	/* start with the primary input list */
	for (i = 0; i < netlist->num_top_input_nodes; i++) {
		if (netlist->top_input_nodes[i] != NULL) {
			depth_traverse_count(netlist->top_input_nodes[i], &count,  COUNT_NODES);
		}
	}

	return count;
}

/*---------------------------------------------------------------------------------------------
 * (function: depth_first_traverse)
 *-------------------------------------------------------------------------------------------*/
void depth_traverse_count(nnode_t *node, int *count, int traverse_mark_number)
{
	int i, j;
	nnode_t *next_node;
	nnet_t *next_net;

	if (node->traverse_visited == traverse_mark_number)
	{
		return;
	}
	else
	{
		/* ELSE - this is a new node so depth visit it */
		(*count) ++;

		/* mark that we have visitied this node now */
		node->traverse_visited = traverse_mark_number;

		for (i = 0; i < node->num_output_pins; i++)
		{
			if (node->output_pins[i]->net == NULL)
				continue;

			next_net = node->output_pins[i]->net;
			for (j = 0; j < next_net->num_fanout_pins; j++)
			{
				if (next_net->fanout_pins[j] == NULL)
					continue;
				next_node = next_net->fanout_pins[j]->node;
				if (next_node == NULL)
					continue;

				/* recursive call point */
				depth_traverse_count(next_node, count, traverse_mark_number);
			}
		}
	}
}

/*---------------------------------------------------------------------------------------------
 * (function:  allocate_netlist)
 *-------------------------------------------------------------------------------------------*/
netlist_t* allocate_netlist()
{
	netlist_t *new_netlist;

	new_netlist = (netlist_t *)my_malloc_struct(sizeof(netlist_t));

	new_netlist->gnd_node = NULL;
	new_netlist->vcc_node = NULL;
	new_netlist->pad_node = NULL;
	new_netlist->zero_net = NULL;
	new_netlist->one_net = NULL;
	new_netlist->pad_net = NULL;
	new_netlist->top_input_nodes = NULL;
	new_netlist->num_top_input_nodes = 0;
	new_netlist->top_output_nodes = NULL;
	new_netlist->num_top_output_nodes = 0;
	new_netlist->ff_nodes = NULL;
	new_netlist->num_ff_nodes = 0;
	new_netlist->internal_nodes = NULL;
	new_netlist->num_internal_nodes = 0;
	new_netlist->clocks = NULL;
	new_netlist->num_clocks = 0;

	new_netlist->forward_levels = NULL;
	new_netlist->num_forward_levels = 0;
	new_netlist->num_at_forward_level = NULL;
	new_netlist->backward_levels = NULL; 
	new_netlist->num_backward_levels = 0;
	new_netlist->num_at_backward_level = NULL;
	new_netlist->sequential_level_nodes = NULL; 
	new_netlist->num_sequential_levels = 0;
	new_netlist->num_at_sequential_level = NULL;
	new_netlist->sequential_level_combinational_termination_node = NULL;
	new_netlist->num_sequential_level_combinational_termination_nodes = 0;
	new_netlist->num_at_sequential_level_combinational_termination_node = NULL;

	/* initialize the string chaches */
	new_netlist->nets_sc = sc_new_string_cache();
	new_netlist->out_pins_sc = sc_new_string_cache();
	new_netlist->nodes_sc = sc_new_string_cache();

	new_netlist->stats = NULL;

	return new_netlist;
}

/*---------------------------------------------------------------------------------------------
 * (function:  free_netlist)
 *-------------------------------------------------------------------------------------------*/
void free_netlist(netlist_t *to_free)
{
	sc_free_string_cache(to_free->nets_sc);
	sc_free_string_cache(to_free->out_pins_sc);
	sc_free_string_cache(to_free->nodes_sc);
}

/*---------------------------------------------------------------------------------------------
 * (function:  add_node_to_netlist)
 *-------------------------------------------------------------------------------------------*/
void add_node_to_netlist(netlist_t *netlist, nnode_t *node, short special_node)
{
	long sc_spot;

//	if (node->type != OUTPUT_NODE)
	{
		/* add the node to the list */
		sc_spot = sc_add_string(netlist->nodes_sc, node->name);
		if (netlist->nodes_sc->data[sc_spot] != NULL) {
			error_message(NETLIST_ERROR, file_line_number, -1, "Two nodes with the same name (%s)\n", node->name);
		}
		netlist->nodes_sc->data[sc_spot] = (void*)node;
	}
		
	if (special_node == INPUT_NODE)
	{
		/* This is for clocks, gnd, and vcc */
		/* store the input nodes for traversal */
		netlist->top_input_nodes = (nnode_t**)realloc(netlist->top_input_nodes, sizeof(nnode_t*)*(netlist->num_top_input_nodes+1));
		netlist->top_input_nodes[netlist->num_top_input_nodes] = node;
		netlist->num_top_input_nodes++;
	}
	else if (node->type == INPUT_NODE)
	{
		/* store the input nodes for traversal */
		netlist->top_input_nodes = (nnode_t**)realloc(netlist->top_input_nodes, sizeof(nnode_t*)*(netlist->num_top_input_nodes+1));
		netlist->top_input_nodes[netlist->num_top_input_nodes] = node;
		netlist->num_top_input_nodes++;
	}
	else if (node->type == OUTPUT_NODE)
	{
		netlist->top_output_nodes = (nnode_t**)realloc(netlist->top_output_nodes, sizeof(nnode_t*)*(netlist->num_top_output_nodes+1));
		netlist->top_output_nodes[netlist->num_top_output_nodes] = node;
		netlist->num_top_output_nodes++;
	}
	else if (node->type == FF_NODE)
	{
		netlist->ff_nodes = (nnode_t**)realloc(netlist->ff_nodes, sizeof(nnode_t*)*(netlist->num_ff_nodes+1));
		netlist->ff_nodes[netlist->num_ff_nodes] = node;
		netlist->num_ff_nodes++;
	}
	else 
	{
		netlist->internal_nodes = (nnode_t**)realloc(netlist->internal_nodes, sizeof(nnode_t*)*(netlist->num_internal_nodes+1));
		netlist->internal_nodes[netlist->num_internal_nodes] = node;
		netlist->num_internal_nodes++;
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: mark_clock_node )
 *-------------------------------------------------------------------------------------------*/
void
mark_clock_node (
	netlist_t *netlist,
	char *clock_name
)
{
	long sc_spot;
	nnet_t *clock_net;
	nnode_t *clock_node;

	/* lookup the node */
	if ((sc_spot = sc_lookup_string(netlist->nets_sc, clock_name)) == -1)
	{
		error_message(NETLIST_ERROR, file_line_number, -1, "clock input does not exist (%s)\n", clock_name);
	}
	clock_net = (nnet_t*)netlist->nets_sc->data[sc_spot];
	clock_node = clock_net->driver_pin->node;

	netlist->clocks = (nnode_t**)realloc(netlist->clocks, sizeof(nnode_t*)*(netlist->num_clocks+1));
	netlist->clocks[netlist->num_clocks] = clock_node;
	netlist->num_clocks++;

	/* Mark it as a special set of inputs */
	clock_node->type = CLOCK_NODE;
}

/*
 * Gets the index of the first output pin with the given mapping
 * on the given node.
 */
int get_output_pin_index_from_mapping(nnode_t *node, const char *name)
{
	int i;
	for (i = 0; i < node->num_output_pins; i++)
	{
		npin_t *pin = node->output_pins[i];
		if (!strcmp(pin->mapping, name))
			return i;
	}

	return -1;
}

/*
 * Gets the index of the first output port containing a pin with the given
 * mapping.
 */
int get_output_port_index_from_mapping(nnode_t *node, const char *name)
{
	int i;
	int pin_number = 0;
	for (i = 0; i < node->num_output_port_sizes; i++)
	{
		int j;
		for (j = 0; j < node->output_port_sizes[i]; j++, pin_number++)
		{
			npin_t *pin = node->output_pins[pin_number];
			if (!strcmp(pin->mapping, name))
				return i;
		}
	}
	return -1;
}

/*
 * Gets the index of the first pin with the given mapping.
 */
int get_input_pin_index_from_mapping(nnode_t *node, const char *name)
{
	int i;
	for (i = 0; i < node->num_input_pins; i++)
	{
		npin_t *pin = node->input_pins[i];
		if (!strcmp(pin->mapping, name))
			return i;
	}

	return -1;
}

/*
 * Gets the port index of the first port containing a pin with
 * the given mapping.
 */
int get_input_port_index_from_mapping(nnode_t *node, const char *name)
{
	int i;
	int pin_number = 0;
	for (i = 0; i < node->num_input_port_sizes; i++)
	{
		int j;
		for (j = 0; j < node->input_port_sizes[i]; j++, pin_number++)
		{
			npin_t *pin = node->input_pins[pin_number];
			if (!strcmp(pin->mapping, name))
				return i;
		}
	}
	return -1;
}

chain_information_t* allocate_chain_info()
{
	chain_information_t *new_node;

	new_node = (chain_information_t *)my_malloc_struct(sizeof(chain_information_t));

	new_node->name = NULL;
	new_node->count = 0;

	return new_node;
}

void remove_fanout_pins_from_net(nnet_t *net, npin_t *pin, int id)
{

	int i;
	for (i = id; i < net->num_fanout_pins - 1; i++)
	{
		net->fanout_pins[i] = net->fanout_pins[i+1];
		if(net->fanout_pins[i] != NULL)
			net->fanout_pins[i]->pin_net_idx = i;
	}
	net->fanout_pins[i] = NULL;
	net->num_fanout_pins--;
}

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
#include "globals.h"
#include "errors.h"
#include "netlist_utils.h"
#include "node_creation_library.h"
#include "hard_blocks.h"
#include "memories.h"

t_model *single_port_rams;
t_model *dual_port_rams;

struct s_linked_vptr *sp_memory_list;
struct s_linked_vptr *dp_memory_list;
struct s_linked_vptr *split_list;
struct s_linked_vptr *memory_instances = NULL;
struct s_linked_vptr *memory_port_size_list = NULL;
int split_size = 0;


void pad_dp_memory_width(nnode_t *node, netlist_t *netlist);
void pad_sp_memory_width(nnode_t *node, netlist_t *netlist);
void pad_memory_output_port(nnode_t *node, netlist_t *netlist, t_model *model, char *port_name);
void pad_memory_input_port(nnode_t *node, netlist_t *netlist, t_model *model, char *port_name);

int
get_memory_port_size(char *name)
{
	struct s_linked_vptr *mpl;

	mpl = memory_port_size_list;
	while (mpl != NULL)
	{
		if (strcmp(((t_memory_port_sizes *)mpl->data_vptr)->name, name) == 0)
			return ((t_memory_port_sizes *)mpl->data_vptr)->size;
		mpl = mpl->next;
	}
	return -1;
}


void 
report_memory_distribution()
{
	nnode_t *node;
	struct s_linked_vptr *temp;
	int i, idx;
	int width = 0;
	int depth = 0;
	int total_memory_bits = 0;

	if ((sp_memory_list == NULL) && (dp_memory_list == NULL))
		return;
	printf("\nHard Logical Memory Distribution\n");
	printf("============================\n");

	temp = sp_memory_list;
	
	int total_memory_block_counter = 0;
	int memory_max_width = 0;
	int memory_max_depth = 0;
	
	while (temp != NULL)
	{
		node = (nnode_t *)temp->data_vptr;
		oassert(node != NULL);
		oassert(node->type == MEMORY);

		/* Need to find the addr and data1 ports */
		idx = 0;
		for (i = 0; i < node->num_input_port_sizes; i++)
		{
			if (strcmp("addr", node->input_pins[idx]->mapping) == 0)
			{
				depth = node->input_port_sizes[i];
			}
			else if (strcmp("data", node->input_pins[idx]->mapping) == 0)
			{
				width = node->input_port_sizes[i];
			}
			idx += node->input_port_sizes[i];
		}

		printf("SPRAM: %d width %d depth\n", width, depth);
		total_memory_bits += width * (1 << depth);
		
		total_memory_block_counter++;
		if (width > memory_max_width) {
			memory_max_width = width;
		}
		if (depth > memory_max_depth) {
			memory_max_depth = depth;
		}
		
		temp = temp->next;
	}

	temp = dp_memory_list;
	while (temp != NULL)
	{
		node = (nnode_t *)temp->data_vptr;
		oassert(node != NULL);
		oassert(node->type == MEMORY);

		/* Need to find the addr and data1 ports */
		idx = 0;
		for (i = 0; i < node->num_input_port_sizes; i++)
		{
			if (strcmp("addr", node->input_pins[idx]->mapping) == 0)
			{
				depth = node->input_port_sizes[i];
			} else if (strcmp("addr1", node->input_pins[idx]->mapping) == 0)
			{
				depth = node->input_port_sizes[i];
			}
			else if (strcmp("data1", node->input_pins[idx]->mapping) == 0)
			{
				width = node->input_port_sizes[i];
			}
			idx += node->input_port_sizes[i];
		}

		printf("DPRAM: %d width %d depth\n", width, depth);
		total_memory_bits += width * (1 << depth);
		
		total_memory_block_counter++;
		if (width > memory_max_width) {
			memory_max_width = width;
		}
		if (depth > memory_max_depth) {
			memory_max_depth = depth;
		}
		
		temp = temp->next;
	}
	
	printf("\nTotal Logical Memory Blocks = %d \n", total_memory_block_counter);
	printf("Total Logical Memory bits = %d \n", total_memory_bits);
	printf("Max Memory Width = %d \n", memory_max_width);
	printf("Max Memory Depth = %d \n", memory_max_depth);
	
	printf("\n");
	return;
}

/*-------------------------------------------------------------------------
 * (function: split_sp_memory_depth)
 *
 * This function works to split the depth of a single port memory into 
 *   several smaller memories.
 *------------------------------------------------------------------------
 */
void
split_sp_memory_depth(nnode_t *node)
{
	int data_port = -1;
	int clk_port  = -1;
	int addr_port = -1;
	int we_port = -1;
	int logical_size;
	int i, j;
	int idx;
	int addr_pin_idx = 0;
	int we_pin_idx = 0;
	nnode_t *new_mem_node;
	nnode_t *and_node, *not_node,  *mux_node, *ff_node;
	npin_t *addr_pin = NULL;
	npin_t *we_pin = NULL;
	npin_t *clk_pin = NULL;
	npin_t *tdout_pin;


	oassert(node->type == MEMORY);


	// Find which port is the addr port
	idx = 0;
	for (i = 0; i < node->num_input_port_sizes; i++)
	{
		//printf("%s\n", node->input_pins[idx]->mapping);
		if (strcmp("addr", node->input_pins[idx]->mapping) == 0)
		{
			addr_port = i;
			addr_pin_idx = idx;
			addr_pin = node->input_pins[idx];
		}
		else if (strcmp("data", node->input_pins[idx]->mapping) == 0)
		{
			data_port = i;
		}
		else if (strcmp("we", node->input_pins[idx]->mapping) == 0)
		{
			we_port = i;
			we_pin = node->input_pins[idx];
			we_pin_idx = idx;
		}
		else if (strcmp("clk", node->input_pins[idx]->mapping) == 0)
		{
			clk_port = i;
			clk_pin = node->input_pins[idx];
		}
		idx += node->input_port_sizes[i];
	}
	if (data_port == -1)
	{
		error_message(1, 0, -1, "No \"data\" port on single port RAM");
	}
	if (addr_port == -1)
	{
		error_message(1, 0, -1, "No \"addr\" port on single port RAM");
	}
	if (we_port == -1)
	{
		error_message(1, 0, -1, "No \"we\" port on single port RAM");
	}
	if (clk_port == -1)
	{
		error_message(1, 0, -1, "No \"clk\" port on single port RAM");
	}
	
	// Check that the memory needs to be split
	// Jason Luu HACK: Logical memory depth determination messed up, forced to use this method
	for(i = 0; i < node->input_port_sizes[addr_port]; i++)
	{
		if(strcmp(node->input_pins[addr_pin_idx + i]->name, "top^ZERO_PAD_ZERO") == 0)
			break;
	}
	logical_size = i;
	if (split_size <= 0)
	{
		printf("Unsupported feature! Split size must be a positive number\n");
		exit(1);
	}
	if ((split_size > 0) && (logical_size <= split_size)) {
		sp_memory_list = insert_in_vptr_list(sp_memory_list, node);
		return;
	}

	// Let's remove the address line from the memory
	for (i = addr_pin_idx; i < node->num_input_pins - 1; i++)
	{
		node->input_pins[i] = node->input_pins[i+1];
		node->input_pins[i]->pin_node_idx--;
	}
	node->input_port_sizes[addr_port]--;
	node->input_pins = realloc(node->input_pins, sizeof(npin_t *) * --node->num_input_pins);

	if (we_pin_idx >= addr_pin_idx)
		we_pin_idx--;

	// Create the new memory node
	new_mem_node = allocate_nnode();
	// Append the new name with an __H
	new_mem_node->name = append_string(node->name, "__H");

	{	// Append the old name with an __S
		char *new_name = append_string(node->name, "__S");
		free(node->name);
		node->name = new_name;
	}

	// Copy properties from the original memory node
	new_mem_node->type = node->type;
	new_mem_node->related_ast_node = node->related_ast_node;
	new_mem_node->traverse_visited = node->traverse_visited;

	add_output_port_information(new_mem_node, node->num_output_pins);
	allocate_more_node_output_pins (new_mem_node, node->num_output_pins);

	for (j = 0; j < node->num_input_port_sizes; j++)
		add_input_port_information(new_mem_node, node->input_port_sizes[j]);

	// Copy over the input pins for the new memory, excluding we
	allocate_more_node_input_pins (new_mem_node, node->num_input_pins);
	for (j = 0; j < node->num_input_pins; j++)
	{
		if (j != we_pin_idx)
			add_a_input_pin_to_node_spot_idx(new_mem_node, copy_input_npin(node->input_pins[j]), j);
	}

	and_node = make_2port_gate(LOGICAL_AND, 1, 1, 1, node, node->traverse_visited);
	add_a_input_pin_to_node_spot_idx(and_node, we_pin, 1);
	add_a_input_pin_to_node_spot_idx(and_node, addr_pin, 0);
	connect_nodes(and_node, 0, node, we_pin_idx);
	node->input_pins[we_pin_idx]->mapping = we_pin->mapping;

	not_node = make_not_gate_with_input(copy_input_npin(addr_pin), new_mem_node, new_mem_node->traverse_visited);
	and_node = make_2port_gate(LOGICAL_AND, 1, 1, 1, new_mem_node, new_mem_node->traverse_visited);
	connect_nodes(not_node, 0, and_node, 0);
	add_a_input_pin_to_node_spot_idx(and_node, copy_input_npin(we_pin), 1);
	connect_nodes(and_node, 0, new_mem_node, we_pin_idx);
	new_mem_node->input_pins[we_pin_idx]->mapping = we_pin->mapping;

	ff_node = make_2port_gate(FF_NODE, 1, 1, 1, node, node->traverse_visited);
	add_a_input_pin_to_node_spot_idx(ff_node, copy_input_npin(addr_pin), 0);
	add_a_input_pin_to_node_spot_idx(ff_node, copy_input_npin(clk_pin), 1);

	// Copy over the output pins for the new memory
	for (j = 0; j < node->num_output_pins; j++)
	{
		mux_node = make_2port_gate(MUX_2, 2, 2, 1, node, node->traverse_visited);
		connect_nodes(ff_node, 0, mux_node, 0);

		not_node = make_not_gate(node, node->traverse_visited);
		connect_nodes(ff_node, 0, not_node, 0);
		connect_nodes(not_node, 0, mux_node, 1);

		tdout_pin = node->output_pins[j];
		remap_pin_to_new_node(tdout_pin, mux_node, 0);

		connect_nodes(node, j, mux_node, 2);
		node->output_pins[j]->mapping = tdout_pin->mapping;

		connect_nodes(new_mem_node, j, mux_node, 3);
		new_mem_node->output_pins[j]->mapping = tdout_pin->mapping;

		tdout_pin->mapping = NULL;

		mux_node->output_pins[0]->name = mux_node->name;
	}

	// must recurse on new memory if it's too small
	if (logical_size <= split_size) {
		sp_memory_list = insert_in_vptr_list(sp_memory_list, new_mem_node);
		sp_memory_list = insert_in_vptr_list(sp_memory_list, node);
	} else {
		split_sp_memory_depth(node);
		split_sp_memory_depth(new_mem_node);
	}

	return;
}

/*-------------------------------------------------------------------------
 * (function: split_dp_memory_depth)
 *
 * This function works to split the depth of a dual port memory into 
 *   several smaller memories.
 *------------------------------------------------------------------------
 */
void 
split_dp_memory_depth(nnode_t *node)
{
	int addr1_port = -1;
	int addr2_port = -1;
	int we1_port = -1;
	int we2_port = -1;
	int logical_size;
	int i, j;
	int idx;
	int addr1_pin_idx = 0;
	int we1_pin_idx = 0;
	int addr2_pin_idx = 0;
	int we2_pin_idx = 0;
	nnode_t *new_mem_node;
	nnode_t *and1_node, *not1_node, *ff1_node, *mux1_node;
	nnode_t *and2_node, *not2_node, *ff2_node, *mux2_node;
	npin_t *addr1_pin = NULL;
	npin_t *addr2_pin = NULL;
	npin_t *we1_pin = NULL;
	npin_t *we2_pin = NULL;
	npin_t *twe_pin, *taddr_pin;
	npin_t *clk_pin = NULL;
	npin_t *tdout_pin;

	oassert(node->type == MEMORY);

	/* Find which ports are the addr1 and addr2 ports */
	idx = 0;
	for (i = 0; i < node->num_input_port_sizes; i++)
	{
		if (strcmp("addr1", node->input_pins[idx]->mapping) == 0)
		{
			addr1_port = i;
			addr1_pin_idx = idx;
			addr1_pin = node->input_pins[idx];
		}
		else if (strcmp("addr2", node->input_pins[idx]->mapping) == 0)
		{
			addr2_port = i;
			addr2_pin_idx = idx;
			addr2_pin = node->input_pins[idx];
		}
		else if (strcmp("we1", node->input_pins[idx]->mapping) == 0)
		{
			we1_port = i;
			we1_pin = node->input_pins[idx];
			we1_pin_idx = idx;
		}
		else if (strcmp("we2", node->input_pins[idx]->mapping) == 0)
		{
			we2_port = i;
			we2_pin = node->input_pins[idx];
			we2_pin_idx = idx;
		}
		else if (strcmp("clk", node->input_pins[idx]->mapping) == 0)
		{
			clk_pin = node->input_pins[idx];
		}
		idx += node->input_port_sizes[i];
	}

	if (addr1_port == -1)
	{
		error_message(1, 0, -1, "No \"addr1\" port on dual port RAM");
	}

	
	/* Jason Luu HACK: Logical memory depth determination messed up, forced to use this method */
	for(i = 0; i < node->input_port_sizes[addr1_port]; i++)
	{
		if(strcmp(node->input_pins[addr1_pin_idx + i]->name, "top^ZERO_PAD_ZERO") == 0)
			break;
	}
	logical_size = i;
 	
	/* Check that the memory needs to be split */
	if (logical_size <= split_size) {
		dp_memory_list = insert_in_vptr_list(dp_memory_list, node);
		return;
	} 

	/* Let's remove the address1 line from the memory */
	for (i = addr1_pin_idx; i < node->num_input_pins - 1; i++)
	{
		node->input_pins[i] = node->input_pins[i+1];
		node->input_pins[i]->pin_node_idx--;
	}
	node->input_port_sizes[addr1_port]--;
	node->input_pins = realloc(node->input_pins, sizeof(npin_t *) * --node->num_input_pins);
	if ((we1_port != -1) && (we1_pin_idx >= addr1_pin_idx))
		we1_pin_idx--;
	if ((we2_port != -1) && (we2_pin_idx >= addr1_pin_idx))
		we2_pin_idx--;
	if ((addr2_port != -1) && (addr2_pin_idx >= addr1_pin_idx))
		addr2_pin_idx--;

	/* Let's remove the address2 line from the memory */
	if (addr2_port != -1)
	{
		for (i = addr2_pin_idx; i < node->num_input_pins - 1; i++)
		{
			node->input_pins[i] = node->input_pins[i+1];
			node->input_pins[i]->pin_node_idx--;
		}
		node->input_port_sizes[addr2_port]--;
		node->input_pins = realloc(node->input_pins, sizeof(npin_t *) * --node->num_input_pins);
		if ((we1_port != -1) && (we1_pin_idx >= addr2_pin_idx))
			we1_pin_idx--;
		if ((we2_port != -1) && (we2_pin_idx >= addr2_pin_idx))
			we2_pin_idx--;
		if (addr1_pin_idx >= addr2_pin_idx)
			addr1_pin_idx--;
	}

	/* Create the new memory node */
	new_mem_node = allocate_nnode();

	// Append the new name with an __H
	new_mem_node->name = append_string(node->name, "__H");

	{	// Append the old name with an __S
		char *new_name = append_string(node->name, "__S");
		free(node->name);
		node->name = new_name;
	}

	/* Copy properties from the original memory node */
	new_mem_node->type = node->type;
	new_mem_node->related_ast_node = node->related_ast_node;
	new_mem_node->traverse_visited = node->traverse_visited;

	// Copy over the port sizes for the new memory
	for (j = 0; j < node->num_output_port_sizes; j++)
		add_output_port_information(new_mem_node, node->output_port_sizes[j]);
	for (j = 0; j < node->num_input_port_sizes; j++)
		add_input_port_information (new_mem_node, node->input_port_sizes[j]);

	// allocate space for pins.
	allocate_more_node_output_pins (new_mem_node, node->num_output_pins);
	allocate_more_node_input_pins  (new_mem_node, node->num_input_pins);

	// Copy over the pins for the new memory
	for (j = 0; j < node->num_input_pins; j++)
		add_a_input_pin_to_node_spot_idx(new_mem_node, copy_input_npin(node->input_pins[j]), j);

	if (we1_pin != NULL)
	{
		and1_node = make_2port_gate(LOGICAL_AND, 1, 1, 1, node, node->traverse_visited);
		twe_pin = copy_input_npin(we1_pin);
		add_a_input_pin_to_node_spot_idx(and1_node, twe_pin, 1);
		taddr_pin = copy_input_npin(addr1_pin);
		add_a_input_pin_to_node_spot_idx(and1_node, taddr_pin, 0);
		connect_nodes(and1_node, 0, node, we1_pin_idx);
		node->input_pins[we1_pin_idx]->mapping = we1_pin->mapping;
	}

	if (we2_pin != NULL)
	{
		and2_node = make_2port_gate(LOGICAL_AND, 1, 1, 1, node, node->traverse_visited);
		twe_pin = copy_input_npin(we2_pin);
		add_a_input_pin_to_node_spot_idx(and2_node, twe_pin, 1);
		taddr_pin = copy_input_npin(addr2_pin);
		add_a_input_pin_to_node_spot_idx(and2_node, taddr_pin, 0);
		connect_nodes(and2_node, 0, node, we2_pin_idx);
		node->input_pins[we2_pin_idx]->mapping = we2_pin->mapping;
	}

	if (we1_pin != NULL)
	{
		taddr_pin = copy_input_npin(addr1_pin);
		not1_node = make_not_gate_with_input(taddr_pin, new_mem_node, new_mem_node->traverse_visited);
		and1_node = make_2port_gate(LOGICAL_AND, 1, 1, 1, new_mem_node, new_mem_node->traverse_visited);
		connect_nodes(not1_node, 0, and1_node, 0);
		add_a_input_pin_to_node_spot_idx(and1_node, we1_pin, 1);
		connect_nodes(and1_node, 0, new_mem_node, we1_pin_idx);
		new_mem_node->input_pins[we1_pin_idx]->mapping = we1_pin->mapping;
	}

	if (we2_pin != NULL)
	{
		taddr_pin = copy_input_npin(addr2_pin);
		not2_node = make_not_gate_with_input(taddr_pin, new_mem_node, new_mem_node->traverse_visited);
		and2_node = make_2port_gate(LOGICAL_AND, 1, 1, 1, new_mem_node, new_mem_node->traverse_visited);
		connect_nodes(not2_node, 0, and2_node, 0);
		add_a_input_pin_to_node_spot_idx(and2_node, we2_pin, 1);
		connect_nodes(and2_node, 0, new_mem_node, we2_pin_idx);
		new_mem_node->input_pins[we2_pin_idx]->mapping = we2_pin->mapping;
	}

	if (node->num_output_pins > 0) /* There is an "out1" output */
	{
		ff1_node = make_2port_gate(FF_NODE, 1, 1, 1, node, node->traverse_visited);
		add_a_input_pin_to_node_spot_idx(ff1_node, addr1_pin, 0);
		add_a_input_pin_to_node_spot_idx(ff1_node, copy_input_npin(clk_pin), 1);

		/* Copy over the output pins for the new memory */
		for (j = 0; j < node->output_port_sizes[0]; j++)
		{
			mux1_node = make_2port_gate(MUX_2, 2, 2, 1, node, node->traverse_visited);
			connect_nodes(ff1_node, 0, mux1_node, 0);
			not1_node = make_not_gate(node, node->traverse_visited);
			connect_nodes(ff1_node, 0, not1_node, 0);
			connect_nodes(not1_node, 0, mux1_node, 1);
			tdout_pin = node->output_pins[j];
			remap_pin_to_new_node(tdout_pin, mux1_node, 0);
			connect_nodes(node, j, mux1_node, 2);
			node->output_pins[j]->mapping = tdout_pin->mapping;
			connect_nodes(new_mem_node, j, mux1_node, 3);
			new_mem_node->output_pins[j]->mapping = tdout_pin->mapping;
			tdout_pin->mapping = NULL;

			mux1_node->output_pins[0]->name = mux1_node->name;
		}
	}

	if (node->num_output_pins > node->output_port_sizes[0]) /* There is an "out2" output */
	{
		ff2_node = make_2port_gate(FF_NODE, 1, 1, 1, node, node->traverse_visited);
		add_a_input_pin_to_node_spot_idx(ff2_node, addr2_pin, 0);
		add_a_input_pin_to_node_spot_idx(ff2_node, copy_input_npin(clk_pin), 1);

		/* Copy over the output pins for the new memory */
		for (j = 0; j < node->output_port_sizes[0]; j++)
		{
			mux2_node = make_2port_gate(MUX_2, 2, 2, 1, node, node->traverse_visited);
			connect_nodes(ff2_node, 0, mux2_node, 0);
			not2_node = make_not_gate(node, node->traverse_visited);

			connect_nodes(ff2_node, 0, not2_node, 0);
			connect_nodes(not2_node, 0, mux2_node, 1);

			tdout_pin = node->output_pins[node->output_port_sizes[0] + j];
			remap_pin_to_new_node(tdout_pin, mux2_node, 0);

			connect_nodes(node, node->output_port_sizes[0] + j, mux2_node, 2);
			node->output_pins[node->output_port_sizes[0] + j]->mapping = tdout_pin->mapping;
			connect_nodes(new_mem_node, node->output_port_sizes[0] + j, mux2_node, 3);
			new_mem_node->output_pins[node->output_port_sizes[0] + j]->mapping = tdout_pin->mapping;
			tdout_pin->mapping = NULL;

			mux2_node->output_pins[0]->name = mux2_node->name;
		}
	}

	/* must recurse on new memory if it's too small */
	if (logical_size <= split_size) {
		dp_memory_list = insert_in_vptr_list(dp_memory_list, new_mem_node);
		dp_memory_list = insert_in_vptr_list(dp_memory_list, node);
	} else {
		split_dp_memory_depth(node);
		split_dp_memory_depth(new_mem_node);
	}

	return;
}

/*-------------------------------------------------------------------------
 * (function: split_sp_memory_width)
 *
 * This function works to split the width of a memory into several smaller
 *   memories.
 *------------------------------------------------------------------------
 */
void 
split_sp_memory_width(nnode_t *node)
{
	int data_port;
	int i, j, k, idx, old_idx, diff;
	nnode_t *new_node;

	oassert(node->type == MEMORY);

	/* Find which port is the data port on the input! */
	idx = 0;
	data_port = -1;
	for (i = 0; i < node->num_input_port_sizes; i++)
	{
		if (strcmp("data", node->input_pins[idx]->mapping) == 0)
			data_port = i;
		idx += node->input_port_sizes[i];
	}
	if (data_port == -1)
	{
		error_message(1, 0, -1, "No \"data\" port on single port RAM");
	}

	diff = node->input_port_sizes[data_port];

	/* Need to create a new node for every data bit */
	for (i = 1; i < node->input_port_sizes[data_port]; i++)
	{
		char BUF[10];
		new_node = allocate_nnode();
		sp_memory_list = insert_in_vptr_list(sp_memory_list, new_node);
		new_node->name = (char *)malloc(strlen(node->name) + 10);
		strcpy(new_node->name, node->name);
		strcat(new_node->name, "-");
		sprintf(BUF, "%d", i);
		strcat(new_node->name, BUF);

		/* Copy properties from the original node */
		new_node->type = node->type;
		new_node->related_ast_node = node->related_ast_node;
		new_node->traverse_visited = node->traverse_visited;
		new_node->node_data = NULL;

		new_node->num_input_port_sizes = node->num_input_port_sizes;
		new_node->input_port_sizes = (int *)malloc(node->num_input_port_sizes * sizeof(int));
		for (j = 0; j < node->num_input_port_sizes; j++)
			new_node->input_port_sizes[j] = node->input_port_sizes[j];

		new_node->input_port_sizes[data_port] = 1;
		new_node->num_output_port_sizes = 1;
		new_node->output_port_sizes = (int *)malloc(sizeof(int));
		new_node->output_port_sizes[0] = 1;

		/* Set the number of input pins and pin entires */
		new_node->num_input_pins = node->num_input_pins - diff + 1;
		new_node->input_pins = (npin_t**)malloc(sizeof(void *) * new_node->num_input_pins);

		idx = 0;
		old_idx = 0;
		for (j = 0; j < new_node->num_input_port_sizes; j++)
		{
			if (j == data_port)
			{
				new_node->input_pins[idx] = node->input_pins[idx + i];
				node->input_pins[idx+i] = NULL;
				new_node->input_pins[idx]->node = new_node;
				new_node->input_pins[idx]->pin_node_idx = idx;
				old_idx = old_idx + node->input_port_sizes[data_port];
				idx++;
			}
			else
			{
				for (k = 0; k < new_node->input_port_sizes[j]; k++)
				{
					new_node->input_pins[idx] = copy_input_npin(node->input_pins[old_idx]);
					new_node->input_pins[idx]->pin_node_idx = idx;
					new_node->input_pins[idx]->node = new_node;
					idx++;
					old_idx++;
				}
			}
		}

		/* Set the number of output pins and pin entry */
		new_node->num_output_pins = 1;
		new_node->output_pins = (npin_t **)malloc(sizeof(void*));
		new_node->output_pins[0] = copy_output_npin(node->output_pins[i]);
		add_a_driver_pin_to_net(node->output_pins[i]->net, new_node->output_pins[0]);
		free_npin(node->output_pins[i]);
		node->output_pins[i] = NULL;
		new_node->output_pins[0]->pin_node_idx = 0;
		new_node->output_pins[0]->node = new_node;
	}

	/* Now need to clean up the original to do 1 bit output - first bit */

	/* Name the node to show first bit! */
	{
		char *new_name = append_string(node->name, "-0");
		free(node->name);
		node->name = new_name;
	}

	/* free the additional output pins */
	for (i = 1; i < node->num_output_pins; i++)
		free_npin(node->output_pins[i]);
	node->num_output_pins = 1;
	node->output_pins = realloc(node->output_pins, sizeof(npin_t *) * 1);
	node->output_port_sizes[0] = 1;

	/* Shuffle the input pins on account of removed input pins */
	idx = old_idx = 0;
	node->input_port_sizes[data_port] = 1;
	for (i = 0; i < node->num_input_port_sizes; i++)
	{
		for (j = 0; j < node->input_port_sizes[i]; j++)
		{
			node->input_pins[idx] = node->input_pins[old_idx];
			node->input_pins[idx]->pin_node_idx = idx;
			idx++;
			old_idx++;
		}
		if (i == data_port)
			old_idx = old_idx + diff - 1;
	}
	node->num_input_pins = node->num_input_pins - diff + 1;
	sp_memory_list = insert_in_vptr_list(sp_memory_list, node);

	return;
}



/*-------------------------------------------------------------------------
 * (function: split_dp_memory_width)
 *
 * This function works to split the width of a memory into several smaller
 *   memories.
 *------------------------------------------------------------------------
 */
void 
split_dp_memory_width(nnode_t *node)
{
	int data_port1, data_port2;
	int i, j, k, idx, old_idx, data_diff1, data_diff2;
	nnode_t *new_node;
	char *tmp_name;

	oassert(node->type == MEMORY);

	/* Find which port is the data port on the input! */
	idx = 0;
	data_port1 = -1;
	data_port2 = -1;
	data_diff1 = 0;
	data_diff2 = 0;

	for (i = 0; i < node->num_input_port_sizes; i++)
	{
		if (strcmp("data1", node->input_pins[idx]->mapping) == 0)
		{
			data_port1 = i;
			data_diff1 = node->input_port_sizes[data_port1] - 1;
		}
		if (strcmp("data2", node->input_pins[idx]->mapping) == 0)
		{
			data_port2 = i;
			data_diff2 = node->input_port_sizes[data_port2] - 1;
		}
		idx += node->input_port_sizes[i];
	}

	if (data_port1 == -1)
	{
		error_message(1, 0, -1, "No \"data1\" port on dual port RAM");
		return;
	}

	/* Need to create a new node for every data bit */
	for (i = 1; i < node->input_port_sizes[data_port1]; i++)
	{
		char BUF[10];
		new_node = allocate_nnode();
		dp_memory_list = insert_in_vptr_list(dp_memory_list, new_node);
		new_node->name = (char *)malloc(strlen(node->name) + 10);
		strcpy(new_node->name, node->name);
		strcat(new_node->name, "-");
		sprintf(BUF, "%d", i);
		strcat(new_node->name, BUF);

		/* Copy properties from the original node */
		new_node->type = node->type;
		new_node->related_ast_node = node->related_ast_node;
		new_node->traverse_visited = node->traverse_visited;
		new_node->node_data = NULL;

		new_node->num_input_port_sizes = node->num_input_port_sizes;
		new_node->input_port_sizes = (int *)malloc(node->num_input_port_sizes * sizeof(int));
		for (j = 0; j < node->num_input_port_sizes; j++)
			new_node->input_port_sizes[j] = node->input_port_sizes[j];

		new_node->input_port_sizes[data_port1] = 1;
		if (data_port2 != -1)
			new_node->input_port_sizes[data_port2] = 1;

		if (data_port2 == -1)
		{
			new_node->num_output_port_sizes = 1;
			new_node->output_port_sizes = (int *)malloc(sizeof(int));
		}
		else
		{
			new_node->num_output_port_sizes = 2;
			new_node->output_port_sizes = (int *)malloc(sizeof(int)*2);
			new_node->output_port_sizes[1] = 1;
		}
		new_node->output_port_sizes[0] = 1;

		/* Set the number of input pins and pin entires */
		new_node->num_input_pins = node->num_input_pins - data_diff1 - data_diff2;
		new_node->input_pins = (npin_t**)malloc(sizeof(void *) * new_node->num_input_pins);


		idx = 0;
		old_idx = 0;
		for (j = 0; j < new_node->num_input_port_sizes; j++)
		{
			if (j == data_port1)
			{
				new_node->input_pins[idx] = node->input_pins[old_idx + i];
				node->input_pins[old_idx+i] = NULL;
				new_node->input_pins[idx]->node = new_node;
				new_node->input_pins[idx]->pin_node_idx = idx;
				old_idx = old_idx + node->input_port_sizes[data_port1];
				idx++;
			}
			else if (j == data_port2)
			{
				new_node->input_pins[idx] = node->input_pins[old_idx + i];
				node->input_pins[old_idx+i] = NULL;
				new_node->input_pins[idx]->node = new_node;
				new_node->input_pins[idx]->pin_node_idx = idx;
				old_idx = old_idx + node->input_port_sizes[data_port2];
				idx++;
			}
			else
			{
				for (k = 0; k < new_node->input_port_sizes[j]; k++)
				{
					new_node->input_pins[idx] = copy_input_npin(node->input_pins[old_idx]);
					new_node->input_pins[idx]->pin_node_idx = idx;
					new_node->input_pins[idx]->node = new_node;
					idx++;
					old_idx++;
				}
			}
		}

		/* Set the number of output pins and pin entry */
		if (data_port2 == -1)
		{
			new_node->num_output_pins = 1;
			new_node->output_pins = (npin_t **)malloc(sizeof(void*));
			new_node->output_pins[0] = node->output_pins[i];
			node->output_pins[i] = NULL;
			new_node->output_pins[0]->pin_node_idx = 0;
			new_node->output_pins[0]->node = new_node;
		}
		else
		{
			new_node->num_output_pins = 2;
			new_node->output_pins = (npin_t **)malloc(sizeof(void*)*2);
			new_node->output_pins[0] = node->output_pins[i];
			node->output_pins[i] = NULL;
			new_node->output_pins[0]->pin_node_idx = 0;
			new_node->output_pins[0]->node = new_node;

			new_node->output_pins[1] = node->output_pins[i+data_diff1+1];
			node->output_pins[i+data_diff1+1] = NULL;
			new_node->output_pins[1]->pin_node_idx = 1;
			new_node->output_pins[1]->node = new_node;
		}
	}

	/* Now need to clean up the original to do 1 bit output - first bit */
	/* Name the node to show first bit! */
	tmp_name = (char *)malloc(strlen(node->name) + 3);
	strcpy(tmp_name, node->name);
	strcat(tmp_name, "-0");
	free(node->name);
	node->name = tmp_name;

	/* free the additional output pins */
	if (data_port2 == -1)
	{
		for (i = 1; i < node->num_output_pins; i++)
			free_npin(node->output_pins[i]);

		node->num_output_pins = 1;
		node->output_pins = realloc(node->output_pins, sizeof(npin_t *) * node->num_output_pins);
		node->output_port_sizes[0] = 1;
	}
	else
	{
		for (i = 1; i < (node->num_output_pins/2); i++)
			free_npin(node->output_pins[i]);
		node->output_pins[1] = node->output_pins[data_diff1 + 1];
		node->output_pins[data_diff1 + 1] = NULL;
		node->output_pins[1]->pin_node_idx = 1;
		for (; i < node->num_output_pins; i++)
			free_npin(node->output_pins[i]);

		node->num_output_pins = 2;
		node->output_pins = realloc(node->output_pins, sizeof(npin_t *) * node->num_output_pins);
		node->output_port_sizes[0] = 1;
		node->output_port_sizes[1] = 1;
	}

	/* Shuffle the input pins on account of removed input pins */
	idx = old_idx = 0;
	node->input_port_sizes[data_port1] = 1;
	if (data_port2 != -1)
		node->input_port_sizes[data_port2] = 1;
	for (i = 0; i < node->num_input_port_sizes; i++)
	{
		for (j = 0; j < node->input_port_sizes[i]; j++)
		{
			node->input_pins[idx] = node->input_pins[old_idx];
			node->input_pins[idx]->pin_node_idx = idx;
			idx++;
			old_idx++;
		}
		if (i == data_port1)
			old_idx = old_idx + data_diff1;
		if (i == data_port2)
			old_idx = old_idx + data_diff2;
	}
	node->num_input_pins = node->num_input_pins - data_diff1;
	if (data_port2 != -1)
		node->num_input_pins = node->num_input_pins - data_diff2;

	node->input_pins = realloc(node->input_pins, sizeof(npin_t *) * node->num_input_pins);
	dp_memory_list = insert_in_vptr_list(dp_memory_list, node);

	return;
}

/*
 * Width-splits the given memory up into chunks the of the
 * width specified in the arch file.
 */
void split_sp_memory_to_arch_width(nnode_t *node)
{
	char *port_name = "data";
	t_model *model = single_port_rams;

	int data_port_number = get_input_port_index_from_mapping(node, port_name);

	oassert(data_port_number != -1);

	int data_port_size = node->input_port_sizes[data_port_number];

	// Get the target width from the arch.
	t_model_ports *ports = get_model_port(model->inputs, port_name);
	int target_size  = ports->size;

	int num_memories = ceil((double)data_port_size / (double)target_size);
	if (data_port_size > target_size)
	{
		int i;
		int data_pins_moved = 0;
		int output_pins_moved = 0;
		for (i = 0; i < num_memories; i++)
		{
			nnode_t *new_node = allocate_nnode();
			new_node->name = append_string(node->name, "-%d",i);
			sp_memory_list = insert_in_vptr_list(sp_memory_list, new_node);

			/* Copy properties from the original node */
			new_node->type = node->type;
			new_node->related_ast_node = node->related_ast_node;
			new_node->traverse_visited = node->traverse_visited;
			new_node->node_data = NULL;

			int j;
			for (j = 0; j < node->num_input_port_sizes; j++)
				add_input_port_information(new_node, 0);

			add_output_port_information(new_node, 0);

			int index = 0;
			int old_index = 0;
			for (j = 0; j < node->num_input_port_sizes; j++)
			{
				// Move this node's share of data pins out of the data port of the original node.
				if (j == data_port_number)
				{
					// Skip over data pins we've already moved.
					old_index += data_pins_moved;
					int k;
					for (k = 0; k < target_size && data_pins_moved < data_port_size; k++)
					{
						allocate_more_node_input_pins(new_node, 1);
						new_node->input_port_sizes[j]++;
						remap_pin_to_new_node(node->input_pins[old_index], new_node, index);
						index++;
						old_index++;
						data_pins_moved++;
					}
					int remaining_data_pins = data_port_size - data_pins_moved;
					// Skip over pins we have yet to copy.
					old_index += remaining_data_pins;
				}
				else
				{
					int k;
					for (k = 0; k < node->input_port_sizes[j]; k++)
					{
						allocate_more_node_input_pins(new_node, 1);
						new_node->input_port_sizes[j]++;
						// Copy pins for all but the last memory. the last one get the original pins moved to it.
						if (i < num_memories - 1)
							add_a_input_pin_to_node_spot_idx(new_node, copy_input_npin(node->input_pins[old_index]), index);
						else
							remap_pin_to_new_node(node->input_pins[old_index], new_node, index);
						index++;
						old_index++;
					}
				}
			}

			index = 0;
			old_index = 0;
			old_index += output_pins_moved;

			int k;
			for (k = 0; k < target_size && output_pins_moved < data_port_size; k++)
			{
				allocate_more_node_output_pins(new_node, 1);
				new_node->output_port_sizes[0]++;
				remap_pin_to_new_node(node->output_pins[old_index], new_node, index);
				index++;
				old_index++;
				output_pins_moved++;
			}
		}
		// Free the original node.
		free_nnode(node);
	}
	else
	{
		sp_memory_list = insert_in_vptr_list(sp_memory_list, node);
	}
}

void split_dp_memory_to_arch_width(nnode_t *node)
{
	char *data1_name = "data1";
	char *data2_name = "data2";
	char *out1_name = "out1";
	char *out2_name = "out2";

	t_model *model = dual_port_rams;

	int data1_port_number = get_input_port_index_from_mapping(node, data1_name);
	int data2_port_number = get_input_port_index_from_mapping(node, data2_name);

	int out1_port_number  = get_output_port_index_from_mapping(node, out1_name);
	int out2_port_number  = get_output_port_index_from_mapping(node, out2_name);

	oassert(data1_port_number != -1);
	oassert(data2_port_number != -1);
	oassert(out1_port_number  != -1);
	oassert(out2_port_number  != -1);

	int data1_port_size = node->input_port_sizes[data1_port_number];
	int data2_port_size = node->input_port_sizes[data2_port_number];

	int out1_port_size  = node->output_port_sizes[out1_port_number];
	int out2_port_size  = node->output_port_sizes[out2_port_number];

	oassert(data1_port_size == data2_port_size);
	oassert(out1_port_size  == out2_port_size);
	oassert(data1_port_size == out1_port_size);

	// Get the target width from the arch.
	t_model_ports *ports = get_model_port(model->inputs, data1_name);
	int target_size  = ports->size;

	int num_memories = ceil((double)data1_port_size / (double)target_size);

	//printf("%d %d\n", data1_port_size, target_size);

	if (data1_port_size > target_size)
	{
		int i;
		int data1_pins_moved = 0;
		int data2_pins_moved = 0;
		int out1_pins_moved  = 0;
		int out2_pins_moved  = 0;
		for (i = 0; i < num_memories; i++)
		{
			nnode_t *new_node = allocate_nnode();
			new_node->name = append_string(node->name, "-%d",i);
			dp_memory_list = insert_in_vptr_list(dp_memory_list, new_node);

			/* Copy properties from the original node */
			new_node->type = node->type;
			new_node->related_ast_node = node->related_ast_node;
			new_node->traverse_visited = node->traverse_visited;
			new_node->node_data = NULL;

			int j;
			for (j = 0; j < node->num_input_port_sizes; j++)
				add_input_port_information(new_node, 0);

			int index = 0;
			int old_index = 0;
			for (j = 0; j < node->num_input_port_sizes; j++)
			{
				// Move this node's share of data pins out of the data port of the original node.
				if (j == data1_port_number)
				{
					// Skip over data pins we've already moved.
					old_index += data1_pins_moved;
					int k;
					for (k = 0; k < target_size && data1_pins_moved < data1_port_size; k++)
					{
						allocate_more_node_input_pins(new_node, 1);
						new_node->input_port_sizes[j]++;
						remap_pin_to_new_node(node->input_pins[old_index], new_node, index);
						index++;
						old_index++;
						data1_pins_moved++;
					}
					int remaining_data_pins = data1_port_size - data1_pins_moved;
					// Skip over pins we have yet to copy.
					old_index += remaining_data_pins;
				}
				else if (j == data2_port_number)
				{
					// Skip over data pins we've already moved.
					old_index += data2_pins_moved;
					int k;
					for (k = 0; k < target_size && data2_pins_moved < data2_port_size; k++)
					{
						allocate_more_node_input_pins(new_node, 1);
						new_node->input_port_sizes[j]++;
						remap_pin_to_new_node(node->input_pins[old_index], new_node, index);
						index++;
						old_index++;
						data2_pins_moved++;
					}
					int remaining_data_pins = data2_port_size - data2_pins_moved;
					// Skip over pins we have yet to copy.
					old_index += remaining_data_pins;
				}
				else
				{
					int k;
					for (k = 0; k < node->input_port_sizes[j]; k++)
					{
						allocate_more_node_input_pins(new_node, 1);
						new_node->input_port_sizes[j]++;
						// Copy pins for all but the last memory. the last one get the original pins moved to it.
						if (i < num_memories - 1)
							add_a_input_pin_to_node_spot_idx(new_node, copy_input_npin(node->input_pins[old_index]), index);
						else
							remap_pin_to_new_node(node->input_pins[old_index], new_node, index);
						index++;
						old_index++;
					}
				}
			}

			for (j = 0; j < node->num_output_port_sizes; j++)
				add_output_port_information(new_node, 0);

			index = 0;
			old_index = 0;
			for (j = 0; j < node->num_output_port_sizes; j++)
			{
				// Move this node's share of data pins out of the data port of the original node.
				if (j == out1_port_number)
				{
					// Skip over data pins we've already moved.
					old_index += out1_pins_moved;
					int k;
					for (k = 0; k < target_size && out1_pins_moved < out1_port_size; k++)
					{
						allocate_more_node_output_pins(new_node, 1);
						new_node->output_port_sizes[j]++;
						remap_pin_to_new_node(node->output_pins[old_index], new_node, index);
						index++;
						old_index++;
						out1_pins_moved++;
					}
					int remaining_pins = out1_port_size - out1_pins_moved;
					// Skip over pins we have yet to copy.
					old_index += remaining_pins;
				}
				else if (j == out2_port_number)
				{
					// Skip over data pins we've already moved.
					old_index += out2_pins_moved;
					int k;
					for (k = 0; k < target_size && out2_pins_moved < out2_port_size; k++)
					{
						allocate_more_node_output_pins(new_node, 1);
						new_node->output_port_sizes[j]++;
						remap_pin_to_new_node(node->output_pins[old_index], new_node, index);
						index++;
						old_index++;
						out2_pins_moved++;
					}
					int remaining_pins = out2_port_size - out2_pins_moved;
					// Skip over pins we have yet to copy.
					old_index += remaining_pins;
				}
				else
				{
					oassert(FALSE);
				}
			}
		}
		// Free the original node.
		free_nnode(node);
	}
	else
	{
		// If we're not splitting, put the original memory node back.
		dp_memory_list = insert_in_vptr_list(dp_memory_list, node);
	}

}


/*-------------------------------------------------------------------------
 * (function: iterate_memories)
 *
 * This function will iterate over all of the memory hard blocks that
 *      exist in the netlist and perform a splitting so that they can
 *      be easily packed into hard memory blocks on the FPGA.
 *-----------------------------------------------------------------------*/
void 
iterate_memories(netlist_t *netlist)
{
	nnode_t *node;
	struct s_linked_vptr *temp;

	/* Split it up on depth */
	if (configuration.split_memory_depth != 0)
	{
		/* Jason Luu: HACK detected: split_size should NOT come from configuration.split_memory_depth, 
		   it should come from the maximum model size for the port, IMPORTANT TODO!!!! */
		split_size = configuration.split_memory_depth;

		temp = sp_memory_list;
		sp_memory_list = NULL;
		while (temp != NULL)
		{
			node = (nnode_t *)temp->data_vptr;
			oassert(node != NULL);
			oassert(node->type == MEMORY);
			temp = delete_in_vptr_list(temp);
			split_sp_memory_depth(node);
		}

		temp = dp_memory_list;
		dp_memory_list = NULL;
		while (temp != NULL)
		{
			node = (nnode_t *)temp->data_vptr;
			oassert(node != NULL);
			oassert(node->type == MEMORY);
			temp = delete_in_vptr_list(temp);
			split_dp_memory_depth(node);
		}
	}

	/* Split memory up on width */
	if (configuration.split_memory_width)
	{
		temp = sp_memory_list;
		sp_memory_list = NULL;
		while (temp != NULL)
		{
			node = (nnode_t *)temp->data_vptr;
			oassert(node != NULL);
			oassert(node->type == MEMORY);
			temp = delete_in_vptr_list(temp);
			split_sp_memory_width(node);
		}

		temp = dp_memory_list;
		dp_memory_list = NULL;
		while (temp != NULL)
		{
			node = (nnode_t *)temp->data_vptr;
			oassert(node != NULL);
			oassert(node->type == MEMORY);
			temp = delete_in_vptr_list(temp);
			split_dp_memory_width(node);
		}
	}
	else
	{
		temp = sp_memory_list;
		sp_memory_list = NULL;
		while (temp != NULL)
		{
			node = (nnode_t *)temp->data_vptr;
			oassert(node != NULL);
			oassert(node->type == MEMORY);
			temp = delete_in_vptr_list(temp);
			split_sp_memory_to_arch_width(node);
		}


		temp = sp_memory_list;
		sp_memory_list = NULL;
		while (temp != NULL)
		{
			node = (nnode_t *)temp->data_vptr;
			oassert(node != NULL);
			oassert(node->type == MEMORY);
			temp = delete_in_vptr_list(temp);
			pad_sp_memory_width(node, netlist);
		}


		temp = dp_memory_list;
		dp_memory_list = NULL;
		while (temp != NULL)
		{
			node = (nnode_t *)temp->data_vptr;
			oassert(node != NULL);
			oassert(node->type == MEMORY);
			temp = delete_in_vptr_list(temp);
			split_dp_memory_to_arch_width(node);
		}

		temp = dp_memory_list;
		dp_memory_list = NULL;
		while (temp != NULL)
		{
			node = (nnode_t *)temp->data_vptr;
			oassert(node != NULL);
			oassert(node->type == MEMORY);
			temp = delete_in_vptr_list(temp);
			pad_dp_memory_width(node, netlist);
		}
	}

	return;
}

/*-------------------------------------------------------------------------
 * (function: clean_memories)
 *
 * Clean up the memory by deleting the list structure of memories
 *      during optimization.
 *-----------------------------------------------------------------------*/
void clean_memories()
{
	while (sp_memory_list != NULL)
		sp_memory_list = delete_in_vptr_list(sp_memory_list);
	while (dp_memory_list != NULL)
		dp_memory_list = delete_in_vptr_list(dp_memory_list);
}

/*
 * Pads the width of a dual port memory to that specified in the arch file.
 */
void pad_dp_memory_width(nnode_t *node, netlist_t *netlist)
{
	oassert(node->type == MEMORY);
	oassert(dual_port_rams != NULL);

	pad_memory_input_port(node, netlist, dual_port_rams, "data1");
	pad_memory_input_port(node, netlist, dual_port_rams, "data2");

	pad_memory_output_port(node, netlist, dual_port_rams, "out1");
	pad_memory_output_port(node, netlist, dual_port_rams, "out2");

	dp_memory_list = insert_in_vptr_list(dp_memory_list, node);
}

/*
 * Pads the width of a single port memory to that specified in the arch file.
 */
void pad_sp_memory_width(nnode_t *node, netlist_t *netlist)
{
	oassert(node->type == MEMORY);
	oassert(single_port_rams != NULL);

	pad_memory_input_port (node, netlist, single_port_rams, "data");

	pad_memory_output_port(node, netlist, single_port_rams, "out");

	sp_memory_list = insert_in_vptr_list(sp_memory_list, node);
}

/*
 * Pads the given output port to the width specified in the given model.
 */
void pad_memory_output_port(nnode_t *node, netlist_t *netlist, t_model *model, char *port_name)
{
	int port_number = get_output_port_index_from_mapping(node, port_name);
	int port_index  = get_output_pin_index_from_mapping (node, port_name);

	int port_size = node->output_port_sizes[port_number];

	t_model_ports *ports = get_model_port(model->outputs, port_name);

	int target_size = ports->size;
	int diff        = target_size - port_size;

	if (diff > 0)
	{
		allocate_more_node_output_pins(node, diff);

		// Shift other pins to the right, if any.
		int i;
		for (i = node->num_output_pins - 1; i >= port_index + target_size; i--)
			move_a_output_pin(node, i - diff, i);

		for (i = port_index + port_size; i < port_index + target_size; i++)
		{
			// Add new pins to the higher order spots.
			npin_t *new_pin = allocate_npin();
			new_pin->mapping = strdup(port_name);
			add_a_output_pin_to_node_spot_idx(node, new_pin, i);
		}
		node->output_port_sizes[port_number] = target_size;
	}
}

/*
 * Pads the given input port to the width specified in the given model.
 */
void pad_memory_input_port(nnode_t *node, netlist_t *netlist, t_model *model, char *port_name)
{
	oassert(node->type == MEMORY);
	oassert(model != NULL);

	int port_number = get_input_port_index_from_mapping(node, port_name);
	int port_index  = get_input_pin_index_from_mapping (node, port_name);

	oassert(port_number != -1);
	oassert(port_index  != -1);

	int port_size = node->input_port_sizes[port_number];

	t_model_ports *ports = get_model_port(model->inputs, port_name);

	int target_size = ports->size;
	int diff        = target_size - port_size;


	// Expand the inputs
	if (diff > 0)
	{
		allocate_more_node_input_pins(node, diff);

		// Shift other pins to the right, if any.
		int i;
		for (i = node->num_input_pins - 1; i >= port_index + target_size; i--)
			move_a_input_pin(node, i - diff, i);

		for (i = port_index + port_size; i < port_index + target_size; i++)
		{
			add_a_input_pin_to_node_spot_idx(node, get_a_pad_pin(netlist), i);
			node->input_pins[i]->mapping = strdup(port_name);
		}

		node->input_port_sizes[port_number] = target_size;
	}
}

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
#include "odin_types.h"
#include "implicit_memory.h"
#include "node_creation_library.h"
#include "odin_util.h"
#include "vtr_util.h"
#include "vtr_memory.h"

#include <unordered_map>

// Hashes the implicit memory name to the implicit_memory structure.
std::unordered_map<std::string,implicit_memory *> implicit_memories;
// Hashes the implicit memory input name to the implicit_memory structure.
std::unordered_map<std::string,implicit_memory *>  implicit_memory_inputs;

void finalize_implicit_memory(implicit_memory *memory);
void add_dummy_output_port_to_implicit_memory(implicit_memory *memory, int size, const char *port_name);
void add_dummy_input_port_to_implicit_memory(implicit_memory *memory, int size, const char *port_name);
void collapse_implicit_memory_to_single_port_ram(implicit_memory *memory);
implicit_memory *lookup_implicit_memory(char *instance_name_prefix, char *identifier);

/*
 * Initialises hashtables to lookup memories based on inputs and names.
 */
void init_implicit_memory_index()
{
	implicit_memories = std::unordered_map<std::string,implicit_memory *>();
	implicit_memory_inputs = std::unordered_map<std::string,implicit_memory *>();
}

/*
 * Looks up an implicit memory by identifier name in the implicit memory lookup table.
 */
implicit_memory *lookup_implicit_memory(char *instance_name_prefix, char *identifier)
{
	char *memory_string = make_full_ref_name(instance_name_prefix, NULL, NULL, identifier, -1);

	std::unordered_map<std::string,implicit_memory *>::const_iterator mem_out = implicit_memories.find(std::string(memory_string));

	vtr::free(memory_string);

	if ( mem_out == implicit_memories.end() )
		return NULL;
	else
		return mem_out->second;
}

/*
 * Looks up an implicit memory by ast reference in the implicit memory lookup table.
 */
implicit_memory *lookup_implicit_memory_reference_ast(char *instance_name_prefix, ast_node_t *node)
{
	if (node && node->num_children == 2 && node->type == ARRAY_REF)
		return lookup_implicit_memory(instance_name_prefix, node->children[0]->types.identifier);
	else if (node && node->num_children == 3 && node->type == ARRAY_REF)
		return lookup_implicit_memory(instance_name_prefix, node->children[0]->types.identifier);
	else if (node && node->type == IDENTIFIERS)
		return lookup_implicit_memory(instance_name_prefix, node->types.identifier);
	else
		return NULL;
}

/*
 * Determines if the given implicit memory reference mode is supported.
 */
char is_valid_implicit_memory_reference_ast(char *instance_name_prefix, ast_node_t *node)
{
	if (node && node->num_children == 2 && node->type == ARRAY_REF
			&& lookup_implicit_memory_reference_ast(instance_name_prefix, node))
		return TRUE;
	else if (node && node->num_children == 3 && node->type == ARRAY_REF
			&& lookup_implicit_memory_reference_ast(instance_name_prefix, node))
		return TRUE;
	else
		return FALSE;
}

/*
 * Creates an implicit memory block with the given depth and data width, and the given name and prefix.
 */
implicit_memory *create_implicit_memory_block(int data_width, long memory_depth, char *name, char *instance_name_prefix)
{
	char implicit_string[] = "implicit_ram";

	oassert(memory_depth > 0
		&& "implicit memory depth must be greater than 0");

	//find closest power of 2 fr memory depth.
	long addr_width = 1;
	while (shift_left_value_with_overflow_check(0x1, addr_width) < memory_depth)
		addr_width++;

	//verify if it is a power of two (only one bit set)
	if(memory_depth - shift_left_value_with_overflow_check(0x1, addr_width) != 0)
	{
		warning_message(NETLIST_ERROR, -1, -1, "Rounding memory <%s> of size <%ld> to closest power of two: %ld.", name, memory_depth, shift_left_value_with_overflow_check(0x1, addr_width));
	}

	nnode_t *node = allocate_nnode();
	node->type = MEMORY;
	node->name = hard_node_name(node, instance_name_prefix, implicit_string, name);

	// Create a fake ast node.
	node->related_ast_node = (ast_node_t *)vtr::calloc(1, sizeof(ast_node_t));
	node->related_ast_node->children = (ast_node_t **)vtr::calloc(1,sizeof(ast_node_t *));
	node->related_ast_node->children[0] = (ast_node_t *)vtr::calloc(1, sizeof(ast_node_t));
	node->related_ast_node->children[0]->types.identifier = vtr::strdup(DUAL_PORT_RAM_string);

	char *full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name, -1);

	implicit_memory *memory = (implicit_memory *)vtr::malloc(sizeof(implicit_memory));
	memory->node = node;
	memory->addr_width = addr_width;
	memory->memory_depth = memory_depth;
	memory->data_width = data_width;
	memory->clock_added = FALSE;
	memory->output_added = FALSE;
	memory->name = full_name;

	implicit_memories.insert({std::string(full_name), memory});

	return memory;
}

/*
 * Adds an input port to the given implicit memory.
 */
void add_input_port_to_implicit_memory(implicit_memory *memory, signal_list_t *signals, const char *port_name)
{
	nnode_t *node = memory->node;

	add_input_port_to_memory(node, signals, port_name);
}

/*
 * Add an output port to the given implicit memory.
 */
void add_output_port_to_implicit_memory(implicit_memory *memory, signal_list_t *signals, const char *port_name)
{
	nnode_t *node = memory->node;

	add_output_port_to_memory(node, signals, port_name);
}

/*
 * Looks up an implicit memory based on the given name.
 */
implicit_memory *lookup_implicit_memory_input(char *name)
{

	std::unordered_map<std::string,implicit_memory *>::const_iterator mem_out = implicit_memory_inputs.find(std::string(name));

	if ( mem_out == implicit_memory_inputs.end() )
		return NULL;
	else
		return mem_out->second;

}

/*
 * Registers the given input name so that the given memory can be looked up based on
 * it.
 */
void register_implicit_memory_input(char *name, implicit_memory *memory)
{
	if (!lookup_implicit_memory_input(name))
		implicit_memory_inputs.insert({std::string(name), memory});
	else
		error_message(NETLIST_ERROR, -1, -1, "Attempted to re-register implicit memory output %s.", name);
}

/*
 * Frees memory used for indexing implicit memories. Finalises each
 * memory, making sure it has the right ports, and collapsing
 * the memory if possible.
 */
void free_implicit_memory_index_and_finalize_memories()
{

	implicit_memory_inputs.clear();

	if (!implicit_memories.empty())
	{
		for (auto mem_it : implicit_memories) 
		{
			finalize_implicit_memory(mem_it.second);
			vtr::free(mem_it.second);
		}
	}
	implicit_memories.clear();
}

/*
 * Adds a zeroed input port with to the given implicit memory
 * with the given size and port name (mapping)
 */
void add_dummy_input_port_to_implicit_memory(implicit_memory *memory, int size, const char *port_name)
{
	signal_list_t *signals = init_signal_list();
	int i;
	for (i = 0; i < size; i++)
		add_pin_to_signal_list(signals, get_zero_pin(verilog_netlist));

	add_input_port_to_implicit_memory(memory, signals, port_name);

	free_signal_list(signals);
}

/*
 * Adds an unconnected output port with to the given implicit memory
 * with the given size and port name (mapping)
 */
void add_dummy_output_port_to_implicit_memory(implicit_memory *memory, int size, const char *port_name)
{
	signal_list_t *signals = init_signal_list();
	static int dummy_output_pin_number = 0;
	int i;
	for (i = 0; i < size; i++)
	{
		npin_t *dummy_pin = allocate_npin();
		// Pad outputs with a unique and descriptive name to avoid collisions.
		dummy_pin->name = append_string("", "dummy_implicit_memory_output~%ld", dummy_output_pin_number++);
		add_pin_to_signal_list(signals, dummy_pin);
	}

	add_output_port_to_implicit_memory(memory, signals, port_name);

	free_signal_list(signals);
}

/*
 * Makes sure the given implicit memory has all necessary ports,
 * and adds any ports which may be missing. Collapses the memory to
 * a single port ram if one port is unused.
 */
void finalize_implicit_memory(implicit_memory *memory)
{
	nnode_t *node = memory->node;

	char has_addr1 = FALSE;
	char has_addr2 = FALSE;
	char has_data1 = FALSE;
	char has_data2 = FALSE;
	char has_we1   = FALSE;
	char has_we2   = FALSE;
	char has_clk   = FALSE;
	char has_out1  = FALSE;
	char has_out2  = FALSE;

	// Determine which input ports are present.
	int i;
	for (i = 0; i < node->num_input_pins; i++)
	{
		npin_t *pin = node->input_pins[i];
		if (!strcmp(pin->mapping, "addr1"))
			has_addr1 = TRUE;
		else if (!strcmp(pin->mapping, "addr2"))
			has_addr2 = TRUE;
		else if (!strcmp(pin->mapping, "data1"))
			has_data1 = TRUE;
		else if (!strcmp(pin->mapping, "data2"))
			has_data2 = TRUE;
		else if (!strcmp(pin->mapping, "we1"))
			has_we1 = TRUE;
		else if (!strcmp(pin->mapping, "we2"))
			has_we2 = TRUE;
		else if (!strcmp(pin->mapping, "clk"))
			has_clk = TRUE;
	}

	// Determine which output ports are present.
	for (i = 0; i < node->num_output_pins; i++)
	{
		npin_t *pin = node->output_pins[i];
		if (!strcmp(pin->mapping, "out1"))
			has_out1 = TRUE;
		else if (!strcmp(pin->mapping, "out2"))
			has_out2 = TRUE;
	}

	if (!has_clk)
	{
		add_dummy_input_port_to_implicit_memory(memory, 1, "clk");
		warning_message(NETLIST_ERROR, -1, -1, "Implicit memory %s is not clocked. Padding clock pin.", memory->name);
	}

	char has_port1 = has_addr1 || has_data1 || has_we1 || has_out1;
	char has_port2 = has_addr2 || has_data2 || has_we2 || has_out2;

	if (has_port1)
	{
		if (!has_addr1) add_dummy_input_port_to_implicit_memory(memory, memory->addr_width, "addr1");
		if (!has_data1) add_dummy_input_port_to_implicit_memory(memory, memory->data_width, "data1");
		if (!has_we1)   add_dummy_input_port_to_implicit_memory(memory, 1, "we1");
		if (!has_out1)  add_dummy_output_port_to_implicit_memory(memory, memory->data_width, "out1");
	}

	if (has_port2)
	{
		if (!has_addr2) add_dummy_input_port_to_implicit_memory (memory, memory->addr_width, "addr2");
		if (!has_data2) add_dummy_input_port_to_implicit_memory (memory, memory->data_width, "data2");
		if (!has_we2)   add_dummy_input_port_to_implicit_memory (memory, 1, "we2");
		if (!has_out2)  add_dummy_output_port_to_implicit_memory(memory, memory->data_width, "out2");
	}

	if (!has_port1 || !has_port2)
		collapse_implicit_memory_to_single_port_ram(memory);

	/*
	 *  If this hard block is supported, register it globally and mark
	 *  it as used. (For splitting and BLIF output.)
	 *
	 *  If it isn't supported, it will be automagically blown out
	 *  into soft logic during the partial map.
	 */
	ast_node_t *ast_node = node->related_ast_node;
	char *hard_block_identifier = ast_node->children[0]->types.identifier;
	t_model *hb_model = find_hard_block(hard_block_identifier);
	if (hb_model)
	{
		hb_model->used = 1;
		if (!strcmp(hard_block_identifier, SINGLE_PORT_RAM_string))
			sp_memory_list = insert_in_vptr_list(sp_memory_list, node);
		else
			dp_memory_list = insert_in_vptr_list(dp_memory_list, node);
	}
}

/*
 * Turns the given implicit memory into a single port ram from the
 * default dual port ram. This is a useful optimisation when one port is unused.
 *
 * All implicit memories are constructed initially as
 * dual port rams.
 */
void collapse_implicit_memory_to_single_port_ram(implicit_memory *memory)
{
	nnode_t *node = memory->node;

	// Change the inputs to single port ram mappings by removing
	// the port numbers (1 or 2) from the mappings. (last char)
	int i;
	for (i = 0; i < node->num_input_pins; i++)
	{
		npin_t *pin = node->input_pins[i];
		if (strcmp(pin->mapping, "clk"))
			pin->mapping[strlen(pin->mapping)-1] = 0;
	}

	// Change the outputs to single port ram mappings by removing
	// the port numbers. (last char)
	for (i = 0; i < node->num_output_pins; i++)
	{
		npin_t *pin = node->output_pins[i];
		pin->mapping[strlen(pin->mapping)-1] = 0;
	}

	ast_node_t *ast_node = node->related_ast_node;
	ast_node->children[0]->types.identifier = vtr::strdup(SINGLE_PORT_RAM_string);
}

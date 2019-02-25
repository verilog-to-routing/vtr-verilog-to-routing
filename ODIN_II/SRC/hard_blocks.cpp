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



#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "odin_types.h"
#include "odin_globals.h"
#include "hard_blocks.h"
#include "memories.h"

STRING_CACHE *hard_block_names = NULL;

void cache_hard_block_names();
void register_hb_port_size(t_model_ports *hb_ports, int size);


void register_hb_port_size(t_model_ports *hb_ports, int size)
{
	if(hb_ports)
		hb_ports->size = size;
	/***
	 * else
	 *	TODO error
	 */

}

t_model_ports *get_model_port(t_model_ports *ports, const char *name)
{
	while (ports && strcmp(ports->name, name))
		ports = ports->next;

	return ports;
}

void cache_hard_block_names()
{
	t_model *hard_blocks = NULL;

	hard_blocks = Arch.models;
	hard_block_names = sc_new_string_cache();
	while (hard_blocks)
	{
		sc_add_string(hard_block_names, hard_blocks->name);
		hard_blocks = hard_blocks->next;
	}
}


void register_hard_blocks()
{
	cache_hard_block_names();
	single_port_rams = find_hard_block(SINGLE_PORT_RAM_string);
	dual_port_rams   = find_hard_block(DUAL_PORT_RAM_string);

	if (single_port_rams)
	{
		if (configuration.split_memory_width)
		{
			register_hb_port_size(
				get_model_port(single_port_rams->inputs, "data")
				, 1);

			register_hb_port_size(
				get_model_port(single_port_rams->outputs, "out")
				, 1);
		}

		register_hb_port_size(
			get_model_port(single_port_rams->inputs, "addr")
			,get_sp_ram_split_depth());
	}

	if (dual_port_rams)
	{
		if (configuration.split_memory_width)
		{
			register_hb_port_size(
				get_model_port(dual_port_rams->inputs, "data1")
				, 1);
			register_hb_port_size(
				get_model_port(dual_port_rams->inputs, "data2")
				, 1);

			register_hb_port_size(
				get_model_port(dual_port_rams->outputs, "out1")
				, 1);
			register_hb_port_size(
				get_model_port(dual_port_rams->outputs, "out2")
				, 1);
		}

		int split_depth = get_dp_ram_split_depth();

		register_hb_port_size(
			get_model_port(dual_port_rams->inputs, "addr1")
			, split_depth);
		register_hb_port_size(
			get_model_port(dual_port_rams->inputs, "addr2")
			, split_depth);
	}
}

void deregister_hard_blocks()
{
	sc_free_string_cache(hard_block_names);
	return;
}

t_model* find_hard_block(const char *name)
{
	t_model *hard_blocks;

	hard_blocks = Arch.models;
	while (hard_blocks)
		if (!strcmp(hard_blocks->name, name))
			return hard_blocks;
		else
			hard_blocks = hard_blocks->next;

	return NULL;
}

void define_hard_block(nnode_t *node, FILE *out)
{
	int i, j;
	int index, port;
	int count;
	char buffer[MAX_BUF];

	/* Assert that every hard block has at least an input and output */
	oassert(node->input_port_sizes[0] > 0);
	oassert(node->output_port_sizes[0] > 0);

	//IF the hard_blocks is an adder or a multiplier, we ignore it.(Already print out in define_add_function and define_mult_function)
	if(strcmp(node->related_ast_node->children[0]->types.identifier, "multiply") == 0 || strcmp(node->related_ast_node->children[0]->types.identifier, "adder") == 0)
		return;

	count = fprintf(out, "\n.subckt ");
	count--;
	count += fprintf(out, "%s", node->related_ast_node->children[0]->types.identifier);

	/* print the input port mappings */
	port = index = 0;
	for (i = 0;  i < node->num_input_pins; i++)
	{
		/* Check that the input pin is driven */
		if (node->input_pins[i]->net->driver_pin == NULL
		&& node->input_pins[i]->net != verilog_netlist->zero_net
		&& node->input_pins[i]->net != verilog_netlist->one_net
		&& node->input_pins[i]->net != verilog_netlist->pad_net)
		{
			warning_message(NETLIST_ERROR, -1, -1, "Signal %s is not driven. padding with ground\n", node->input_pins[i]->name);
			add_fanout_pin_to_net(verilog_netlist->zero_net, node->input_pins[i]);
		}

		if (node->input_port_sizes[port] == 1)
			j = odin_sprintf(buffer, " %s=%s", node->input_pins[i]->mapping, node->input_pins[i]->net->driver_pin->node->name);
		else
		{
			if (node->input_pins[i]->net->driver_pin->name != NULL)
				j = odin_sprintf(buffer, " %s[%ld]=%s", node->input_pins[i]->mapping, index, node->input_pins[i]->net->driver_pin->name);
			else
				j = odin_sprintf(buffer, " %s[%ld]=%s", node->input_pins[i]->mapping, index, node->input_pins[i]->net->driver_pin->node->name);
		}

		if (count + j > 79)
		{
			fprintf(out, "\\\n");
			count = 0;
		}
		count += fprintf(out, "%s", buffer);

		index++;
		if (node->input_port_sizes[port] == index)
		{
			index = 0;
			port++;
		}
	}

	/* print the output port mappings */
	port = index = 0;
	for (i = 0; i < node->num_output_pins; i++)
	{
		if (node->output_port_sizes[port] != 1)
			j = odin_sprintf(buffer, " %s[%ld]=%s", node->output_pins[i]->mapping, index, node->output_pins[i]->name);
		else
			j = odin_sprintf(buffer, " %s=%s", node->output_pins[i]->mapping, node->output_pins[i]->name);

		if (count + j > 79)
		{
			fprintf(out, "\\\n");
			count = 0;
		}
		count += fprintf(out, "%s", buffer);

		index++;
		if (node->output_port_sizes[port] == index)
		{
			index = 0;
			port++;
		}
	}

	count += fprintf(out, "\n\n");
	return;
}

void output_hard_blocks(FILE *out)
{
	t_model_ports *hb_ports;
	t_model *hard_blocks;
	char buffer[MAX_BUF];
	int count;
	int i;

	oassert(out != NULL);
	hard_blocks = Arch.models;
	while (hard_blocks != NULL)
	{
		if (hard_blocks->used == 1) /* Hard Block is utilized */
		{
			//IF the hard_blocks is an adder or a multiplier, we ignore it.(Already print out in add_the_blackbox_for_adds and add_the_blackbox_for_mults)
			if(strcmp(hard_blocks->name, "adder") == 0 ||strcmp(hard_blocks->name, "multiply") == 0)
			{
				hard_blocks = hard_blocks->next;
				break;
			}

			fprintf(out, "\n.model %s\n", hard_blocks->name);
			count = fprintf(out, ".inputs");
			hb_ports = hard_blocks->inputs;
			while (hb_ports != NULL)
			{
				for (i = 0; i < hb_ports->size; i++)
				{
					if (hb_ports->size == 1)
						count = count + odin_sprintf(buffer, " %s", hb_ports->name);
					else
						count = count + odin_sprintf(buffer, " %s[%ld]", hb_ports->name, i);

					if (count >= 78)
						count = fprintf(out, " \\\n%s", buffer) - 3;
					else
						fprintf(out, "%s", buffer);
				}
				hb_ports = hb_ports->next;
			}

			count = fprintf(out, "\n.outputs") - 1;
			hb_ports = hard_blocks->outputs;
			while (hb_ports != NULL)
			{
				for (i = 0; i < hb_ports->size; i++)
				{
					if (hb_ports->size == 1)
						count = count + odin_sprintf(buffer, " %s", hb_ports->name);
					else
						count = count + odin_sprintf(buffer, " %s[%ld]", hb_ports->name, i);

					if (count >= 78)
						count = fprintf(out, " \\\n%s", buffer) - 3;
					else
						fprintf(out, "%s", buffer);
				}
				hb_ports = hb_ports->next;
			}

			fprintf(out, "\n.blackbox\n.end\n\n");
		}
		hard_blocks = hard_blocks->next;
	}

	return;
}

void
instantiate_hard_block(nnode_t *node, short mark, netlist_t * /*netlist*/)
{
	int i, port, index;

	port = index = 0;
	/* Give names to the output pins */
	for (i = 0; i < node->num_output_pins;  i++)
	{
		if (node->output_pins[i]->name == NULL)
			node->output_pins[i]->name = make_full_ref_name(node->name, NULL, NULL, node->output_pins[i]->mapping, -1);

		index++;
		if (node->output_port_sizes[port] == index)
		{
			index = 0;
			port++;
		}
	}

	node->traverse_visited = mark;
	return;
}

int
hard_block_port_size(t_model *hb, char *pname)
{
	t_model_ports *tmp;

	if (hb == NULL)
		return 0;

	/* Indicates that the port size is different for this hard block
	 *  depending on the instance of the hard block. May want to extend
	 *  this list of blocks in the future.
	 */
	if ((strcmp(hb->name, SINGLE_PORT_RAM_string) == 0) ||
		(strcmp(hb->name, DUAL_PORT_RAM_string) == 0))
	{
		return -1;
	}

	tmp = hb->inputs;
	while (tmp != NULL)
		if ((tmp->name != NULL) && (strcmp(tmp->name, pname) == 0))
			return tmp->size;
		else
			tmp = tmp->next;

	tmp = hb->outputs;
	while (tmp != NULL)
		if ((tmp->name != NULL) && (strcmp(tmp->name, pname) == 0))
			return tmp->size;
		else
			tmp = tmp->next;

	return 0;
}

enum PORTS
hard_block_port_direction(t_model *hb, char *pname)
{
	t_model_ports *tmp;

	if (hb == NULL)
		return ERR_PORT;

	tmp = hb->inputs;
	while (tmp != NULL)
		if ((tmp->name != NULL) && (strcmp(tmp->name, pname) == 0))
			return tmp->dir;
		else
			tmp = tmp->next;

	tmp = hb->outputs;
	while (tmp != NULL)
		if ((tmp->name != NULL) && (strcmp(tmp->name, pname) == 0))
			return tmp->dir;
		else
			tmp = tmp->next;

	return ERR_PORT;
}

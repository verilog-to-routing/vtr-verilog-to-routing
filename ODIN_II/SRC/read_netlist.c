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

/* This reads a .net file from t-vpack into the OdinII format.  This code is derived from the read_netlist.c in VPR 5.0 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "util.h"
#include "globals.h"
#include "netlist_utils.h"
#include "errors.h"
#include "read_xml_arch_file.h"
#include "read_netlist.h"
#include "ReadLine.h"
#include "string_cache.h"

#ifdef VPR5
enum special_blk { NORMAL = 0, INPAD, OUTPAD };
enum pass { DRIVERS = 0, DRIVEN, STOP };

static FILE *net_file;
int linenum;

char *netlist_one_string = "ONE_VCC_CNS";
char *netlist_zero_string = "ZERO_GND_ZERO";

nnode_t * add_block (char *node_name, char **pin_tokens, t_type_ptr type, short pass, netlist_t *netlist);
static void add_io_node_and_driver ( short io_type, netlist_t *netlist, char *io_pin_name);
void add_subblock_to_node(nnode_t *current_block, char ***token_list, int num_subblocks, t_type_ptr type);

void FreeTokens ( char ***TokensPtr);
int CountTokens ( char **Tokens);
char ** ReadLineTokens ( FILE * InFile, int *LineNum);

static t_type_ptr get_type_by_name (
    const char *name,
    int ntypes,
    const struct s_type_descriptor block_types[],
    t_type_ptr IO_type,
    const char *net_filename,
    int line,
    enum special_blk *overide);

/*---------------------------------------------------------------------------------------------
 * (function: read_netlist)
 *-------------------------------------------------------------------------------------------*/
netlist_t *
read_netlist (
    const char *net_filename,
    int ntypes,
    const struct s_type_descriptor block_types[],
    t_type_ptr IO_type)
{
	int i;
	int num_blocks, subblock_count;
	int prev_line;
	enum special_blk overide;
	char **block_tokens;
	char **pin_tokens;
	char **tokens;
	t_type_ptr type = NULL;
	netlist_t *netlist = allocate_netlist();
	npin_t *new_pin;
	enum pass pass;

	/* create the constant nets */
	/* ZERO */
	netlist->zero_net = allocate_nnet();
	netlist->gnd_node = allocate_nnode();
	netlist->gnd_node->type = GND_NODE;
	allocate_more_node_output_pins(netlist->gnd_node, 1);
	add_output_port_information(netlist->gnd_node, 1);
	new_pin = allocate_npin();
	add_a_output_pin_to_node_spot_idx(netlist->gnd_node, new_pin, 0);
	add_a_driver_pin_to_net(netlist->zero_net, new_pin);
	netlist->gnd_node->name = netlist_zero_string;
	netlist->zero_net->name = netlist_zero_string;
	/* ONE */
	netlist->one_net = allocate_nnet();
	netlist->vcc_node = allocate_nnode();
	netlist->vcc_node->type = VCC_NODE;
	allocate_more_node_output_pins(netlist->vcc_node, 1);
	add_output_port_information(netlist->vcc_node, 1);
	new_pin = allocate_npin();
	add_a_output_pin_to_node_spot_idx(netlist->vcc_node, new_pin, 0);
	add_a_driver_pin_to_net(netlist->one_net, new_pin);
	netlist->vcc_node->name = netlist_one_string;
	netlist->one_net->name = netlist_one_string;

	/* open the file to parse */
	net_file = fopen (net_filename, "r");

	for (pass = 0; pass < STOP; ++pass)
	{
		rewind (net_file);
		linenum = 0;
		num_blocks = 0;
		overide = NORMAL;

		/* Read file line by line */
		block_tokens = ReadLineTokens (net_file, &linenum);
		prev_line = linenum;
		while (block_tokens)
		{
			/* .global directives have special meaning - CLOCKS */
			if (0 == strcmp (block_tokens[0], ".global"))
			{
				if (pass == DRIVEN)
				{	
					/* mark this in the clock list for later aquisition */
					mark_clock_node (netlist, block_tokens[1]);
				}

				/* Don't do any more processing on this */
				FreeTokens (&block_tokens);
				block_tokens = ReadLineTokens (net_file, &linenum);
				prev_line = linenum;
				continue;
			}

			/* read the pin tokens */
			pin_tokens = ReadLineTokens (net_file, &linenum);

			if (CountTokens (block_tokens) != 2)
			{
				error_message(NETLIST_FILE_ERROR, prev_line, -1, "'%s':%d - block type linenum should " "be in form '.type_name block_name'\n", net_filename, prev_line);
			}
			if (NULL == pin_tokens)
			{
				error_message(NETLIST_FILE_ERROR, prev_line, -1, "'%s':%d - blocks must be follow by a 'pinlist:' linenum\n", net_filename, linenum);
			}
			if (0 != strcmp ("pinlist:", pin_tokens[0]))
			{
				error_message(NETLIST_FILE_ERROR, prev_line, -1, "'%s':%d - 'pinlist:' linenum must follow " "block type linenum\n", net_filename, linenum);
			}

			type = get_type_by_name (block_tokens[0], ntypes, block_types, IO_type, net_filename, prev_line, &overide);

			/* Check if we are overiding the pinlist format for this block since it is an INPUT or OUTPUT */
			if (overide)
			{
				if (CountTokens (pin_tokens) != 2)
				{		/* 'pinlist:' and name */
					error_message(NETLIST_FILE_ERROR, prev_line, -1,"'%s':%d - pinlist for .input and .output should " "only have one item.\n", net_filename, linenum);
				}
				if (pass == DRIVERS)
				{
					switch (overide)
					{
						case INPAD:
							add_io_node_and_driver (INPUT_NODE, netlist, block_tokens[1]);
							break;
						case OUTPAD:
							add_io_node_and_driver (OUTPUT_NODE, netlist, block_tokens[1]);
							break;
						default:
							break;
					}
				}
					
				/* need to read the next line */
				tokens = ReadLineTokens (net_file, &linenum);
			}
			else /* this is a block with subblocks in it */
			{
				nnode_t *current_block; 
				char ***tokens_list = NULL;

				if (CountTokens (pin_tokens) != (type->num_pins + 1))
				{
					error_message(NETLIST_FILE_ERROR, prev_line, -1, "'%s':%d - 'pinlist:' linenum has %d pins instead of " "expect %d pins.\n", net_filename, linenum, CountTokens (pin_tokens) - 1, type->num_pins);
				}
	
				/* create the connections and nodes between the clusters */
				current_block = add_block (block_tokens[1], pin_tokens, type, pass, netlist);
	
				/* Use the subblock data to make internal cluster representation */
				tokens = ReadLineTokens (net_file, &linenum);
				prev_line = linenum;
				subblock_count = 0;
				while (tokens && (0 == strcmp (tokens[0], "subblock:")))
				{
					/* Create subblock as internal subgraph of type netlist_t */
					if (DRIVEN == pass)
					{
						tokens_list = (char ***)realloc(tokens_list, sizeof(char**)*(subblock_count+1));
						tokens_list[subblock_count] = tokens;
					}
	
					++subblock_count;		/* Next subblock */
	
					tokens = ReadLineTokens (net_file, &linenum);
					prev_line = linenum;
				}

				/* now process all the recorded data for the tokens making up the subblocks */
				if (DRIVEN == pass)
				{
					add_subblock_to_node(current_block, tokens_list, subblock_count, type); 

					/* free up recorded data */
					for (i = 0; i < subblock_count; i++)
					{
						FreeTokens (&tokens_list[i]);
					}
					if (tokens_list != NULL)
					{
						free(tokens_list);
					}
				}
			}
	
			num_blocks++;			/* End of this block */

			FreeTokens (&block_tokens);
			FreeTokens (&pin_tokens);

			/* done here since the last read of subblocks put us with this linenum */
			block_tokens = tokens;
		}
	}
	
	fclose (net_file);

	return netlist;
}

/*---------------------------------------------------------------------------------------------
 * (function: add_io_node_and_driver )
 *-------------------------------------------------------------------------------------------*/
static void
add_io_node_and_driver (
	short io_type,
	netlist_t *netlist,
	char *io_pin_name
)
{
	long sc_spot;
	nnode_t *new_node;
	npin_t *node_output_pin;
	nnet_t *new_net;

	/* create a new node for this input */
	new_node = allocate_nnode();
	/* create node */
	new_node->related_ast_node = NULL;
	new_node->type = io_type;

	/* create the unique name for this gate */
	new_node->name = make_full_ref_name(NULL, NULL, NULL, io_pin_name, -1);

	/* add node to netlist */
	add_node_to_netlist(netlist, new_node, -1);

	if (io_type == INPUT_NODE)
	{
		/* add the output pin */
		allocate_more_node_output_pins(new_node, 1);
		add_output_port_information(new_node, 1);

		/* add the pin */
		node_output_pin = allocate_npin();
		add_a_output_pin_to_node_spot_idx(new_node, node_output_pin, 0);

		/* now add the net and store it in the string hash */
		new_net = allocate_nnet();
		new_net->name = new_node->name;
		add_a_driver_pin_to_net(new_net, node_output_pin);

		/* add to the driver hash */
		sc_spot = sc_add_string(netlist->nets_sc, new_node->name);
		if (netlist->nets_sc->data[sc_spot] != NULL)
		{
			error_message(NETLIST_ERROR, linenum, -1, "Two netlist outputs with the same name (%s)\n", new_node->name);
		}
		netlist->nets_sc->data[sc_spot] = (void*)new_net;
	}
	else if (io_type == OUTPUT_NODE)
	{
		/* allocate input pins.  Is 1 less than i where the last i is the output name */
		allocate_more_node_input_pins(new_node, 1);
		add_input_port_information(new_node, 1);

		/* add to the driver hash */
		sc_spot = sc_add_string(netlist->out_pins_sc, io_pin_name);
		if (netlist->out_pins_sc->data[sc_spot] != NULL)
		{
			error_message(NETLIST_ERROR, linenum, -1, "Two outputs pins with the same name (%s)\n", io_pin_name);
		}
		netlist->out_pins_sc->data[sc_spot] = (void*)new_node;
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: add_block)
 *-------------------------------------------------------------------------------------------*/
nnode_t *
add_block (char *node_name, char **pin_tokens, t_type_ptr type, short pass, netlist_t *netlist)
{
	long sc_spot;
	nnode_t *new_node;
	npin_t *node_output_pin;
	nnet_t *new_net;
	int num_input_pins = 0;
	int num_output_pins = 0;
	npin_t *node_input_pin = NULL;
	nnet_t *driver_net;
	int k;

	/* now that we know the node name, find it */
	if ((sc_spot = sc_lookup_string(netlist->nodes_sc, node_name)) == -1)
	{
		new_node = allocate_nnode();
		/* create node */
		new_node->related_ast_node = NULL;
		new_node->type = NETLIST_FUNCTION;
	
		/* create the unique name for this gate */
		new_node->name = make_full_ref_name(NULL, NULL, NULL, node_name, -1);

		/* add node to netlist */
		add_node_to_netlist(netlist, new_node, -1);
	}
	else
	{
		/* already made.  Note that the netlist format is the cluster name is the first output name, but as long as the cluster is an output name in its output list, then it is fine. */
		new_node = ((nnode_t*)netlist->nodes_sc->data[sc_spot]);
	}

	/* Examine pin list to determine nets */
	for (k = 1; k < type->num_pins+1; ++k)
	{
		if  (DRIVER == type->class_inf[type->pin_class[k-1]].type)
		{
			if (pass == DRIVERS) 
			{
				oassert(k >= type->num_receivers+1);
				oassert(k-1-type->num_receivers == num_output_pins);
				/* allocate the pins needed even if they're unused */
				allocate_more_node_output_pins(new_node, 1);
				add_output_port_information(new_node, 1);

				if (0 != strcmp ("open", pin_tokens[k]))
				{
					/* Add the driver pin */
					node_output_pin = allocate_npin();
					add_a_output_pin_to_node_spot_idx(new_node, node_output_pin, num_output_pins);
			
					/* now add the net and store it in the string hash */
					new_net = allocate_nnet();
					new_net->name = strdup(pin_tokens[k]);
					add_a_driver_pin_to_net(new_net, node_output_pin);
				
					/* add to the driver hash */
					if ((sc_spot = sc_lookup_string(netlist->out_pins_sc, pin_tokens[k])) != -1)
					{
						/* Special case when we hookup a driver to an output pin */
						nnode_t *output_node = (nnode_t*)netlist->out_pins_sc->data[sc_spot];
						npin_t *node_input_pin;
			
						/* hookup this output net to that node */
						node_input_pin = allocate_npin();
						/* add to the driver net */
						add_a_fanout_pin_to_net(new_net, node_input_pin);
						/* add pin to node */
						add_a_input_pin_to_node_spot_idx(output_node, node_input_pin, 0);
			
						/* add to the driver hash */
						sc_spot = sc_add_string(netlist->nets_sc, pin_tokens[k]);
						if (netlist->nets_sc->data[sc_spot] != NULL)
						{
							error_message(NETLIST_ERROR, linenum, -1, "Two netlist outputs with the same name (%s)\n", pin_tokens[k]);
						}
						netlist->nets_sc->data[sc_spot] = (void*)new_net;
					}
					else
					{
						/* add to the driver hash */
						sc_spot = sc_add_string(netlist->nets_sc, pin_tokens[k]);
						if (netlist->nets_sc->data[sc_spot] != NULL)
						{
							error_message(NETLIST_ERROR, linenum, -1, "Two netlist outputs with the same name (%s)\n", pin_tokens[k]);
						}
						netlist->nets_sc->data[sc_spot] = (void*)new_net;
					}
				}

				num_output_pins ++;
			}
		}
		else 
		{
			if (pass == DRIVEN)
			{
				/* allocate input pins even if it is open. */
				allocate_more_node_input_pins(new_node, 1);
				add_input_port_information(new_node, 1);

				if (0 != strcmp ("open", pin_tokens[k]))
				{
					/* Hookup driven port (input) to a driver net */
					if ((sc_spot = sc_lookup_string(netlist->nets_sc, pin_tokens[k])) == -1)
					{
						error_message(NETLIST_ERROR, linenum, -1, "Netlist input does not exist (%s)\n", pin_tokens[k]);
					}
					driver_net = (nnet_t*)netlist->nets_sc->data[sc_spot];
	
					node_input_pin = allocate_npin();
					node_input_pin->name = strdup(pin_tokens[k]);
					/* add to the driver net */
					add_a_fanout_pin_to_net(driver_net, node_input_pin);
	
					add_a_input_pin_to_node_spot_idx(new_node, node_input_pin, num_input_pins);
				}

				num_input_pins ++;
			}
		}
	}

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: add_subblock_to_node)
 *-------------------------------------------------------------------------------------------*/
void
add_subblock_to_node(nnode_t *current_block, char ***tokens_list, int num_subblocks, t_type_ptr type)
{
	int i, j, k;
	nnode_t **subblock_nodes = (nnode_t**)malloc(sizeof(nnode_t*)*num_subblocks);
	npin_t *node_output_pin;
	nnet_t *new_net;
	int live_index_count;
	int *input_idx = (int*)malloc(sizeof(int)*current_block->num_input_pins);
	int *output_idx = (int*)malloc(sizeof(int)*current_block->num_output_pins);

	/* allocate the internal netlist */
	current_block->internal_netlist = allocate_netlist();
	current_block->internal_netlist->type = type;

	/* create all the inputs and outputs that exist outside the cluster internally */
	live_index_count = 0;
	for (i = 0; i < current_block->num_input_pins; i++)
	{
		if (current_block->input_pins[i] != NULL)
		{
			nnode_t *new_input_node;

			/* create a new node for this input */
			new_input_node = allocate_nnode();
			/* create node */
			new_input_node->related_ast_node = NULL;
			new_input_node->type = INPUT_NODE;
			/* create the unique name for this gate */
			new_input_node->name = make_full_ref_name(NULL, NULL, NULL, current_block->input_pins[i]->net->name, -1);

			/* add node to netlist */
			add_node_to_netlist(current_block->internal_netlist, new_input_node, -1);
	
			/* add the output pin */
			allocate_more_node_output_pins(new_input_node, 1);
			add_output_port_information(new_input_node, 1);
	
			/* add the pin */
			node_output_pin = allocate_npin();
			add_a_output_pin_to_node_spot_idx(new_input_node, node_output_pin, 0);
	
			/* now add the net and store it in the string hash */
			new_net = allocate_nnet();
			new_net->name = new_input_node->name;
			add_a_driver_pin_to_net(new_net, node_output_pin);

			/* record the index externally */
			input_idx[i] = live_index_count;
			live_index_count++;
		}
		else
		{
			input_idx[i] = -1;
		}
	}
	live_index_count = 0;
	for (i = 0; i < current_block->num_output_pins; i++)
	{
		if (current_block->output_pins[i] != NULL)
		{
			nnode_t *new_output_node;
			char *output_name = (char*)malloc(sizeof(char)*(strlen(current_block->output_pins[i]->net->name)+1+4));
			sprintf(output_name, "out:%s", current_block->output_pins[i]->net->name);

			/* create a new node for this input */
			new_output_node = allocate_nnode();
			/* create node */
			new_output_node->related_ast_node = NULL;
			new_output_node->type = OUTPUT_NODE;
			/* create the unique name for this gate */
			new_output_node->name = make_full_ref_name(NULL, NULL, NULL, output_name, -1);

			/* add node to netlist */
			add_node_to_netlist(current_block->internal_netlist, new_output_node, -1);

			/* allocate input pins.  Is 1 less than i where the last i is the output name */
			allocate_more_node_input_pins(new_output_node, 1);
			add_input_port_information(new_output_node, 1);

			/* record the index externally */
			output_idx[i] = live_index_count;
			live_index_count++;
		}
		else
		{
			output_idx[i] = -1;
		}
	}

	/* create the subblocks with their respective inputs and outputs */
	for (i = 0; i < num_subblocks; i++)
	{
		subblock_nodes[i] = allocate_nnode();
		/* the name of the node is the second token since the first is "subblock:" */
		subblock_nodes[i]->name = strdup(tokens_list[i][1]);

		allocate_more_node_input_pins(subblock_nodes[i], type->max_subblock_inputs + 1); /* + 1 for clock */
		allocate_more_node_output_pins(subblock_nodes[i], type->max_subblock_outputs);

		/* create the output pins and nets */
		for (j = 0; j < type->max_subblock_outputs; j++)
		{
			/* Add the driver pin */
			node_output_pin = allocate_npin();
			add_a_output_pin_to_node_spot_idx(subblock_nodes[i], node_output_pin, j);
			
			/* now add the net and store it in the string hash */
			new_net = allocate_nnet();
			add_a_driver_pin_to_net(new_net, node_output_pin);
		}

		/* add the node to the netlist */
		add_node_to_netlist(current_block->internal_netlist, subblock_nodes[i], -1);
	}

	for (i = 0; i < num_subblocks; i++)
	{
		int idx;

		for (k = 0; k < type->max_subblock_inputs; ++k)
		{
			/* Check prefix and load pin num */
			idx = 2 + k;
			if (0 == strncmp ("ble_", tokens_list[i][idx], 4))
			{
				/* IF this is a ble connection then hookup to that ble then hookup this input to the output of the ble number */
				int ble_idx = atoi(tokens_list[i][idx] + 4);	/* Skip the 'ble_' part */
				npin_t *node_input_pin = allocate_npin();

				oassert(subblock_nodes[ble_idx]->num_output_pins == 1);

				/* add to the driver net */
				add_a_fanout_pin_to_net(subblock_nodes[ble_idx]->output_pins[0]->net, node_input_pin);
				/* add pin to node */
				add_a_input_pin_to_node_spot_idx(subblock_nodes[i], node_input_pin, k);
			}
			else if (0 != strcmp ("open", tokens_list[i][idx]))
			{
				/* IF this is connected to an input find out which one */
				int in_idx = input_idx[atoi(tokens_list[i][idx])];
				oassert(in_idx < type->num_receivers);
				oassert(in_idx != -1);

				npin_t *node_input_pin = allocate_npin();

				/* add to the driver net */
				add_a_fanout_pin_to_net(current_block->internal_netlist->top_input_nodes[in_idx]->output_pins[0]->net, node_input_pin);
				/* add pin to node */
				add_a_input_pin_to_node_spot_idx(subblock_nodes[i], node_input_pin, k);
			}
		}
		for (k = 0; k < type->max_subblock_outputs; ++k)
		{
			idx = 2 + type->max_subblock_inputs + k;
			if (0 != strcmp ("open", tokens_list[i][idx]))
			{
				/* IF this output goes to a cluster output pin then hook it up */
				int pin_idx = atoi(tokens_list[i][idx]) - type->num_receivers;
				int out_idx = output_idx[pin_idx];
				nnode_t *output_node;
				npin_t *node_input_pin;
				oassert(out_idx < type->num_drivers);

				if (current_block->output_pins[pin_idx] == NULL)
				{
					/* IF this subblock does not go external */
					continue;
				}

				oassert(out_idx != -1);

				output_node = current_block->internal_netlist->top_output_nodes[out_idx];
			
				/* hookup this output net to that node */
				node_input_pin = allocate_npin();
				/* add to the driver net */
				add_a_fanout_pin_to_net(subblock_nodes[i]->output_pins[k]->net, node_input_pin);
				/* add pin to node */
				add_a_input_pin_to_node_spot_idx(output_node, node_input_pin, k);
			}
		}
		idx = 2 + type->max_subblock_inputs + type->max_subblock_outputs;
		if (0 != strcmp ("open", tokens_list[i][idx]))
		{
			int clock_idx = input_idx[atoi (tokens_list[i][idx]) - (type->num_receivers + type->num_drivers)];
			npin_t *node_input_pin = allocate_npin();
			oassert(clock_idx != -1);

			/* add to the driver net */
			add_a_fanout_pin_to_net(current_block->internal_netlist->top_input_nodes[clock_idx]->output_pins[0]->net, node_input_pin);
			/* add pin to node */
			add_a_input_pin_to_node_spot_idx(subblock_nodes[i], node_input_pin, type->max_subblock_inputs);
		}
	}

	/* free the list */
	free(output_idx);
	free(input_idx);
}

/*---------------------------------------------------------------------------------------------
 * (function: get_type_by_name)
 * From VPR 5.0
 *-------------------------------------------------------------------------------------------*/
static t_type_ptr
get_type_by_name (
    const char *name,
    int ntypes,
    const struct s_type_descriptor block_types[],
    t_type_ptr IO_type,
    const char *net_filename,
    int line,
    enum special_blk *overide)
{

	/* Just does a simple linear search for now */
	int i;

	/* pin_overide is used to specify that the .input and .output
	 * blocks only have one pin specified and should be treated
	 * as if used as a .io with a full pin spec */
	*overide = NORMAL;

	/* .input and .output are special names that map to the basic IO type */
	if (0 == strcmp (".input", name))
	{
		*overide = INPAD;
		return IO_type;
	}
	if (0 == strcmp (".output", name))
	{
		*overide = OUTPAD;
		return IO_type;
	}

	/* Linear type search. Don't really expect to have too many
	 * types for it to be a problem */
	for (i = 0; i < ntypes; ++i)
	{
		if (0 == strcmp (block_types[i].name, name))
		{
			return (block_types + i);
		}
	}

	/* No type matched */
	error_message(NETLIST_FILE_ERROR, line, -1, "'%s':%d - Invalid block type '%s'\n", net_filename, line, name);
	exit (1);
}
#endif

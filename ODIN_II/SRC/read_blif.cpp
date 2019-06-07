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

#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include "odin_globals.h"
#include "read_blif.h"
#include "string_cache.h"
#include "netlist_utils.h"

#include "netlist_utils.h"
#include "odin_types.h"
#include "Hashtable.hpp"
#include "netlist_check.h"
#include "node_creation_library.h"
#include "simulate_blif.h"
#include "vtr_util.h"
#include "vtr_memory.h"

#define TOKENS     " \t\n"
#define GND_NAME   "gnd"
#define VCC_NAME   "vcc"
#define HBPAD_NAME "unconn"

#define READ_BLIF_BUFFER 1048576 // 1MB

long file_line_number;
int line_count;

// Stores pin names of the form port[pin]
typedef struct {
	int count;
	char **names;
	// Maps name to index.
	Hashtable *index;
} hard_block_pins;

// Stores port names, and their sizes.
typedef struct {
	char *signature;
	int count;
	int *sizes;
	char **names;
	// Maps portname to index.
	Hashtable *index;
} hard_block_ports;

// Stores all information pertaining to a hard block model. (.model)
typedef struct {
	char *name;

	hard_block_pins *inputs;
	hard_block_pins *outputs;

	hard_block_ports *input_ports;
	hard_block_ports *output_ports;
} hard_block_model;

// A cache structure for models.
typedef struct {
	hard_block_model **models;
	int count;
	// Maps name to model
	Hashtable *index;
} hard_block_models;


netlist_t * blif_netlist;
bool static skip_reading_bit_map=false;
bool insert_global_clock;


void rb_create_top_driver_nets(const char *instance_name_prefix, Hashtable *output_nets_hash);
void rb_look_for_clocks();// not sure if this is needed
void add_top_input_nodes(FILE *file, Hashtable *output_nets_hash);
void rb_create_top_output_nodes(FILE *file);
int read_tokens (char *buffer, hard_block_models *models, FILE *file, Hashtable *output_nets_hash);
static void dum_parse (char *buffer, FILE *file);
void create_internal_node_and_driver(FILE *file, Hashtable *output_nets_hash);
operation_list assign_node_type_from_node_name(char * output_name);// function will decide the node->type of the given node
operation_list read_bit_map_find_unknown_gate(int input_count, nnode_t * node, FILE *file);
void create_latch_node_and_driver(FILE *file, Hashtable *output_nets_hash);
void create_hard_block_nodes(hard_block_models *models, FILE *file, Hashtable *output_nets_hash);
void hook_up_nets(Hashtable *output_nets_hash);
void hook_up_node(nnode_t *node, Hashtable *output_nets_hash);
char* search_clock_name(FILE *file);
void free_hard_block_model(hard_block_model *model);
char *get_hard_block_port_name(char *name);
long get_hard_block_pin_number(char *original_name);
static int compare_hard_block_pin_names(const void *p1, const void *p2);
hard_block_ports *get_hard_block_ports(char **pins, int count);
Hashtable *index_names(char **names, int count);
Hashtable *associate_names(char **names1, char **names2, int count);
void free_hard_block_pins(hard_block_pins *p);
void free_hard_block_ports(hard_block_ports *p);


hard_block_model *get_hard_block_model(char *name, hard_block_ports *ports, hard_block_models *models);
void add_hard_block_model(hard_block_model *m, hard_block_ports *ports, hard_block_models *models);
char *generate_hard_block_ports_signature(hard_block_ports *ports);
int verify_hard_block_ports_against_model(hard_block_ports *ports, hard_block_model *model);
hard_block_model *read_hard_block_model(char *name_subckt, hard_block_ports *ports, FILE *file);


void free_hard_block_models(hard_block_models *models);

hard_block_models *create_hard_block_models();

int count_blif_lines(FILE *file);

/*
 * Reads a blif file with the given filename and produces
 * a netlist which is referred to by the global variable
 * "blif_netlist".
 */
netlist_t *read_blif()
{
	insert_global_clock = true;
	current_parse_file = 0;
	blif_netlist = allocate_netlist();
	/*Opening the blif file */
	FILE *file = vtr::fopen (configuration.list_of_file_names[current_parse_file].c_str(), "r");
	if (file == NULL)
	{
		error_message(ARG_ERROR, -1, current_parse_file, "cannot open file: %s\n", configuration.list_of_file_names[current_parse_file].c_str());
	}
	int num_lines = count_blif_lines(file);

	Hashtable *output_nets_hash = new Hashtable();

	printf("Reading top level module\n"); fflush(stdout);
	/* create the top level module */
	rb_create_top_driver_nets("top", output_nets_hash);

	/* Extracting the netlist by reading the blif file */
	printf("Reading blif netlist..."); fflush(stdout);

	file_line_number  = 0;
	line_count = 0;
	int position   = -1;
	double time    = wall_time();
	// A cache of hard block models indexed by name. As each one is read, it's stored here to be used again.
	hard_block_models *models = create_hard_block_models();
	printf("\n");
	char buffer[READ_BLIF_BUFFER];
	while (vtr::fgets(buffer, READ_BLIF_BUFFER, file) && read_tokens(buffer, models, file, output_nets_hash))
	{	// Print a progress bar indicating completeness.
		position = print_progress_bar((++line_count)/(double)num_lines, position, 50, wall_time() - time);
	}
	free_hard_block_models(models);
	/* Now look for high-level signals */
	rb_look_for_clocks();
	// We the estimate of completion is rough...make sure we end up at 100%. ;)
	print_progress_bar(1.0, position, 50, wall_time() - time);
	printf("-------------------------------------\n"); fflush(stdout);

	// Outputs netlist graph.
	check_netlist(blif_netlist);
	delete output_nets_hash;
	fclose (file);
	return blif_netlist;
}



/*---------------------------------------------------------------------------------------------
 * (function: read_tokens)
 *
 * Parses the given line from the blif file. Returns true if there are more lines
 * to read.
 *-------------------------------------------------------------------------------------------*/
int read_tokens (char *buffer, hard_block_models *models, FILE *file, Hashtable *output_nets_hash)
{
	/* Figures out which, if any token is at the start of this line and *
	 * takes the appropriate action.                                    */
	char *token = vtr::strtok (buffer, TOKENS, file, buffer);

	if (token)
	{
		if(skip_reading_bit_map && !token && ((token[0] == '0') || (token[0] == '1') || (token[0] == '-')))
		{
			dum_parse(buffer, file);
		}
		else
		{
			skip_reading_bit_map= false;
			if (strcmp (token, ".inputs") == 0)
			{
				add_top_input_nodes(file, output_nets_hash);// create the top input nodes
			}
			else if (strcmp (token, ".outputs") == 0)
			{
				rb_create_top_output_nodes(file);// create the top output nodes
			}
			else if (strcmp (token, ".names") == 0)
			{
				create_internal_node_and_driver(file, output_nets_hash);
			}
			else if (strcmp(token,".latch") == 0)
			{
				create_latch_node_and_driver(file, output_nets_hash);
			}
			else if (strcmp(token,".subckt") == 0)
			{
				create_hard_block_nodes(models, file, output_nets_hash);
			}
			else if (strcmp(token,".end")==0)
			{
				// Marks the end of the main module of the blif
				// Call function to hook up the nets
				hook_up_nets(output_nets_hash);
				return false;
			}
			else if (strcmp(token,".model")==0)
			{
				// Ignore models.
				dum_parse(buffer, file);
			}
		}
	}
	return true;
}


/*---------------------------------------------------------------------------------------------
   * function:assign_node_type_from_node_name(char *)
     This function tries to assign the node->type by looking at the name
     Else return GENERIC
*-------------------------------------------------------------------------------------------*/
operation_list assign_node_type_from_node_name(char * output_name)
{
	//variable to extract the type
	operation_list result = GENERIC;

	int start, end;
	int length_string = strlen(output_name);
	for(start = length_string-1; (start >= 0) && (output_name[start] != '^'); start--);
	for(end   = length_string-1; (end   >= 0) && (output_name[end]   != '~'); end--  );

	if((start < end) && (end > 0))
	{
		// Stores the extracted string
		char *extracted_string = (char*)vtr::calloc(end-start+2, sizeof(char));
		int i, j;
		for(i = start + 1, j = 0; i < end; i++, j++)
		{
			extracted_string[j] = output_name[i];
		}

		extracted_string[j]='\0';
		for(i=0; i<operation_list_END; i++)
		{
			if(!strcmp(extracted_string,operation_list_STR[i][ODIN_LONG_STRING])
			|| !strcmp(extracted_string,operation_list_STR[i][ODIN_SHORT_STRING]))
			{
				result = static_cast<operation_list>(i);
				break;
			}
		}

		vtr::free(extracted_string);
	}
	return result;
}

/*---------------------------------------------------------------------------------------------
   * function:create_latch_node_and_driver
     to create an ff node and driver from that node
     format .latch <input> <output> [<type> <control/clock>] <initial val>
*-------------------------------------------------------------------------------------------*/
void create_latch_node_and_driver(FILE *file, Hashtable *output_nets_hash)
{
	/* Storing the names of the input and the final output in array names */
	char ** names = NULL;       // Store the names of the tokens
	int input_token_count = 0; /*to keep track whether controlling clock is specified or not */
	/*input_token_count=3 it is not and =5 it is */
	char *ptr;
	char buffer[READ_BLIF_BUFFER];
	while ((ptr = vtr::strtok (NULL, TOKENS, file, buffer)) != NULL)
	{
		if(input_token_count == 0)
			names = (char**)vtr::malloc((sizeof(char*)));
		else
			names = (char**)vtr::realloc(names, (sizeof(char*))* (input_token_count + 1));
		names[input_token_count++] = vtr::strdup(ptr);
	}

	/* assigning the new_node */
	if(input_token_count != 5)
	{
		/* supported added for the ABC .latch output without control */
		if(input_token_count == 3)
		{
			char *clock_name = search_clock_name(file);
			input_token_count = 5;
			names = (char**)vtr::realloc(names, sizeof(char*) * input_token_count);

			if(clock_name) names[3] = vtr::strdup(clock_name);
			else           names[3] = NULL;

			names[4] = vtr::strdup(names[2]);
			names[2] = vtr::strdup("re");
		}
		else
		{
			std::string line = "";
			for(int i=0; i< input_token_count; i++)
			{
				line +=  names[i];
				line += " ";
			}

			error_message(NETLIST_ERROR,file_line_number,current_parse_file, "This .latch Format not supported: <%s> \n\t required format :.latch <input> <output> [<type> <control/clock>] <initial val>",
			line.c_str());
		}
	}

	nnode_t *new_node = allocate_nnode();
	new_node->related_ast_node = NULL;
	new_node->type = FF_NODE;
	new_node->edge_type = edge_type_blif_enum(names[2]);

	/* Read in the initial value of the latch.
	   Possible values from a blif file are:
	   0: LOW
	   1: HIGH
	   2: DON'T CARE
	   3: UNKNOWN

	   2 and 3 are treated in the same way */
	int initial_value = atoi(names[4]);
	if(initial_value == 0 || initial_value == 1){
		new_node->initial_value = initial_value;
		new_node->has_initial_value = true;
	}

	/* allocate the output pin (there is always one output pin) */
	allocate_more_output_pins(new_node, 1);
	add_output_port_information(new_node, 1);

	/* allocate the input pin */
	allocate_more_input_pins(new_node,2);/* input[1] is clock */

	/* add the port information */
	int i;
	for(i = 0; i < 2; i++)
	{
		add_input_port_information(new_node,1);
	}

	/* add names and type information to the created input pins */
	npin_t *new_pin = allocate_npin();
	new_pin->name = names[0];
	new_pin->type = INPUT;
	add_input_pin_to_node(new_node, new_pin,0);

	new_pin = allocate_npin();
	new_pin->name = names[3];
	new_pin->type = INPUT;
	add_input_pin_to_node(new_node, new_pin,1);

	/* add a name for the node, keeping the name of the node same as the output */
	new_node->name = make_full_ref_name(names[1],NULL, NULL, NULL,-1);

	/*add this node to blif_netlist as an ff (flip-flop) node */
	blif_netlist->ff_nodes = (nnode_t **)vtr::realloc(blif_netlist->ff_nodes, sizeof(nnode_t*)*(blif_netlist->num_ff_nodes+1));
	blif_netlist->ff_nodes[blif_netlist->num_ff_nodes++] = new_node;
	new_node->file_number = current_parse_file;
	new_node->line_number = line_count;

	/*add name information and a net(driver) for the output */
	nnet_t *new_net = allocate_nnet();
	new_net->name = new_node->name;

	new_pin = allocate_npin();
	new_pin->name = new_node->name;
	new_pin->type = OUTPUT;
	add_output_pin_to_node(new_node, new_pin, 0);
	add_driver_pin_to_net(new_net, new_pin);

	output_nets_hash->add(new_node->name, new_net);

	/* Free the char** names */
	vtr::free(names);
	vtr::free(ptr);
}

/*---------------------------------------------------------------------------------------------
   * function: search_clock_name
     to search the clock if the control in the latch
     is not mentioned
*-------------------------------------------------------------------------------------------*/
char* search_clock_name(FILE* file)
{
	fpos_t pos;
	int last_line = file_line_number;
	fgetpos(file,&pos);
	rewind(file);

	char ** input_names = NULL;
	int input_names_count = 0;
	int found = 0;
	while(!found)
	{
		char buffer[READ_BLIF_BUFFER];
		vtr::fgets(buffer,READ_BLIF_BUFFER,file);

		// not sure if this is needed
		if(feof(file))
			break;

		char *ptr;
		if((ptr = vtr::strtok(buffer, TOKENS, file, buffer)))
		{
			if(!strcmp(ptr,".end"))
				break;

			if(!strcmp(ptr,".inputs"))
			{
				/* store the inputs in array of string */
				while((ptr = vtr::strtok (NULL, TOKENS, file, buffer)))
				{
					input_names = (char**)vtr::realloc(input_names,sizeof(char*) * (input_names_count + 1));
					input_names[input_names_count++] = vtr::strdup(ptr);
				}
			}
			else if(!strcmp(ptr,".names") || !strcmp(ptr,".latch"))
			{
				while((ptr = vtr::strtok (NULL, TOKENS,file, buffer)))
				{
					int i;
					for(i = 0; i < input_names_count; i++)
					{
						if(!strcmp(ptr,input_names[i]))
						{
							vtr::free(input_names[i]);
							input_names[i] = input_names[--input_names_count];
						}
					}
				}
			}
			else if(input_names_count == 1)
			{
				found = 1;
			}
		}
	}
	file_line_number = last_line;
	fsetpos(file,&pos);

	if (found) return input_names[0];
	else       return vtr::strdup(DEFAULT_CLOCK_NAME);
}



/*---------------------------------------------------------------------------------------------
   * function:create_hard_block_nodes
     to create the hard block nodes
*-------------------------------------------------------------------------------------------*/
void create_hard_block_nodes(hard_block_models *models, FILE *file, Hashtable *output_nets_hash)
{
	char buffer[READ_BLIF_BUFFER];
	char *subcircuit_name = vtr::strtok (NULL, TOKENS, file, buffer);

	/* storing the names on the formal-actual parameter */
	char *token;
	int count = 0;
	// Contains strings of the form port[pin]=port~pin
	char **names_parameters = NULL;
	while ((token = vtr::strtok (NULL, TOKENS, file, buffer)) != NULL)
  	{
		names_parameters          = (char**)vtr::realloc(names_parameters, sizeof(char*)*(count + 1));
		names_parameters[count++] = vtr::strdup(token);
  	}

	// Split the name parameters at the equals sign.
	char **mappings = (char**)vtr::malloc(sizeof(char*) * count);
	char **names    = (char**)vtr::malloc(sizeof(char*) * count);
	int i = 0;
	for (i = 0; i < count; i++)
	{
		mappings[i] = vtr::strdup(strtok(names_parameters[i], "="));
		names[i]    = vtr::strdup(strtok(NULL, "="));
	}

	// Associate mappings with their connections.
	Hashtable *mapping_index = associate_names(mappings, names, count);

	// Sort the mappings.
	qsort(mappings,  count,  sizeof(char *), compare_hard_block_pin_names);

	for(i = 0; i < count; i++)
		vtr::free(names_parameters[i]);

	vtr::free(names_parameters);

	// Index the mappings in a hard_block_ports struct.
	hard_block_ports *ports = get_hard_block_ports(mappings, count);

	// Look up the model in the models cache.
 	hard_block_model *model;
 	if (!(model = get_hard_block_model(subcircuit_name, ports, models)))
 	{
 		// If the model isn's present, scan ahead and find it.
 		model = read_hard_block_model(subcircuit_name, ports, file);
 		// Add it to the cache.
 		add_hard_block_model(model, ports, models);
 	}

	nnode_t *new_node = allocate_nnode();

	// Name the node subcircuit_name~hard_block_number so that the name is unique.
	static long hard_block_number = 0;
	odin_sprintf(buffer, "%s~%ld", subcircuit_name, hard_block_number++);
	new_node->name = make_full_ref_name(buffer, NULL, NULL, NULL,-1);

	// Determine the type of hard block.
	char *subcircuit_name_prefix = vtr::strdup(subcircuit_name);
	subcircuit_name_prefix[5] = '\0';
	if (!strcmp(subcircuit_name, "multiply") || !strcmp(subcircuit_name_prefix, "mult_"))
		new_node->type = MULTIPLY;
	else if (!strcmp(subcircuit_name, "adder") || !strcmp(subcircuit_name_prefix, "adder"))
		new_node->type = ADD;
	else if (!strcmp(subcircuit_name, "sub") || !strcmp(subcircuit_name_prefix, "sub"))
			new_node->type = MINUS;
	else
	{
		new_node->type = MEMORY;
		new_node->memory_directory = {};
	}
	vtr::free(subcircuit_name_prefix);

	/* Add input and output ports to the new node. */
	{
		hard_block_ports *p;
		p = model->input_ports;
		for (i = 0; i < p->count; i++)
			add_input_port_information(new_node, p->sizes[i]);

		p = model->output_ports;
		for (i = 0; i < p->count; i++)
			add_output_port_information(new_node, p->sizes[i]);
	}

	// Allocate pins positions.
	if (model->inputs->count  > 0)
		allocate_more_input_pins (new_node, model->inputs->count);
	if (model->outputs->count > 0)
		allocate_more_output_pins(new_node, model->outputs->count);

	// Add input pins.
  	for(i = 0; i < model->inputs->count; i++)
  	{
  		char *mapping = model->inputs->names[i];
  		char *name    = (char *)mapping_index->get(mapping);

  		if (!name)
  			error_message(NETLIST_ERROR, file_line_number, current_parse_file, "Invalid hard block mapping: %s", mapping);

		npin_t *new_pin = allocate_npin();
		new_pin->name = vtr::strdup(name);
		new_pin->type = INPUT;
		new_pin->mapping = get_hard_block_port_name(mapping);

		add_input_pin_to_node(new_node, new_pin, i);
  	}

	// Add output pins, nets, and index each net.
  	for(i = 0; i < model->outputs->count; i++)
  	{
  		char *mapping = model->outputs->names[i];
  		char *name = (char *)mapping_index->get(mapping);

  		if (!name) error_message(NETLIST_ERROR, file_line_number, current_parse_file,"Invalid hard block mapping: %s", model->outputs->names[i]);

		npin_t *new_pin = allocate_npin();
		new_pin->name = vtr::strdup(name);
		new_pin->type = OUTPUT;
		new_pin->mapping = get_hard_block_port_name(mapping);

		add_output_pin_to_node(new_node, new_pin, i);

		nnet_t *new_net = allocate_nnet();
		new_net->name = vtr::strdup(name);

		add_driver_pin_to_net(new_net,new_pin);

		// Index the net by name.
		output_nets_hash->add(name, new_net);
	}

  	// Create a fake ast node.
	new_node->related_ast_node = (ast_node_t *)vtr::calloc(1, sizeof(ast_node_t));
	new_node->related_ast_node->children = (ast_node_t **)vtr::calloc(1,sizeof(ast_node_t *));
	new_node->related_ast_node->children[0] = (ast_node_t *)vtr::calloc(1, sizeof(ast_node_t));
	new_node->related_ast_node->children[0]->types.identifier = vtr::strdup(subcircuit_name);

  	/*add this node to blif_netlist as an internal node */
  	blif_netlist->internal_nodes = (nnode_t **)vtr::realloc(blif_netlist->internal_nodes, sizeof(nnode_t*) * (blif_netlist->num_internal_nodes + 1));
  	blif_netlist->internal_nodes[blif_netlist->num_internal_nodes++] = new_node;
	new_node->file_number = current_parse_file;
	new_node->line_number = line_count;

  	free_hard_block_ports(ports);
  	mapping_index->destroy_free_items();
	delete mapping_index;
  	vtr::free(mappings);
  	vtr::free(names);


}

/*---------------------------------------------------------------------------------------------
   * function:create_internal_node_and_driver
     to create an internal node and driver from that node
*-------------------------------------------------------------------------------------------*/

void create_internal_node_and_driver(FILE *file, Hashtable *output_nets_hash)
{
	/* Storing the names of the input and the final output in array names */
	char *ptr;
	char **names = NULL; // stores the names of the input and the output, last name stored would be of the output
	int input_count = 0;
	char buffer[READ_BLIF_BUFFER];
	while ((ptr = vtr::strtok (NULL, TOKENS, file, buffer)))
	{
		names = (char**)vtr::realloc(names, sizeof(char*) * (input_count + 1));
		names[input_count++]= vtr::strdup(ptr);
	}

	/* assigning the new_node */
	nnode_t *new_node = allocate_nnode();
	new_node->related_ast_node = NULL;

	/* gnd vcc unconn already created as top module so ignore them */
	if (
			   !strcmp(names[input_count-1],"gnd")
			|| !strcmp(names[input_count-1],"vcc")
			|| !strcmp(names[input_count-1],"unconn")
	)
	{
		skip_reading_bit_map = true;
	}
	else
	{
		/* assign the node type by seeing the name */
		operation_list node_type = (operation_list)assign_node_type_from_node_name(names[input_count-1]);

		if(node_type != GENERIC)
		{
			new_node->type = node_type;
			skip_reading_bit_map = true;
		}
		/* Check for GENERIC type , change the node by reading the bit map */
		else if(node_type == GENERIC)
		{
			new_node->type = (operation_list)read_bit_map_find_unknown_gate(input_count-1, new_node, file);
			skip_reading_bit_map = true;
		}

		/* allocate the input pin (= input_count-1)*/
		if (input_count-1 > 0) // check if there is any input pins
		{
			allocate_more_input_pins(new_node, input_count-1);

			/* add the port information */
			if(new_node->type == MUX_2)
			{
				add_input_port_information(new_node, (input_count-1)/2);
				add_input_port_information(new_node, (input_count-1)/2);
			}
			else
			{
				int i;
				for(i = 0; i < input_count-1; i++)
					add_input_port_information(new_node, 1);
			}
		}

		/* add names and type information to the created input pins */
		int i;
		for(i = 0; i <= input_count-2; i++)
		{
			npin_t *new_pin = allocate_npin();
			new_pin->name = names[i];
			new_pin->type = INPUT;
			add_input_pin_to_node(new_node, new_pin, i);
		}

		/* add information for the intermediate VCC and GND node (appears in ABC )*/
		if(new_node->type == GND_NODE)
		{
			allocate_more_input_pins(new_node,1);
			add_input_port_information(new_node, 1);

			npin_t *new_pin = allocate_npin();
			new_pin->name = vtr::strdup(GND_NAME);
			new_pin->type = INPUT;
			add_input_pin_to_node(new_node, new_pin,0);
		}

		if(new_node->type == VCC_NODE)
		{
			allocate_more_input_pins(new_node,1);
			add_input_port_information(new_node, 1);

			npin_t *new_pin = allocate_npin();
			new_pin->name = vtr::strdup(VCC_NAME);
			new_pin->type = INPUT;
			add_input_pin_to_node(new_node, new_pin,0);
		}

		/* allocate the output pin (there is always one output pin) */
		allocate_more_output_pins(new_node, 1);
		add_output_port_information(new_node, 1);

		/* add a name for the node, keeping the name of the node same as the output */
		new_node->name = make_full_ref_name(names[input_count-1],NULL, NULL, NULL,-1);

		/*add this node to blif_netlist as an internal node */
		blif_netlist->internal_nodes = (nnode_t**)vtr::realloc(blif_netlist->internal_nodes, sizeof(nnode_t*)*(blif_netlist->num_internal_nodes+1));
		blif_netlist->internal_nodes[blif_netlist->num_internal_nodes++] = new_node;
		new_node->file_number = current_parse_file;
		new_node->line_number = line_count;

		/*add name information and a net(driver) for the output */

		npin_t *new_pin = allocate_npin();
		new_pin->name = new_node->name;
		new_pin->type = OUTPUT;

		add_output_pin_to_node(new_node, new_pin, 0);

		nnet_t *new_net = allocate_nnet();
		new_net->name = new_node->name;

		add_driver_pin_to_net(new_net,new_pin);

		output_nets_hash->add(new_node->name, new_net);

		/* Free the char** names */
		vtr::free(names);
	}
}

/*
*---------------------------------------------------------------------------------------------
   * function: read_bit_map_find_unknown_gate
     read the bit map for simulation
*-------------------------------------------------------------------------------------------*/
operation_list read_bit_map_find_unknown_gate(int input_count, nnode_t *node, FILE *file)
{
	fpos_t pos;
	int last_line = file_line_number;
	const char *One = "1";
	const char *Zero = "0";
	fgetpos(file,&pos);

	if(!input_count)
	{
		char buffer[READ_BLIF_BUFFER];
		vtr::fgets (buffer, READ_BLIF_BUFFER, file);

		file_line_number = last_line;
		fsetpos(file,&pos);

		char *ptr = vtr::strtok(buffer,"\t\n", file, buffer);
		if      (!strcmp(ptr," 0")) return GND_NODE;
		else if (!strcmp(ptr," 1")) return VCC_NODE;
		else if (!ptr)              return GND_NODE;
		else                        return VCC_NODE;
	}

	char **bit_map = NULL;
	char *output_bit_map = NULL;// to distinguish whether for the bit_map output is 1 or 0
	int line_count_bitmap = 0; //stores the number of lines in a particular bit map
	char buffer[READ_BLIF_BUFFER];
	while(1)
	{
		vtr::fgets (buffer, READ_BLIF_BUFFER, file);
		if(!(buffer[0] == '0' || buffer[0] == '1' || buffer[0] == '-'))
			break;

		bit_map = (char**)vtr::realloc(bit_map,sizeof(char*) * (line_count_bitmap + 1));
		bit_map[line_count_bitmap++] = vtr::strdup(vtr::strtok(buffer,TOKENS, file, buffer));
		if (output_bit_map != NULL) vtr::free(output_bit_map);
		output_bit_map = vtr::strdup(vtr::strtok(NULL,TOKENS, file, buffer));
	}

	if (!strcmp(output_bit_map, One))
	{
		vtr::free(output_bit_map);
		output_bit_map = vtr::strdup(One);
		node->generic_output = 1;
	}
	else
	{
		vtr::free(output_bit_map);
		output_bit_map = vtr::strdup(Zero);
		node->generic_output = 0;
	}

	file_line_number = last_line;
	fsetpos(file,&pos);

	/*Patern recognition for faster simulation*/
	if(node->generic_output){
		//On-gate recognition
		//TODO move off-logic parts to appropriate code block
		/* Single line bit map : */
		if(line_count_bitmap == 1)
		{
			// GT
			if(!strcmp(bit_map[0],"100"))
				return GT;

			// LT
			if(!strcmp(bit_map[0],"010"))
				return LT;

			/* LOGICAL_AND and LOGICAL_NAND for ABC*/
			int i;
			for(i = 0; i < input_count && bit_map[0][i] == '1'; i++);

			if(i == input_count)
			{
				if (!strcmp(output_bit_map,"1"))
					return LOGICAL_AND;
				else if (!strcmp(output_bit_map,"0"))
					return LOGICAL_NAND;
			}

			/* BITWISE_NOT */
			if(!strcmp(bit_map[0],"0"))
				return BITWISE_NOT;

			/* LOGICAL_NOR and LOGICAL_OR for ABC */
			for(i = 0; i < input_count && bit_map[0][i] == '0'; i++);
			if(i == input_count)
			{
				if (!strcmp(output_bit_map,"1"))
					return LOGICAL_NOR;
				else if (!strcmp(output_bit_map,"0"))
					return LOGICAL_OR;
			}
		}
		/* Assumption that bit map is in order when read from blif */
		else if(line_count_bitmap == 2)
		{
			/* LOGICAL_XOR */
			if((strcmp(bit_map[0],"01")==0) && (strcmp(bit_map[1],"10")==0)) return LOGICAL_XOR;
			/* LOGICAL_XNOR */
			if((strcmp(bit_map[0],"00")==0) && (strcmp(bit_map[1],"11")==0)) return LOGICAL_XNOR;
		}
		else if (line_count_bitmap == 4)
		{
			/* ADDER_FUNC */
			if (
					   (!strcmp(bit_map[0],"001"))
					&& (!strcmp(bit_map[1],"010"))
					&& (!strcmp(bit_map[2],"100"))
					&& (!strcmp(bit_map[3],"111"))
			)
				return ADDER_FUNC;
			/* CARRY_FUNC */
			if(
					   (!strcmp(bit_map[0],"011"))
					&& (!strcmp(bit_map[1],"101"))
					&& (!strcmp(bit_map[2],"110"))
					&& (!strcmp(bit_map[3],"111"))
			)
				return 	CARRY_FUNC;
			/* LOGICAL_XOR */
			if(
					   (!strcmp(bit_map[0],"001"))
					&& (!strcmp(bit_map[1],"010"))
					&& (!strcmp(bit_map[2],"100"))
					&& (!strcmp(bit_map[3],"111"))
			)
				return 	LOGICAL_XOR;
			/* LOGICAL_XNOR */
			if(
					   (!strcmp(bit_map[0],"000"))
					&& (!strcmp(bit_map[1],"011"))
					&& (!strcmp(bit_map[2],"101"))
					&& (!strcmp(bit_map[3],"110"))
			)
				return 	LOGICAL_XNOR;
		}


		if(line_count_bitmap == input_count)
		{
			/* LOGICAL_OR */
			int i;
			for(i = 0; i < line_count_bitmap; i++)
			{
				if(bit_map[i][i] == '1')
				{
					int j;
					for(j = 1; j < input_count; j++)
						if(bit_map[i][(i+j)% input_count]!='-')
							break;

					if(j != input_count)
						break;
				}
				else
				{
					break;
				}
			}

			if(i == line_count_bitmap)
				return LOGICAL_OR;

			/* LOGICAL_NAND */
			for(i = 0; i < line_count_bitmap; i++)
			{
				if(bit_map[i][i]=='0')
				{
					int j;
					for(j = 1; j < input_count; j++)
						if(bit_map[i][(i+j)% input_count]!='-')
							break;

					if(j != input_count) break;
				}
				else
				{
					break;
				}
			}

			if(i == line_count_bitmap)
				return LOGICAL_NAND;
		}

		/* MUX_2 */
		if(line_count_bitmap*2 == input_count)
		{
			int i;
			for(i = 0; i < line_count_bitmap; i++)
			{
				if (
						   (bit_map[i][i]=='1')
						&& (bit_map[i][i+line_count_bitmap] =='1')
				)
				{
					int j;
					for (j = 1; j < line_count_bitmap; j++)
					{
						if (
								   (bit_map[i][ (i+j) % line_count_bitmap] != '-')
								|| (bit_map[i][((i+j) % line_count_bitmap) + line_count_bitmap] != '-')
						)
						{
							break;
						}
					}

					if(j != input_count)
						break;
				}
				else
				{
					break;
				}
			}

			if(i == line_count_bitmap)
				return MUX_2;
		}
	} else if (node->generic_output == 0){
		//Off-gate recognition
		//TODO
	}

	 /* assigning the bit_map to the node if it is GENERIC */

	node->bit_map = bit_map;
	node->bit_map_line_count = line_count_bitmap;
	return GENERIC;
}

/*
*---------------------------------------------------------------------------------------------
   * function: add_top_input_nodes
     to add the top level inputs to the netlist
*-------------------------------------------------------------------------------------------*/
static void build_top_input_node(const char *name_str, Hashtable *output_nets_hash)
{
	char *temp_string = make_full_ref_name(name_str, NULL, NULL,NULL, -1);

	/* create a new top input node and net*/

	nnode_t *new_node = allocate_nnode();

	new_node->related_ast_node = NULL;
	new_node->type = INPUT_NODE;

	/* add the name of the input variable */
	new_node->name = temp_string;

	new_node->file_number = current_parse_file;
	new_node->line_number = line_count;

	/* allocate the pins needed */
	allocate_more_output_pins(new_node, 1);
	add_output_port_information(new_node, 1);

	/* Create the pin connection for the net */
	npin_t *new_pin = allocate_npin();
	new_pin->name = vtr::strdup(temp_string);
	new_pin->type = OUTPUT;

	/* hookup the pin, net, and node */
	add_output_pin_to_node(new_node, new_pin, 0);

	nnet_t *new_net = allocate_nnet();
	new_net->name = vtr::strdup(temp_string);

	add_driver_pin_to_net(new_net, new_pin);

	blif_netlist->top_input_nodes = (nnode_t**)vtr::realloc(blif_netlist->top_input_nodes, sizeof(nnode_t*)*(blif_netlist->num_top_input_nodes+1));
	blif_netlist->top_input_nodes[blif_netlist->num_top_input_nodes++] = new_node;

	//long sc_spot = sc_add_string(output_nets_sc, temp_string);
	//if (output_nets_sc->data[sc_spot])
	//warning_message(NETLIST_ERROR,linenum,-1, "Net (%s) with the same name already created\n",temp_string);

	//output_nets_sc->data[sc_spot] = new_net;

	output_nets_hash->add(temp_string, new_net);
}

void add_top_input_nodes(FILE *file, Hashtable *output_nets_hash)
{
	/**
	 * insert a global clock for fall back. 
	 * in case of undriven internal clocks, they will attach to the global clock
	 * this also fix the issue of constant verilog (no input)
	 * that cannot simulate due to empty input vector
	 */
	if(insert_global_clock)
	{
		insert_global_clock = false;
		build_top_input_node(DEFAULT_CLOCK_NAME, output_nets_hash);
	}

	char *ptr;
	char buffer[READ_BLIF_BUFFER];
	while ((ptr = vtr::strtok (NULL, TOKENS, file, buffer)))
	{
		build_top_input_node(ptr, output_nets_hash);
	}
}

/*---------------------------------------------------------------------------------------------
   * function: create_top_output_nodes
     to add the top level outputs to the netlist
*-------------------------------------------------------------------------------------------*/
void rb_create_top_output_nodes(FILE *file)
{
	char *ptr;
	char buffer[READ_BLIF_BUFFER];

	while ((ptr = vtr::strtok (NULL, TOKENS, file, buffer)))
	{
		char *temp_string = make_full_ref_name(ptr, NULL, NULL,NULL, -1);;

		/*add_a_fanout_pin_to_net((nnet_t*)output_nets_sc->data[sc_spot], new_pin);*/

		/* create a new top output node and */
		nnode_t *new_node = allocate_nnode();
		new_node->related_ast_node = NULL;
		new_node->type = OUTPUT_NODE;

		/* add the name of the output variable */
		new_node->name = temp_string;

		/* allocate the input pin needed */
		allocate_more_input_pins(new_node, 1);
		add_input_port_information(new_node, 1);

		/* Create the pin connection for the net */
		npin_t *new_pin = allocate_npin();
		new_pin->name   = temp_string;
		/* hookup the pin, net, and node */
		add_input_pin_to_node(new_node, new_pin, 0);

		/*adding the node to the blif_netlist output nodes
		add_node_to_netlist() function can also be used */
		blif_netlist->top_output_nodes = (nnode_t**)vtr::realloc(blif_netlist->top_output_nodes, sizeof(nnode_t*)*(blif_netlist->num_top_output_nodes+1));
		blif_netlist->top_output_nodes[blif_netlist->num_top_output_nodes++] = new_node;
		new_node->file_number = current_parse_file;
		new_node->line_number = line_count;
	}
}


/*---------------------------------------------------------------------------------------------
   * (function: look_for_clocks)
 *-------------------------------------------------------------------------------------------*/

void rb_look_for_clocks()
{
	int i;
	for (i = 0; i < blif_netlist->num_ff_nodes; i++)
	{
		if (blif_netlist->ff_nodes[i]->input_pins[1]->net->driver_pin->node->type != CLOCK_NODE)
		{
			blif_netlist->ff_nodes[i]->input_pins[1]->net->driver_pin->node->type = CLOCK_NODE;
		}
	}

}

/*
----------------------------------------------------------------------------
function: Creates the drivers for the top module
   Top module is :
                * Special as all inputs are actually drivers.
                * Also make the 0 and 1 constant nodes at this point.
---------------------------------------------------------------------------
*/

void rb_create_top_driver_nets(const char *instance_name_prefix, Hashtable *output_nets_hash)
{
	npin_t *new_pin;
	/* create the constant nets */

	/* ZERO net */
	/* description given for the zero net is same for other two */
	blif_netlist->zero_net = allocate_nnet(); // allocate memory to net pointer
	blif_netlist->gnd_node = allocate_nnode(); // allocate memory to node pointer
	blif_netlist->gnd_node->type = GND_NODE;  // mark the type
	allocate_more_output_pins(blif_netlist->gnd_node, 1);// alloacate 1 output pin pointer to this node
	add_output_port_information(blif_netlist->gnd_node, 1);// add port info. this port has 1 pin ,till now number of port for this is one
	new_pin = allocate_npin();
	add_output_pin_to_node(blif_netlist->gnd_node, new_pin, 0);// add this pin to output pin pointer array of this node
	add_driver_pin_to_net(blif_netlist->zero_net,new_pin);// add this pin to net as driver pin

	/*ONE net*/
	blif_netlist->one_net = allocate_nnet();
	blif_netlist->vcc_node = allocate_nnode();
	blif_netlist->vcc_node->type = VCC_NODE;
	allocate_more_output_pins(blif_netlist->vcc_node, 1);
	add_output_port_information(blif_netlist->vcc_node, 1);
	new_pin = allocate_npin();
	add_output_pin_to_node(blif_netlist->vcc_node, new_pin, 0);
	add_driver_pin_to_net(blif_netlist->one_net, new_pin);

	/* Pad net */
	blif_netlist->pad_net = allocate_nnet();
	blif_netlist->pad_node = allocate_nnode();
	blif_netlist->pad_node->type = PAD_NODE;
	allocate_more_output_pins(blif_netlist->pad_node, 1);
	add_output_port_information(blif_netlist->pad_node, 1);
	new_pin = allocate_npin();
	add_output_pin_to_node(blif_netlist->pad_node, new_pin, 0);
	add_driver_pin_to_net(blif_netlist->pad_net, new_pin);

	/* CREATE the driver for the ZERO */
	blif_netlist->zero_net->name = make_full_ref_name(instance_name_prefix, NULL, NULL, zero_string, -1);
	output_nets_hash->add(GND_NAME, blif_netlist->zero_net);

	/* CREATE the driver for the ONE and store twice */
	blif_netlist->one_net->name = make_full_ref_name(instance_name_prefix, NULL, NULL, one_string, -1);
	output_nets_hash->add(VCC_NAME, blif_netlist->one_net);

	/* CREATE the driver for the PAD */
	blif_netlist->pad_net->name = make_full_ref_name(instance_name_prefix, NULL, NULL, pad_string, -1);
	output_nets_hash->add(HBPAD_NAME, blif_netlist->pad_net);

	blif_netlist->vcc_node->name = vtr::strdup(VCC_NAME);
	blif_netlist->gnd_node->name = vtr::strdup(GND_NAME);
	blif_netlist->pad_node->name = vtr::strdup(HBPAD_NAME);

}

/*---------------------------------------------------------------------------------------------
 * (function: dum_parse)
 *-------------------------------------------------------------------------------------------*/
static void dum_parse (char *buffer, FILE *file)
{
	/* Continue parsing to the end of this (possibly continued) line. */
	while (vtr::strtok (NULL, TOKENS, file, buffer));
}



/*---------------------------------------------------------------------------------------------
 * function: hook_up_nets()
 * find the output nets and add the corresponding nets
 *-------------------------------------------------------------------------------------------*/
void hook_up_nets(Hashtable *output_nets_hash)
{
	nnode_t **node_sets[] = {blif_netlist->internal_nodes,     blif_netlist->ff_nodes,     blif_netlist->top_output_nodes};
	int          counts[] = {blif_netlist->num_internal_nodes, blif_netlist->num_ff_nodes, blif_netlist->num_top_output_nodes};
	int        num_sets   = 3;

	/* hook all the input pins in all the internal nodes to the net */
	int i;
	for (i = 0; i < num_sets; i++)
	{
		int j;
		for(j = 0; j < counts[i]; j++)
		{
			nnode_t *node = node_sets[i][j];
			hook_up_node(node, output_nets_hash);
		}
	}
}

/*
 * Connect the given node's input pins to their corresponding nets by
 * looking each one up in the output_nets_sc.
 */
void hook_up_node(nnode_t *node, Hashtable *output_nets_hash)
{
	int j;
	for(j = 0; j < node->num_input_pins; j++)
	{
		npin_t *input_pin = node->input_pins[j];

		nnet_t *output_net = (nnet_t *)output_nets_hash->get(input_pin->name);

		if(!output_net)
			error_message(NETLIST_ERROR,file_line_number, current_parse_file, "Error: Could not hook up the pin %s: not available.", input_pin->name);

		add_fanout_pin_to_net(output_net, input_pin);
	}
}

/*
 * Scans ahead in the given file to find the
 * model for the hard block by the given name.
 * Returns the file to its original position when finished.
 */
hard_block_model *read_hard_block_model(char *name_subckt, hard_block_ports *ports, FILE *file)
{
	// Store the current position in the file.
	fpos_t pos;
	int last_line = file_line_number;
	fgetpos(file,&pos);

	hard_block_model *model;

	while(1) {
		model = NULL;

		// Search the file for .model followed buy the subcircuit name.
		char buffer[READ_BLIF_BUFFER];
		while (vtr::fgets(buffer, READ_BLIF_BUFFER, file))
		{
			char *token = vtr::strtok(buffer,TOKENS, file, buffer);
			// match .model followed buy the subcircuit name.
			if (token && !strcmp(token,".model") && !strcmp(vtr::strtok(NULL,TOKENS, file, buffer), name_subckt))
			{
				model = (hard_block_model *)vtr::malloc(sizeof(hard_block_model));
				model->name = vtr::strdup(name_subckt);
				model->inputs = (hard_block_pins *)vtr::malloc(sizeof(hard_block_pins));
				model->inputs->count = 0;
				model->inputs->names = NULL;

				model->outputs = (hard_block_pins *)vtr::malloc(sizeof(hard_block_pins));
				model->outputs->count = 0;
				model->outputs->names = NULL;

				// Read the inputs and outputs.
				while (vtr::fgets(buffer, READ_BLIF_BUFFER, file))
				{
					char *first_word = vtr::strtok(buffer, TOKENS, file, buffer);
					if(!strcmp(first_word, ".inputs"))
					{
						char *name;
						while ((name = vtr::strtok(NULL, TOKENS, file, buffer)))
						{
							model->inputs->names = (char **)vtr::realloc(model->inputs->names, sizeof(char *) * (model->inputs->count + 1));
							model->inputs->names[model->inputs->count++] = vtr::strdup(name);
						}
					}
					else if(!strcmp(first_word, ".outputs"))
					{
						char *name;
						while ((name = vtr::strtok(NULL, TOKENS, file, buffer)))
						{
							model->outputs->names = (char **)vtr::realloc(model->outputs->names, sizeof(char *) * (model->outputs->count + 1));
							model->outputs->names[model->outputs->count++] = vtr::strdup(name);
						}
					}
					else if(!strcmp(first_word, ".end"))
					{
						break;
					}
				}
				break;
			}
		}

		if(!model || feof(file))
			error_message(NETLIST_ERROR, last_line, current_parse_file, "A subcircuit model for '%s' with matching ports was not found.",name_subckt);

		// Sort the names.
		qsort(model->inputs->names,  model->inputs->count,  sizeof(char *), compare_hard_block_pin_names);
		qsort(model->outputs->names, model->outputs->count, sizeof(char *), compare_hard_block_pin_names);

		// Index the names.
		model->inputs->index  = index_names(model->inputs->names, model->inputs->count);
		model->outputs->index = index_names(model->outputs->names, model->outputs->count);

		// Organise the names into ports.
		model->input_ports  = get_hard_block_ports(model->inputs->names,  model->inputs->count);
		model->output_ports = get_hard_block_ports(model->outputs->names, model->outputs->count);

		// Check that the model we've read matches the ports of the instance we are trying to match.
		if (verify_hard_block_ports_against_model(ports, model))
		{
			break;
		}
		else
		{	// If not, free it, and keep looking.
			free_hard_block_model(model);
		}
	}

	// Restore the original position in the file.
	file_line_number = last_line;
 	fsetpos(file,&pos);

	return model;
}

/*
 * Callback function for qsort which compares pin names
 * of the form port_name[pin_number] primarily
 * on the port_name, and on the pin_number if the port_names
 * are identical.
 */
static int compare_hard_block_pin_names(const void *p1, const void *p2)
{
	char *name1 = *(char * const *)p1;
	char *name2 = *(char * const *)p2;

	char *port_name1 = get_hard_block_port_name(name1);
	char *port_name2 = get_hard_block_port_name(name2);
	int portname_difference = strcmp(port_name1, port_name2);
	vtr::free(port_name1);
	vtr::free(port_name2);

	// If the portnames are the same, compare the pin numbers.
	if (!portname_difference)
	{
		int n1 = get_hard_block_pin_number(name1);
		int n2 = get_hard_block_pin_number(name2);
		return n1 - n2;
	}
	else
	{
		return portname_difference;
	}
}

/*
 * Creates a hashtable index for an array of strings of
 * the form names[i]=>i.
 */
Hashtable *index_names(char **names, int count)
{
	Hashtable *index = new Hashtable();
	for (long i = 0; i < count; i++)
	{
		int *offset = (int *)vtr::malloc(sizeof(int));
		*offset = i;
		index->add(names[i], offset);
	}
	return index;
}

/*
 * Create an associative index of names1[i]=>names2[i]
 */
Hashtable *associate_names(char **names1, char **names2, int count)
{
	Hashtable *index = new Hashtable();
	for (long i = 0; i < count; i++)
		index->add(names1[i], names2[i]);

	return index;
}


/*
 * Organises the given strings representing pin names on a hard block
 * model into ports, and indexes the ports by name. Returns the organised
 * ports as a hard_block_ports struct.
 */
hard_block_ports *get_hard_block_ports(char **pins, int count)
{
	// Count the input port sizes.
	hard_block_ports *ports = (hard_block_ports *)vtr::malloc(sizeof(hard_block_ports));
	ports->count = 0;
	ports->sizes = 0;
	ports->names = 0;
	char *prev_portname = 0;
	int i;
	for (i = 0; i < count; i++)
	{
		char *portname = get_hard_block_port_name(pins[i]);
		// Compare the part of the name before the "["
		if (!i || strcmp(prev_portname, portname))
		{
			ports->sizes = (int *)vtr::realloc(ports->sizes, sizeof(int) * (ports->count + 1));
			ports->names = (char **)vtr::realloc(ports->names, sizeof(char *) * (ports->count + 1));

			ports->sizes[ports->count] = 0;
			ports->names[ports->count] = portname;
			ports->count++;
		}

		ports->sizes[ports->count-1]++;
		prev_portname = portname;
	}

	ports->signature = generate_hard_block_ports_signature(ports);
	ports->index     = index_names(ports->names, ports->count);

	return ports;
}

/*
 * Check for inconsistencies between the hard block model and the ports found
 * in the hard block instance. Returns false if differences are found.
 */
int verify_hard_block_ports_against_model(hard_block_ports *ports, hard_block_model *model)
{
	hard_block_ports *port_sets[] = {model->input_ports, model->output_ports};
	int i;
	for (i = 0; i < 2; i++)
	{
		hard_block_ports *p = port_sets[i];
		int j;
		for (j = 0; j < p->count; j++)
		{
			// Look up each port from the model in "ports"
			char *name = p->names[j];
			int   size = p->sizes[j];
			int  *idx  = (int *)ports->index->get(name);
			// Model port not specified in ports.
			if (!idx)
			{
				//printf("Model port not specified in ports. %s\n", name);
				return false;
			}

			// Make sure they match in size.
			int instance_size = ports->sizes[*idx];
			// Port sizes differ.
			if (size != instance_size)
			{
				//printf("Port sizes differ. %s\n", name);
				return false;
			}
		}
	}

	hard_block_ports *in = model->input_ports;
	hard_block_ports *out = model->output_ports;
	int j;
	for (j = 0; j < ports->count; j++)
	{
		// Look up each port from the subckt to make sure it appears in the model.
		char *name   = ports->names[j];
		int *in_idx  = (int *)in->index->get(name);
		int *out_idx = (int *)out->index->get(name);
		// Port does not appear in the model.
		if (!in_idx && !out_idx)
		{
			//printf("Port does not appear in the model. %s\n", name);
			return false;
		}
	}

	return true;
}

/*
 * Generates string which represents the geometry of the given hard block ports.
 */
char *generate_hard_block_ports_signature(hard_block_ports *ports)
{
	char buffer[READ_BLIF_BUFFER];
	buffer[0] = '\0';

	strcat(buffer, "_");

	int j;
	for (j = 0; j < ports->count; j++)
	{
		char buffer1[READ_BLIF_BUFFER];
		odin_sprintf(buffer1, "%s_%ld_", ports->names[j], ports->sizes[j]);
		strcat(buffer, buffer1);
	}
	return vtr::strdup(buffer);
}

/*
 * Gets the text in the given string which occurs
 * before the first instance of "[". The string is
 * presumably of the form "port[pin_number]"
 *
 * The retuned string is strduped and must be freed.
 * The original string is unaffected.
 */
char *get_hard_block_port_name(char *name)
{
	name = vtr::strdup(name);
	if (strchr(name,'['))
		return strtok(name,"[");
	else
		return name;
}

/*
 * Parses a port name of the form port[pin_number]
 * and returns the pin number as a long. Returns -1
 * if there is no [pin_number] in the name. Throws an
 * error if pin_number is not parsable as a long.
 *
 * The original string is unaffected.
 */
long get_hard_block_pin_number(char *original_name)
{
	if (!strchr(original_name,'['))
		return -1;

	char *name = vtr::strdup(original_name);
	strtok(name,"[");
	char *endptr;
	char *pin_number_string = strtok(NULL,"]");
	long pin_number = strtol(pin_number_string, &endptr, 10);

	if (pin_number_string == endptr)
		error_message(NETLIST_ERROR,file_line_number, current_parse_file,"The given port name \"%s\" does not contain a valid pin number.", original_name);

	vtr::free(name);

	return pin_number;
}

/*
 * Adds the given model to the hard block model cache.
 */
void add_hard_block_model(hard_block_model *m, hard_block_ports *ports, hard_block_models *models)
{
	char needle[READ_BLIF_BUFFER];
	needle[0] = '\0';
	strcat(needle, m->name);
	strcat(needle, ports->signature);
	models->models = (hard_block_model **)vtr::realloc(models->models, (models->count * sizeof(hard_block_model *)) + 1);
	models->models[models->count++] = m;
	models->index->add(needle, m);
}

/*
 * Looks up a hard block model by name. Returns null if the
 * model is not found.
 */
hard_block_model *get_hard_block_model(char *name, hard_block_ports *ports, hard_block_models *models)
{
	char needle[READ_BLIF_BUFFER];
	needle[0] = '\0';
	strcat(needle, name);
	strcat(needle, ports->signature);
	return (hard_block_model *)models->index->get(needle);
}

/*
 * Creates a new hard block model cache.
 */
hard_block_models *create_hard_block_models()
{
	hard_block_models *m = (hard_block_models *)vtr::malloc(sizeof(hard_block_models));
	m->models = 0;
	m->count  = 0;
	m->index  = new Hashtable();

	return m;
}

/*
 * Counts the number of lines in the given blif file
 * before a .end token is hit.
 */
int count_blif_lines(FILE *file)
{
	int num_lines = 0;
	char buffer[READ_BLIF_BUFFER];
	while (vtr::fgets(buffer, READ_BLIF_BUFFER, file))
	{
		if (strstr(buffer, ".end"))
			break;
		num_lines++;
	}
	rewind(file);
	return num_lines;
}

/*
 * Frees the hard block model cache, freeing
 * all encapsulated hard block models.
 */
void free_hard_block_models(hard_block_models *models)
{
	//does not delete the items in the hash
	delete models->index;
	int i;
	for (i = 0; i < models->count; i++)
		free_hard_block_model(models->models[i]);

	vtr::free(models->models);
	vtr::free(models);
}


/*
 * Frees a hard_block_model.
 */
void free_hard_block_model(hard_block_model *model)
{
	free_hard_block_pins(model->inputs);
	free_hard_block_pins(model->outputs);

	free_hard_block_ports(model->input_ports);
	free_hard_block_ports(model->output_ports);

	vtr::free(model);
}

/*
 * Frees hard_block_pins
 */
void free_hard_block_pins(hard_block_pins *p)
{
	while (p->count--)
		vtr::free(p->names[p->count]);

	vtr::free(p->names);

	p->index->destroy_free_items();
	delete p->index;
	vtr::free(p);
}

/*
 * Frees hard_block_ports
 */
void free_hard_block_ports(hard_block_ports *p)
{
	while(p->count--)
		vtr::free(p->names[p->count]);

	vtr::free(p->signature);
	vtr::free(p->names);
	vtr::free(p->sizes);

	p->index->destroy_free_items();
	delete p->index;
	vtr::free(p);
}

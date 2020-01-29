
#include "odin_types.h"
#include "odin_globals.h"

#include "netlist_utils.h"
#include "node_creation_library.h"
#include "odin_util.h"

#include "partial_map.h"
#include "multipliers.h"
#include "hard_blocks.h"
#include "math.h"
#include "memories.h"
#include "adders.h"
#include "subtractions.h"
#include "vtr_memory.h"
#include "vtr_util.h"

#include "ga_adder.hpp"

adder_t **adders_list;
adder_type_e *chromosome;
long num_of_adders = 0;

adder_type_e *fittest;
int chromosome_fitness;

npin_t **cloud_pins_list;
nnet_t  **cloud_nets_list;
nnode_t **cloud_nodes_list;

int num_cloud_pins;
int num_cloud_nets;
int num_cloud_nodes;

void instantiate_soft_adder(nnode_t *node, short mark, netlist_t *netlist)
{
	// instantiate_add_w_carry(node, traverse_number, netlist);
	adders_list = (adder_t **)vtr::realloc (adders_list, sizeof(adder_t*)*(num_of_adders+1));
	chromosome = (adder_type_e*)vtr::realloc (chromosome, sizeof(adder_type_e)*(num_of_adders+1));
	adders_list[num_of_adders] = allocate_adder_t();

	//set the variable of the the adder_list array
	adders_list[num_of_adders]->node = node;

	adders_list[num_of_adders]->input->pins = node->input_pins;
	adders_list[num_of_adders]->input->count = node->num_input_pins;
	adders_list[num_of_adders]->input->is_adder = 'y';
	
	adders_list[num_of_adders]->output->pins = node->output_pins;
	adders_list[num_of_adders]->output->count = node->num_output_pins;
	adders_list[num_of_adders]->output->is_adder = 'y';

	chromosome[num_of_adders] = adders_list[num_of_adders]->type;
	num_of_adders++;
}

/*----------------------------------------------------------------------
 * (function: allocate_adder_t) 
 *--------------------------------------------------------------------*/
adder_t* allocate_adder_t ()
{
	adder_t* new_adder = (adder_t *)vtr::malloc(sizeof(adder_t));

	new_adder->input = init_signal_list();
	new_adder->output = init_signal_list();

	return new_adder;
}

/*----------------------------------------------------------------------
 * (function: partial_map_adders_top) 
 *--------------------------------------------------------------------*/
void partial_map_adders_top(netlist_t *netlist)
{
	generation_t *previous_generation = NULL;
	generation_t *current_generation = NULL;

	for(int generation_counter = 0; generation_counter < 10; generation_counter++)
	{
		current_generation = create_generation(previous_generation, generation_counter);

		for (int i=0; i<GENERATION_SIZE; i++)
		{
			vtr::free(chromosome);
			chromosome = current_generation->chromosomes[i];
			partial_map_adders(netlist);
			// calculate fitness and find the fittest of the generation and change the flag of while if neccesary
			destroy_adders();			
		}

		previous_generation = current_generation;
		if(NULL == fittest || false /* add fitness function here */)
		{
			int size_of_chromosome = sizeof(adder_type_e) * num_of_adders;
			fittest = (adder_type_e*) vtr::malloc(size_of_chromosome);
			memcpy(fittest, chromosome ,size_of_chromosome);
		}
	}

	// create logic based on the fittest configuration
	vtr::free(chromosome);
	chromosome = fittest;
	partial_map_adders(netlist);

}

/*----------------------------------------------------------------------
 * (function: partial_map_adders) This function employ the new type of 
 * adders into the adder list and after that, it instantiates adders and 
 * shrinks them into logics
 *--------------------------------------------------------------------*/
void partial_map_adders(/*short traverse_number, */netlist_t *netlist)
{
	for (int i=0; i<num_of_adders; i++)
	{
		adders_list[i]->type = chromosome[i];
		instantiate_add_w_carry(adders_list[i]->type, adders_list[i]->node, /*traverse_number*/PARTIAL_MAP_TRAVERSE_VALUE, netlist);
	}
}

/*---------------------------------------------------------------- 
* This function destroys all adders in the netlist and create them
* again as new one.
*---------------------------------------------------------------*/
void destroy_adders()
{	
	for (int i=0; i<num_of_adders; i++)
	{
		/*---------------------------------------------- 
		 * create new empty adder with the same pins 
		 * as the previous main adder (not pins related
		 *  to nodes in the adder could)
		 *--------------------------------------------*/
		adder_t *new_adder = create_empty_adder(adders_list[i]);

		// Free all created small-detailed logic nodes
		destroy_adder_cloud(adders_list[i]);
		 
		// Replace the new empty adder with previous one in adder list
		adders_list[i] = new_adder;
	}
}

/*------------------------------------------------------------------- 
* This function creates an empty adder with the previous inpu/output
* pins. It is useful when we want to delete the detailed cloud of an
* adder which was created in partial map section.
*------------------------------------------------------------------*/
adder_t *create_empty_adder (adder_t *previous_adder)
{
	adder_t *new_adder = allocate_adder_t();

	new_adder->node = (nnode_t *) vtr::malloc(sizeof(nnode_t));
	// Make copy of input pins
	new_adder->input->pins = make_copy_of_pins (previous_adder->input->pins, previous_adder->input->count);
	new_adder->input->count = previous_adder->input->count;
	new_adder->input->is_adder = previous_adder->input->is_adder;
	// Make copy of output pins
	new_adder->output->pins = make_copy_of_pins (previous_adder->output->pins, previous_adder->output->count);
	new_adder->output->count = previous_adder->output->count;
	new_adder->output->is_adder = previous_adder->output->is_adder;

	return new_adder;
}

/*------------------------------------------- 
* This function creates a copy of a given pin
*------------------------------------------*/
npin_t** make_copy_of_pins (npin_t **copy, long copy_size)
{
	npin_t **paste = (npin_t **) vtr::malloc(sizeof(npin_t *)*copy_size);

	for (int i=0; i<copy_size; i++)

		paste[i] = copy_npin(copy[i]);

	return paste;
}

/*------------------------------------------------------------------- 
* At first, this function calls the recursive function to get all the
* pointers. After that, it frees all pointers realted to the cloud
*------------------------------------------------------------------*/
void destroy_adder_cloud (adder_t *adder)
{
	num_cloud_pins = 0;
	num_cloud_nets = 0;
	num_cloud_nodes = 0;

	for (int i=0; i<adder->output->count; i++)
		recursive_save_pointers (adder, adder->output->pins[i]->node);

	// Free all pointers related to the cloud
	while (num_cloud_pins != 0)
	{
		free_npin(cloud_pins_list[num_cloud_pins]);
		num_cloud_pins--;
	}
	while (num_cloud_nets != 0)
	{
		free_nnet(cloud_nets_list[num_cloud_nets]);
		num_cloud_nets--;
	}
	while (num_cloud_nodes != 0)
	{
		free_nnode(cloud_nodes_list[num_cloud_nodes]);
		num_cloud_nodes--;
	}

	vtr::free (cloud_pins_list);
	vtr::free (cloud_nets_list);
	vtr::free (cloud_nodes_list);

}

/*----------------------------------------------------------- 
* This function, recursively, adds any pointers of pins, nets 
* and nodes related to the cloud which was created in partial
* map for add function
*----------------------------------------------------------*/
void recursive_save_pointers (adder_t *adder, nnode_t * node)
{
	cloud_nodes_list = (nnode_t**) vtr::realloc (cloud_nodes_list, sizeof(nnode_t *)*(num_cloud_nodes+1));
	cloud_nodes_list[num_cloud_nodes] = node;
	num_cloud_nodes++;

	for (int i=0; i<node->num_input_pins; i++)
	{
		cloud_pins_list = (npin_t **) vtr::realloc (cloud_pins_list, sizeof(npin_t *)*(num_cloud_pins+1));
		cloud_pins_list[num_cloud_pins] = node->input_pins[i];
		num_cloud_pins++;

		cloud_nets_list = (nnet_t **) vtr::realloc(cloud_nets_list, sizeof(nnet_t *)*(num_cloud_nets+1));
		cloud_nets_list[num_cloud_nets] = node->input_pins[i]->net;
		num_cloud_nets++;

		cloud_pins_list = (npin_t **) vtr::realloc (cloud_pins_list, sizeof(npin_t *)*(num_cloud_pins+1));
		for ( int j=0; j<adder->input->count; j++ )
			if ( node->input_pins[i]->net->driver_pin == adder->input->pins[j]->net->driver_pin )
				return;
		cloud_pins_list[num_cloud_pins] = node->input_pins[i]->net->driver_pin;
		num_cloud_pins++;

		recursive_save_pointers (adder, node->input_pins[i]->net->driver_pin->node);
	}
	
}

/*--------------------------------------------------------------------- 
* This function creates a new generation. It changes the values of the 
* genes by doing mutation. In each generation, except of the first one,
* the parent of the next generation will be the fittest chromosome in 
* the previous generation.
*--------------------------------------------------------------------*/
generation_t* create_generation (generation_t *previous_generation, int generation_counter)
{
	// Initialization the structure of new generation
	generation_t *new_generation = (generation_t *) vtr::malloc (sizeof(generation_t));
	// Initilization of chromosomes in new generation
	new_generation->fittest = (adder_type_e *) vtr::malloc(sizeof(adder_type_e)*num_of_adders);
	new_generation->chromosomes = (adder_type_e **) vtr::malloc(sizeof(adder_type_e*)*GENERATION_SIZE);
	// Here the value setting of the new generation chromosomes will occur
	int chromosome_index = 0;
	// The first chromosome of each generation is the parent
	int parent_chromosome = chromosome_index;
	chromosome_index += 1;

	if (generation_counter == 0)
	{
		// Considering the parent of the first generation as the chromosome which is produced by the first traverse
		new_generation->chromosomes[parent_chromosome] = chromosome;
		new_generation->fitnesses[parent_chromosome] = chromosome_fitness;
	}
	else
	{
		new_generation->chromosomes[parent_chromosome] = previous_generation->fittest;
		new_generation->fitnesses[parent_chromosome] = -1;
	}
	while (chromosome_index < GENERATION_SIZE)
	{
		new_generation->chromosomes[chromosome_index] = (adder_type_e *) vtr::malloc (sizeof(adder_type_e)*num_of_adders);

		// The children of each generation will be mutated with their parent chromosome
		new_generation->chromosomes[chromosome_index] = mutate (new_generation->chromosomes[parent_chromosome]);
		new_generation->fitnesses[chromosome_index] = -1;
		chromosome_index += 1;
	}

	return new_generation;
}

/*--------------------------------------------------------------------- 
* This function changes the value of the genes in the given chromosome.
* Based on the specified mutation rate, it finds the position of genes 
* which should be mutated randomly, and after that, it changes their 
* values through rand function.
*--------------------------------------------------------------------*/
adder_type_e *mutate (adder_type_e *parent)
{
	adder_type_e *new_chromosome = (adder_type_e*) vtr::malloc (sizeof(adder_type_e)*num_of_adders);
	// Find a point and flip the chromosome from there
	// int cross_point = rand()%num_of_adders;
	// Find points based on the mutation rate and change their values randomly
	int mutation_rate = 0.05;
	int num_of_genes_will_be_changed = mutation_rate*num_of_adders;

	for (int i=0; i<num_of_adders; i++)
	{
		new_chromosome[i] = parent[i];
	}

	while (num_of_genes_will_be_changed != 0)
	{
		int point = rand()%num_of_adders;
		new_chromosome[point] = static_cast<adder_type_e>( rand() % adder_type_END );

		num_of_genes_will_be_changed--;
	}
	
	return new_chromosome;
}
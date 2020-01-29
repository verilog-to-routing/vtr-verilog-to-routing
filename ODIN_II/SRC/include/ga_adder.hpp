#include "odin_types.h"
#include "adders.h"

#define GENERATION_SIZE 6

// Definitions
struct generation_t 
{
    adder_type_e **chromosomes;
    adder_type_e *fittest;
    int   *fitnesses;
};

struct adder_t {

    adder_type_e type = RCA;

    signal_list_t *input;
    signal_list_t *output;

    nnode_t *node;
};

/**
 * TODO !!! use this for each generation, parent is generation[0]
 * 
struct adder_list_t
{
    adder_type_e *chromosome;
    adder_t **adders;
    int size;
};

 */

// Declarations
void instantiate_soft_adder(nnode_t *node, short mark, netlist_t *netlist);
adder_t* allocate_adder_t ();
void partial_map_adders_top(netlist_t *netlist);
void partial_map_adders(/*short traverse_number,*/ netlist_t *netlist);
void destroy_adders();
void destroy_adder_cloud (adder_t *adder);
void recursive_save_pointers (adder_t *adder, nnode_t * node);
npin_t** make_copy_of_pins (npin_t **copy, long copy_size);
adder_t *create_empty_adder (adder_t *previous_adder);
generation_t* create_generation (generation_t *previous_generation, int generation_counter);
adder_type_e *mutate (adder_type_e *parent);
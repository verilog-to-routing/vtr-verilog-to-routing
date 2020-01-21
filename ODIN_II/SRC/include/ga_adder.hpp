#include "odin_types.h"
#include "adders.h"

#define DEFAULT  0
#define CSLA     1
#define BEC_CSLA 2

#define CHILDREN_NUM 5

// Definitions
struct generation_t {

    short **chromosomes;
    short *fittest;
    int   *fitnesses;
};

struct adder_t {

    short type = DEFAULT;

    signal_list_t *input;
    signal_list_t *output;

    nnode_t *node;
};

// Global Variables
extern adder_t **adders_list;
extern short *chromosome;

extern long num_of_adders;

// Declarations
void partial_map_adders_top(netlist_t *netlist);
void partial_map_adders(/*short traverse_number,*/ netlist_t *netlist);
void destroy_adders();
void destroy_adder_cloud (adder_t *adder);
void recursive_save_pointers (adder_t *adder, nnode_t * node);
npin_t** make_copy_of_pins (npin_t **copy, long copy_size);
adder_t *create_empty_adder (adder_t *previous_adder);
generation_t* create_generation (generation_t *previous_generation, int generation_counter);
short *mutate (short *parent);
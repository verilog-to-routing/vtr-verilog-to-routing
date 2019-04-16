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
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "odin_types.h"
#include "odin_globals.h"

#include "netlist_utils.h"
#include "vtr_util.h"
#include "vtr_memory.h"


/* Used in the nnode_t.node_data field to mark if the node was already visited
 * during a forward or backward sweep traversal or the removal phase */
int _visited_forward, _visited_backward, _visited_removal;
#define VISITED_FORWARD ((void*)&_visited_forward)
#define VISITED_BACKWARD ((void*)&_visited_backward)
#define VISITED_REMOVAL ((void*)&_visited_removal)


/* Simple linked list of nodes structure */
typedef struct node_list_t_t{
	nnode_t *node;
	struct node_list_t_t *next;
} node_list_t;

node_list_t useless_nodes; // List of the nodes to be removed
node_list_t *removal_list_next = &useless_nodes; // Tail of the nodes to be removed

node_list_t addsub_nodes; // List of the adder/subtractor nodes
node_list_t *addsub_list_next = &addsub_nodes; // Tail of the adder/subtractor node list


/* Function declarations */
node_list_t* insert_node_list(node_list_t* node_list, nnode_t *node);
void traverse_backward(nnode_t *node);
void traverse_forward(nnode_t *node, int toplevel, int remove_me);
void mark_output_dependencies(netlist_t *netlist);
void identify_unused_nodes(netlist_t *netlist);
void remove_unused_nodes(node_list_t *remove);
void calculate_addsub_statistics(node_list_t *addsub);
void remove_unused_logic(netlist_t *netlist);




node_list_t* insert_node_list(node_list_t* node_list, nnode_t *node){
	node_list->node = node;
	node_list->next = (node_list_t*)vtr::calloc(1, sizeof(node_list_t));
	return node_list->next;
}

/* Traverse the netlist backwards, moving from outputs to inputs */
void traverse_backward(nnode_t *node){
	if(node->node_data == VISITED_BACKWARD) return; // Already visited
	node->node_data = VISITED_BACKWARD; // Mark as visited
	int i;
	for(i = 0; i < node->num_input_pins; i++){
		if(node->input_pins[i]->net->driver_pin){ // ensure this net has a driver (i.e. skip undriven outputs)
			traverse_backward(node->input_pins[i]->net->driver_pin->node); // Visit the drivers of this node
		}
	}
}

/* Traverse the netlist forward, moving from inputs to outputs.
 * Adds nodes that do not affect any outputs to the useless_nodes list
 * Arguments:
 * 	node: the current node in the netlist
 * 	toplevel: are we at one of the top-level nodes? (GND, VCC, PAD or INPUT)
 * 	remove_me: should the current node be removed?
 * */
void traverse_forward(nnode_t *node, int toplevel, int remove_me){
	if(node == NULL) return; // Shouldn't happen, but check just in case
	if(node->node_data == VISITED_FORWARD) return; // Already visited, shouldn't happen anyway

	/* We want to remove this node if either its parent was removed,
	 * or if it was not visited on the backwards sweep */
	remove_me = remove_me || ((node->node_data != VISITED_BACKWARD) && (toplevel == FALSE));

	/* Mark this node as visited */
	node->node_data = VISITED_FORWARD;

	if(remove_me) {
		/* Add this node to the list of nodes to remove */
		removal_list_next = insert_node_list(removal_list_next, node);
	}

	if(node->type == ADD || node->type == MINUS){
        // check if adders/subtractors are starting using a global gnd/vcc node or a pad node
        auto ADDER_START_NODE = PAD_NODE;
        if (configuration.adder_cin_global) {
            if (node->type == ADD) ADDER_START_NODE = GND_NODE;
            else ADDER_START_NODE = VCC_NODE;
        }
		/* Check if we've found the head of an adder or subtractor chain */
		if(node->input_pins[node->num_input_pins-1]->net->driver_pin->node->type == ADDER_START_NODE) {
			addsub_list_next = insert_node_list(addsub_list_next, node);
		}
	}

	/* Iterate through every fanout node */
	int i, j;
	for(i = 0; i < node->num_output_pins; i++){
		if(node->output_pins[i] && node->output_pins[i]->net){
			for(j = 0; j < node->output_pins[i]->net->num_fanout_pins; j++){
				if(node->output_pins[i]->net->fanout_pins[j]){
					nnode_t *child = node->output_pins[i]->net->fanout_pins[j]->node;
					if(child){
						/* If this child hasn't already been visited, visit it now */
						if(child->node_data != VISITED_FORWARD){
							traverse_forward(child, FALSE, remove_me);
						}
					}
				}
			}
		}
	}
}


/* Start at each of the top level output nodes and traverse backwards to the inputs
to determine which nodes have an effect on the outputs */
void mark_output_dependencies(netlist_t *netlist){
	int i;
	for(i = 0; i < netlist->num_top_output_nodes; i++){
		traverse_backward(netlist->top_output_nodes[i]);
	}
}

/* Traversed the netlist forward from the top level inputs and special nodes
(VCC, GND, PAD) */
void identify_unused_nodes(netlist_t *netlist){

	useless_nodes.node = NULL;
	useless_nodes.next = NULL;

	addsub_nodes.node = NULL;
	addsub_nodes.next = NULL;

	traverse_forward(netlist->gnd_node, TRUE, FALSE);
	traverse_forward(netlist->vcc_node, TRUE, FALSE);
	traverse_forward(netlist->pad_node, TRUE, FALSE);
	int i;
	for(i = 0; i < netlist->num_top_input_nodes; i++){
		traverse_forward(netlist->top_input_nodes[i], TRUE, FALSE);
	}
}

/* Note: This does not actually free the unused logic, but simply detaches
it from the rest of the circuit */
void remove_unused_nodes(node_list_t *remove){
	while(remove != NULL && remove->node != NULL){
		int i;
		for(i = 0; i < remove->node->num_input_pins; i++){
			npin_t *input_pin = remove->node->input_pins[i];
			input_pin->net->fanout_pins[input_pin->pin_net_idx] = NULL; // Remove the fanout pin from the net
		}
		remove->node->node_data = VISITED_REMOVAL;
		remove = remove->next;
	}
}

/* Since we are traversing the entire netlist anyway, we can use this
 * opportunity to keep track of the heads of adder/subtractors chains
 * and then compute statistics on them */
long adder_chain_count = 0;
long longest_adder_chain = 0;
long total_adders = 0;

long subtractor_chain_count = 0;
long longest_subtractor_chain = 0;
long total_subtractors = 0;

double geomean_addsub_length = 0.0; // Geometric mean of add/sub chain length
double sum_of_addsub_logs = 0.0; // Sum of the logarithms of the add/sub chain lengths; used for geomean
double total_addsub_chain_count = 0.0;

void calculate_addsub_statistics(node_list_t *addsub){
	while(addsub != NULL && addsub->node != NULL){
		int found_tail = FALSE;
		nnode_t *node = addsub->node;
		int chain_depth = 0;
		while(!found_tail){
			if(node->node_data == VISITED_REMOVAL){
				found_tail = TRUE;
				break;
			}
			chain_depth += 1;

			/* Carry out is always output pin 0 */
			nnet_t *carry_out_net = node->output_pins[0]->net;
			if(carry_out_net == NULL || carry_out_net->fanout_pins[0] == NULL) found_tail = TRUE;
			else node = carry_out_net->fanout_pins[0]->node;
		}
		if(chain_depth > 0){
			if(node->type == ADD){
				adder_chain_count += 1;
				total_adders += chain_depth;
				if(chain_depth > longest_adder_chain) longest_adder_chain = chain_depth;
			}
			else if(node->type == MINUS){
				subtractor_chain_count += 1;
				total_subtractors += chain_depth;
				if(chain_depth > longest_subtractor_chain) longest_subtractor_chain = chain_depth;
			}

			sum_of_addsub_logs += log(chain_depth);
			total_addsub_chain_count += 1.0;
		}

		addsub = addsub->next;
	}
	/* Calculate the geometric mean carry chain length */
	geomean_addsub_length = exp(sum_of_addsub_logs / total_addsub_chain_count);
}

/* Perform the backwards and forward sweeps and remove the unused nodes */
void remove_unused_logic(netlist_t *netlist){
	mark_output_dependencies(netlist);
	identify_unused_nodes(netlist);
	remove_unused_nodes(&useless_nodes);
	calculate_addsub_statistics(&addsub_nodes);
}

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
#include "types.h"
#include "globals.h"
#include "errors.h"
#include "netlist_utils.h"
#include "util.h"


/* Used in the nnode_t.node_data field to mark if the node was already visited
 * during a forward or backward sweep traversal */
int _visited_forward, _visited_backward;
#define VISITED_FORWARD ((void*)&_visited_forward)
#define VISITED_BACKWARD ((void*)&_visited_backward)

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

/* Simple linked list to keep track of which nodes should be removed at the end */
typedef struct removal_list_t_t{
	nnode_t *node;
	struct removal_list_t_t *next;
} removal_list_t;

removal_list_t useless_nodes; // List of the nodes to be removed
removal_list_t *removal_list_next; // Tail of the nodes to be removed

/* Traverse the netlist forward, moving from inputs to outputs.
 * Adds nodes that do not affect any outputs to the useless_nodes list */
void traverse_forward(nnode_t *node, int toplevel){
	
	if(node == NULL) return;
	if(node->node_data == VISITED_FORWARD) return; // Already visited
	
	int remove_me = (node->node_data != VISITED_BACKWARD) && (toplevel == FALSE);
	
	if(remove_me) {
		removal_list_next->node = node;
		removal_list_next->next = (removal_list_t*)calloc(1, sizeof(removal_list_t));
		removal_list_next = removal_list_next->next;
	}
	node->node_data = VISITED_FORWARD;
	int i, j;
	for(i = 0; i < node->num_output_pins; i++){
		if(node->output_pins[i] && node->output_pins[i]->net){
			for(j = 0; j < node->output_pins[i]->net->num_fanout_pins; j++){
				if(node->output_pins[i]->net->fanout_pins[j]){
					traverse_forward(node->output_pins[i]->net->fanout_pins[j]->node, FALSE);
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
	removal_list_next = &useless_nodes;
	
	traverse_forward(netlist->gnd_node, TRUE);
	traverse_forward(netlist->vcc_node, TRUE);
	traverse_forward(netlist->pad_node, TRUE);
	int i;
	for(i = 0; i < netlist->num_top_input_nodes; i++){
		traverse_forward(netlist->top_input_nodes[i], TRUE);
	}
}

/* Note: This does not actually free the unused logic, but simply detaches
it from the rest of the circuit */
void remove_unused_nodes(removal_list_t *remove){
	while(remove != NULL && remove->node != NULL){
		int i;
		for(i = 0; i < remove->node->num_input_pins; i++){
			npin_t *input_pin = remove->node->input_pins[i];
			input_pin->net->fanout_pins[input_pin->pin_net_idx] = NULL; // Remove the fanout pin from the net
		}
		remove = remove->next;
	}
}

/* Perform the backwards and forward sweeps and remove the unused nodes */
void remove_unused_logic(netlist_t *netlist){
	mark_output_dependencies(netlist);
	identify_unused_nodes(netlist);
	remove_unused_nodes(&useless_nodes);
}

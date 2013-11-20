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

/* Since we are traversing the entire netlist anyway, we can use this
 * opportunity to collect statistics about hard adders/subtractors */
int adder_chain_count =0;
int longest_adder_chain =0;
int total_adders = 0;

int subtractor_chain_count = 0;
int longest_subtractor_chain = 0;
int total_subtractors = 0;

/* Traverse the netlist forward, moving from inputs to outputs.
 * Adds nodes that do not affect any outputs to the useless_nodes list 
 * Arguments:
 * 	node: the current node in the netlist
 * 	toplevel: are we at one of the top-level nodes? (GND, VCC, PAD or INPUT)
 * 	remove_me: should the current node be removed?
 * 	prev_type: the type of the parent node
 *  depth: how deep in a chain this node is (for adders and subtractors) 
 * Returns: TRUE if the child will be removed, FALSE otherwise
 * */
int traverse_forward(nnode_t *node, int toplevel, int remove_me, operation_list prev_type, int depth){
	if(node == NULL) return FALSE; // Shouldn't happen, but check just in case
	if(node->node_data == VISITED_FORWARD) return FALSE; // Already visited, shouldn't happen anyway
	
	/* We want to remove this node if either its parent was removed, 
	 * or if it was not visited on the backwards sweep */
	remove_me = remove_me || ((node->node_data != VISITED_BACKWARD) && (toplevel == FALSE));
	
	/* Mark this node as visited */
	node->node_data = VISITED_FORWARD;
	
	if(remove_me) {
		/* Add this node to the list of nodes to remove */
		removal_list_next->node = node;
		removal_list_next->next = (removal_list_t*)calloc(1, sizeof(removal_list_t));
		removal_list_next = removal_list_next->next;
	}
	
	/* If this node has the same type as its parent, we're still in a potential chain,
	 * so increase the depth */
	if(node->type == prev_type) depth += 1;
	else depth = 0;
	
	/* TRUE if this node has a child with the same type */
	int has_similar_child = 0;
	
	/* Iterate through every fanout node */
	int i, j;
	for(i = 0; i < node->num_output_pins; i++){
		if(node->output_pins[i] && node->output_pins[i]->net){
			for(j = 0; j < node->output_pins[i]->net->num_fanout_pins; j++){
				if(node->output_pins[i]->net->fanout_pins[j]){
					nnode_t *child = node->output_pins[i]->net->fanout_pins[j]->node;
					if(child){
						/* Ensure we visit the head of an adder or subtractor chain first,
						 * before any successive nodes. The carry in (last input pin) of a
						 * hard adder/subtractor will be  unconnected (i.e. PAD_NODE) if 
						 * and only if it is the head of a chain. */ 
						if(node->type != ADD && child->type == ADD && child->input_pins[child->num_input_pins-1]->net->driver_pin->node->type != PAD_NODE){
							continue;
						}
						if(node->type != MINUS && child->type == MINUS && child->input_pins[child->num_input_pins-1]->net->driver_pin->node->type != PAD_NODE){
							continue;
						}
						
						/* If this child hasn't already been visited, visit it now */
						if(child->node_data != VISITED_FORWARD){
							int child_used = !traverse_forward(child, FALSE, remove_me, node->type, depth);
							if(child->type == node->type && child_used) {
								has_similar_child = 1;
							}
						}
					}
				}
			}
		}
	}
	
	if(!remove_me){
		/* Update the adder and subtractor statistics */
		
		/* If the current node is an adder but has no children that are 
		 * adders, then it must be the tail of a chain */
		if(node->type == ADD && !has_similar_child){
			adder_chain_count++;
			if(depth > longest_adder_chain) longest_adder_chain = depth + 1;
			total_adders += depth + 1;
		}
	
		if(node->type == MINUS && !has_similar_child){
			subtractor_chain_count++;
			if(depth > longest_subtractor_chain) longest_subtractor_chain = depth + 1;
			total_subtractors += depth + 1;
		}
	}
	
	return remove_me;
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
	
	traverse_forward(netlist->gnd_node, TRUE, FALSE, GND_NODE, 0);
	traverse_forward(netlist->vcc_node, TRUE, FALSE, VCC_NODE, 0);
	traverse_forward(netlist->pad_node, TRUE, FALSE, PAD_NODE, 0);
	int i;
	for(i = 0; i < netlist->num_top_input_nodes; i++){
		traverse_forward(netlist->top_input_nodes[i], TRUE, FALSE, INPUT_NODE, 0);
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

/* 
  Creates a lb_rr_node graph to represent interconnect within a logic block.

  Author: Jason Luu
  Date: July 22, 2013
 */

#include <cstdio>
#include <cstring>
using namespace std;

#include <assert.h>

#include "util.h"
#include "physical_types.h"
#include "vpr_types.h"
#include "globals.h"
#include "pack_types.h"
#include "lb_rr_graph.h"

static t_lb_type_rr_node* alloc_and_load_lb_type_rr_graph_for_type(t_type_ptr lb_type);



/* Populate each logic block type (type_descriptor) with a directed graph that respresents the interconnect within it.
*/
t_lb_type_rr_node** alloc_and_load_all_lb_type_rr_graph() {
	t_lb_type_rr_node **lb_type_rr_graphs;

	lb_type_rr_graphs = (t_lb_type_rr_node **)my_calloc(num_types, sizeof(t_lb_type_rr_node *));
	
	for(int i = 0; i < num_types; i++) {
		if(&type_descriptors[i] != EMPTY_TYPE) {
			 lb_type_rr_graphs[i] = alloc_and_load_lb_type_rr_graph_for_type(&type_descriptors[i]);
		}
	}
	return lb_type_rr_graphs;
}


static t_lb_type_rr_node* alloc_and_load_lb_type_rr_graph_for_type(t_type_ptr lb_type) {
	t_lb_type_rr_node *lb_type_rr_node_graph;
	t_pb_type *pb_type;
	t_pb_graph_node *pb_graph_head;
	
	pb_type = lb_type->pb_type;
	pb_graph_head = lb_type->pb_graph_head;

	lb_type_rr_node_graph = (t_lb_type_rr_node *)
										my_calloc(pb_graph_head->total_pb_pins +
										pb_type->num_input_pins +
										pb_type->num_output_pins +
										pb_type->num_clock_pins, sizeof(t_lb_type_rr_node));

	for(int i = 0; i < pb_graph_head->total_pb_pins; i++) {
	}

	return lb_type_rr_node_graph;
}


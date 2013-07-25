/* 
  Creates a lb_type_rr_node graph to represent interconnect within a logic block type.
  
  For a given logic block, the internal routing resource graph is constructed as follows:
	  Each pb_graph_pin is a node in this graph and is indexed by the pin number of the pb_graph pin
	  External nets that drive into the logic block come from one source indexed by the total number of pins
	  External nets that the logic block drives are represented as one sink indexed by the total number of pins + 1
	  Each primitive input pin drives a sink.  Since the input pin has a corresponding lb_type_rr_node, the sink node driven by
	  the pin must have a different index.  I append these sinks to the end of the vector.
	  Each primitive output pin is a source.

  Common acryonyms:
      rr - routing resource
	  lb - logic block
	  pb - phsyical block (the top level physical block is the logic block, a leaf physical block is a primitive)

  Author: Jason Luu
  Date: July 22, 2013
 */

#include <cstdio>
#include <cstring>
using namespace std;

#include <assert.h>
#include <vector>
#include <cmath>

#include "util.h"
#include "physical_types.h"
#include "vpr_types.h"
#include "globals.h"
#include "pack_types.h"
#include "lb_rr_graph.h"

static void alloc_and_load_lb_type_rr_graph_for_type(INP t_type_ptr lb_type, 
													 INOUTP vector<t_lb_type_rr_node> &lb_type_rr_node_graph);
static void alloc_and_load_lb_type_rr_graph_for_pb_graph_node(INP const t_pb_graph_node *pb_graph_node,
														INOUTP vector<t_lb_type_rr_node> &lb_type_rr_node_graph);
static float get_cost_of_pb_edge(t_pb_graph_edge *edge);

/* Populate each logic block type (type_descriptor) with a directed graph that respresents the interconnect within it.
*/
vector<t_lb_type_rr_node> *alloc_and_load_all_lb_type_rr_graph() {
	vector<t_lb_type_rr_node> *lb_type_rr_graphs;
	lb_type_rr_graphs = new vector<t_lb_type_rr_node> [num_types];

	for(int i = 0; i < num_types; i++) {
		if(&type_descriptors[i] != EMPTY_TYPE) {
			 alloc_and_load_lb_type_rr_graph_for_type(&type_descriptors[i], lb_type_rr_graphs[i]);
		}
	}
	return lb_type_rr_graphs;
}


/* Given a logic block type, build its internal routing resource graph
   Each pb_graph_pin has a corresponding lb_type_rr_node that is indexed by the pin index of that pb_graph_pin
   Extra sources and sinks in lb_type_rr_node_graph that do not correspond to a pb_graph_pin are appended in indices following total_pb_pins
   of the pb_type
*/
static void alloc_and_load_lb_type_rr_graph_for_type(INP t_type_ptr lb_type, 
													 INOUTP vector<t_lb_type_rr_node> &lb_type_rr_node_graph) {
	t_pb_type *pb_type;
	t_pb_graph_node *pb_graph_head;
	int ext_source_index, ext_sink_index;
	int ifanout;
	
	assert(lb_type_rr_node_graph.empty());

	pb_type = lb_type->pb_type;
	pb_graph_head = lb_type->pb_graph_head;

	/* Indices [0..total_pb_pins - 1] have a 1-to-1 matching with a pb_graph_pin
	   Index total_pb_pins represents all external sources to logic block type
	   Index total_pin_pins + 1 represents all external sinks from logic block type
	   All nodes after total_pin_pins + 1 are other nodes that do not have a corresponding pb_graph_pin (eg. a sink node that is driven by one or more pb_graph_pins of a primitive)
	*/
	lb_type_rr_node_graph.resize(pb_graph_head->total_pb_pins + 2);

	alloc_and_load_lb_type_rr_graph_for_pb_graph_node(pb_graph_head, lb_type_rr_node_graph);

	/* Define the external source and sink for the routing resource graph of the logic block type */
	ext_source_index = pb_graph_head->total_pb_pins;
	ext_sink_index = pb_graph_head->total_pb_pins + 1;
	assert(	lb_type_rr_node_graph[ext_source_index].type == NUM_LB_RR_TYPES && 
			lb_type_rr_node_graph[ext_sink_index].type == NUM_LB_RR_TYPES);

	/* External source node drives all inputs going into logic block type */
	lb_type_rr_node_graph[ext_source_index].capacity = pb_type->num_input_pins + pb_type->num_clock_pins;
	lb_type_rr_node_graph[ext_source_index].num_fanout = (short*)my_malloc(sizeof (short));
	lb_type_rr_node_graph[ext_source_index].num_fanout[0] = pb_type->num_input_pins + pb_type->num_clock_pins;
	lb_type_rr_node_graph[ext_source_index].fanout = (int**)my_malloc(sizeof (int*));
	lb_type_rr_node_graph[ext_source_index].fanout[0] = (int*)my_malloc(lb_type_rr_node_graph[ext_source_index].num_fanout[0] * sizeof (int));
	lb_type_rr_node_graph[ext_source_index].fanout_intrinsic_cost = (float**)my_malloc(sizeof (float*));
	lb_type_rr_node_graph[ext_source_index].fanout_intrinsic_cost[0] = (float*)my_malloc(lb_type_rr_node_graph[ext_source_index].num_fanout[0] * sizeof (float));
	lb_type_rr_node_graph[ext_source_index].type = LB_SOURCE;
	
	/* Connect external souce node to all input and clock pins of logic block type */
	ifanout = 0;
	
	for(int iport = 0; iport < pb_graph_head->num_input_ports; iport++) {
		for(int ipin = 0; ipin < pb_graph_head->num_input_pins[iport]; ipin++) {
			lb_type_rr_node_graph[ext_source_index].fanout[0][ifanout] = pb_graph_head->input_pins[iport][ipin].pin_count_in_cluster;
			lb_type_rr_node_graph[ext_source_index].fanout_intrinsic_cost[0][ifanout] = 1;
			ifanout++;
		}
	}

	for(int iport = 0; iport < pb_graph_head->num_clock_ports; iport++) {
		for(int ipin = 0; ipin < pb_graph_head->num_clock_pins[iport]; ipin++) {
			lb_type_rr_node_graph[ext_source_index].fanout[0][ifanout] = pb_graph_head->clock_pins[iport][ipin].pin_count_in_cluster;
			lb_type_rr_node_graph[ext_source_index].fanout_intrinsic_cost[0][ifanout] = 1;
			ifanout++;
		}
	}
	
	/* Check that the fanout indices are correct */
	assert(ifanout == pb_type->num_input_pins + pb_type->num_clock_pins);

	/* External sink node driven by all outputs exiting logic block type */
	lb_type_rr_node_graph[ext_sink_index].capacity = pb_type->num_output_pins;
	lb_type_rr_node_graph[ext_sink_index].num_fanout = (short*)my_malloc(sizeof (short));
	lb_type_rr_node_graph[ext_sink_index].num_fanout[0] = 0; /* Teriminal node */
	lb_type_rr_node_graph[ext_sink_index].type = LB_SINK;
}

/* Free routing resource graph for all logic block types */
void free_all_lb_type_rr_graph(INOUTP vector<t_lb_type_rr_node> **lb_type_rr_graphs) {
	/* jedit TODO */
}

/* Given a pb_graph_node, build the routing resource data for it.
   Each pb_graph_pin has a corresponding rr node.  This rr node is indexed by the pb_graph_pin pin index.
   This function populates the rr node for the pb_graph_pin of the current pb_graph_node then recursively
   repeats this on all children of the pb_graph_node
*/
static void alloc_and_load_lb_type_rr_graph_for_pb_graph_node(INP const t_pb_graph_node *pb_graph_node,
													INOUTP vector<t_lb_type_rr_node> &lb_type_rr_node_graph) {
	t_pb_type *pb_type;
	int pin_index;
	t_pb_graph_pin *pb_pin;
	t_pb_graph_node *parent_node;

	pb_type = pb_graph_node->pb_type;
	parent_node = pb_graph_node->parent_pb_graph_node;
	int num_modes;

	if(pb_type->num_modes == 0) {
		/* This pb_graph_node is a terminating leaf node (primitive) */

		/* alloc and load input pins that connect to sinks */
		for(int iport = 0; iport < pb_graph_node->num_input_ports; iport++) {
			boolean port_equivalent = FALSE;
			int sink_index = OPEN;
			for(int ipin = 0; ipin < pb_graph_node->num_input_pins[iport]; ipin++) {
				/* load intermediate indices */
				pb_pin = &pb_graph_node->input_pins[iport][ipin];
				port_equivalent = pb_pin->port->equivalent;
				pin_index = pb_pin->pin_count_in_cluster;

				/* alloc and load rr node info */
				lb_type_rr_node_graph[pin_index].capacity = 1;
				lb_type_rr_node_graph[pin_index].num_fanout = (short*)my_malloc(sizeof (short));
				lb_type_rr_node_graph[pin_index].num_fanout[0] = 1;
				lb_type_rr_node_graph[pin_index].fanout = (int**)my_malloc(sizeof (int*));
				lb_type_rr_node_graph[pin_index].fanout[0] = (int*)my_malloc(sizeof (int));
				lb_type_rr_node_graph[pin_index].fanout_intrinsic_cost = (float**)my_malloc(sizeof (float*));
				lb_type_rr_node_graph[pin_index].fanout_intrinsic_cost[0] = (float*)my_malloc(sizeof (float));
				lb_type_rr_node_graph[pin_index].fanout_intrinsic_cost[0][0] = 1;
				lb_type_rr_node_graph[pin_index].type = LB_INTERMEDIATE;

				if(port_equivalent == TRUE && sink_index != OPEN) {
					/* If port is equivalent and a sink for this port is already created, use that port */
					lb_type_rr_node_graph[pin_index].fanout[0][0] = sink_index;					
				} else {
					/* Create new sink for input to primitive */
					t_lb_type_rr_node new_sink;					
					if(port_equivalent == TRUE) {
						new_sink.capacity = pb_pin->port->num_pins;
					} else {
						new_sink.capacity = 1;
					}
					new_sink.num_fanout = (short*)my_malloc(sizeof (short));
					new_sink.num_fanout[0] = 0;
					new_sink.type = LB_SINK;					
					sink_index = lb_type_rr_node_graph.size();
					lb_type_rr_node_graph.push_back(new_sink);					
				}
			}			
		}

		/* alloc and load output pins that are represented as rr sources */
		for(int iport = 0; iport < pb_graph_node->num_output_ports; iport++) {
			for(int ipin = 0; ipin < pb_graph_node->num_output_pins[iport]; ipin++) {
				/* load intermediate indices */
				pb_pin = &pb_graph_node->output_pins[iport][ipin];
				pin_index = pb_pin->pin_count_in_cluster;
				num_modes = parent_node->pb_type->num_modes;

				/* alloc and load rr node info */
				lb_type_rr_node_graph[pin_index].capacity = 1;
				lb_type_rr_node_graph[pin_index].num_fanout = (short*)my_calloc(num_modes, sizeof (short));
				lb_type_rr_node_graph[pin_index].fanout = (int**)my_calloc(num_modes, sizeof (int*));
				lb_type_rr_node_graph[pin_index].fanout_intrinsic_cost = (float**)my_calloc(num_modes, sizeof (float*));
				
				/* Count number of mode-dependant fanout */
				for(int iedge = 0; iedge < pb_pin->num_output_edges; iedge++) {
					assert(pb_pin->output_edges[iedge]->num_output_pins == 1);
					int pmode;
					pmode = pb_pin->output_edges[iedge]->interconnect->parent_mode->index;
					lb_type_rr_node_graph[pin_index].num_fanout[pmode]++;
				}

				/* Allocate space based on fanout */
				for(int imode = 0; imode < num_modes; imode++) {
					lb_type_rr_node_graph[pin_index].fanout[imode] = 
						(int*)my_calloc(lb_type_rr_node_graph[pin_index].num_fanout[imode], sizeof (int));
					lb_type_rr_node_graph[pin_index].fanout_intrinsic_cost[imode] = 
						(float*)my_calloc(lb_type_rr_node_graph[pin_index].num_fanout[imode], sizeof (float));
					lb_type_rr_node_graph[pin_index].num_fanout[imode] = 0; /* reset to 0 so that we can reuse this variable to populate fanout stats */
				}

				/* Load edges */
				for(int iedge = 0; iedge < pb_pin->num_output_edges; iedge++) {
					assert(pb_pin->output_edges[iedge]->num_output_pins == 1);
					int pmode;
					int ifanout;
					pmode = pb_pin->output_edges[iedge]->interconnect->parent_mode->index;
					ifanout = lb_type_rr_node_graph[pin_index].num_fanout[pmode];
					lb_type_rr_node_graph[pin_index].fanout[pmode][ifanout] = 
						pb_pin->output_edges[iedge]->output_pins[0]->pin_count_in_cluster;
					lb_type_rr_node_graph[pin_index].fanout_intrinsic_cost[pmode][ifanout] = 
						get_cost_of_pb_edge(pb_pin->output_edges[iedge]);
					lb_type_rr_node_graph[pin_index].num_fanout[pmode]++;					
				}

				lb_type_rr_node_graph[pin_index].type = LB_SOURCE;				
			}			
		}


		/* alloc and load clock pins that connect to sinks */
		for(int iport = 0; iport < pb_graph_node->num_clock_ports; iport++) {
			boolean port_equivalent = FALSE;
			int sink_index = OPEN;
			for(int ipin = 0; ipin < pb_graph_node->num_clock_pins[iport]; ipin++) {
				/* load intermediate indices */
				pb_pin = &pb_graph_node->clock_pins[iport][ipin];
				port_equivalent = pb_pin->port->equivalent;
				pin_index = pb_pin->pin_count_in_cluster;

				/* alloc and load rr node info */
				lb_type_rr_node_graph[pin_index].capacity = 1;
				lb_type_rr_node_graph[pin_index].num_fanout = (short*)my_malloc(sizeof (short));
				lb_type_rr_node_graph[pin_index].num_fanout[0] = 1;
				lb_type_rr_node_graph[pin_index].fanout = (int**)my_malloc(sizeof (int*));
				lb_type_rr_node_graph[pin_index].fanout[0] = (int*)my_malloc(sizeof (int));
				lb_type_rr_node_graph[pin_index].fanout_intrinsic_cost = (float**)my_malloc(sizeof (float*));
				lb_type_rr_node_graph[pin_index].fanout_intrinsic_cost[0] = (float*)my_malloc(sizeof (float));
				lb_type_rr_node_graph[pin_index].fanout_intrinsic_cost[0][0] = 1;
				lb_type_rr_node_graph[pin_index].type = LB_INTERMEDIATE;

				if(port_equivalent == TRUE && sink_index != OPEN) {
					/* If port is equivalent and a sink for this port is already created, use that port */
					lb_type_rr_node_graph[pin_index].fanout[0][0] = sink_index;					
				} else {
					/* Create new sink for clock to primitive */
					t_lb_type_rr_node new_sink;					
					if(port_equivalent == TRUE) {
						new_sink.capacity = pb_pin->port->num_pins;
					} else {
						new_sink.capacity = 1;
					}
					new_sink.num_fanout = (short*)my_malloc(sizeof (short));
					new_sink.num_fanout[0] = 0;
					new_sink.type = LB_SINK;					
					sink_index = lb_type_rr_node_graph.size();
					lb_type_rr_node_graph.push_back(new_sink);					
				}
			}
		}
	} else {
		/* This pb_graph_node is a logic block or subcluster */
		for(int imode = 0; imode < pb_type->num_modes; imode++) {
			for(int ipb_type = 0; ipb_type < pb_type->modes[imode].num_pb_type_children; ipb_type++) {
				for(int ipb = 0; ipb < pb_type->modes[imode].pb_type_children[ipb_type].num_pb; ipb++) {
					alloc_and_load_lb_type_rr_graph_for_pb_graph_node(
						&pb_graph_node->child_pb_graph_nodes[imode][ipb_type][ipb], lb_type_rr_node_graph);
				}
			}
		}

		
		/* alloc and load input pins that drive other rr nodes */
		for(int iport = 0; iport < pb_graph_node->num_input_ports; iport++) {
			for(int ipin = 0; ipin < pb_graph_node->num_input_pins[iport]; ipin++) {
				/* load intermediate indices */
				pb_pin = &pb_graph_node->input_pins[iport][ipin];
				pin_index = pb_pin->pin_count_in_cluster;
				num_modes = pb_graph_node->pb_type->num_modes;

				/* alloc and load rr node info */
				lb_type_rr_node_graph[pin_index].capacity = 1;
				lb_type_rr_node_graph[pin_index].num_fanout = (short*)my_calloc(num_modes, sizeof (short));
				lb_type_rr_node_graph[pin_index].fanout = (int**)my_calloc(num_modes, sizeof (int*));
				lb_type_rr_node_graph[pin_index].fanout_intrinsic_cost = (float**)my_calloc(num_modes, sizeof (float*));
				
				/* Count number of mode-dependant fanout */
				for(int iedge = 0; iedge < pb_pin->num_output_edges; iedge++) {
					assert(pb_pin->output_edges[iedge]->num_output_pins == 1);
					int pmode;
					pmode = pb_pin->output_edges[iedge]->interconnect->parent_mode->index;
					lb_type_rr_node_graph[pin_index].num_fanout[pmode]++;
				}

				/* Allocate space based on fanout */
				for(int imode = 0; imode < num_modes; imode++) {
					lb_type_rr_node_graph[pin_index].fanout[imode] = 
						(int*)my_calloc(lb_type_rr_node_graph[pin_index].num_fanout[imode], sizeof (int));
					lb_type_rr_node_graph[pin_index].fanout_intrinsic_cost[imode] = 
						(float*)my_calloc(lb_type_rr_node_graph[pin_index].num_fanout[imode], sizeof (float));
					lb_type_rr_node_graph[pin_index].num_fanout[imode] = 0; /* reset to 0 so that we can reuse this variable to populate fanout stats */
				}

				/* Load edges */
				for(int iedge = 0; iedge < pb_pin->num_output_edges; iedge++) {
					assert(pb_pin->output_edges[iedge]->num_output_pins == 1);
					int pmode;
					int ifanout;
					pmode = pb_pin->output_edges[iedge]->interconnect->parent_mode->index;
					ifanout = lb_type_rr_node_graph[pin_index].num_fanout[pmode];
					lb_type_rr_node_graph[pin_index].fanout[pmode][ifanout] = 
						pb_pin->output_edges[iedge]->output_pins[0]->pin_count_in_cluster;
					lb_type_rr_node_graph[pin_index].fanout_intrinsic_cost[pmode][ifanout] = 
						get_cost_of_pb_edge(pb_pin->output_edges[iedge]);
					lb_type_rr_node_graph[pin_index].num_fanout[pmode]++;					
				}

				lb_type_rr_node_graph[pin_index].type = LB_INTERMEDIATE;	
			}			
		}

		/* alloc and load output pins that drive other output pins */
		if(parent_node != NULL) {
			/* Top level output pins go to other CLBs, represented different */
			for(int iport = 0; iport < pb_graph_node->num_output_ports; iport++) {
				for(int ipin = 0; ipin < pb_graph_node->num_output_pins[iport]; ipin++) {
					/* load intermediate indices */
					pb_pin = &pb_graph_node->output_pins[iport][ipin];
					pin_index = pb_pin->pin_count_in_cluster;
					num_modes = parent_node->pb_type->num_modes;

					/* alloc and load rr node info */
					lb_type_rr_node_graph[pin_index].capacity = 1;
					lb_type_rr_node_graph[pin_index].num_fanout = (short*)my_calloc(num_modes, sizeof (short));
					lb_type_rr_node_graph[pin_index].fanout = (int**)my_calloc(num_modes, sizeof (int*));
					lb_type_rr_node_graph[pin_index].fanout_intrinsic_cost = (float**)my_calloc(num_modes, sizeof (float*));
				
					/* Count number of mode-dependant fanout */
					for(int iedge = 0; iedge < pb_pin->num_output_edges; iedge++) {
						assert(pb_pin->output_edges[iedge]->num_output_pins == 1);
						int pmode;
						pmode = pb_pin->output_edges[iedge]->interconnect->parent_mode->index;
						lb_type_rr_node_graph[pin_index].num_fanout[pmode]++;
					}

					/* Allocate space based on fanout */
					for(int imode = 0; imode < num_modes; imode++) {
						lb_type_rr_node_graph[pin_index].fanout[imode] = 
							(int*)my_calloc(lb_type_rr_node_graph[pin_index].num_fanout[imode], sizeof (int));
						lb_type_rr_node_graph[pin_index].fanout_intrinsic_cost[imode] = 
							(float*)my_calloc(lb_type_rr_node_graph[pin_index].num_fanout[imode], sizeof (float));
						lb_type_rr_node_graph[pin_index].num_fanout[imode] = 0; /* reset to 0 so that we can reuse this variable to populate fanout stats */
					}

					/* Load edges */
					for(int iedge = 0; iedge < pb_pin->num_output_edges; iedge++) {
						assert(pb_pin->output_edges[iedge]->num_output_pins == 1);
						int pmode;
						int ifanout;
						pmode = pb_pin->output_edges[iedge]->interconnect->parent_mode->index;
						ifanout = lb_type_rr_node_graph[pin_index].num_fanout[pmode];
						lb_type_rr_node_graph[pin_index].fanout[pmode][ifanout] = 
							pb_pin->output_edges[iedge]->output_pins[0]->pin_count_in_cluster;
						lb_type_rr_node_graph[pin_index].fanout_intrinsic_cost[pmode][ifanout] = 
							get_cost_of_pb_edge(pb_pin->output_edges[iedge]);
						lb_type_rr_node_graph[pin_index].num_fanout[pmode]++;					
					}

					lb_type_rr_node_graph[pin_index].type = LB_INTERMEDIATE;				
				}			
			}
		}

		/* alloc and load clock pins that drive other rr nodes */
		for(int iport = 0; iport < pb_graph_node->num_clock_ports; iport++) {
			for(int ipin = 0; ipin < pb_graph_node->num_clock_pins[iport]; ipin++) {
				/* load intermediate indices */
				pb_pin = &pb_graph_node->clock_pins[iport][ipin];
				pin_index = pb_pin->pin_count_in_cluster;
				num_modes = pb_graph_node->pb_type->num_modes;

				/* alloc and load rr node info */
				lb_type_rr_node_graph[pin_index].capacity = 1;
				lb_type_rr_node_graph[pin_index].num_fanout = (short*)my_calloc(num_modes, sizeof (short));
				lb_type_rr_node_graph[pin_index].fanout = (int**)my_calloc(num_modes, sizeof (int*));
				lb_type_rr_node_graph[pin_index].fanout_intrinsic_cost = (float**)my_calloc(num_modes, sizeof (float*));
				
				/* Count number of mode-dependant fanout */
				for(int iedge = 0; iedge < pb_pin->num_output_edges; iedge++) {
					assert(pb_pin->output_edges[iedge]->num_output_pins == 1);
					int pmode;
					pmode = pb_pin->output_edges[iedge]->interconnect->parent_mode->index;
					lb_type_rr_node_graph[pin_index].num_fanout[pmode]++;
				}

				/* Allocate space based on fanout */
				for(int imode = 0; imode < num_modes; imode++) {
					lb_type_rr_node_graph[pin_index].fanout[imode] = 
						(int*)my_calloc(lb_type_rr_node_graph[pin_index].num_fanout[imode], sizeof (int));
					lb_type_rr_node_graph[pin_index].fanout_intrinsic_cost[imode] = 
						(float*)my_calloc(lb_type_rr_node_graph[pin_index].num_fanout[imode], sizeof (float));
					lb_type_rr_node_graph[pin_index].num_fanout[imode] = 0; /* reset to 0 so that we can reuse this variable to populate fanout stats */
				}

				/* Load edges */
				for(int iedge = 0; iedge < pb_pin->num_output_edges; iedge++) {
					assert(pb_pin->output_edges[iedge]->num_output_pins == 1);
					int pmode;
					int ifanout;
					pmode = pb_pin->output_edges[iedge]->interconnect->parent_mode->index;
					ifanout = lb_type_rr_node_graph[pin_index].num_fanout[pmode];
					lb_type_rr_node_graph[pin_index].fanout[pmode][ifanout] = 
						pb_pin->output_edges[iedge]->output_pins[0]->pin_count_in_cluster;
					lb_type_rr_node_graph[pin_index].fanout_intrinsic_cost[pmode][ifanout] = 
						get_cost_of_pb_edge(pb_pin->output_edges[iedge]);
					lb_type_rr_node_graph[pin_index].num_fanout[pmode]++;					
				}

				lb_type_rr_node_graph[pin_index].type = LB_INTERMEDIATE;	
			}
		}
	}
}

/*****************************************************************************************
* Accessor functions 
******************************************************************************************/

/* Return external source index for logic block type internal routing resource graph */
int get_lb_type_rr_graph_ext_source_index(t_type_ptr lb_type) {
	return lb_type->pb_graph_head->total_pb_pins;
}

/* Return external source index for logic block type internal routing resource graph */
int get_lb_type_rr_graph_ext_sink_index(t_type_ptr lb_type) {
	return lb_type->pb_graph_head->total_pb_pins + 1;
}

static float get_cost_of_pb_edge(t_pb_graph_edge *edge) {
	if(edge->delay_max < 0) {
		return 1;
	}
	return 1 + sqrt(edge->delay_max) * 10000; /* jedit I need to better normalize delays, this has fidelity but poor normalization */
}


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
#include <vector>
#include <cmath>

#include "vtr_assert.h"
#include "vtr_memory.h"

#include "vtr_util.h"
#include "physical_types.h"
#include "vpr_types.h"
#include "globals.h"
#include "pack_types.h"
#include "lb_type_rr_graph.h"

using namespace std;

constexpr float INTERNAL_INTERCONNECT_COST = 1.; /* default cost */
constexpr float EXTERNAL_INTERCONNECT_COST = 1000.; /* set cost high to avoid using external interconnect unless necessary */

/*****************************************************************************************
* Internal functions declarations
******************************************************************************************/
static void alloc_and_load_lb_type_rr_graph_for_type(const t_type_ptr lb_type, 
													 t_lb_type_rr_graph& lb_type_rr_graph);
static void alloc_and_load_lb_type_rr_graph_for_pb_graph_node(const t_pb_graph_node *pb_graph_node,
														t_lb_type_rr_graph& lb_type_rr_graph,
														const int ext_rr_index,
                                                        const std::map<int,float>& cluster_pin_to_max_Fc);
static float get_cost_of_pb_edge(t_pb_graph_edge *edge);
static void print_lb_type_rr_graph(FILE *fp, const t_lb_type_rr_graph& lb_type_rr_graph);

/*****************************************************************************************
* Constructor/Destructor functions 
******************************************************************************************/

/* Populate each logic block type (type_descriptor) with a directed graph that respresents the interconnect within it.
*/
std::vector<t_lb_type_rr_graph> alloc_and_load_all_lb_type_rr_graph() {
    auto& device_ctx = g_vpr_ctx.device();

	std::vector<t_lb_type_rr_graph> lb_type_rr_graphs(device_ctx.num_block_types);

	for(int i = 0; i < device_ctx.num_block_types; i++) {
		if(&device_ctx.block_types[i] != device_ctx.EMPTY_TYPE) {
			 alloc_and_load_lb_type_rr_graph_for_type(&device_ctx.block_types[i], lb_type_rr_graphs[i]);
		}
	}
	return lb_type_rr_graphs;
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

/* Returns total number of modes that this lb_type_rr_node can take */
int get_num_modes_of_lb_type_rr_node(const t_lb_type_rr_node &lb_type_rr_node) {
	int num_modes = 1;
	t_pb_graph_node *pb_graph_node;
	t_pb_type *pb_type;
	if(lb_type_rr_node.pb_graph_pin != nullptr) {
		if(lb_type_rr_node.pb_graph_pin->port->type == OUT_PORT) {
			pb_graph_node = lb_type_rr_node.pb_graph_pin->parent_node->parent_pb_graph_node;
			if (pb_graph_node == nullptr) {
				/* Top level logic block output pins connect to external routing */
				num_modes = 1;
			} else {
				pb_type = pb_graph_node->pb_type;
				num_modes = pb_type->num_modes;
			}
		} else {
			pb_type = lb_type_rr_node.pb_graph_pin->parent_node->pb_type;
			num_modes = pb_type->num_modes;
			if(num_modes == 0) {
				num_modes = 1; /* The rr graph is designed so that minimum number for a mode is 1 */
			}
		}
	}
	return num_modes;
}

/*****************************************************************************************
* Debug functions 
******************************************************************************************/

/* Output all logic block type pb graphs */
void echo_lb_type_rr_graphs(char *filename, const std::vector<t_lb_type_rr_graph>& lb_type_rr_graphs) {
	FILE *fp;
	fp = vtr::fopen(filename, "w");

    auto& device_ctx = g_vpr_ctx.device();
	for(int itype = 0; itype < device_ctx.num_block_types; itype++) {
		if(&device_ctx.block_types[itype] != device_ctx.EMPTY_TYPE) {
			fprintf(fp, "--------------------------------------------------------------\n");
			fprintf(fp, "Intra-Logic Block Routing Resource For Type %s\n", device_ctx.block_types[itype].name);
			fprintf(fp, "--------------------------------------------------------------\n");
			fprintf(fp, "\n");
			print_lb_type_rr_graph(fp, lb_type_rr_graphs[itype]);
		}
	}

	fclose(fp);
}


/******************************************************************************************
* Internal functions
******************************************************************************************/

/* Given a logic block type, build its internal routing resource graph
   Each pb_graph_pin has a corresponding lb_type_rr_node that is indexed by the pin index of that pb_graph_pin
   Extra sources and sinks in lb_type_rr_graph that do not correspond to a pb_graph_pin are appended in indices following total_pb_pins
   of the pb_type
*/
static void alloc_and_load_lb_type_rr_graph_for_type(const t_type_ptr lb_type, 
													 t_lb_type_rr_graph& lb_type_rr_graph) {
	t_pb_type *pb_type;
	t_pb_graph_node *pb_graph_head;
	int ext_source_index, ext_sink_index, ext_rr_index;
	
	VTR_ASSERT(lb_type_rr_graph.nodes.empty());

	pb_type = lb_type->pb_type;
	pb_graph_head = lb_type->pb_graph_head;

	/* Indices [0..total_pb_pins - 1] have a 1-to-1 matching with a pb_graph_pin
	   Index total_pb_pins represents all external sources to logic block type
	   Index total_pin_pins + 1 represents all external sinks from logic block type
	   Index total_pin_pins + 2 represents feedback connections from external rr graph back into cluster
	   All nodes after total_pin_pins + 2 are other nodes that do not have a corresponding pb_graph_pin (eg. a sink node that is driven by one or more pb_graph_pins of a primitive)
	*/
	lb_type_rr_graph.nodes.resize(pb_graph_head->total_pb_pins + 3);

	/* Define the external source, sink, and external interconnect for the routing resource graph of the logic block type */
	ext_source_index = pb_graph_head->total_pb_pins;
	ext_sink_index = pb_graph_head->total_pb_pins + 1;
	ext_rr_index = pb_graph_head->total_pb_pins + 2;

	VTR_ASSERT(	lb_type_rr_graph.nodes[ext_source_index].type == NUM_LB_RR_TYPES && 
			lb_type_rr_graph.nodes[ext_sink_index].type == NUM_LB_RR_TYPES);

    //Build a pin-to-fc_spec look-up
    // Note that since we really only care about unconnected (fc_value == 0) cases,
    // we don't need to check whether the Fc value type, since both absolute and fractional
    // are equivalent at zero
    std::map<int,float> pin_to_max_fc;
    for (auto& fc_spec : lb_type->fc_specs) {
        for (auto pin : fc_spec.pins) {
            if (pin_to_max_fc.count(pin)) {
                pin_to_max_fc[pin] = fc_spec.fc_value;
            } else {
                pin_to_max_fc[pin] = std::max(pin_to_max_fc[pin], fc_spec.fc_value);
            }
        }
    }

	/*******************************************************************************
	* Build logic block source node 
	*******************************************************************************/

	/* External source node drives all inputs going into logic block type */
    lb_type_rr_graph.nodes[ext_source_index].outedges.resize(1); //Default mode
	lb_type_rr_graph.nodes[ext_source_index].type = LB_SOURCE;
	
    //Connect external souce node to input and clock pins of logic block type
    //
    // Note that we *only* connect pins with non-zero maximum Fc to the generic soruce
    // (i.e. skipping those which are not connected to the global routing network).
    // 
    // This ensures the packer does not try to use such pins to connect to signals on
    // the global routing network.
    // 
    // Failure to do this causes global routing to fail, since the global routing network
    // can not actually reach Fc = 0 pins.
	for(int iport = 0; iport < pb_graph_head->num_input_ports; iport++) {
		for(int ipin = 0; ipin < pb_graph_head->num_input_pins[iport]; ipin++) {
            int cluster_pin = pb_graph_head->input_pins[iport][ipin].pin_count_in_cluster;
            VTR_ASSERT_MSG(pin_to_max_fc.count(cluster_pin), "Fc must be specified for pin");
            if (pin_to_max_fc[cluster_pin] != 0.) {
                lb_type_rr_graph.nodes[ext_source_index].outedges[0].emplace_back(cluster_pin, INTERNAL_INTERCONNECT_COST);
            }
		}
	}

	for(int iport = 0; iport < pb_graph_head->num_clock_ports; iport++) {
		for(int ipin = 0; ipin < pb_graph_head->num_clock_pins[iport]; ipin++) {
            int cluster_pin = pb_graph_head->clock_pins[iport][ipin].pin_count_in_cluster;
            VTR_ASSERT_MSG(pin_to_max_fc.count(cluster_pin), "Fc must be specified for pin");
            if (pin_to_max_fc[cluster_pin] != 0.) {
                lb_type_rr_graph.nodes[ext_source_index].outedges[0].emplace_back(cluster_pin, INTERNAL_INTERCONNECT_COST);
            }
		}
	}
	
	lb_type_rr_graph.nodes[ext_source_index].capacity = lb_type_rr_graph.nodes[ext_source_index].outedges[0].size();

	VTR_ASSERT(lb_type_rr_graph.nodes[ext_source_index].capacity <= pb_type->num_input_pins + pb_type->num_clock_pins);

	/*******************************************************************************
	* Build logic block sink node 
	*******************************************************************************/

	/* External sink node driven by all outputs exiting logic block type */
	lb_type_rr_graph.nodes[ext_sink_index].capacity = pb_type->num_output_pins;
	lb_type_rr_graph.nodes[ext_sink_index].outedges.resize(1);
	lb_type_rr_graph.nodes[ext_sink_index].type = LB_SINK;

	/*******************************************************************************
	* Build node that approximates external interconnect
	*******************************************************************************/

	/* External rr node that drives all existing logic block input pins and is driven by all outputs exiting logic block type */
	lb_type_rr_graph.nodes[ext_rr_index].capacity = pb_type->num_output_pins;
	lb_type_rr_graph.nodes[ext_rr_index].outedges.resize(1);
	lb_type_rr_graph.nodes[ext_rr_index].type = LB_INTERMEDIATE;
	
	/* Connect opin of logic block to sink */
	lb_type_rr_graph.nodes[ext_rr_index].outedges[0].emplace_back(ext_sink_index, INTERNAL_INTERCONNECT_COST);

	/* Connect opin of logic block to all input and clock pins of logic block type */
	for(int iport = 0; iport < pb_graph_head->num_input_ports; iport++) {
		for(int ipin = 0; ipin < pb_graph_head->num_input_pins[iport]; ipin++) {
            int cluster_pin = pb_graph_head->input_pins[iport][ipin].pin_count_in_cluster;
            VTR_ASSERT_MSG(pin_to_max_fc.count(cluster_pin), "Fc must be specified for pin");
            if (pin_to_max_fc[cluster_pin] != 0.) {
                lb_type_rr_graph.nodes[ext_rr_index].outedges[0].emplace_back(cluster_pin, EXTERNAL_INTERCONNECT_COST);
            }
		}
	}
	for(int iport = 0; iport < pb_graph_head->num_clock_ports; iport++) {
		for(int ipin = 0; ipin < pb_graph_head->num_clock_pins[iport]; ipin++) {
            int cluster_pin = pb_graph_head->clock_pins[iport][ipin].pin_count_in_cluster;
            VTR_ASSERT_MSG(pin_to_max_fc.count(cluster_pin), "Fc must be specified for pin");
            if (pin_to_max_fc[cluster_pin] != 0.) {
                lb_type_rr_graph.nodes[ext_rr_index].outedges[0].emplace_back(cluster_pin, EXTERNAL_INTERCONNECT_COST);
            }
		}
	}	

	alloc_and_load_lb_type_rr_graph_for_pb_graph_node(pb_graph_head, lb_type_rr_graph, ext_rr_index, pin_to_max_fc);

    lb_type_rr_graph.nodes.shrink_to_fit();
}


/* Given a pb_graph_node, build the routing resource data for it.
   Each pb_graph_pin has a corresponding rr node.  This rr node is indexed by the pb_graph_pin pin index.
   This function populates the rr node for the pb_graph_pin of the current pb_graph_node then recursively
   repeats this on all children of the pb_graph_node
*/
static void alloc_and_load_lb_type_rr_graph_for_pb_graph_node(const t_pb_graph_node *pb_graph_node,
													t_lb_type_rr_graph &lb_type_rr_graph,
													const int ext_rr_index,
                                                    const std::map<int,float>& cluster_pin_to_max_Fc) {
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
			bool port_equivalent = false;
			int sink_index = OPEN;
			for(int ipin = 0; ipin < pb_graph_node->num_input_pins[iport]; ipin++) {
				/* load intermediate indices */
				pb_pin = &pb_graph_node->input_pins[iport][ipin];
                port_equivalent = pb_pin->port->equivalent;
				pin_index = pb_pin->pin_count_in_cluster;

				/* alloc and load rr node info */
				lb_type_rr_graph.nodes[pin_index].capacity = 1;
                lb_type_rr_graph.nodes[pin_index].outedges.resize(1);
				lb_type_rr_graph.nodes[pin_index].type = LB_INTERMEDIATE;
				lb_type_rr_graph.nodes[pin_index].pb_graph_pin = pb_pin;

				if(port_equivalent != true || sink_index == OPEN) {
					/* Create new sink for input to primitive */
					t_lb_type_rr_node new_sink;					
					if(port_equivalent == true) {
						new_sink.capacity = pb_pin->port->num_pins;
					} else {
						new_sink.capacity = 1;
					}
					new_sink.type = LB_SINK;				
                    new_sink.outedges.resize(1);
					sink_index = lb_type_rr_graph.nodes.size();
					lb_type_rr_graph.nodes.push_back(new_sink);					
				}
				lb_type_rr_graph.nodes[pin_index].outedges[0].emplace_back(sink_index, INTERNAL_INTERCONNECT_COST);
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
				lb_type_rr_graph.nodes[pin_index].capacity = 1;
                lb_type_rr_graph.nodes[pin_index].outedges.resize(num_modes);
				lb_type_rr_graph.nodes[pin_index].pb_graph_pin = pb_pin;
				
                lb_type_rr_graph.nodes[pin_index].outedges.resize(num_modes);

				/* Load edges */
				for(int iedge = 0; iedge < pb_pin->num_output_edges; iedge++) {
					VTR_ASSERT(pb_pin->output_edges[iedge]->num_output_pins == 1);
					int pmode = pb_pin->output_edges[iedge]->interconnect->parent_mode->index;

                    int cluster_pin = pb_pin->output_edges[iedge]->output_pins[0]->pin_count_in_cluster;

                    lb_type_rr_graph.nodes[pin_index].outedges[pmode].emplace_back(cluster_pin, get_cost_of_pb_edge(pb_pin->output_edges[iedge]));
				}

				lb_type_rr_graph.nodes[pin_index].type = LB_SOURCE;				
			}			
		}


		/* alloc and load clock pins that connect to sinks */
		for(int iport = 0; iport < pb_graph_node->num_clock_ports; iport++) {
			bool port_equivalent = false;
			int sink_index = OPEN;
			for(int ipin = 0; ipin < pb_graph_node->num_clock_pins[iport]; ipin++) {
				/* load intermediate indices */
				pb_pin = &pb_graph_node->clock_pins[iport][ipin];
				port_equivalent = pb_pin->port->equivalent;
				pin_index = pb_pin->pin_count_in_cluster;

				/* alloc and load rr node info */
				lb_type_rr_graph.nodes[pin_index].capacity = 1;
                lb_type_rr_graph.nodes[pin_index].outedges.resize(1);
				lb_type_rr_graph.nodes[pin_index].type = LB_INTERMEDIATE;
				lb_type_rr_graph.nodes[pin_index].pb_graph_pin = pb_pin;

				if(port_equivalent != true || sink_index == OPEN) {
					/* Create new sink for clock to primitive */
					t_lb_type_rr_node new_sink;					
					if(port_equivalent == true) {
						new_sink.capacity = pb_pin->port->num_pins;
					} else {
						new_sink.capacity = 1;
					}
					new_sink.type = LB_SINK;					
                    new_sink.outedges.resize(1);
					sink_index = lb_type_rr_graph.nodes.size();
					lb_type_rr_graph.nodes.push_back(new_sink);					
				}
				lb_type_rr_graph.nodes[pin_index].outedges[0].emplace_back(sink_index, INTERNAL_INTERCONNECT_COST);
			}
		}
	} else {
		/* This pb_graph_node is a logic block or subcluster */
		for(int imode = 0; imode < pb_type->num_modes; imode++) {
			for(int ipb_type = 0; ipb_type < pb_type->modes[imode].num_pb_type_children; ipb_type++) {
				for(int ipb = 0; ipb < pb_type->modes[imode].pb_type_children[ipb_type].num_pb; ipb++) {
					alloc_and_load_lb_type_rr_graph_for_pb_graph_node(
						&pb_graph_node->child_pb_graph_nodes[imode][ipb_type][ipb], lb_type_rr_graph, ext_rr_index, cluster_pin_to_max_Fc);
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
				lb_type_rr_graph.nodes[pin_index].capacity = 1;
                lb_type_rr_graph.nodes[pin_index].outedges.resize(num_modes);
				lb_type_rr_graph.nodes[pin_index].pb_graph_pin = pb_pin;
				
				/* Load edges */
				for(int iedge = 0; iedge < pb_pin->num_output_edges; iedge++) {
					VTR_ASSERT(pb_pin->output_edges[iedge]->num_output_pins == 1);
					int pmode = pb_pin->output_edges[iedge]->interconnect->parent_mode->index;
                    int cluster_pin = pb_pin->output_edges[iedge]->output_pins[0]->pin_count_in_cluster;

					lb_type_rr_graph.nodes[pin_index].outedges[pmode].emplace_back(cluster_pin, get_cost_of_pb_edge(pb_pin->output_edges[iedge]));
				}

				lb_type_rr_graph.nodes[pin_index].type = LB_INTERMEDIATE;	
			}
		}

		/* alloc and load output pins that drive other output pins */
		if(parent_node == nullptr) {
			/* Top level output pins go to other CLBs, represented different */
			for(int iport = 0; iport < pb_graph_node->num_output_ports; iport++) {
				for(int ipin = 0; ipin < pb_graph_node->num_output_pins[iport]; ipin++) {
					/* load intermediate indices */
					pb_pin = &pb_graph_node->output_pins[iport][ipin];
					pin_index = pb_pin->pin_count_in_cluster;
					num_modes = 1; /* One edge to external sinks */
					lb_type_rr_graph.nodes[pin_index].type = LB_INTERMEDIATE;

					/* alloc and load rr node info */
					lb_type_rr_graph.nodes[pin_index].capacity = 1;
					lb_type_rr_graph.nodes[pin_index].pb_graph_pin = pb_pin;

					lb_type_rr_graph.nodes[pin_index].outedges.resize(num_modes);

					/* Load one edge to external opin */;
                    int cluster_pin = pb_pin->pin_count_in_cluster;
                    auto itr = cluster_pin_to_max_Fc.find(cluster_pin);
                    VTR_ASSERT_MSG(itr != cluster_pin_to_max_Fc.end(), "Fc must be specified for pin");
                    if (itr->second != 0.) {
                        //This pin connects to global routing
                        lb_type_rr_graph.nodes[pin_index].outedges[0].emplace_back(ext_rr_index, INTERNAL_INTERCONNECT_COST);
                    }
				}			
			}
		} else {
			/* Subcluster output pins */
			for(int iport = 0; iport < pb_graph_node->num_output_ports; iport++) {
				for(int ipin = 0; ipin < pb_graph_node->num_output_pins[iport]; ipin++) {
					/* load intermediate indices */
					pb_pin = &pb_graph_node->output_pins[iport][ipin];
					pin_index = pb_pin->pin_count_in_cluster;
					num_modes = parent_node->pb_type->num_modes;

					/* alloc and load rr node info */
					lb_type_rr_graph.nodes[pin_index].capacity = 1;
                    lb_type_rr_graph.nodes[pin_index].outedges.resize(num_modes);
					lb_type_rr_graph.nodes[pin_index].pb_graph_pin = pb_pin;
				
					/* Load edges */
					for(int iedge = 0; iedge < pb_pin->num_output_edges; iedge++) {
						VTR_ASSERT(pb_pin->output_edges[iedge]->num_output_pins == 1);
						int pmode = pb_pin->output_edges[iedge]->interconnect->parent_mode->index;
                        int cluster_pin = pb_pin->output_edges[iedge]->output_pins[0]->pin_count_in_cluster;
						lb_type_rr_graph.nodes[pin_index].outedges[pmode].emplace_back(cluster_pin, get_cost_of_pb_edge(pb_pin->output_edges[iedge]));
					}

					lb_type_rr_graph.nodes[pin_index].type = LB_INTERMEDIATE;				
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
				lb_type_rr_graph.nodes[pin_index].capacity = 1;
				lb_type_rr_graph.nodes[pin_index].outedges.resize(num_modes);
				lb_type_rr_graph.nodes[pin_index].pb_graph_pin = pb_pin;

				/* Load edges */
				for(int iedge = 0; iedge < pb_pin->num_output_edges; iedge++) {
					VTR_ASSERT(pb_pin->output_edges[iedge]->num_output_pins == 1);
					int pmode = pb_pin->output_edges[iedge]->interconnect->parent_mode->index;
                    int cluster_pin = pb_pin->output_edges[iedge]->output_pins[0]->pin_count_in_cluster;
					lb_type_rr_graph.nodes[pin_index].outedges[pmode].emplace_back(cluster_pin, get_cost_of_pb_edge(pb_pin->output_edges[iedge]));
				}

				lb_type_rr_graph.nodes[pin_index].type = LB_INTERMEDIATE;	
			}
		}
	}
}

/* Determine intrinsic cost of an edge that joins two pb_graph_pins */
static float get_cost_of_pb_edge(t_pb_graph_edge* /*edge*/) {
	return 1;
}

/* Print logic block type routing resource graph */
static void print_lb_type_rr_graph(FILE *fp, const t_lb_type_rr_graph& lb_type_rr_graph) {
	for(unsigned int inode = 0; inode < lb_type_rr_graph.nodes.size(); inode++) {
		fprintf(fp, "Node %d\n", inode);
		if(lb_type_rr_graph.nodes[inode].pb_graph_pin != nullptr) {
			t_pb_graph_node *pb_graph_node = lb_type_rr_graph.nodes[inode].pb_graph_pin->parent_node;
			fprintf(fp, "\t%s[%d].%s[%d]\n", pb_graph_node->pb_type->name,
											 pb_graph_node->placement_index,
											 lb_type_rr_graph.nodes[inode].pb_graph_pin->port->name,
											 lb_type_rr_graph.nodes[inode].pb_graph_pin->pin_number
				);

		}
		fprintf(fp, "\tType: %s\n", lb_rr_type_str[(int) lb_type_rr_graph.nodes[inode].type]);
		fprintf(fp, "\tCapacity: %d\n", lb_type_rr_graph.nodes[inode].capacity);
		fprintf(fp, "\tIntrinsic Cost: %g\n", lb_type_rr_graph.nodes[inode].intrinsic_cost);
		for(int imode = 0; imode < get_num_modes_of_lb_type_rr_node(lb_type_rr_graph.nodes[inode]); imode++) {
            fprintf(fp, "\tMode: %d   # Outedges: %d\n\t\t", imode, lb_type_rr_graph.nodes[inode].num_fanout(imode));
            int count = 0;
            for(int iedge = 0; iedge < lb_type_rr_graph.nodes[inode].num_fanout(imode); iedge++) {
                if(count % 5 == 0) {
                    /* Formatting to prevent very long lines */
                    fprintf(fp, "\n");
                }
                count++;
                fprintf(fp, "(%d, %g) ", lb_type_rr_graph.nodes[inode].outedges[imode][iedge].node_index, 
                    lb_type_rr_graph.nodes[inode].outedges[imode][iedge].intrinsic_cost);
            }
            fprintf(fp, "\n");
		}

		fprintf(fp, "\n");
	}
}


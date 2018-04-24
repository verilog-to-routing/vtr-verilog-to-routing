/* 
  Functions to creates, manipulate, and free the lb_type_rr_node graph that represents interconnect within a logic block type.

  Author: Jason Luu
  Date: July 22, 2013
 */

#ifndef LB_TYPE_RR_GRAPH_H
#define LB_TYPE_RR_GRAPH_H

#include "pack_types.h"

/**************************************************************************
* Intra-Logic Block Routing Data Structures (by type)
***************************************************************************/
/* Describes different types of intra-logic cluster routing resource nodes */
enum e_lb_rr_type {
	LB_SOURCE = 0, LB_SINK, LB_INTERMEDIATE, NUM_LB_RR_TYPES
};
const std::vector<const char*> lb_rr_type_str {
    "LB_SOURCE", "LB_SINK", "LB_INTERMEDIATE", "INVALID"
};


/* Output edges of a t_lb_type_rr_node */
struct t_lb_type_rr_node_edge {
    t_lb_type_rr_node_edge (int node, float cost)
        : node_index(node), intrinsic_cost(cost) {}

	int node_index;
	float intrinsic_cost;
};

/* Describes a routing resource node within a logic cluster type */
struct t_lb_type_rr_node {
	short capacity;			/* Number of nets that can simultaneously use this node */
	enum e_lb_rr_type type;	/* Type of logic cluster_ctx.blocks resource node */	

    std::vector<std::vector<t_lb_type_rr_node_edge>> outedges; /* [0..num_modes - 1][0..num_fanout-1] index and cost of out edges */

	t_pb_graph_pin *pb_graph_pin;	/* pb_graph_pin associated with this lb_rr_node if exists, NULL otherwise */
	float intrinsic_cost;					/* cost of this node */
	
	t_lb_type_rr_node() {
		capacity = 0;
		type = NUM_LB_RR_TYPES;
		pb_graph_pin = nullptr;
		intrinsic_cost = 0;
	}

    short num_fanout(int mode) const { 
        VTR_ASSERT(mode < (int)outedges.size());
        return outedges[mode].size();
    }
};

/*
 * The routing resource graph for a logic cluster type
 */
struct t_lb_type_rr_graph {
    std::vector<t_lb_type_rr_node> nodes;

    std::vector<int> external_sources; //External source nodes
    std::vector<int> external_sinks; //External sink nodes
    std::vector<int> external_rr; //External routing nodes
};


/* Constructors/Destructors */
std::vector <t_lb_type_rr_graph> alloc_and_load_all_lb_type_rr_graph();

/* Accessor functions */
int get_lb_type_rr_graph_ext_source_index(t_type_ptr lb_type);
int get_lb_type_rr_graph_ext_sink_index(t_type_ptr lb_type);
int get_num_modes_of_lb_type_rr_node(const t_lb_type_rr_node &lb_type_rr_node);

/* Debug functions */
void echo_lb_type_rr_graphs(char *filename, const std::vector<t_lb_type_rr_graph>& lb_type_rr_graphs);

#endif



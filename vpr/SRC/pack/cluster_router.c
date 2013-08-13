/* 
  Intra-logic block router determines if a candidate packing solution (or intermediate solution) can route.
  
  Global Inputs: Architecture and netlist
  Input arguments: clustering info for one cluster (t_pb info)
  Working data set: t_routing_data contains intermediate work
  Output: Routable? True/False.  If true, store/return the routed solution.

  Routing algorithm used is Pathfinder.

  Author: Jason Luu
  Date: July 22, 2013
 */

#include <cstdio>
#include <cstring>
using namespace std;

#include <assert.h>
#include <vector>
#include <map>
#include <queue>
#include <cmath>

#include "util.h"
#include "physical_types.h"
#include "vpr_types.h"
#include "globals.h"
#include "vpr_utils.h"
#include "pack_types.h"
#include "lb_type_rr_graph.h"
#include "cluster_router.h"

/*****************************************************************************************
* Internal data structures
******************************************************************************************/

enum e_commit_remove {RT_COMMIT, RT_REMOVE};

/*****************************************************************************************
* Internal functions declarations
******************************************************************************************/
static void free_intra_lb_nets(vector <t_intra_lb_net> *intra_lb_nets);
static void free_lb_trace(t_lb_trace *lb_trace);
static void add_pin_to_rt_terminals(t_lb_router_data *router_data, int iatom, int iport, int ipin, t_model_ports *model_port);
static void remove_pin_from_rt_terminals(t_lb_router_data *router_data, int iatom, int iport, int ipin, t_model_ports *model_port);


static void commit_remove_rt(t_lb_router_data *router_data, int inet, e_commit_remove op);
static void add_source_to_rt(t_lb_router_data *router_data, int inet);
static void expand_rt(t_lb_router_data *router_data, int inet, vector<int> &explored_nodes, 
	priority_queue<t_expansion_node, vector <t_expansion_node>, compare_expansion_node> &pq);
static void expand_node(t_lb_router_data *router_data, t_expansion_node exp_node, 
	priority_queue<t_expansion_node, vector <t_expansion_node>, compare_expansion_node> &pq);
static void add_to_rt(t_lb_trace *rt, int node_index, t_explored_node_tb *node_traceback);
static boolean is_route_success(t_lb_router_data *router_data);
static t_lb_trace *find_node_in_rt(t_lb_trace *rt, int rt_index);
/*****************************************************************************************
* Constructor/Destructor functions 
******************************************************************************************/

/**
 Build data structures used by intra-logic block router
 */
t_lb_router_data *alloc_and_load_router_data(INP vector<t_lb_type_rr_node> *lb_type_graph) {
	t_lb_router_data *router_data = new t_lb_router_data;
	int size;

	router_data->lb_type_graph = lb_type_graph;
	size = router_data->lb_type_graph->size();
	router_data->lb_rr_node_stats = new t_lb_rr_node_stats[size];
	router_data->intra_lb_nets = new vector<t_intra_lb_net>;

	return router_data;
}

/* free data used by router */
void free_router_data(INOUTP t_lb_router_data *router_data) {
	if(router_data != NULL && router_data->lb_type_graph != NULL) {
		delete [] router_data->lb_rr_node_stats;
		router_data->lb_type_graph = NULL;
		router_data->lb_rr_node_stats = NULL;
		free_intra_lb_nets(router_data->intra_lb_nets);
		router_data->intra_lb_nets = NULL;
		delete router_data;
	}
}


/*****************************************************************************************
* Routing Functions
******************************************************************************************/

/* Add pins of netlist atom to to current routing drivers/targets */
void add_atom_as_target(INOUTP t_lb_router_data *router_data, INP int iatom) {
	t_pb *pb;
	t_pb_type *pb_type;
	t_pb_graph_node *pb_graph_node;
	t_model *model;
	t_model_ports *model_ports;
	int iport, inet;

	pb = logical_block[iatom].pb;
	pb_graph_node = pb->pb_graph_node;
	pb_type = pb_graph_node->pb_type;	

	set_reset_pb_modes(router_data, pb, TRUE);

	model = logical_block[iatom].model;
	
	/* Add inputs to route tree sources/targets */	
	model_ports = model->inputs;
	while(model_ports != NULL) {
		if(model_ports->is_clock == FALSE) {
			iport = model_ports->index;
			for (int ipin = 0; ipin > model_ports->size; ipin++) {
				inet = logical_block[iatom].input_nets[iport][ipin];
				if(inet != OPEN) {
					add_pin_to_rt_terminals(router_data, iatom, iport, ipin, model_ports);
				}
			}
		}
	}

	/* Add outputs to route tree sources/targets */	
	model_ports = model->outputs;
	while(model_ports != NULL) {
		iport = model_ports->index;
		for (int ipin = 0; ipin > model_ports->size; ipin++) {
			inet = logical_block[iatom].output_nets[iport][ipin];
			if(inet != OPEN) {
				add_pin_to_rt_terminals(router_data, iatom, iport, ipin, model_ports);
			}
		}
	}

	/* Add clock to route tree sources/targets */	
	model_ports = model->inputs;
	while(model_ports != NULL) {
		if(model_ports->is_clock == TRUE) {
			iport = model_ports->index;
			assert(iport == 0);
			for (int ipin = 0; ipin > model_ports->size; ipin++) {
				assert(ipin == 0);
				inet = logical_block[iatom].clock_net;
				if(inet != OPEN) {
					add_pin_to_rt_terminals(router_data, iatom, iport, ipin, model_ports);
				}
			}
		}
	}
}

/* Remove pins of netlist atom from current routing drivers/targets */
void remove_atom_from_target(INOUTP t_lb_router_data *router_data, INP int iatom) {
	t_pb *pb;
	t_pb_type *pb_type;
	t_pb_graph_node *pb_graph_node;
	t_model *model;
	t_model_ports *model_ports;
	int iport, inet;

	pb = logical_block[iatom].pb;
	pb_graph_node = pb->pb_graph_node;
	pb_type = pb_graph_node->pb_type;	
	
	set_reset_pb_modes(router_data, pb, FALSE);
		
	model = logical_block[iatom].model;
	
	/* Remove inputs from route tree sources/targets */	
	model_ports = model->inputs;
	while(model_ports != NULL) {
		if(model_ports->is_clock == FALSE) {
			iport = model_ports->index;
			for (int ipin = 0; ipin > model_ports->size; ipin++) {
				inet = logical_block[iatom].input_nets[iport][ipin];
				if(inet != OPEN) {
					remove_pin_from_rt_terminals(router_data, iatom, iport, ipin, model_ports);
				}
			}
		}
	}

	/* Remove outputs from route tree sources/targets */	
	model_ports = model->outputs;
	while(model_ports != NULL) {
		iport = model_ports->index;
		for (int ipin = 0; ipin > model_ports->size; ipin++) {
			inet = logical_block[iatom].output_nets[iport][ipin];
			if(inet != OPEN) {
				remove_pin_from_rt_terminals(router_data, iatom, iport, ipin, model_ports);
			}
		}
	}

	/* Remove clock from route tree sources/targets */	
	model_ports = model->inputs;
	while(model_ports != NULL) {
		if(model_ports->is_clock == TRUE) {
			iport = model_ports->index;
			assert(iport == 0);
			for (int ipin = 0; ipin > model_ports->size; ipin++) {
				assert(ipin == 0);
				inet = logical_block[iatom].clock_net;
				if(inet != OPEN) {
					remove_pin_from_rt_terminals(router_data, iatom, iport, ipin, model_ports);
				}
			}
		}
	}
}

/* Set/Reset mode of rr nodes to the pb used.  If set == TRUE, then set all modes of the rr nodes affected by pb to the mode of the pb.
   Set all modes related to pb to 0 otherwise */
void set_reset_pb_modes(INOUTP t_lb_router_data *router_data, INP t_pb *pb, INP boolean set) {
	t_pb_type *pb_type;
	t_pb_graph_node *pb_graph_node;
	int mode = pb->mode;
	int inode;
		
	pb_graph_node = pb->pb_graph_node;
	pb_type = pb_graph_node->pb_type;

	/* Input and clock pin modes are based on current pb mode */
	for(int iport = 0; iport < pb_graph_node->num_input_ports; iport++) {
		for(int ipin = 0; ipin < pb_graph_node->num_input_pins[iport]; ipin++) {
			inode = pb_graph_node->input_pins[iport][ipin].pin_count_in_cluster;
			router_data->lb_rr_node_stats[inode].mode = (set == TRUE) ? mode : 0;
		}
	}
	for(int iport = 0; iport < pb_graph_node->num_clock_ports; iport++) {
		for(int ipin = 0; ipin < pb_graph_node->num_clock_pins[iport]; ipin++) {
			inode = pb_graph_node->clock_pins[iport][ipin].pin_count_in_cluster;
			router_data->lb_rr_node_stats[inode].mode = (set == TRUE) ? mode : 0;
		}
	}

	/* Output pin modes are based on parent pb, so set children to use new mode */
	for(int ichild_type = 0; ichild_type < pb_type->modes[mode].num_pb_type_children; ichild_type++) {
		for(int ichild = 0; ichild < pb_type->modes[mode].pb_type_children[ichild_type].num_pb; ichild++) {
			t_pb_graph_node *child_pb_graph_node = &pb_graph_node->child_pb_graph_nodes[mode][ichild_type][ichild];
			for(int iport = 0; iport < child_pb_graph_node->num_output_ports; iport++) {
				for(int ipin = 0; ipin < child_pb_graph_node->num_output_pins[iport]; ipin++) {
					inode = child_pb_graph_node->output_pins[iport][ipin].pin_count_in_cluster;
					router_data->lb_rr_node_stats[inode].mode = (set == TRUE) ? mode : 0;
				}
			}
		}
	}
}

/* Save current route solution 
   Requires that current route solution is correct
*/
void save_current_route(INOUTP t_lb_router_data *router_data) {
	assert(router_data->is_routed == TRUE);
	vector<t_intra_lb_net> &lb_nets = *router_data->intra_lb_nets;
	for(unsigned int inet = 0; inet < lb_nets.size(); inet++) {
		free_lb_trace(lb_nets[inet].saved_rt_tree);
		lb_nets[inet].saved_rt_tree = (*router_data->intra_lb_nets)[inet].rt_tree;
		lb_nets[inet].rt_tree = NULL;
	}	
}

/* Restore previous route solution 
   Assumes previous route solution is correct
*/
void restore_previous_route(INOUTP t_lb_router_data *router_data) {
	vector<t_intra_lb_net> &lb_nets = *router_data->intra_lb_nets;
	router_data->is_routed = TRUE;
	for(unsigned int inet = 0; inet < lb_nets.size(); inet++) {
		free_lb_trace(lb_nets[inet].rt_tree);
		lb_nets[inet].rt_tree = (*router_data->intra_lb_nets)[inet].saved_rt_tree;
		lb_nets[inet].saved_rt_tree = NULL;
	}	
}


/* Attempt to route routing driver/targets on the current architecture 
   Follows pathfinder negotiated congestion algorithm
*/
boolean try_route(INOUTP t_lb_router_data *router_data) {
	vector <t_intra_lb_net> & lb_nets = *router_data->intra_lb_nets;
	vector <t_lb_type_rr_node> & lb_type_graph = *router_data->lb_type_graph;
	boolean is_routed = FALSE;
	boolean is_impossible = FALSE;
	t_expansion_node exp_node;

	/* Stores state info during route */
	priority_queue <t_expansion_node, vector <t_expansion_node>, compare_expansion_node> pq;	/* Priority queue used to find lowest cost next node to explore */
	vector<int> explored_nodes;				/* List of already explored nodes to clear incrementally later */
	t_explored_node_tb *node_traceback = new t_explored_node_tb[lb_type_graph.size()]; /* Store traceback info for explored nodes */
	
	/*	Iteratively remove congestion until a successful route is found.  
		Cap the total number of iterations tried so that if a solution does not exist, then the router won't run indefinately */
	for(int iter = 0; iter < router_data->params.max_iterations && is_routed == FALSE && is_impossible == FALSE; iter++) {

		/* Iterate across all nets internal to logic block */
		for(unsigned int inet = 0; inet < lb_nets.size() && is_impossible == FALSE; inet++) {
			commit_remove_rt(router_data, inet, RT_REMOVE);
			add_source_to_rt(router_data, inet);

			/* Route each sink of net */
			for(unsigned int itarget = 1; itarget < lb_nets[inet].terminals.size() && is_impossible == FALSE; itarget++) {
				/* Get lowest cost next node, repeat until a path is found or if it is impossible to route */
				expand_rt(router_data, inet, explored_nodes, pq);
				do {
					if(pq.empty()) {
						/* No connection possible */
						is_impossible = TRUE;
					} else {
						exp_node = pq.top();
						pq.pop();
						if(exp_node.node_index != lb_nets[inet].terminals[itarget] && 
							node_traceback[exp_node.node_index].isExplored == FALSE) {
							/* First time node is popped implies path to this node is the lowest cost.
							   If the node is popped a second time, then the path to that node is higher than this path so
							   ignore.
							*/
							node_traceback[exp_node.node_index].isExplored = TRUE;
							node_traceback[exp_node.node_index].prev_index = exp_node.prev_index;
							explored_nodes.push_back(exp_node.node_index);		
							expand_node(router_data, exp_node, pq);
						}
					}
				} while(exp_node.node_index == lb_nets[inet].terminals[itarget] && is_impossible == FALSE);

				if(exp_node.node_index == lb_nets[inet].terminals[itarget]) {
					/* Net terminal is routed, add this to the route tree, clear data structures, and keep going */
					add_to_rt(lb_nets[inet].rt_tree, exp_node.node_index, node_traceback);

					/* Clear explored nodes, do incrementally */
					for(unsigned int iexp = 0; iexp < explored_nodes.size(); iexp++) {
						node_traceback[iexp].isExplored = FALSE;
						node_traceback[iexp].prev_index = OPEN;
					}

					/* jedit error checking, delete me later */
					for(unsigned int iexp = 0; iexp < lb_type_graph.size(); iexp++) {
						assert(node_traceback[iexp].isExplored == FALSE && node_traceback[iexp].prev_index == OPEN);
					}
				}
				pq = priority_queue<t_expansion_node, vector <t_expansion_node>, compare_expansion_node>(); /* Reset priority queue */
				explored_nodes.clear(); /* Clear list of explored nodes */
			}
			commit_remove_rt(router_data, inet, RT_COMMIT);
		}
		if(is_impossible == FALSE) {
			is_routed = is_route_success(router_data);
		} else {
			is_routed = FALSE;
		}
	}


	delete [] node_traceback;
	return is_routed;
}

/*****************************************************************************************
* Accessor Functions
******************************************************************************************/

/* Returns an array representing the final route.  This array is sized [0..num_lb_type_rr_nodes=1].
   The index of the array stores the net (or OPEN if no net is used) that occupies the rr node for the logic
   block instance

   Note: This function assumes that the atoms packed inside the cluster has already been successfully routed
*/
t_lb_traceback *get_final_route(INOUTP t_lb_router_data *router_data) {
	return NULL;
}





/***************************************************************************
Internal Functions
****************************************************************************/


static void free_intra_lb_nets(vector <t_intra_lb_net> *intra_lb_nets) {
	/* jedit free later */
	delete intra_lb_nets;
}

/* Free trace for intra-logic block routing */
static void free_lb_trace(t_lb_trace *lb_trace) {
	if(lb_trace != NULL) {
		delete (lb_trace);
	}
}

/* Given a pin of a net, assign route tree terminals for it 
   Assumes that pin is not already assigned
*/
static void add_pin_to_rt_terminals(t_lb_router_data *router_data, int iatom, int iport, int ipin, t_model_ports *model_port) {
	vector <t_intra_lb_net> & lb_nets = *router_data->intra_lb_nets;
	vector <t_lb_type_rr_node> & lb_type_graph = *router_data->lb_type_graph;
	t_type_ptr lb_type = router_data->lb_type;
	boolean found = FALSE;
	unsigned int ipos;
	int inet;
	t_pb *pb;
	t_pb_graph_pin *pb_graph_pin;

	pb = logical_block[iatom].pb;
	assert(pb != NULL);

	pb_graph_pin = get_pb_graph_node_pin_from_model_port_pin(model_port, ipin, pb->pb_graph_node);

	/* Determine net */
	if(model_port->dir == IN_PORT) {
		if(model_port->is_clock == TRUE) {
			inet = logical_block[iatom].clock_net;
		} else {
			inet = logical_block[iatom].input_nets[iport][ipin];
		}
	} else {
		assert(model_port->dir == OUT_PORT);
		inet = logical_block[iatom].output_nets[iport][ipin];
	}

	if(inet == OPEN) {
		/* This is not a valid net */
		return;
	}
	
	/* Find if current net is in route tree, if not, then add to rt.
	   Code assumes that # of nets in cluster is small so a linear search through vector is faster than using more complex data structures
	*/
	for(ipos = 0; ipos < lb_nets.size(); ipos++) {
		if(lb_nets[ipos].atom_net_index == inet) {
			found = TRUE;
			break;
		}
	}
	if(found == FALSE) {
		struct t_intra_lb_net new_net;
		new_net.atom_net_index = inet;
		ipos = lb_nets.size();
		lb_nets.push_back(new_net);
	}
	assert(lb_nets[ipos].atom_net_index == inet);

	/*
	Determine whether or not this is a new intra lb net, if yes, then add to list of intra lb nets
	*/	
	if(lb_nets[ipos].terminals.empty()) {
		/* Add terminals */
		lb_nets[ipos].terminals.resize(2);
		lb_nets[ipos].terminals[0] = get_lb_type_rr_graph_ext_source_index(lb_type);
		lb_nets[ipos].terminals[1] = get_lb_type_rr_graph_ext_sink_index(lb_type);
		assert(lb_type_graph[lb_nets[ipos].terminals[0]].type == LB_SOURCE);
		assert(lb_type_graph[lb_nets[ipos].terminals[1]].type == LB_SINK);
	}

	if(model_port->dir == OUT_PORT) {
		/* Net driver pin takes 0th position in terminals */
		assert(lb_nets[ipos].terminals[0] == get_lb_type_rr_graph_ext_source_index(lb_type));
		lb_nets[ipos].terminals[0] = pb_graph_pin->pin_count_in_cluster;
		assert(lb_type_graph[lb_nets[ipos].terminals[0]].type == LB_SOURCE);
	} else {
		/* Determine if the sinks of the net are all contained in the logic block, if yes, then the net does not need to route out of the logic block
		so the external sink can be removed
		*/
		if(lb_nets[ipos].terminals.size() == g_atoms_nlist.net[inet].nodes.size()) {
			assert(lb_nets[ipos].terminals[1] == get_lb_type_rr_graph_ext_source_index(lb_type));		
			lb_nets[ipos].terminals[1] = pb_graph_pin->pin_count_in_cluster;
		} else {
			/* append sink to end of list of terminals */
			int pin_index = pb_graph_pin->pin_count_in_cluster;
			assert(get_num_modes_of_lb_type_rr_node(lb_type_graph[pin_index]) == 1);
			assert(lb_type_graph[pin_index].num_fanout[0] == 1);
			pin_index = lb_type_graph[pin_index].outedges[0][0].node_index;
			assert(lb_type_graph[pin_index].type == LB_SINK);
			lb_nets[ipos].terminals.push_back(pin_index);
		}
	}
}



/* Given a pin of a net, remove route tree terminals from it 
*/
static void remove_pin_from_rt_terminals(t_lb_router_data *router_data, int iatom, int iport, int ipin, t_model_ports *model_port) {
	vector <t_intra_lb_net> & lb_nets = *router_data->intra_lb_nets;
	vector <t_lb_type_rr_node> & lb_type_graph = *router_data->lb_type_graph;
	t_type_ptr lb_type = router_data->lb_type;
	boolean found = FALSE;
	unsigned int ipos;
	int inet;
	t_pb *pb;
	t_pb_graph_pin *pb_graph_pin;

	pb = logical_block[iatom].pb;
	assert(pb != NULL);

	pb_graph_pin = get_pb_graph_node_pin_from_model_port_pin(model_port, ipin, pb->pb_graph_node);

	/* Determine net */
	if(model_port->dir == IN_PORT) {
		if(model_port->is_clock == TRUE) {
			inet = logical_block[iatom].clock_net;
		} else {
			inet = logical_block[iatom].input_nets[iport][ipin];
		}
	} else {
		assert(model_port->dir == OUT_PORT);
		inet = logical_block[iatom].output_nets[iport][ipin];
	}

	if(inet == OPEN) {
		/* This is not a valid net */
		return;
	}
	
	/* Find current net in route tree
	   Code assumes that # of nets in cluster is small so a linear search through vector is faster than using more complex data structures
	*/
	for(ipos = 0; ipos < lb_nets.size(); ipos++) {
		if(lb_nets[ipos].atom_net_index == inet) {
			found = TRUE;
			break;
		}
	}
	assert(found == TRUE);
	assert(lb_nets[ipos].atom_net_index == inet);
	
	if(model_port->dir == OUT_PORT) {
		/* Net driver pin takes 0th position in terminals */
		assert(lb_nets[ipos].terminals[0] == pb_graph_pin->pin_count_in_cluster);
		lb_nets[ipos].terminals[0] = get_lb_type_rr_graph_ext_source_index(lb_type);		
	} else {
		assert(model_port->dir == IN_PORT);

		/* Remove sink from list of terminals */
		int pin_index = pb_graph_pin->pin_count_in_cluster;
		unsigned int iterm;

		assert(get_num_modes_of_lb_type_rr_node(lb_type_graph[pin_index]) == 1);
		assert(lb_type_graph[pin_index].num_fanout[0] == 1);
		pin_index = lb_type_graph[pin_index].outedges[0][0].node_index;
		assert(lb_type_graph[pin_index].type == LB_SINK);
			
		found = FALSE;
		for(iterm = 0; iterm < lb_nets[ipos].terminals.size(); iterm++) {
			if(lb_nets[ipos].terminals[iterm] == pin_index) {
				found = TRUE;
				break;
			}
		}
		assert(found == TRUE);
		assert(lb_nets[ipos].terminals[iterm] == pin_index);
		assert(iterm > 0);
		if(iterm == 1) {
			/* Index 1 is a special position reserved for net connection going out of cluster */
			lb_nets[ipos].terminals[iterm] = get_lb_type_rr_graph_ext_sink_index(lb_type);
		} else if (lb_nets[ipos].terminals[1] != get_lb_type_rr_graph_ext_sink_index(lb_type)) {
			/* Index 1 is a special position reserved for net connection going out of cluster */
			lb_nets[ipos].terminals[iterm] = lb_nets[ipos].terminals[1];
			lb_nets[ipos].terminals[1] = get_lb_type_rr_graph_ext_sink_index(lb_type);
		} else {
			/* Must drop vector size */
			lb_nets[ipos].terminals[iterm] = lb_nets[ipos].terminals.back();
			lb_nets[ipos].terminals.pop_back();
		}
	}

	if(lb_nets[ipos].terminals.size() == 2 && 
		lb_nets[ipos].terminals[0] == get_lb_type_rr_graph_ext_source_index(lb_type) &&
		lb_nets[ipos].terminals[1] == get_lb_type_rr_graph_ext_sink_index(lb_type)) {
		/* This net is not routed, remove from list of nets in lb */
		lb_nets[ipos] = lb_nets.back();
		lb_nets.pop_back();
	}
}

/* Commit or remove route tree from currently routed solution */
static void commit_remove_rt(t_lb_router_data *router_data, int inet, e_commit_remove op) {
}

/* At source mode as starting point to existing route tree */
static void add_source_to_rt(t_lb_router_data *router_data, int inet) {
}

/* Expand all nodes found in route tree into priority queue */
static void expand_rt(t_lb_router_data *router_data, int inet, vector<int> &explored_nodes, 
	priority_queue<t_expansion_node, vector <t_expansion_node>, compare_expansion_node> &pq) {
}

/* Expand all nodes found in route tree into priority queue */
static void expand_node(t_lb_router_data *router_data, t_expansion_node exp_node, 
	priority_queue<t_expansion_node, vector <t_expansion_node>, compare_expansion_node> &pq) {
}

/* Add new path from existing route tree to target sink */
static void add_to_rt(t_lb_trace *rt, int node_index, t_explored_node_tb *node_traceback) {
	vector <int> trace_forward;	
	int rt_index, cur_index;
	t_lb_trace *link_node;
	t_lb_trace curr_node;


	/* Store path all the way back to route tree */
	rt_index = node_index;
	while(node_traceback[rt_index].isExplored == TRUE) {
		trace_forward.push_back(rt_index);
		rt_index = node_traceback[rt_index].prev_index;
	}

	/* Find rt_index on the route tree */
	link_node = find_node_in_rt(rt, rt_index);
	assert(link_node != NULL);

	/* Add path to root tree */
	trace_forward.pop_back();
	while(!trace_forward.empty()) {
		cur_index = trace_forward.back();
		curr_node.current_node = cur_index;
		link_node->next_nodes.push_back(curr_node);
		trace_forward.pop_back();
	}
}

/* Determine if a completed route is valid.  A successful route has no congestion (ie. no routing resource is used by two nets). */
static boolean is_route_success(t_lb_router_data *router_data) {
	vector <t_lb_type_rr_node> & lb_type_graph = *router_data->lb_type_graph;

	for(unsigned int inode = 0; inode < lb_type_graph.size(); inode++) {
		if(router_data->lb_rr_node_stats[inode].occ > lb_type_graph[inode].capacity) {
			return FALSE;
		}
	}

	return TRUE;
}

/* Given a route tree and an index of a node on the route tree, return a pointer to the trace corresponding to that index */
static t_lb_trace *find_node_in_rt(t_lb_trace *rt, int rt_index) {
	t_lb_trace *cur;
	if(rt->current_node == rt_index) {
		return rt;
	} else {
		for(unsigned int i = 0; i < rt->next_nodes.size(); i++) {
			cur = find_node_in_rt(&rt->next_nodes[i], rt_index);
			if(cur != NULL) {
				return cur;
			}
		}
	}
	return NULL;
}

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

/* #define PRINT_INTRA_LB_ROUTE */

/*****************************************************************************************
* Internal data structures
******************************************************************************************/

enum e_commit_remove {RT_COMMIT, RT_REMOVE};

// jedit check if this hacky class memory reserve thing is still necessary, if not, then delete
/* Packing uses a priority queue that requires a large number of elements.  This backdoor
allows me to use a priority queue where I can pre-allocate the # of elements in the underlying container 
for efficiency reasons.  Note: Must use vector with this */
template <class T, class U, class V>
class reservable_pq: public priority_queue<T, U, V>
{
	public:
		typedef typename priority_queue<T>::size_type size_type;    
		reservable_pq(size_type capacity = 0) { 
			reserve(capacity); 
			cur_cap = capacity;
		};
		void reserve(size_type capacity) { 
			this->c.reserve(capacity); 
			cur_cap = capacity;
		}
		void clear() {
			this->c.clear();
			this->c.reserve(cur_cap); 			
		}
	private:
		size_type cur_cap;
};

/*****************************************************************************************
* Internal functions declarations
******************************************************************************************/
static void free_lb_net_rt(t_lb_trace *lb_trace);
static void free_lb_trace(t_lb_trace *lb_trace);
static void add_pin_to_rt_terminals(t_lb_router_data *router_data, int iatom, int iport, int ipin, t_model_ports *model_port);
static void remove_pin_from_rt_terminals(t_lb_router_data *router_data, int iatom, int iport, int ipin, t_model_ports *model_port);


static void commit_remove_rt(t_lb_trace *rt, t_lb_router_data *router_data, e_commit_remove op);
static boolean is_skip_route_net(t_lb_trace *rt, t_lb_router_data *router_data);
static void add_source_to_rt(t_lb_router_data *router_data, int inet);
static void expand_rt(t_lb_router_data *router_data, int inet, reservable_pq<t_expansion_node, vector <t_expansion_node>, compare_expansion_node> &pq, int irt_net);
static void expand_rt_rec(t_lb_trace *rt, int prev_index, t_explored_node_tb *explored_node_tb, 
	reservable_pq<t_expansion_node, vector <t_expansion_node>, compare_expansion_node> &pq, int irt_net, int explore_id_index);
static void expand_node(t_lb_router_data *router_data, t_expansion_node exp_node, 
	reservable_pq<t_expansion_node, vector <t_expansion_node>, compare_expansion_node> &pq, int net_fanout);
static void add_to_rt(t_lb_trace *rt, int node_index, t_explored_node_tb *explored_node_tb, int irt_net);
static boolean is_route_success(t_lb_router_data *router_data);
static t_lb_trace *find_node_in_rt(t_lb_trace *rt, int rt_index);
static void reset_explored_node_tb(t_lb_router_data *router_data);
static void save_and_reset_lb_route(INOUTP t_lb_router_data *router_data);
static void load_trace_to_pb_pin_route_stats(INOUTP t_pb_pin_route_stats *pb_pin_route_stats, INP int total_pins, INP int atom_net, INP int prev_pin_id, INP const t_lb_trace *trace);

/*****************************************************************************************
* Debug functions declarations
******************************************************************************************/
#ifdef PRINT_INTRA_LB_ROUTE
static void print_route(char *filename, t_lb_router_data *router_data);
#endif
static void print_trace(FILE *fp, t_lb_trace *trace);

/*****************************************************************************************
* Constructor/Destructor functions 
******************************************************************************************/

/**
 Build data structures used by intra-logic block router
 */
t_lb_router_data *alloc_and_load_router_data(INP vector<t_lb_type_rr_node> *lb_type_graph, t_type_ptr type) {
	t_lb_router_data *router_data = new t_lb_router_data;
	int size;

	router_data->lb_type_graph = lb_type_graph;
	size = router_data->lb_type_graph->size();
	router_data->lb_rr_node_stats = new t_lb_rr_node_stats[size];
	router_data->explored_node_tb = new t_explored_node_tb[size];
	router_data->intra_lb_nets = new vector<t_intra_lb_net>;
	router_data->atoms_added = new map<int, boolean>;
	router_data->lb_type = type;

	return router_data;
}

/* free data used by router */
void free_router_data(INOUTP t_lb_router_data *router_data) {
	if(router_data != NULL && router_data->lb_type_graph != NULL) {
		delete [] router_data->lb_rr_node_stats;
		router_data->lb_rr_node_stats = NULL;
		delete [] router_data->explored_node_tb;
		router_data->explored_node_tb = NULL;
		router_data->lb_type_graph = NULL;
		delete router_data->atoms_added;
		router_data->atoms_added = NULL;
		free_intra_lb_nets(router_data->intra_lb_nets);
		free_intra_lb_nets(router_data->saved_lb_nets);
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
	map <int, boolean> & atoms_added = *router_data->atoms_added;

	pb = logical_block[iatom].pb;
	pb_graph_node = pb->pb_graph_node;
	pb_type = pb_graph_node->pb_type;

	if(atoms_added.count(iatom) > 0) {
		vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__, "Atom %s [%d] added twice to router\n", logical_block[iatom].name, iatom);
	}
	atoms_added[iatom] = TRUE;

	set_reset_pb_modes(router_data, pb, TRUE);

	model = logical_block[iatom].model;
	
	/* Add inputs to route tree sources/targets */	
	model_ports = model->inputs;
	while(model_ports != NULL) {
		if(model_ports->is_clock == FALSE) {
			iport = model_ports->index;
			for (int ipin = 0; ipin < model_ports->size; ipin++) {
				inet = logical_block[iatom].input_nets[iport][ipin];
				if(inet != OPEN) {
					add_pin_to_rt_terminals(router_data, iatom, iport, ipin, model_ports);
				}
			}
		}
		model_ports = model_ports->next;
	}

	/* Add outputs to route tree sources/targets */	
	model_ports = model->outputs;
	while(model_ports != NULL) {
		iport = model_ports->index;
		for (int ipin = 0; ipin < model_ports->size; ipin++) {
			inet = logical_block[iatom].output_nets[iport][ipin];
			if(inet != OPEN) {
				add_pin_to_rt_terminals(router_data, iatom, iport, ipin, model_ports);
			}
		}
		model_ports = model_ports->next;
	}

	/* Add clock to route tree sources/targets */	
	model_ports = model->inputs;
	while(model_ports != NULL) {
		if(model_ports->is_clock == TRUE) {
			iport = model_ports->index;
			assert(iport == 0);
			for (int ipin = 0; ipin < model_ports->size; ipin++) {
				assert(ipin == 0);
				inet = logical_block[iatom].clock_net;
				if(inet != OPEN) {
					add_pin_to_rt_terminals(router_data, iatom, iport, ipin, model_ports);
				}
			}
		}
		model_ports = model_ports->next;
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
	map <int, boolean> & atoms_added = *router_data->atoms_added;

	
	if(atoms_added.count(iatom) == 0) {
		return;
	}

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
			for (int ipin = 0; ipin < model_ports->size; ipin++) {
				inet = logical_block[iatom].input_nets[iport][ipin];
				if(inet != OPEN) {
					remove_pin_from_rt_terminals(router_data, iatom, iport, ipin, model_ports);
				}
			}
		}
		model_ports = model_ports->next;
	}

	/* Remove outputs from route tree sources/targets */	
	model_ports = model->outputs;
	while(model_ports != NULL) {
		iport = model_ports->index;
		for (int ipin = 0; ipin < model_ports->size; ipin++) {
			inet = logical_block[iatom].output_nets[iport][ipin];
			if(inet != OPEN) {
				remove_pin_from_rt_terminals(router_data, iatom, iport, ipin, model_ports);
			}
		}
		model_ports = model_ports->next;
	}

	/* Remove clock from route tree sources/targets */	
	model_ports = model->inputs;
	while(model_ports != NULL) {
		if(model_ports->is_clock == TRUE) {
			iport = model_ports->index;
			assert(iport == 0);
			for (int ipin = 0; ipin < model_ports->size; ipin++) {
				assert(ipin == 0);
				inet = logical_block[iatom].clock_net;
				if(inet != OPEN) {
					remove_pin_from_rt_terminals(router_data, iatom, iport, ipin, model_ports);
				}
			}
		}
		model_ports = model_ports->next;
	}

	atoms_added.erase(iatom);
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

	/* Output pin modes are based on parent pb, so set children to use new mode 
	   Output pin of top-level logic block is also set to mode 0
	*/
	if(pb_type->num_modes != 0) {
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
}

/* Attempt to route routing driver/targets on the current architecture 
   Follows pathfinder negotiated congestion algorithm
*/
boolean try_intra_lb_route(INOUTP t_lb_router_data *router_data) {
	vector <t_intra_lb_net> & lb_nets = *router_data->intra_lb_nets;
	vector <t_lb_type_rr_node> & lb_type_graph = *router_data->lb_type_graph;
	boolean is_routed = FALSE;
	boolean is_impossible = FALSE;
	t_expansion_node exp_node;

	/* Stores state info during route */
	reservable_pq <t_expansion_node, vector <t_expansion_node>, compare_expansion_node> pq;

	reset_explored_node_tb(router_data);

	/* Reset current routing */
	for(unsigned int inet = 0; inet < lb_nets.size(); inet++) {
		free_lb_net_rt(lb_nets[inet].rt_tree);
		lb_nets[inet].rt_tree = NULL;
	}
	for(unsigned int inode = 0; inode < lb_type_graph.size(); inode++) {
		router_data->lb_rr_node_stats[inode].historical_usage = 0;
		router_data->lb_rr_node_stats[inode].occ = 0;
	}

	/*	Iteratively remove congestion until a successful route is found.  
		Cap the total number of iterations tried so that if a solution does not exist, then the router won't run indefinately */
	router_data->pres_con_fac = router_data->params.pres_fac;
	for(int iter = 0; iter < router_data->params.max_iterations && is_routed == FALSE && is_impossible == FALSE; iter++) {
		unsigned int inet;
		/* Iterate across all nets internal to logic block */
		for(inet = 0; inet < lb_nets.size() && is_impossible == FALSE; inet++) {
			int idx = inet;
			if (is_skip_route_net(lb_nets[idx].rt_tree, router_data) == TRUE) {
				continue;
			}
			commit_remove_rt(lb_nets[idx].rt_tree, router_data, RT_REMOVE);
			free_lb_net_rt(lb_nets[idx].rt_tree);
			lb_nets[idx].rt_tree = NULL;
			add_source_to_rt(router_data, idx);

			/* Route each sink of net */
			for(unsigned int itarget = 1; itarget < lb_nets[idx].terminals.size() && is_impossible == FALSE; itarget++) {
				pq.clear();
				/* Get lowest cost next node, repeat until a path is found or if it is impossible to route */
				expand_rt(router_data, idx, pq, idx);
				do {
					if(pq.empty()) {
						/* No connection possible */
						is_impossible = TRUE;
					} else {
						exp_node = pq.top();
						pq.pop();

						if(router_data->explored_node_tb[exp_node.node_index].explored_id != router_data->explore_id_index) {
							/* First time node is popped implies path to this node is the lowest cost.
								If the node is popped a second time, then the path to that node is higher than this path so
								ignore.
							*/
							router_data->explored_node_tb[exp_node.node_index].explored_id = router_data->explore_id_index;
							router_data->explored_node_tb[exp_node.node_index].prev_index = exp_node.prev_index;
							if(exp_node.node_index != lb_nets[idx].terminals[itarget]) {								
								expand_node(router_data, exp_node, pq, lb_nets[idx].terminals.size() - 1);
							}
						}
					}
				} while(exp_node.node_index != lb_nets[idx].terminals[itarget] && is_impossible == FALSE);

				if(exp_node.node_index == lb_nets[idx].terminals[itarget]) {
					/* Net terminal is routed, add this to the route tree, clear data structures, and keep going */
					add_to_rt(lb_nets[idx].rt_tree, exp_node.node_index, router_data->explored_node_tb, idx);					
				}

				router_data->explore_id_index++;
				if(router_data->explore_id_index > 2000000000) {
					/* overflow protection */
					for(unsigned int id = 0; id < lb_type_graph.size(); id++) {
						router_data->explored_node_tb[id].explored_id = OPEN;
						router_data->explored_node_tb[id].enqueue_id = OPEN;
						router_data->explore_id_index = 1;
					}
				}								
			}
			
			commit_remove_rt(lb_nets[idx].rt_tree, router_data, RT_COMMIT);
		}
		if(is_impossible == FALSE) {
			is_routed = is_route_success(router_data);
		} else {
			--inet;
			printf(WARNTAG "jedit net %s %d is impossible\n", vpack_net[lb_nets[inet].atom_net_index].name, inet);
			is_routed = FALSE;
		}
		router_data->pres_con_fac *= router_data->params.pres_fac_mult;
	}

	if (is_routed) {
		save_and_reset_lb_route(router_data);
	}
	else {
		for (unsigned int inet = 0; inet < lb_nets.size(); inet++) {
			free_lb_net_rt(lb_nets[inet].rt_tree);
			lb_nets[inet].rt_tree = NULL;
		}
#ifdef PRINT_INTRA_LB_ROUTE
		print_route("jedit_failed_route.echo", router_data);
		printf(WARNTAG "jedit route FAIL\n");
#endif
	}
	return is_routed;
}


/*****************************************************************************************
* Accessor Functions
******************************************************************************************/

/* Creates an array [0..num_pb_graph_pins-1] lookup for intra-logic block routing.  Given pb_graph_pin id for clb, lookup atom net that uses that pin.
   If pin is not used, stores OPEN at that pin location */
t_pb_pin_route_stats *alloc_and_load_pb_pin_route_stats(const vector <t_intra_lb_net> *intra_lb_nets, t_pb_graph_node *pb_graph_head) {
	const vector <t_intra_lb_net> &lb_nets = *intra_lb_nets;
	int total_pins = pb_graph_head->total_pb_pins;
	t_pb_pin_route_stats * pb_pin_route_stats = new t_pb_pin_route_stats[pb_graph_head->total_pb_pins];

	for(int ipin = 0; ipin < total_pins; ipin++) {
		pb_pin_route_stats[ipin].atom_net_idx = OPEN;
		pb_pin_route_stats[ipin].prev_pb_pin_id = OPEN;
	}

	for(int inet = 0; inet < (int)lb_nets.size(); inet++) {
		load_trace_to_pb_pin_route_stats(pb_pin_route_stats, total_pins, lb_nets[inet].atom_net_index, OPEN, lb_nets[inet].rt_tree);
	}

	return pb_pin_route_stats;
}

/* Free pin-to-atomic_net array lookup */
void free_pb_pin_route_stats(t_pb_pin_route_stats *pb_pin_route_stats) {
	if(pb_pin_route_stats != NULL) {
		delete []pb_pin_route_stats;
	}
}

void free_intra_lb_nets(vector <t_intra_lb_net> *intra_lb_nets) {
	if(intra_lb_nets == NULL) {
		return;
	}
	vector <t_intra_lb_net> &lb_nets = *intra_lb_nets;
	for(unsigned int i = 0; i < lb_nets.size(); i++) {
		lb_nets[i].terminals.clear();
		free_lb_net_rt(lb_nets[i].rt_tree);
		lb_nets[i].rt_tree = NULL;
	}		
	delete intra_lb_nets;
}


/***************************************************************************
Internal Functions
****************************************************************************/

/* Recurse through route tree trace to populate pb pin to atom net lookup array */
static void load_trace_to_pb_pin_route_stats(INOUTP t_pb_pin_route_stats *pb_pin_route_stats, INP int total_pins, INP int atom_net, INP int prev_pin_id, INP const t_lb_trace *trace) {
	int ipin = trace->current_node;
	int prev_pb_pin_id = prev_pin_id;
	int cur_pin_id = OPEN;
	if(ipin < total_pins) {
		/* This routing node corresponds with a pin.  This node is virtual (ie. sink or source node) */
		cur_pin_id = ipin;
		if(pb_pin_route_stats[cur_pin_id].atom_net_idx == OPEN) {
			pb_pin_route_stats[cur_pin_id].atom_net_idx = atom_net;
			pb_pin_route_stats[cur_pin_id].prev_pb_pin_id = prev_pb_pin_id;
		} else {
			assert(pb_pin_route_stats[cur_pin_id].atom_net_idx == atom_net);
		}		
	}
	for(int itrace = 0; itrace< (int)trace->next_nodes.size(); itrace++) {
		load_trace_to_pb_pin_route_stats(pb_pin_route_stats, total_pins, atom_net, cur_pin_id, &trace->next_nodes[itrace]);
	}
}


/* Free route tree for intra-logic block routing */
static void free_lb_net_rt(t_lb_trace *lb_trace) {
	if(lb_trace != NULL) {
		for(unsigned int i = 0; i < lb_trace->next_nodes.size(); i++) {
			free_lb_trace(&lb_trace->next_nodes[i]);
		}
		lb_trace->next_nodes.clear();
		delete lb_trace;
	}
}

/* Free trace for intra-logic block routing */
static void free_lb_trace(t_lb_trace *lb_trace) {
	if(lb_trace != NULL) {
		for(unsigned int i = 0; i < lb_trace->next_nodes.size(); i++) {
			free_lb_trace(&lb_trace->next_nodes[i]);
		}
		lb_trace->next_nodes.clear();
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
		int source_terminal;
		source_terminal = get_lb_type_rr_graph_ext_source_index(lb_type);
		lb_nets[ipos].terminals.push_back(source_terminal);
		assert(lb_type_graph[lb_nets[ipos].terminals[0]].type == LB_SOURCE);
	}

	if(model_port->dir == OUT_PORT) {
		/* Net driver pin takes 0th position in terminals */
		int sink_terminal;
		assert(lb_nets[ipos].terminals[0] == get_lb_type_rr_graph_ext_source_index(lb_type));
		lb_nets[ipos].terminals[0] = pb_graph_pin->pin_count_in_cluster;

		assert(lb_type_graph[lb_nets[ipos].terminals[0]].type == LB_SOURCE);
		

		if(lb_nets[ipos].terminals.size() < g_atoms_nlist.net[inet].pins.size()) {
			/* Must route out of cluster, put out of cluster sink terminal as first terminal */
			if(lb_nets[ipos].terminals.size() == 1) {
				sink_terminal = get_lb_type_rr_graph_ext_sink_index(lb_type);
				lb_nets[ipos].terminals.push_back(sink_terminal);
			} else {
				sink_terminal = lb_nets[ipos].terminals[1];
				lb_nets[ipos].terminals.push_back(sink_terminal);
				sink_terminal = get_lb_type_rr_graph_ext_sink_index(lb_type);
				lb_nets[ipos].terminals[1] = sink_terminal;
			}
			assert(lb_type_graph[lb_nets[ipos].terminals[1]].type == LB_SINK);
		}
	} else {
		/* Sink for input pins is one past the primitive input pin
		*/
		int pin_index = pb_graph_pin->pin_count_in_cluster;
		assert(get_num_modes_of_lb_type_rr_node(lb_type_graph[pin_index]) == 1);
		assert(lb_type_graph[pin_index].num_fanout[0] == 1);
		
		pin_index = lb_type_graph[pin_index].outedges[0][0].node_index;
		assert(lb_type_graph[pin_index].type == LB_SINK);

		if(lb_nets[ipos].terminals.size() == g_atoms_nlist.net[inet].pins.size() &&
			lb_nets[ipos].terminals[1] == get_lb_type_rr_graph_ext_sink_index(lb_type)) {

		    /* If all sinks of net are all contained in the logic block, then the net does not need to route out of the logic block,
			so the external sink can be removed
			*/
			lb_nets[ipos].terminals[1] = pin_index;
		} else {
			lb_nets[ipos].terminals.push_back(pin_index);
		}
	}

	int num_lb_terminals = lb_nets[ipos].terminals.size();
	assert(num_lb_terminals <= (int) g_atoms_nlist.net[inet].pins.size());
	assert(num_lb_terminals > 1);
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
		int sink_terminal;
		assert(lb_nets[ipos].terminals[0] == pb_graph_pin->pin_count_in_cluster);
		lb_nets[ipos].terminals[0] = get_lb_type_rr_graph_ext_source_index(lb_type);		

		/* source terminal is now coming from outside logic block, do not need to route signal out of logic block */
		sink_terminal = get_lb_type_rr_graph_ext_sink_index(lb_type);
		if(lb_nets[ipos].terminals[1] == sink_terminal) {
			lb_nets[ipos].terminals[1] = lb_nets[ipos].terminals.back();
			lb_nets[ipos].terminals.pop_back();
		}
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
		
		/* Drop terminal from list */
		lb_nets[ipos].terminals[iterm] = lb_nets[ipos].terminals.back();
		lb_nets[ipos].terminals.pop_back();
		
		if(lb_nets[ipos].terminals.size() == 1 && lb_nets[ipos].terminals[0] != get_lb_type_rr_graph_ext_source_index(lb_type)) {
			/* The removed sink must be driven by an atom found in the cluster, add in special sink outside of cluster to represent this */
			lb_nets[ipos].terminals.push_back(get_lb_type_rr_graph_ext_sink_index(lb_type));
		}

		if(lb_nets[ipos].terminals.size() > 1 &&
			lb_nets[ipos].terminals[1] != get_lb_type_rr_graph_ext_sink_index(lb_type) && 
			lb_nets[ipos].terminals[0] != get_lb_type_rr_graph_ext_source_index(lb_type)) {
			
			/* The removed sink must be driven by an atom found in the cluster, add in special sink outside of cluster to represent this */
			int terminal = lb_nets[ipos].terminals[1];
			lb_nets[ipos].terminals.push_back(terminal);
			lb_nets[ipos].terminals[1] = get_lb_type_rr_graph_ext_sink_index(lb_type);

		}
	}

	if(lb_nets[ipos].terminals.size() == 1 && 
		lb_nets[ipos].terminals[0] == get_lb_type_rr_graph_ext_source_index(lb_type)) {
		/* This net is not routed, remove from list of nets in lb */
		lb_nets[ipos] = lb_nets.back();
		lb_nets.pop_back();
	}
}

/* Commit or remove route tree from currently routed solution */
static void commit_remove_rt(t_lb_trace *rt, t_lb_router_data *router_data, e_commit_remove op) {
	t_lb_rr_node_stats *lb_rr_node_stats;
	t_explored_node_tb *explored_node_tb;
	vector <t_lb_type_rr_node> & lb_type_graph = *router_data->lb_type_graph;
	int inode;
	int incr;

	lb_rr_node_stats = router_data->lb_rr_node_stats;
	explored_node_tb = router_data->explored_node_tb;

	if(rt == NULL) {
		return;
	}

	inode = rt->current_node;

	/* Determine if node is being used or removed */
	if (op == RT_COMMIT) {
		incr = 1;
		if (lb_rr_node_stats[inode].occ >= lb_type_graph[inode].capacity) {
			lb_rr_node_stats[inode].historical_usage += (lb_rr_node_stats[inode].occ - lb_type_graph[inode].capacity + 1); /* store historical overuse */
		}		
	} else {
		incr = -1;
		explored_node_tb[inode].inet = OPEN;
	}

	lb_rr_node_stats[inode].occ += incr;
	assert(lb_rr_node_stats[inode].occ >= 0);

	/* Recursively update route tree */
	for(unsigned int i = 0; i < rt->next_nodes.size(); i++) {
		commit_remove_rt(&rt->next_nodes[i], router_data, op);
	}
}

/* Should net be skipped?  If the net does not conflict with another net, then skip routing this net */
static boolean is_skip_route_net(t_lb_trace *rt, t_lb_router_data *router_data) {
	t_lb_rr_node_stats *lb_rr_node_stats;
	t_explored_node_tb *explored_node_tb;
	vector <t_lb_type_rr_node> & lb_type_graph = *router_data->lb_type_graph;
	int inode;
	
	lb_rr_node_stats = router_data->lb_rr_node_stats;
	explored_node_tb = router_data->explored_node_tb;

	if (rt == NULL) {
		return FALSE; /* Net is not routed, therefore must route net */
	}

	inode = rt->current_node;

	/* Determine if node is overused */
	if (lb_rr_node_stats[inode].occ > lb_type_graph[inode].capacity) {
		/* Conflict between this net and another net at this node, reroute net */
		return FALSE;
	}

	/* Recursively check that rest of route tree does not have a conflict */
	for (unsigned int i = 0; i < rt->next_nodes.size(); i++) {
		if (is_skip_route_net(&rt->next_nodes[i], router_data) == FALSE) {
			return FALSE;
		}
	}

	/* No conflict, this net's current route is legal, skip routing this net */
	return TRUE;
}


/* At source mode as starting point to existing route tree */
static void add_source_to_rt(t_lb_router_data *router_data, int inet) {
	assert((*router_data->intra_lb_nets)[inet].rt_tree == NULL);
	(*router_data->intra_lb_nets)[inet].rt_tree = new t_lb_trace;
	(*router_data->intra_lb_nets)[inet].rt_tree->current_node = (*router_data->intra_lb_nets)[inet].terminals[0];
}

/* Expand all nodes found in route tree into priority queue */
static void expand_rt(t_lb_router_data *router_data, int inet, 
	reservable_pq<t_expansion_node, vector <t_expansion_node>, compare_expansion_node> &pq, int irt_net) {

	vector<t_intra_lb_net> &lb_nets = *router_data->intra_lb_nets;
	
	assert(pq.empty());

	expand_rt_rec(lb_nets[inet].rt_tree, OPEN, router_data->explored_node_tb, pq, irt_net, router_data->explore_id_index);
}


/* Expand all nodes found in route tree into priority queue recursively */
static void expand_rt_rec(t_lb_trace *rt, int prev_index, t_explored_node_tb *explored_node_tb, 
	reservable_pq<t_expansion_node, vector <t_expansion_node>, compare_expansion_node> &pq, int irt_net, int explore_id_index) {
	
	t_expansion_node enode;

	/* Perhaps should use a cost other than zero */
	enode.cost = 0;
	enode.node_index = rt->current_node;
	enode.prev_index = prev_index;
	pq.push(enode);
	explored_node_tb[enode.node_index].inet = irt_net;
	explored_node_tb[enode.node_index].explored_id = OPEN;
	explored_node_tb[enode.node_index].enqueue_id = explore_id_index;
	explored_node_tb[enode.node_index].enqueue_cost = 0;
	explored_node_tb[enode.node_index].prev_index = prev_index;
	

	for(unsigned int i = 0; i < rt->next_nodes.size(); i++) {
		expand_rt_rec(&rt->next_nodes[i], rt->current_node, explored_node_tb, pq, irt_net, explore_id_index);
	}
}


/* Expand all nodes found in route tree into priority queue */
static void expand_node(t_lb_router_data *router_data, t_expansion_node exp_node, 
	reservable_pq<t_expansion_node, vector <t_expansion_node>, compare_expansion_node> &pq, int net_fanout) {

	int cur_node;
	float cur_cost, incr_cost;
	int mode, usage;
	t_expansion_node enode;
	vector <t_lb_type_rr_node> & lb_type_graph = *router_data->lb_type_graph;
	t_lb_rr_node_stats *lb_rr_node_stats = router_data->lb_rr_node_stats;

	cur_node = exp_node.node_index;
	cur_cost = exp_node.cost;
	mode = lb_rr_node_stats[cur_node].mode;
	t_lb_router_params params = router_data->params;


	for(int iedge = 0; iedge < lb_type_graph[cur_node].num_fanout[mode]; iedge++) {
		int next_mode;

		/* Init new expansion node */
		enode.prev_index = cur_node;
		enode.node_index = lb_type_graph[cur_node].outedges[mode][iedge].node_index;
		enode.cost = cur_cost;

		/* Determine incremental cost of using expansion node */
		usage = lb_rr_node_stats[enode.node_index].occ + 1 - lb_type_graph[enode.node_index].capacity;
		incr_cost = lb_type_graph[enode.node_index].intrinsic_cost;
		incr_cost += lb_type_graph[cur_node].outedges[mode][iedge].intrinsic_cost;
		incr_cost += params.hist_fac * lb_rr_node_stats[enode.node_index].historical_usage;
		if(usage > 0) {
			incr_cost *= (usage * router_data->pres_con_fac);
		}		
				
		/* Adjust cost so that higher fanout nets prefer higher fanout routing nodes while lower fanout nets prefer lower fanout routing nodes */
		float fanout_factor = 1.0;
		next_mode = lb_rr_node_stats[enode.node_index].mode;
		if (lb_type_graph[enode.node_index].num_fanout[next_mode] > 1) {
			fanout_factor = 0.85 + (0.25 / net_fanout);
		}
		else {
			fanout_factor = 1.15 - (0.25 / net_fanout);
		}
		incr_cost *= fanout_factor;
		enode.cost = cur_cost + incr_cost;


		/* Add to queue if cost is lower than lowest cost path to this enode */
		if(router_data->explored_node_tb[enode.node_index].enqueue_id == router_data->explore_id_index) {
			if(enode.cost < router_data->explored_node_tb[enode.node_index].enqueue_cost) {
				pq.push(enode);
			}
		} else {
			router_data->explored_node_tb[enode.node_index].enqueue_id = router_data->explore_id_index;
			router_data->explored_node_tb[enode.node_index].enqueue_cost = enode.cost;
			pq.push(enode);
		}
	}
	
}



/* Add new path from existing route tree to target sink */
static void add_to_rt(t_lb_trace *rt, int node_index, t_explored_node_tb *explored_node_tb, int irt_net) {
	vector <int> trace_forward;	
	int rt_index, trace_index;
	t_lb_trace *link_node;
	t_lb_trace curr_node;

	/* Store path all the way back to route tree */
	rt_index = node_index;
	while(explored_node_tb[rt_index].inet != irt_net) {
		trace_forward.push_back(rt_index);
		rt_index = explored_node_tb[rt_index].prev_index;
		assert(rt_index != OPEN);
	}

	/* Find rt_index on the route tree */
	link_node = find_node_in_rt(rt, rt_index);
	assert(link_node != NULL);

	/* Add path to root tree */
	while(!trace_forward.empty()) {
		trace_index = trace_forward.back();
		curr_node.current_node = trace_index;
		link_node->next_nodes.push_back(curr_node);
		link_node = &link_node->next_nodes.back();
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

#ifdef PRINT_INTRA_LB_ROUTE
/* Debug routine, print out current intra logic block route */
static void print_route(char *filename, t_lb_router_data *router_data) {
	FILE *fp;
	vector <t_intra_lb_net> & lb_nets = *router_data->intra_lb_nets;
	vector <t_lb_type_rr_node> & lb_type_graph = *router_data->lb_type_graph;

	fp = my_fopen(filename, "w", 0);
	
	for(unsigned int inode = 0; inode < lb_type_graph.size(); inode++) {
		fprintf(fp, "node %d occ %d cap %d\n", inode, router_data->lb_rr_node_stats[inode].occ, lb_type_graph[inode].capacity);
	}

	fprintf(fp, "\n\n----------------------------------------------------\n\n");

	for(unsigned int inet = 0; inet < lb_nets.size(); inet++) {
		int atom_net;
		atom_net = lb_nets[inet].atom_net_index;
		fprintf(fp, "net %s (%d) num targets %d \n", g_atoms_nlist.net[atom_net].name, atom_net, (int)lb_nets[inet].terminals.size());
		print_trace(fp, lb_nets[inet].rt_tree);
		fprintf(fp, "\n\n");
	}
	fclose(fp);
}
#endif

/* Debug routine, print out trace of net */
static void print_trace(FILE *fp, t_lb_trace *trace) {
	if(trace == NULL) {
		fprintf(fp, "NULL");
		return;
	}
	for(unsigned int ibranch = 0; ibranch < trace->next_nodes.size(); ibranch++) {
		if(trace->next_nodes.size() > 1) {
			fprintf(fp, "B(%d-->%d) ", trace->current_node, trace->next_nodes[ibranch].current_node);
		} else {
			fprintf(fp, "(%d-->%d) ", trace->current_node, trace->next_nodes[ibranch].current_node);
		}
		print_trace(fp, &trace->next_nodes[ibranch]);
	}
}

static void reset_explored_node_tb(t_lb_router_data *router_data) {
	vector <t_lb_type_rr_node> & lb_type_graph = *router_data->lb_type_graph;
	for(unsigned int inode = 0; inode < lb_type_graph.size(); inode++) {
		router_data->explored_node_tb[inode].prev_index = OPEN;
		router_data->explored_node_tb[inode].explored_id = OPEN;
		router_data->explored_node_tb[inode].inet = OPEN;
		router_data->explored_node_tb[inode].enqueue_id = OPEN;
		router_data->explored_node_tb[inode].enqueue_cost = 0;
	}
}



/* Save last successful intra-logic block route and reset current traceback */
static void save_and_reset_lb_route(INOUTP t_lb_router_data *router_data) {
	vector <t_intra_lb_net> & lb_nets = *router_data->intra_lb_nets;

	/* Free old saved lb nets if exist */
	if(router_data->saved_lb_nets != NULL) {		
		free_intra_lb_nets(router_data->saved_lb_nets);
		router_data->saved_lb_nets = NULL;
	}

	/* Save current routed solution */
	router_data->saved_lb_nets = new vector<t_intra_lb_net> (lb_nets.size());
	vector <t_intra_lb_net> & saved_lb_nets = *router_data->saved_lb_nets;

	for(int inet = 0; inet < (int)saved_lb_nets.size(); inet++) {
		/* 
		  Save and reset route tree data
		*/
		saved_lb_nets[inet].atom_net_index = lb_nets[inet].atom_net_index;
		saved_lb_nets[inet].terminals.resize(lb_nets[inet].terminals.size());
		for(int iterm = 0; iterm < (int) lb_nets[inet].terminals.size(); iterm++) {
			saved_lb_nets[inet].terminals[iterm] = lb_nets[inet].terminals[iterm];
		}
		saved_lb_nets[inet].rt_tree = lb_nets[inet].rt_tree;
		lb_nets[inet].rt_tree = NULL;
	}
}




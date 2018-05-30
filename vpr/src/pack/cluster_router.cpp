/* 
  Intra-logic block router determines if a candidate packing solution (or intermediate solution) can route.
  
  Global Inputs: Architecture and netlist
  Input arguments: clustering info for one cluster (t_pb info)
  Working data set: t_routing_data contains intermediate work
  Output: Routable? true/false.  If true, store/return the routed solution.

  Routing algorithm used is Pathfinder.

  Author: Jason Luu
  Date: July 22, 2013
 */

#include <cstdio>
#include <cstring>
#include <vector>
#include <map>
#include <queue>
#include <cmath>
using namespace std;

#include "vtr_assert.h"
#include "vtr_log.h"

#include "vpr_error.h"
#include "vpr_types.h"

#include "physical_types.h"
#include "globals.h"
#include "atom_netlist.h"
#include "vpr_utils.h"
#include "pack_types.h"
#include "lb_type_rr_graph.h"
#include "cluster_router.h"

/* #define PRINT_INTRA_LB_ROUTE */

/*****************************************************************************************
* Internal data structures
******************************************************************************************/

enum e_commit_remove {RT_COMMIT, RT_REMOVE};

// TODO: check if this hacky class memory reserve thing is still necessary, if not, then delete
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
static void add_pin_to_rt_terminals(t_lb_router_data *router_data, const AtomPinId pin_id);

static void update_required_external_connections(t_intra_lb_net& net, t_lb_router_data& router_data);

static std::vector<t_intra_lb_net>::iterator find_lb_net(std::vector<t_intra_lb_net>& lb_nets, const AtomNetId atom_net);
static std::vector<t_intra_lb_net>::iterator find_create_lb_net(std::vector<t_intra_lb_net>& lb_nets, const AtomNetId atom_net);
static int get_pb_graph_pin_sink_rr(const t_pb_graph_pin* pb_graph_pin, const t_lb_type_rr_graph& lb_rr_graph);
static int get_pb_graph_pin_source_rr(const t_pb_graph_pin* pb_graph_pin, const t_lb_type_rr_graph& lb_rr_graph);
static bool driver_is_internal(const t_intra_lb_net& net, const t_lb_type_rr_graph& lb_rr_graph);
static bool sink_is_internal(int sink_rr, const t_lb_type_rr_graph& lb_rr_graph);
static void add_lb_external_sink(t_intra_lb_net& net, const t_lb_router_data& router_data);
static void add_lb_external_driver(t_intra_lb_net& net, const t_lb_router_data& router_data);
static void remove_pin_from_rt_terminals(t_lb_router_data *router_data, const AtomPinId pin_id);
static bool check_lb_net(const t_intra_lb_net& lb_net, const t_lb_type_rr_graph& lb_rr_graph);
bool check_routing_targets(const t_lb_router_data& router_data);

static void fix_duplicate_equivalent_pins(t_lb_router_data *router_data);

static void commit_remove_rt(t_lb_trace *rt, t_lb_router_data *router_data, e_commit_remove op);
static bool is_skip_route_net(t_lb_trace *rt, t_lb_router_data *router_data);
static void add_source_to_rt(t_lb_router_data *router_data, int inet);
static void expand_rt(t_lb_router_data *router_data, int inet, reservable_pq<t_expansion_node, vector <t_expansion_node>, compare_expansion_node> &pq, int irt_net);
static void expand_rt_rec(t_lb_trace *rt, int prev_index, t_explored_node_tb *explored_node_tb, 
	reservable_pq<t_expansion_node, vector <t_expansion_node>, compare_expansion_node> &pq, int irt_net, int explore_id_index);
static void expand_node(t_lb_router_data *router_data, t_expansion_node exp_node, 
	reservable_pq<t_expansion_node, vector <t_expansion_node>, compare_expansion_node> &pq, int net_fanout);
static void add_to_rt(t_lb_trace *rt, int node_index, t_explored_node_tb *explored_node_tb, int irt_net);
static bool is_route_success(t_lb_router_data *router_data);
static t_lb_trace *find_node_in_rt(t_lb_trace *rt, int rt_index);
static void reset_explored_node_tb(t_lb_router_data *router_data);
static void save_and_reset_lb_route(t_lb_router_data *router_data);
static void load_trace_to_pb_route(t_pb_route *pb_route, const int total_pins, const AtomNetId net_id, const int prev_pin_id, const t_lb_trace *trace);

int get_lb_rr_graph_ext_source(const t_intra_lb_net& lb_net, const t_lb_type_rr_graph& lb_rr_graph, const t_lb_type_rr_graph_info& lb_rr_graph_info);
int get_lb_rr_graph_ext_sink(const t_intra_lb_net& lb_net, const t_lb_type_rr_graph& lb_rr_graph, const t_lb_type_rr_graph_info& lb_rr_graph_info);

int get_lb_type_rr_graph_ext_source_index(t_type_ptr lb_type, const t_lb_type_rr_graph& lb_rr_graph, const AtomPinId pin);
int get_lb_type_rr_graph_ext_sink_index(t_type_ptr lb_type, const t_lb_type_rr_graph& lb_rr_graph, const AtomPinId pin);

static std::string describe_lb_type_rr_node(int inode,
                                            const t_lb_router_data* router_data);

static std::vector<int> find_congested_rr_nodes(const t_lb_type_rr_graph& lb_type_graph,
                                                const t_lb_rr_node_stats* lb_rr_node_stats);
static std::vector<int> find_incoming_rr_nodes(int dst_node, const t_lb_router_data* router_data);
static std::string describe_congested_rr_nodes(const std::vector<int>& congested_rr_nodes,
                                               const t_lb_router_data* router_data);
/*****************************************************************************************
* Debug functions declarations
******************************************************************************************/
#ifdef PRINT_INTRA_LB_ROUTE
static void print_route(const char *filename, t_lb_router_data *router_data);
static void print_trace(FILE *fp, t_lb_trace *trace);
#endif

/*****************************************************************************************
* Constructor/Destructor functions 
******************************************************************************************/

/**
 Build data structures used by intra-logic block router
 */
t_lb_router_data *alloc_and_load_router_data(const t_lb_type_rr_graph& lb_type_graph, const t_lb_type_rr_graph_info& lb_type_graph_info, t_type_ptr type) {
	t_lb_router_data *router_data = new t_lb_router_data;
	int size;

	router_data->lb_type_graph = &lb_type_graph;
	router_data->lb_type_graph_info = &lb_type_graph_info;
	size = router_data->lb_type_graph->nodes.size();
	router_data->lb_rr_node_stats = new t_lb_rr_node_stats[size];
	router_data->explored_node_tb = new t_explored_node_tb[size];
	router_data->intra_lb_nets = new vector<t_intra_lb_net>;
	router_data->atoms_added = new map<AtomBlockId, bool>;
	router_data->lb_type = type;

	return router_data;
}

/* free data used by router */
void free_router_data(t_lb_router_data *router_data) {
	if(router_data != nullptr && router_data->lb_type_graph != nullptr) {
		delete [] router_data->lb_rr_node_stats;
		router_data->lb_rr_node_stats = nullptr;
		delete [] router_data->explored_node_tb;
		router_data->explored_node_tb = nullptr;
		router_data->lb_type_graph = nullptr;
		delete router_data->atoms_added;
		router_data->atoms_added = nullptr;
		free_intra_lb_nets(router_data->intra_lb_nets);
		free_intra_lb_nets(router_data->saved_lb_nets);
		router_data->intra_lb_nets = nullptr;
		delete router_data;
	}
}


/*****************************************************************************************
* Routing Functions
******************************************************************************************/

/* Add pins of netlist atom to to current routing drivers/targets */
void add_atom_as_target(t_lb_router_data *router_data, const AtomBlockId blk_id) {
	const t_pb *pb;
    auto& atom_ctx = g_vpr_ctx.atom();

    std::map<AtomBlockId, bool>& atoms_added = *router_data->atoms_added;


	if(atoms_added.count(blk_id) > 0) {
		vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__, "Atom %s added twice to router\n", atom_ctx.nlist.block_name(blk_id).c_str());
	}

	pb = atom_ctx.lookup.atom_pb(blk_id);

    VTR_ASSERT(pb);
	
	atoms_added[blk_id] = true;

	set_reset_pb_modes(router_data, pb, true);

    for(auto pin_id : atom_ctx.nlist.block_pins(blk_id)) {
        add_pin_to_rt_terminals(router_data, pin_id);
        VTR_ASSERT_SAFE(check_routing_targets(*router_data));
    }

    fix_duplicate_equivalent_pins(router_data);
}

/* Remove pins of netlist atom from current routing drivers/targets */
void remove_atom_from_target(t_lb_router_data *router_data, const AtomBlockId blk_id) {
    auto& atom_ctx = g_vpr_ctx.atom();

	map <AtomBlockId, bool> & atoms_added = *router_data->atoms_added;

	const t_pb* pb = atom_ctx.lookup.atom_pb(blk_id);
	
	if(atoms_added.count(blk_id) == 0) {
		return;
	}
	
	set_reset_pb_modes(router_data, pb, false);
		
    for(auto pin_id : atom_ctx.nlist.block_pins(blk_id)) {
        remove_pin_from_rt_terminals(router_data, pin_id);
        VTR_ASSERT_SAFE(check_routing_targets(*router_data));
    }
    
	atoms_added.erase(blk_id);
}

/* Set/Reset mode of rr nodes to the pb used.  If set == true, then set all modes of the rr nodes affected by pb to the mode of the pb.
   Set all modes related to pb to 0 otherwise */
void set_reset_pb_modes(t_lb_router_data *router_data, const t_pb *pb, const bool set) {
	t_pb_type *pb_type;
	t_pb_graph_node *pb_graph_node;
	int mode = pb->mode;
	int inode;

    VTR_ASSERT(mode >= 0);
		
	pb_graph_node = pb->pb_graph_node;
	pb_type = pb_graph_node->pb_type;

	/* Input and clock pin modes are based on current pb mode */
	for(int iport = 0; iport < pb_graph_node->num_input_ports; iport++) {
		for(int ipin = 0; ipin < pb_graph_node->num_input_pins[iport]; ipin++) {
			inode = pb_graph_node->input_pins[iport][ipin].pin_count_in_cluster;
			router_data->lb_rr_node_stats[inode].mode = (set == true) ? mode : 0;
		}
	}
	for(int iport = 0; iport < pb_graph_node->num_clock_ports; iport++) {
		for(int ipin = 0; ipin < pb_graph_node->num_clock_pins[iport]; ipin++) {
			inode = pb_graph_node->clock_pins[iport][ipin].pin_count_in_cluster;
			router_data->lb_rr_node_stats[inode].mode = (set == true) ? mode : 0;
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
						router_data->lb_rr_node_stats[inode].mode = (set == true) ? mode : 0;
					}
				}
			}
		}
	}
}

/* Attempt to route routing driver/targets on the current architecture 
   Follows pathfinder negotiated congestion algorithm
*/
bool try_intra_lb_route(t_lb_router_data *router_data,
                        bool debug_clustering) {
	vector <t_intra_lb_net> & lb_nets = *router_data->intra_lb_nets;
	const t_lb_type_rr_graph& lb_type_graph = *router_data->lb_type_graph;
	bool is_routed = false;
	bool is_impossible = false;
	t_expansion_node exp_node;

	/* Stores state info during route */
	reservable_pq <t_expansion_node, vector <t_expansion_node>, compare_expansion_node> pq;

	reset_explored_node_tb(router_data);

	/* Reset current routing */
	for(unsigned int inet = 0; inet < lb_nets.size(); inet++) {
		free_lb_net_rt(lb_nets[inet].rt_tree);
		lb_nets[inet].rt_tree = nullptr;
	}
	for(unsigned int inode = 0; inode < lb_type_graph.nodes.size(); inode++) {
		router_data->lb_rr_node_stats[inode].historical_usage = 0;
		router_data->lb_rr_node_stats[inode].occ = 0;
	}

	/*	Iteratively remove congestion until a successful route is found.  
		Cap the total number of iterations tried so that if a solution does not exist, then the router won't run indefinately */
	router_data->pres_con_fac = router_data->params.pres_fac;
	for(int iter = 0; iter < router_data->params.max_iterations && is_routed == false && is_impossible == false; iter++) {
		unsigned int inet;
		/* Iterate across all nets internal to logic block */
		for(inet = 0; inet < lb_nets.size() && is_impossible == false; inet++) {
			int idx = inet;
			if (is_skip_route_net(lb_nets[idx].rt_tree, router_data) == true) {
				continue;
			}
			commit_remove_rt(lb_nets[idx].rt_tree, router_data, RT_REMOVE);
			free_lb_net_rt(lb_nets[idx].rt_tree);
			lb_nets[idx].rt_tree = nullptr;
			add_source_to_rt(router_data, idx);

            VTR_ASSERT_SAFE_MSG(lb_nets[idx].terminals[t_intra_lb_net::DRIVER_INDEX] != OPEN, "Must have valid driver");

			/* Route each sink of net */
			for(unsigned int itarget = 1; itarget < lb_nets[idx].terminals.size() && is_impossible == false; itarget++) {
                VTR_ASSERT_SAFE_MSG(lb_nets[idx].terminals[itarget] != OPEN, "Must have valid sink target");

				pq.clear();
				/* Get lowest cost next node, repeat until a path is found or if it is impossible to route */
				expand_rt(router_data, idx, pq, idx);
				do {
					if(pq.empty()) {
						/* No connection possible */
						is_impossible = true;

                        //Print detailed debug info
                        auto& atom_nlist = g_vpr_ctx.atom().nlist;
                        AtomNetId net_id = lb_nets[inet].atom_net_id;
                        AtomPinId driver_pin = lb_nets[inet].atom_pins[0];
                        AtomPinId sink_pin = lb_nets[inet].atom_pins[itarget];
                        int driver_rr_node = lb_nets[inet].terminals[0];
                        int sink_rr_node = lb_nets[inet].terminals[itarget];

                        VTR_ASSERT(driver_rr_node != OPEN);
                        VTR_ASSERT(sink_rr_node != OPEN);

                        if (debug_clustering) {
                            vtr::printf("No possible routing path from %s to %s: needed for netlist net '%s' from net pin '%s'",
                                        describe_lb_type_rr_node(driver_rr_node, router_data).c_str(),
                                        describe_lb_type_rr_node(sink_rr_node, router_data).c_str(),
                                        atom_nlist.net_name(net_id).c_str(),
                                        atom_nlist.pin_name(driver_pin).c_str());
                            if (sink_pin) {
                                vtr::printf(" to net pin '%s'", atom_nlist.pin_name(sink_pin).c_str());
                            }
                            vtr::printf("\n");
                        }
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
				} while(exp_node.node_index != lb_nets[idx].terminals[itarget] && is_impossible == false);

				if(exp_node.node_index == lb_nets[idx].terminals[itarget]) {
					/* Net terminal is routed, add this to the route tree, clear data structures, and keep going */
					add_to_rt(lb_nets[idx].rt_tree, exp_node.node_index, router_data->explored_node_tb, idx);					
				}

				router_data->explore_id_index++;
				if(router_data->explore_id_index >= std::numeric_limits<decltype(router_data->explore_id_index)>::max()) {
					/* overflow protection */
					for(unsigned int id = 0; id < lb_type_graph.nodes.size(); id++) {
						router_data->explored_node_tb[id].explored_id = OPEN;
						router_data->explored_node_tb[id].enqueue_id = OPEN;
						router_data->explore_id_index = 1;
					}
				}								
			}
			
			commit_remove_rt(lb_nets[idx].rt_tree, router_data, RT_COMMIT);
		}
		if(is_impossible == false) {
			is_routed = is_route_success(router_data);
		} else {
			--inet;
            auto& atom_ctx = g_vpr_ctx.atom();
			vtr::printf("Net '%s' is impossible to route within proposed %s cluster\n", atom_ctx.nlist.net_name(lb_nets[inet].atom_net_id).c_str(), router_data->lb_type->name);
			is_routed = false;
		}
		router_data->pres_con_fac *= router_data->params.pres_fac_mult;
	}

	if (is_routed) {
		save_and_reset_lb_route(router_data);
	} else {
        //Unroutable

        if (debug_clustering) {
            if (!is_impossible) {
                //Report the congested nodes and associated nets
                auto congested_rr_nodes = find_congested_rr_nodes(lb_type_graph, router_data->lb_rr_node_stats);
                if (!congested_rr_nodes.empty()) {
                    vtr::printf("%s\n", describe_congested_rr_nodes(congested_rr_nodes, router_data).c_str());
                }
            }
        }

        //Clean-up
		for (unsigned int inet = 0; inet < lb_nets.size(); inet++) {
			free_lb_net_rt(lb_nets[inet].rt_tree);
			lb_nets[inet].rt_tree = nullptr;
		}
#ifdef PRINT_INTRA_LB_ROUTE
		print_route("intra_lb_failed_route.echo", router_data);
#endif
	}
	return is_routed;
}


/*****************************************************************************************
* Accessor Functions
******************************************************************************************/

/* Creates an array [0..num_pb_graph_pins-1] lookup for intra-logic block routing.  Given pb_graph_pin id for clb, lookup atom net that uses that pin.
   If pin is not used, stores OPEN at that pin location */
t_pb_route *alloc_and_load_pb_route(const vector <t_intra_lb_net> *intra_lb_nets, t_pb_graph_node *pb_graph_head) {
	const vector <t_intra_lb_net> &lb_nets = *intra_lb_nets;
	int total_pins = pb_graph_head->total_pb_pins;
	t_pb_route * pb_route = new t_pb_route[pb_graph_head->total_pb_pins];

	for(int ipin = 0; ipin < total_pins; ipin++) {
		pb_route[ipin].atom_net_id = AtomNetId::INVALID();
		pb_route[ipin].driver_pb_pin_id = OPEN;
	}

	for(int inet = 0; inet < (int)lb_nets.size(); inet++) {
		load_trace_to_pb_route(pb_route, total_pins, lb_nets[inet].atom_net_id, OPEN, lb_nets[inet].rt_tree);
	}

	return pb_route;
}

/* Free pin-to-atomic_net array lookup */
void free_pb_route(t_pb_route *pb_route) {
	if(pb_route != nullptr) {
		delete []pb_route;
	}
}

void free_intra_lb_nets(vector <t_intra_lb_net> *intra_lb_nets) {
	if(intra_lb_nets == nullptr) {
		return;
	}
	vector <t_intra_lb_net> &lb_nets = *intra_lb_nets;
	for(unsigned int i = 0; i < lb_nets.size(); i++) {
		lb_nets[i].terminals.clear();
		free_lb_net_rt(lb_nets[i].rt_tree);
		lb_nets[i].rt_tree = nullptr;
	}		
	delete intra_lb_nets;
}


/***************************************************************************
Internal Functions
****************************************************************************/

/* Recurse through route tree trace to populate pb pin to atom net lookup array */
static void load_trace_to_pb_route(t_pb_route *pb_route, const int total_pins, const AtomNetId net_id, const int prev_pin_id, const t_lb_trace *trace) {
	int ipin = trace->current_node;
	int driver_pb_pin_id = prev_pin_id;
	int cur_pin_id = OPEN;
	if(ipin < total_pins) {
		/* This routing node corresponds with a pin.  This node is virtual (ie. sink or source node) */
		cur_pin_id = ipin;
		if(!pb_route[cur_pin_id].atom_net_id) {
			pb_route[cur_pin_id].atom_net_id = net_id;
			pb_route[cur_pin_id].driver_pb_pin_id = driver_pb_pin_id;
		} else {
			VTR_ASSERT(pb_route[cur_pin_id].atom_net_id == net_id);
		}		
	}
	for(int itrace = 0; itrace< (int)trace->next_nodes.size(); itrace++) {
		load_trace_to_pb_route(pb_route, total_pins, net_id, cur_pin_id, &trace->next_nodes[itrace]);
	}
}


/* Free route tree for intra-logic block routing */
static void free_lb_net_rt(t_lb_trace *lb_trace) {
	if(lb_trace != nullptr) {
		for(unsigned int i = 0; i < lb_trace->next_nodes.size(); i++) {
			free_lb_trace(&lb_trace->next_nodes[i]);
		}
		lb_trace->next_nodes.clear();
		delete lb_trace;
	}
}

/* Free trace for intra-logic block routing */
static void free_lb_trace(t_lb_trace *lb_trace) {
	if(lb_trace != nullptr) {
		for(unsigned int i = 0; i < lb_trace->next_nodes.size(); i++) {
			free_lb_trace(&lb_trace->next_nodes[i]);
		}
		lb_trace->next_nodes.clear();
	}
}


/* Given a pin of a net, assign route tree terminals for it 
   Assumes that pin is not already assigned
*/
static void add_pin_to_rt_terminals(t_lb_router_data *router_data, const AtomPinId pin_id) {
	vector <t_intra_lb_net> & lb_nets = *router_data->intra_lb_nets;
	const t_lb_type_rr_graph& lb_rr_graph = *router_data->lb_type_graph;
    auto& atom_ctx = g_vpr_ctx.atom();

    const t_pb_graph_pin* pb_graph_pin = find_pb_graph_pin(atom_ctx.nlist, atom_ctx.lookup, pin_id);
    VTR_ASSERT(pb_graph_pin);

    AtomNetId net_id = atom_ctx.nlist.pin_net(pin_id);

	if(!net_id) {
        //No net connected to this pin, so nothing to route
		return;
	}

    constexpr int DRIVER_IDX = t_intra_lb_net::DRIVER_INDEX;

    //Find or create the lb net corresponding to the atom net
    auto lb_net_iter = find_create_lb_net(lb_nets, net_id);

	VTR_ASSERT(lb_net_iter->atom_net_id == net_id);
    VTR_ASSERT(lb_net_iter->atom_pins.size() == lb_net_iter->terminals.size());

    //
    //Add the internal source/sinks for the atom pin
    //
    if (atom_ctx.nlist.pin_type(pin_id) == PinType::DRIVER) {
        //This primitive pin is the net driver, set the net's driver to the corresponding LB_SINK
        VTR_ASSERT_MSG(lb_net_iter->terminals.size() >= 1, "LB Net must at least have space for driver");

        int rr_driver_index = get_pb_graph_pin_source_rr(pb_graph_pin, lb_rr_graph);

		lb_net_iter->terminals[DRIVER_IDX] = rr_driver_index;
		lb_net_iter->atom_pins[DRIVER_IDX] = pin_id;

        VTR_ASSERT_SAFE(driver_is_internal(*lb_net_iter, lb_rr_graph));

    } else {
        //This primitive pin is a net sink
        VTR_ASSERT(atom_ctx.nlist.pin_type(pin_id) == PinType::SINK);

        int rr_sink_index = get_pb_graph_pin_sink_rr(pb_graph_pin, lb_rr_graph);

        //Add to the net
        if (lb_net_iter->terminals.size() == atom_ctx.nlist.net_pins(net_id).size()
            && atom_ctx.nlist.pin_type(pin_id) == PinType::SINK
            && lb_net_iter->external_sink_index != OPEN) {
            //This is the last sink of the net to be added to this cluster. 
            //As a result we can overwrite the existing external sink,
            //since the net is now fully absorbed within the cluster.
            lb_net_iter->terminals[lb_net_iter->external_sink_index] = rr_sink_index;
            lb_net_iter->atom_pins[lb_net_iter->external_sink_index] = pin_id;
            lb_net_iter->external_sink_index = OPEN; //No longer any external sink
        } else {
            //Not last sink, or no external sink to overwrite
            lb_net_iter->terminals.push_back(rr_sink_index);
            lb_net_iter->atom_pins.push_back(pin_id);
        }
    }

    //
    //Handle external net connections
    //

    //Add external driver/sink if needed
    update_required_external_connections(*lb_net_iter, *router_data);

    VTR_ASSERT_SAFE(check_lb_net(*lb_net_iter, lb_rr_graph));
}

//Adds the external source/sink to an intera_lb_net if required
static void update_required_external_connections(t_intra_lb_net& net, t_lb_router_data& router_data) {
	const t_lb_type_rr_graph& lb_rr_graph = *router_data.lb_type_graph;

    //External Driver
    if (net.terminals[t_intra_lb_net::DRIVER_INDEX] == OPEN) {
        //There is no driver set, we must add an external driver
        // NOTE: If this pin where an internal driver, the driver would 
        // already have been already set to the internal source

        add_lb_external_driver(net, router_data);
    }

    //External Sink
    if (driver_is_internal(net, lb_rr_graph)) {
        //We only (potentially) need an external sink if the driver is internal to this cluster.
        //
        // If the driver is external, which ever cluster contains the driver is responsible for making it
        // accessible.

        if (net.external_sink_index == OPEN) {
            //There is no potential external sink set, we must add an external sink
            add_lb_external_sink(net, router_data);
        }
    }
}

static std::vector<t_intra_lb_net>::iterator find_lb_net(std::vector<t_intra_lb_net>& lb_nets, const AtomNetId atom_net) {

    return std::find_if(lb_nets.begin(), lb_nets.end(), 
                [&](const t_intra_lb_net& lb_net) {
                    return lb_net.atom_net_id == atom_net;
                });
}

//Returns an iterator to the lb net corresponding to the specified atom_net
// Will create the lb net if it doesn't already exist
static std::vector<t_intra_lb_net>::iterator find_create_lb_net(std::vector<t_intra_lb_net>& lb_nets, const AtomNetId atom_net) {
    auto lb_net_iter = find_lb_net(lb_nets, atom_net); 
    if (lb_net_iter == lb_nets.end()) {
        //Not found, create it
        
        lb_nets.emplace_back();
        lb_net_iter = lb_nets.end() - 1;

        lb_net_iter->atom_net_id = atom_net;
    }

    return lb_net_iter;
}

//Returns the RR node assoicated with the given pb_graph source pin
static int get_pb_graph_pin_source_rr(const t_pb_graph_pin* pb_graph_pin, const t_lb_type_rr_graph& lb_rr_graph) {
    int rr_source_index = pb_graph_pin->pin_count_in_cluster;
    VTR_ASSERT(lb_rr_graph.nodes[rr_source_index].type == LB_SOURCE);

    return rr_source_index;
}

//Returns the RR node assoicated with the given pb_graph sink pin
static int get_pb_graph_pin_sink_rr(const t_pb_graph_pin* pb_graph_pin, const t_lb_type_rr_graph& lb_rr_graph) {
    //Get the rr node index associated with the pin
    int rr_pin_index = pb_graph_pin->pin_count_in_cluster;
    VTR_ASSERT(lb_rr_graph.nodes[rr_pin_index].num_modes() == 1);
    VTR_ASSERT(lb_rr_graph.nodes[rr_pin_index].num_fanout(0) == 1);
    
    //We actually route to the sink (to handle logically equivalent pins).
    //The sink is one edge past the primitive input pin.
    int rr_sink_index = lb_rr_graph.nodes[rr_pin_index].outedges[0][0].node_index;
    VTR_ASSERT(lb_rr_graph.nodes[rr_sink_index].type == LB_SINK);

    return rr_sink_index;
}

//Returns true if the driver of net is internal to the cluster
static bool driver_is_internal(const t_intra_lb_net& net, const t_lb_type_rr_graph& lb_rr_graph) {
    int driver_rr = net.terminals[t_intra_lb_net::DRIVER_INDEX];
    VTR_ASSERT(lb_rr_graph.nodes[driver_rr].type == LB_SOURCE);
    return !lb_rr_graph.is_external_src_sink_node(driver_rr);
}

//Returns true if the specified sink RR is internal to the cluster
static bool sink_is_internal(int sink_rr, const t_lb_type_rr_graph& lb_rr_graph) {
    VTR_ASSERT(lb_rr_graph.nodes[sink_rr].type == LB_SINK);
    return !lb_rr_graph.is_external_src_sink_node(sink_rr);
}

//Adds a cluster-external driver to the given net
static void add_lb_external_driver(t_intra_lb_net& net, const t_lb_router_data& router_data) {
    constexpr int DRIVER_IDX = t_intra_lb_net::DRIVER_INDEX;
    auto& atom_ctx = g_vpr_ctx.atom();

    auto& lb_rr_graph = *router_data.lb_type_graph;
    auto& lb_rr_graph_info = *router_data.lb_type_graph_info;

    VTR_ASSERT(!net.atom_pins[DRIVER_IDX]);

    net.terminals[DRIVER_IDX] = get_lb_rr_graph_ext_source(net, lb_rr_graph, lb_rr_graph_info);
    net.atom_pins[DRIVER_IDX] = atom_ctx.nlist.net_driver(net.atom_net_id);

    VTR_ASSERT_MSG(lb_rr_graph.nodes[net.terminals[DRIVER_IDX]].type == LB_SOURCE, "Driver RR must be a source");
}

//Adds a cluster-external sink to the given net
static void add_lb_external_sink(t_intra_lb_net& net, const t_lb_router_data& router_data) {
    auto& atom_ctx = g_vpr_ctx.atom();

    auto& lb_rr_graph = *router_data.lb_type_graph;
    auto& lb_rr_graph_info = *router_data.lb_type_graph_info;

    VTR_ASSERT_MSG(net.terminals.size() - 1 < atom_ctx.nlist.net_pins(net.atom_net_id).size(), "There should be net sinks outside of cluster");

    //Find a valid external sink node
    int external_sink_rr = get_lb_rr_graph_ext_sink(net, lb_rr_graph, lb_rr_graph_info);
    if (external_sink_rr == OPEN) {
        return;
    }
    VTR_ASSERT(external_sink_rr != OPEN);

    //Note where the external sink is located within the cluster net
    VTR_ASSERT_MSG(net.external_sink_index == OPEN, "Should only be a single external sink per net");
    net.external_sink_index = net.terminals.size();

    //Find the external sink
    net.terminals.push_back(external_sink_rr);
    net.atom_pins.push_back(AtomPinId::INVALID()); //Doesn't represent a single pin, but the set of external sinks

    //Post-conditions
    VTR_ASSERT(net.external_sink_index != OPEN);
    VTR_ASSERT(!net.atom_pins[net.external_sink_index]);
    VTR_ASSERT(net.terminals[net.external_sink_index] == external_sink_rr);
}

/* Given a pin of a net, remove route tree terminals from it 
*/
static void remove_pin_from_rt_terminals(t_lb_router_data *router_data, const AtomPinId pin_id) {
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& lb_nets = *router_data->intra_lb_nets;
    auto& lb_rr_graph = *router_data->lb_type_graph;

    const t_pb_graph_pin* pb_graph_pin = find_pb_graph_pin(atom_ctx.nlist, atom_ctx.lookup, pin_id);
    VTR_ASSERT(pb_graph_pin);

    AtomNetId net_id = atom_ctx.nlist.pin_net(pin_id);
	if(!net_id) {
        //No net connected to this pin, so nothing to route
		return;
	}

    constexpr int DRIVER_IDX = t_intra_lb_net::DRIVER_INDEX;

    auto lb_net_iter = find_lb_net(lb_nets, net_id);
    VTR_ASSERT_MSG(lb_net_iter != lb_nets.end(), "Net must already exist");


	VTR_ASSERT(lb_net_iter->atom_net_id == net_id);
    VTR_ASSERT(lb_net_iter->atom_pins.size() == lb_net_iter->terminals.size());

    //
    //Remove the internal source/sink for the atom pin
    //
    if (atom_ctx.nlist.pin_type(pin_id) == PinType::DRIVER) {
        //This primitive pin is the net driver, clear the net's driver
        VTR_ASSERT_MSG(lb_net_iter->terminals.size() >= 1, "LB Net must at least have space for driver");

		lb_net_iter->terminals[DRIVER_IDX] = OPEN;
		lb_net_iter->atom_pins[DRIVER_IDX] = AtomPinId::INVALID();

    } else {
        //This primitive pin is a net sink, mark it invalid
        VTR_ASSERT(atom_ctx.nlist.pin_type(pin_id) == PinType::SINK);

        //Add to the net
        auto pin_iter = std::find(lb_net_iter->atom_pins.begin(), lb_net_iter->atom_pins.end(), pin_id);

        int pin_idx = std::distance(lb_net_iter->atom_pins.begin(), pin_iter);

        if (lb_net_iter->external_sink_index != OPEN && pin_idx == lb_net_iter->external_sink_index) {
            //Mark invalid
            lb_net_iter->terminals[pin_idx] = OPEN;
            lb_net_iter->atom_pins[pin_idx] = AtomPinId::INVALID();
        } else {
            //Remove
            lb_net_iter->terminals.erase(lb_net_iter->terminals.begin() + pin_idx);
            lb_net_iter->atom_pins.erase(lb_net_iter->atom_pins.begin() + pin_idx);
        }
    }

    //
    //Handle external net connections
    //

    if ((lb_net_iter->terminals[DRIVER_IDX] == OPEN || !driver_is_internal(*lb_net_iter, lb_rr_graph)) 
        && (lb_net_iter->external_sink_index == OPEN || !sink_is_internal(lb_net_iter->terminals[lb_net_iter->external_sink_index], lb_rr_graph))
        && (lb_net_iter->terminals.size() == 2)) {
        //The driver and external sink are either OPEN (i.e. removed) or external RR nodes,
        //and there are no other internal sinks
        //
        //This means the net has been completely removed from this cluster (no internal driver, 
        //no internal sinks).
        //
        //We remove it from the set of nets to be routed in this cluster
        lb_nets.erase(lb_net_iter); 
    } else {
        //Add external driver/sink if needed
        update_required_external_connections(*lb_net_iter, *router_data);
    }

    VTR_ASSERT_SAFE(check_lb_net(*lb_net_iter, lb_rr_graph));
}

static bool check_lb_net(const t_intra_lb_net& lb_net, const t_lb_type_rr_graph& lb_rr_graph) {
    //Sanity checks on for net consistency

    auto& atom_ctx = g_vpr_ctx.atom();
    int num_extern_sources = 0;
    int num_extern_sinks = 0;
    for (size_t iterm = 0; iterm < lb_net.terminals.size(); ++iterm) {
        int inode = lb_net.terminals[iterm];
        VTR_ASSERT(inode != OPEN);

        AtomPinId atom_pin = lb_net.atom_pins[iterm];
        if (iterm == t_intra_lb_net::DRIVER_INDEX) {
            //Net driver
            VTR_ASSERT_MSG(lb_rr_graph.nodes[inode].type == LB_SOURCE, "Driver must be a source RR node");
            VTR_ASSERT_MSG(atom_pin, "Driver have an assoicated atom pin");
            VTR_ASSERT_MSG(atom_ctx.nlist.pin_type(atom_pin) == PinType::DRIVER, "Source RR must be associated with a driver pin in atom netlist");
            if (lb_rr_graph.is_external_src_sink_node(inode)) {
                ++num_extern_sources;
            }
        } else {
            //Net sink
            VTR_ASSERT_MSG(lb_rr_graph.nodes[inode].type == LB_SINK, "Non-driver must be a sink");

            if (lb_rr_graph.is_external_src_sink_node(inode)) {
                //External sink may have multiple potentially matching atom pins, so it's atom pin is left invalid
                VTR_ASSERT_MSG(!atom_pin, "Cluster external sink should have no valid atom pin");
                ++num_extern_sinks;
            } else {
                VTR_ASSERT_MSG(atom_pin, "Intra-cluster sink must have an assoicated atom pin");
                VTR_ASSERT_MSG(atom_ctx.nlist.pin_type(atom_pin) == PinType::SINK, "Intra-cluster Sink RR must be associated with a sink pin in atom netlist");
            }
        }
    }
    VTR_ASSERT_MSG(num_extern_sinks >= 0 && num_extern_sinks <= 1, "Net must have at most one external sink");
    VTR_ASSERT_MSG(num_extern_sources >= 0 && num_extern_sources <= 1, "Net must have at most one external source");

    return true;
}

//Sanity check that there are no invalid routing targets
bool check_routing_targets(const t_lb_router_data& router_data) {
    auto& lb_rr_graph = *router_data.lb_type_graph;
    for (auto& lb_net : *router_data.intra_lb_nets) {
        
        VTR_ASSERT(lb_net.atom_net_id);
        VTR_ASSERT(lb_net.terminals.size() == lb_net.atom_pins.size());

        for (size_t iterm = 0; iterm < lb_net.terminals.size(); ++iterm) {
            int inode = lb_net.terminals[iterm];
            VTR_ASSERT(inode != OPEN);
            VTR_ASSERT(inode >= 0);
            VTR_ASSERT(inode < (int) lb_rr_graph.nodes.size());

            auto& node = lb_rr_graph.nodes[inode];
            if (iterm == t_intra_lb_net::DRIVER_INDEX) {
                VTR_ASSERT(node.type == LB_SOURCE);
            } else {
                VTR_ASSERT(node.type == LB_SINK);
            }
        }
    }
    return true;
}

//It is possible that a net may connect multiple times to a logically equivalent set of primitive pins.
//The cluster router will only route one connection for a particular net to the common sink of the
//equivalent pins.
//
//To work around this, we fix all but one of these duplicate connections to route to specific pins,
//(instead of the common sink). This ensures a legal routing is produced and that the duplicate pins
//are not 'missing' in the clustered netlist.
static void fix_duplicate_equivalent_pins(t_lb_router_data *router_data) {
    auto& atom_ctx = g_vpr_ctx.atom();

	const t_lb_type_rr_graph& lb_type_graph = *router_data->lb_type_graph;
	vector <t_intra_lb_net> & lb_nets = *router_data->intra_lb_nets;

    for(size_t ilb_net = 0; ilb_net < lb_nets.size(); ++ilb_net) {

        //Collect all the sink terminals indicies which target a particular node
        std::map<int,std::vector<int>> duplicate_terminals;
        for(size_t iterm = 1; iterm < lb_nets[ilb_net].terminals.size(); ++iterm) {
            int node = lb_nets[ilb_net].terminals[iterm];

            duplicate_terminals[node].push_back(iterm);
        }

        for(auto kv : duplicate_terminals) {
            if(kv.second.size() < 2) continue; //Only process duplicates

            //Remap all the duplicate terminals so they target the pin instead of the sink
            for(size_t idup_term = 0; idup_term < kv.second.size(); ++idup_term) {
                int iterm = kv.second[idup_term]; //The index in terminals which is duplicated

                VTR_ASSERT(lb_nets[ilb_net].atom_pins.size() == lb_nets[ilb_net].terminals.size());
                AtomPinId atom_pin = lb_nets[ilb_net].atom_pins[iterm];
                VTR_ASSERT(atom_pin);

                const t_pb_graph_pin* pb_graph_pin = find_pb_graph_pin(atom_ctx.nlist, atom_ctx.lookup, atom_pin);
                VTR_ASSERT(pb_graph_pin);

                if(!pb_graph_pin->port->equivalent) continue; //Only need to remap equivalent ports

                //Remap this terminal to an explicit pin instead of the common sink
                int pin_index = pb_graph_pin->pin_count_in_cluster;

                vtr::printf_warning(__FILE__, __LINE__, 
                            "Found duplicate nets connected to logically equivalent pins. "
                            "Remapping intra lb net %d (atom net %zu '%s') from common sink "
                            "pb_route %d to fixed pin pb_route %d\n",
                            ilb_net, size_t(lb_nets[ilb_net].atom_net_id), atom_ctx.nlist.net_name(lb_nets[ilb_net].atom_net_id).c_str(),
                            kv.first, pin_index);

                VTR_ASSERT(lb_type_graph.nodes[pin_index].type == LB_INTERMEDIATE);
                VTR_ASSERT(lb_type_graph.nodes[pin_index].num_fanout(0) == 1);
                int sink_index = lb_type_graph.nodes[pin_index].outedges[0][0].node_index;
                VTR_ASSERT(lb_type_graph.nodes[sink_index].type == LB_SINK);
                VTR_ASSERT_MSG(sink_index == lb_nets[ilb_net].terminals[iterm], "Remapped pin must be connected to original sink");


                //Change the target
                lb_nets[ilb_net].terminals[iterm] = pin_index;
            }
        }
    }
}

/* Commit or remove route tree from currently routed solution */
static void commit_remove_rt(t_lb_trace *rt, t_lb_router_data *router_data, e_commit_remove op) {
	t_lb_rr_node_stats *lb_rr_node_stats;
	t_explored_node_tb *explored_node_tb;
	const t_lb_type_rr_graph& lb_type_graph = *router_data->lb_type_graph;
	int inode;
	int incr;

	lb_rr_node_stats = router_data->lb_rr_node_stats;
	explored_node_tb = router_data->explored_node_tb;

	if(rt == nullptr) {
		return;
	}

	inode = rt->current_node;

	/* Determine if node is being used or removed */
	if (op == RT_COMMIT) {
		incr = 1;
		if (lb_rr_node_stats[inode].occ >= lb_type_graph.nodes[inode].capacity) {
			lb_rr_node_stats[inode].historical_usage += (lb_rr_node_stats[inode].occ - lb_type_graph.nodes[inode].capacity + 1); /* store historical overuse */
		}		
	} else {
		incr = -1;
		explored_node_tb[inode].inet = OPEN;
	}

	lb_rr_node_stats[inode].occ += incr;
	VTR_ASSERT(lb_rr_node_stats[inode].occ >= 0);

	/* Recursively update route tree */
	for(unsigned int i = 0; i < rt->next_nodes.size(); i++) {
		commit_remove_rt(&rt->next_nodes[i], router_data, op);
	}
}

/* Should net be skipped?  If the net does not conflict with another net, then skip routing this net */
static bool is_skip_route_net(t_lb_trace *rt, t_lb_router_data *router_data) {
	t_lb_rr_node_stats *lb_rr_node_stats;
	const t_lb_type_rr_graph& lb_type_graph = *router_data->lb_type_graph;
	int inode;
	
	lb_rr_node_stats = router_data->lb_rr_node_stats;
	
	if (rt == nullptr) {
		return false; /* Net is not routed, therefore must route net */
	}

	inode = rt->current_node;

	/* Determine if node is overused */
	if (lb_rr_node_stats[inode].occ > lb_type_graph.nodes[inode].capacity) {
		/* Conflict between this net and another net at this node, reroute net */
		return false;
	}

	/* Recursively check that rest of route tree does not have a conflict */
	for (unsigned int i = 0; i < rt->next_nodes.size(); i++) {
		if (is_skip_route_net(&rt->next_nodes[i], router_data) == false) {
			return false;
		}
	}

	/* No conflict, this net's current route is legal, skip routing this net */
	return true;
}


/* At source mode as starting point to existing route tree */
static void add_source_to_rt(t_lb_router_data *router_data, int inet) {
	VTR_ASSERT((*router_data->intra_lb_nets)[inet].rt_tree == nullptr);
	(*router_data->intra_lb_nets)[inet].rt_tree = new t_lb_trace;
	(*router_data->intra_lb_nets)[inet].rt_tree->current_node = (*router_data->intra_lb_nets)[inet].terminals[0];
}

/* Expand all nodes found in route tree into priority queue */
static void expand_rt(t_lb_router_data *router_data, int inet, 
	reservable_pq<t_expansion_node, vector <t_expansion_node>, compare_expansion_node> &pq, int irt_net) {

	vector<t_intra_lb_net> &lb_nets = *router_data->intra_lb_nets;
	
	VTR_ASSERT(pq.empty());

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
	const t_lb_type_rr_graph& lb_type_graph = *router_data->lb_type_graph;
	t_lb_rr_node_stats *lb_rr_node_stats = router_data->lb_rr_node_stats;

	cur_node = exp_node.node_index;
	cur_cost = exp_node.cost;
	mode = lb_rr_node_stats[cur_node].mode;
	t_lb_router_params params = router_data->params;


	for(int iedge = 0; iedge < lb_type_graph.nodes[cur_node].num_fanout(mode); iedge++) {
		int next_mode;

		/* Init new expansion node */
		enode.prev_index = cur_node;
		enode.node_index = lb_type_graph.nodes[cur_node].outedges[mode][iedge].node_index;
		enode.cost = cur_cost;

		/* Determine incremental cost of using expansion node */
		usage = lb_rr_node_stats[enode.node_index].occ + 1 - lb_type_graph.nodes[enode.node_index].capacity;
		incr_cost = lb_type_graph.nodes[enode.node_index].intrinsic_cost;
		incr_cost += lb_type_graph.nodes[cur_node].outedges[mode][iedge].intrinsic_cost;
		incr_cost += params.hist_fac * lb_rr_node_stats[enode.node_index].historical_usage;
		if(usage > 0) {
			incr_cost *= (usage * router_data->pres_con_fac);
		}		
				
		/* Adjust cost so that higher fanout nets prefer higher fanout routing nodes while lower fanout nets prefer lower fanout routing nodes */
		float fanout_factor = 1.0;
		next_mode = lb_rr_node_stats[enode.node_index].mode;
        VTR_ASSERT(next_mode >= 0);
		if (lb_type_graph.nodes[enode.node_index].num_fanout(next_mode) > 1) {
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
		VTR_ASSERT(rt_index != OPEN);
	}

	/* Find rt_index on the route tree */
	link_node = find_node_in_rt(rt, rt_index);
	VTR_ASSERT(link_node != nullptr);

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
static bool is_route_success(t_lb_router_data *router_data) {
	const t_lb_type_rr_graph& lb_type_graph = *router_data->lb_type_graph;

	for(unsigned int inode = 0; inode < lb_type_graph.nodes.size(); inode++) {
		if(router_data->lb_rr_node_stats[inode].occ > lb_type_graph.nodes[inode].capacity) {
			return false;
		}
	}

	return true;
}

/* Given a route tree and an index of a node on the route tree, return a pointer to the trace corresponding to that index */
static t_lb_trace *find_node_in_rt(t_lb_trace *rt, int rt_index) {
	t_lb_trace *cur;
	if(rt->current_node == rt_index) {
		return rt;
	} else {
		for(unsigned int i = 0; i < rt->next_nodes.size(); i++) {
			cur = find_node_in_rt(&rt->next_nodes[i], rt_index);
			if(cur != nullptr) {
				return cur;
			}
		}
	}
	return nullptr;
}

#ifdef PRINT_INTRA_LB_ROUTE
/* Debug routine, print out current intra logic block route */
static void print_route(const char *filename, t_lb_router_data *router_data) {
	FILE *fp;
	vector <t_intra_lb_net> & lb_nets = *router_data->intra_lb_nets;
	vector <t_lb_type_rr_node> & lb_type_graph = *router_data->lb_type_graph;

	fp = fopen(filename, "w");
	
	for(unsigned int inode = 0; inode < lb_type_graph.nodes.size(); inode++) {
		fprintf(fp, "node %d occ %d cap %d\n", inode, router_data->lb_rr_node_stats[inode].occ, lb_type_graph.nodes[inode].capacity);
	}

	fprintf(fp, "\n\n----------------------------------------------------\n\n");

    auto& atom_ctx = g_vpr_ctx.atom();

	for(unsigned int inet = 0; inet < lb_nets.size(); inet++) {
		AtomNetId net_id = lb_nets[inet].atom_net_id;
		fprintf(fp, "net %s num targets %d \n", atom_ctx.nlist.net_name(net_id).c_str(), (int)lb_nets[inet].terminals.size());
		print_trace(fp, lb_nets[inet].rt_tree);
		fprintf(fp, "\n\n");
	}
	fclose(fp);
}

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
#endif

static void reset_explored_node_tb(t_lb_router_data *router_data) {
	const t_lb_type_rr_graph& lb_type_graph = *router_data->lb_type_graph;
	for(unsigned int inode = 0; inode < lb_type_graph.nodes.size(); inode++) {
		router_data->explored_node_tb[inode].prev_index = OPEN;
		router_data->explored_node_tb[inode].explored_id = OPEN;
		router_data->explored_node_tb[inode].inet = OPEN;
		router_data->explored_node_tb[inode].enqueue_id = OPEN;
		router_data->explored_node_tb[inode].enqueue_cost = 0;
	}
}



/* Save last successful intra-logic block route and reset current traceback */
static void save_and_reset_lb_route(t_lb_router_data *router_data) {
	vector <t_intra_lb_net> & lb_nets = *router_data->intra_lb_nets;

	/* Free old saved lb nets if exist */
	if(router_data->saved_lb_nets != nullptr) {		
		free_intra_lb_nets(router_data->saved_lb_nets);
		router_data->saved_lb_nets = nullptr;
	}

	/* Save current routed solution */
	router_data->saved_lb_nets = new vector<t_intra_lb_net> (lb_nets.size());
	vector <t_intra_lb_net> & saved_lb_nets = *router_data->saved_lb_nets;

	for(int inet = 0; inet < (int)saved_lb_nets.size(); inet++) {
		/* 
		  Save and reset route tree data
		*/
		saved_lb_nets[inet].atom_net_id = lb_nets[inet].atom_net_id;
		saved_lb_nets[inet].terminals.resize(lb_nets[inet].terminals.size());
		for(int iterm = 0; iterm < (int) lb_nets[inet].terminals.size(); iterm++) {
			saved_lb_nets[inet].terminals[iterm] = lb_nets[inet].terminals[iterm];
		}
		saved_lb_nets[inet].rt_tree = lb_nets[inet].rt_tree;
		lb_nets[inet].rt_tree = nullptr;
	}
}

//Selects the 
int get_lb_rr_graph_ext_source(const t_intra_lb_net& lb_net, const t_lb_type_rr_graph& /*lb_rr_graph*/, const t_lb_type_rr_graph_info& lb_rr_graph_info) {
    //We need to determine the potental (external) sources 
    //which can can reach all the internal sinks
    std::set<int> potential_external_sources;

    std::vector<std::set<int>> external_sources_reaching_sinks;

    //Collect the set of external sources which reach each sink
    for (size_t isink = t_intra_lb_net::POTENTIAL_EXTERNAL_SINK_INDEX; isink < lb_net.terminals.size(); ++isink) {
        int sink_rr = lb_net.terminals[isink];

        if (sink_rr == OPEN) {
            //Since we add pins incrementally, it is possible for the external net sink to not yet have been specified.
            //All other sinks should have specified target rr nodes.
            VTR_ASSERT_MSG(isink == t_intra_lb_net::POTENTIAL_EXTERNAL_SINK_INDEX, "Only potential external sink should be unspecified");
            continue;
        }

        auto& sources_reaching_sink = lb_rr_graph_info.external_sources_reaching(sink_rr);
        external_sources_reaching_sinks.emplace_back(sources_reaching_sink.begin(), sources_reaching_sink.end());
    }

    //We require sources which reaches *all* the sinks (i.e. the intersection 
    //of all sources which reach each sink)
    for (size_t i = 0; i < external_sources_reaching_sinks.size(); ++i) {

        if (i == 0) {
            //Intially, all sources reaching the first valid sink
            potential_external_sources = external_sources_reaching_sinks[i];
        } else {
            //Later, the intersection of the subset of sources reaching all previous sinks, 
            //and those reaching the current sink
            std::set<int> intersection;
            std::set_intersection(potential_external_sources.begin(), potential_external_sources.end(),
                                  external_sources_reaching_sinks[i].begin(), external_sources_reaching_sinks[i].end(),
                                  std::inserter(intersection, intersection.begin()));

            potential_external_sources = intersection;
        }
    }

    //Now that we have a set of valid sources, pick the best one

    //TODO: For now we just pick the first. Should do something better...
    VTR_ASSERT(!potential_external_sources.empty());
    return *potential_external_sources.begin();
}

int get_lb_rr_graph_ext_sink(const t_intra_lb_net& lb_net, const t_lb_type_rr_graph& /*lb_rr_graph*/, const t_lb_type_rr_graph_info& lb_rr_graph_info) {
    int driver_rr = lb_net.terminals[t_intra_lb_net::DRIVER_INDEX];
    VTR_ASSERT(driver_rr != OPEN);

    auto& driver_reachable_external_sinks = lb_rr_graph_info.external_sinks_reachable(driver_rr);

    //Now that we have a set of valid sinks, pick the best one

    if (driver_reachable_external_sinks.empty()) {
        return OPEN; //No external sink reachable, assume will be set by another atom in the cluster
    } else {
        //TODO: For now we just pick the first. Should do something better...
        return *driver_reachable_external_sinks.begin();
    }
}

static std::vector<int> find_congested_rr_nodes(const t_lb_type_rr_graph& lb_type_graph,
                                                const t_lb_rr_node_stats* lb_rr_node_stats) {
    std::vector<int> congested_rr_nodes;
    for (size_t inode = 0; inode < lb_type_graph.nodes.size(); ++inode) {
        const t_lb_type_rr_node& rr_node = lb_type_graph.nodes[inode];
        const t_lb_rr_node_stats& rr_node_stats = lb_rr_node_stats[inode];
        
        if (rr_node_stats.occ > rr_node.capacity) {
            congested_rr_nodes.push_back(inode);
        }
    }

    return congested_rr_nodes;
}

static std::string describe_lb_type_rr_node(int inode,
                                            const t_lb_router_data* router_data) {
    std::string description;

    const t_lb_type_rr_node& rr_node = (*router_data->lb_type_graph).nodes[inode];

    const t_pb_graph_pin* pb_graph_pin = rr_node.pb_graph_pin;

    if (pb_graph_pin) {
        description += "'" + describe_pb_graph_pin(pb_graph_pin) + "'";
        description += " (";
        description += lb_rr_type_str[rr_node.type];
        description += ")";
    } else if (rr_node.type == LB_INTERMEDIATE) {
        if (router_data->lb_type_graph->is_external_node(inode)) {
            description = "cluster-external routing (LB_INTERMEDIATE)";
        } else {
            description = "cluster-internal routing (LB_INTERMEDIATE)";
        }
    } else if (rr_node.type == LB_SOURCE) {
        if (router_data->lb_type_graph->is_external_node(inode)) {
            description = "cluster-external source (LB_SOURCE)";
        } else {
            description = "cluster-internal source (LB_SOURCE)";
        }
    } else if (rr_node.type == LB_SINK) {
        if (router_data->lb_type_graph->is_external_node(inode)) {
            description = "cluster-external sink (LB_SINK)";
        } else {
            description = "cluster-internal sink (LB_SINK accessible via architecture pins: ";

            //To account for equivalent pins multiple pins may route to a single sink.
            //As a result we need to fin all the nodes which connect to this sink in order
            //to give user-friendly pin names
            std::vector<std::string> pin_descriptions;
            std::vector<int> pin_rrs = find_incoming_rr_nodes(inode, router_data);
            for (int pin_rr_idx : pin_rrs) {
                const t_pb_graph_pin* pin_pb_gpin = (*router_data->lb_type_graph).nodes[pin_rr_idx].pb_graph_pin;
                pin_descriptions.push_back(describe_pb_graph_pin(pin_pb_gpin));
            }

            description += vtr::join(pin_descriptions, ", ");
            description += ")";
        }
    } else {
        description = "<unkown lb_type_rr_node>";
    }

    description += " #" + std::to_string(inode);

    return description;
}

static std::vector<int> find_incoming_rr_nodes(int dst_node, const t_lb_router_data* router_data) {
    std::vector<int> incoming_rr_nodes;
    const auto& lb_rr_node_stats = router_data->lb_rr_node_stats;
    const auto& lb_rr_graph = *router_data->lb_type_graph;
    for (size_t inode = 0; inode < lb_rr_graph.nodes.size(); ++inode) {
        const t_lb_type_rr_node& rr_node = lb_rr_graph.nodes[inode];
        int mode = lb_rr_node_stats[inode].mode;

        for (int iedge = 0; iedge < rr_node.num_fanout(mode); ++iedge) {
            const t_lb_type_rr_node_edge& rr_edge = rr_node.outedges[mode][iedge];

            if (rr_edge.node_index == dst_node) {
                //The current node connects to the destination node
                incoming_rr_nodes.push_back(inode);
            }
        }
    }
    return incoming_rr_nodes;
}

static std::string describe_congested_rr_nodes(const std::vector<int>& congested_rr_nodes,
                                               const t_lb_router_data* router_data) {
    std::string description;

    const auto& lb_nets = *router_data->intra_lb_nets;
    const auto& lb_type_graph = *router_data->lb_type_graph;
    const auto& lb_rr_node_stats = router_data->lb_rr_node_stats;

    std::map<AtomNetId,int> atom_to_intra_lb_net;
    std::multimap<size_t,AtomNetId> congested_rr_node_to_nets; //From rr_node to net
    for (unsigned int inet = 0; inet < lb_nets.size(); inet++) {
        AtomNetId atom_net = lb_nets[inet].atom_net_id;
        atom_to_intra_lb_net.emplace(atom_net, inet);
        
        //Walk the traceback to find congested RR nodes for each net
        std::queue<t_lb_trace> q;

        if (lb_nets[inet].rt_tree) {
            q.push(*lb_nets[inet].rt_tree);
        }
        while (!q.empty()) {
            t_lb_trace curr = q.front();
            q.pop();

            for (const t_lb_trace& next_trace : curr.next_nodes) {
                q.push(next_trace);
            }

            int inode = curr.current_node;
            const t_lb_type_rr_node& rr_node = lb_type_graph.nodes[inode];
            const t_lb_rr_node_stats& rr_node_stats = lb_rr_node_stats[inode];
            
            if (rr_node_stats.occ > rr_node.capacity) {
                //Congested
                congested_rr_node_to_nets.insert({inode, atom_net});
            }
        }
    }

    VTR_ASSERT(!congested_rr_node_to_nets.empty());
    VTR_ASSERT(!congested_rr_nodes.empty());
    auto& atom_ctx = g_vpr_ctx.atom();
    for (int inode : congested_rr_nodes) {
        const t_lb_type_rr_node& rr_node = lb_type_graph.nodes[inode];
        const t_lb_rr_node_stats& rr_node_stats = lb_rr_node_stats[inode];
        description += vtr::string_fmt("RR Node %d (%s) is congested (occ: %d > capacity: %d) with the following nets:\n",
                inode,
                describe_lb_type_rr_node(inode, router_data).c_str(),
                rr_node_stats.occ,
                rr_node.capacity);
        auto range = congested_rr_node_to_nets.equal_range(inode);
        for (auto itr = range.first; itr != range.second; ++itr) {
            AtomNetId net = itr->second;
            description += vtr::string_fmt("\tNet: %s\n",
                    atom_ctx.nlist.net_name(net).c_str());

            VTR_ASSERT(atom_to_intra_lb_net.count(net));
            int inet = atom_to_intra_lb_net[net];
            for (size_t iterm = 0; iterm < lb_nets[inet].terminals.size(); ++iterm) {
                if (iterm == t_intra_lb_net::DRIVER_INDEX) {
                    description += vtr::string_fmt("\t\tDriver RR: #%d\n", lb_nets[inet].terminals[iterm]);
                } else {
                    description += vtr::string_fmt("\t\tSink   RR: #%d\n", lb_nets[inet].terminals[iterm]);

                }
            }
        }

    }

    return description;
}

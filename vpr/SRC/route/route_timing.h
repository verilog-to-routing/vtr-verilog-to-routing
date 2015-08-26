#pragma once
#include <unordered_map>
#include <vector>

int get_max_pins_per_net(void);

/***************** Incremental connection based rerouting **********************/
// lookup and persistent scratch-space resources used for incremental reroute through
// pruning the route tree of large fanouts. Instead of rerouting to each sink of a congested net,
// reroute only the connections to the ones that did not have a legal connection the previous time
class Incremental_reroute_resources {
	// conceptually works like rr_sink_node_to_pin[net_index][sink_node_index] to get the pin index for that net
	// each net maps SINK node index -> PIN index for net
	// only need to be built once at the start since the SINK nodes never change
	// the reverse lookup of net_rr_terminals
	std::vector<std::unordered_map<int,int>> rr_sink_node_to_pin;

	// a property of each net, but only valid after pruning the previous route tree
	// the "targets" in question can be either node indices or pin indices, the
	// conversion from node to pin being performed by this class
	std::vector<int> remaining_targets;
	std::vector<t_rt_node*> reached_sinks;	// used to populate rt_node_of_sink after building route tree from traceback

public:
	Incremental_reroute_resources() {

		/* Initialize the persistent data structures for incremental rerouting
		 * this includes rr_sink_node_to_pin, which provides pin lookup given a
		 * sink node for a specific net.
		 *
		 * remaining_targets will reserve enough space to ensure it won't need
		 * to grow while storing the sinks that still need routing after pruning
		 *
		 * reached_sinks will also reserve enough space, but instead of
		 * indices, it will store the pointers to route tree nodes */

		// can have as many targets as sink pins (total number of pins - SOURCE pin)
		// supposed to be used as persistent vector growing with push_back and clearing at the start of each net routing iteration
		auto max_sink_pins_per_net = get_max_pins_per_net() - 1;
		remaining_targets.reserve(max_sink_pins_per_net);
		reached_sinks.reserve(max_sink_pins_per_net);
		// an array of hash tables from terminal node ---> pin on net
		// supposed to be used as a constant lookup table, to only be used with const operator[] after initialization
		size_t local_num_nets {g_clbs_nlist.net.size()};
		rr_sink_node_to_pin.resize(local_num_nets);
		for (unsigned int inet = 0; inet < local_num_nets; ++inet) {
			// unordered_map<int,int> net_node_to_pin;
			auto& net_node_to_pin = rr_sink_node_to_pin[inet];

			unsigned int num_pins = g_clbs_nlist.net[inet].pins.size();
			net_node_to_pin.reserve(num_pins - 1);	// not looking up on the SOURCE pin

			for (unsigned int ipin = 1; ipin < num_pins; ++ipin) {
				net_node_to_pin.insert({net_rr_terminals[inet][ipin], ipin});
			}
		}
	}

	// get a handle on the resources
	std::vector<int>& get_remaining_targets() {return remaining_targets;}
	std::vector<t_rt_node*>& get_reached_sinks() {return reached_sinks;}

	void convert_sink_nodes_to_net_pins(unsigned inet, vector<int>& sink_nodes) const {

		/* Turn a vector of node indices, assumed to be of sinks for a net *
		 * into the pin indices of the same net. */

		const auto& node_to_pin_mapping = rr_sink_node_to_pin[inet];

		for (size_t s = 0; s < sink_nodes.size(); ++s) {

			auto mapping = node_to_pin_mapping.find(sink_nodes[s]);
			// should always expect it find a pin mapping for its own net
			INCREMENTAL_REROUTING_ASSERT(mapping != node_to_pin_mapping.end());

			sink_nodes[s] = mapping->second;
		}
	}

	void put_sink_rt_nodes_in_net_pins_lookup(unsigned inet, const vector<t_rt_node*>& sink_rt_nodes,
	 t_rt_node** rt_node_of_sink) const {

		/* Load rt_node_of_sink (which maps a PIN index to a route tree node)
		 * with a vector of route tree sink nodes. */

		// a net specific mapping from node index to pin index
		const auto& node_to_pin_mapping = rr_sink_node_to_pin[inet];

		for (t_rt_node* rt_node : sink_rt_nodes) {
			auto mapping = node_to_pin_mapping.find(rt_node->inode);
			// element should be able to find itself
			INCREMENTAL_REROUTING_ASSERT(mapping != node_to_pin_mapping.end());

			rt_node_of_sink[mapping->second] = rt_node;
		}
	}

#ifdef DEBUG_INCREMENTAL_REROUTING
	bool sanity_check_lookup() {
		for (unsigned int inet = 0; inet < g_clbs_nlist.net.size(); ++inet) {
			const auto& net_node_to_pin = rr_sink_node_to_pin[inet];

			for (auto mapping : net_node_to_pin) {
				auto sanity = net_node_to_pin.find(mapping.first);
				if (sanity == net_node_to_pin.end()) {
					vpr_printf_info("%d cannot find itself (net %d)\n", mapping.first, inet);
					return false;
				}
				assert(net_rr_terminals[inet][mapping.second] == mapping.first);
			}		
		}	
		return true;
	}
#endif	
};

bool try_timing_driven_route(struct s_router_opts router_opts,
		float **net_delay, t_slack * slacks, t_ivec ** clb_opins_used_locally,
        bool timing_analysis_enabled, const t_timing_inf &timing_inf);
bool try_timing_driven_route_net(int inet, int itry, float pres_fac, 
		struct s_router_opts router_opts,
		Incremental_reroute_resources& incremental_reroute_res,
		float* pin_criticality, 
		t_rt_node** rt_node_of_sink, float** net_delay, t_slack* slacks);

bool timing_driven_route_net(int inet, int itry, float pres_fac, float max_criticality,
		float criticality_exp, float astar_fac, float bend_cost,
		Incremental_reroute_resources& incremental_reroute_res,
		float *pin_criticality, int min_incremental_reroute_fanout, t_rt_node ** rt_node_of_sink, 
		float *net_delay, t_slack * slacks);

/*
 * NOTE:
 * Suggest using a timing_driven_route_structs struct (below) . Memory is managed for you.
 */
void alloc_timing_driven_route_structs(float **pin_criticality_ptr,
		int **sink_order_ptr, t_rt_node *** rt_node_of_sink_ptr);
void free_timing_driven_route_structs(float *pin_criticality, int *sink_order,
		t_rt_node ** rt_node_of_sink);

struct timing_driven_route_structs {
	float* pin_criticality;
	int* sink_order;
	t_rt_node** rt_node_of_sink;

	timing_driven_route_structs();
	~timing_driven_route_structs();
};

// profiling functions
#ifdef PROFILE
void time_on_fanout_analysis();
#endif
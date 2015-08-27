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
	// the "targets" in question can be either rr_node indices or pin indices, the
	// conversion from node to pin being performed by this class
	std::vector<int> remaining_targets;

	// contains rt_nodes representing sinks reached legally while pruning the route tree
	// used to populate rt_node_of_sink after building route tree from traceback
	// order does not matter
	std::vector<t_rt_node*> reached_sinks;	

public:
	Incremental_reroute_resources();

	// get a handle on the resources
	std::vector<int>& get_remaining_targets() {return remaining_targets;}
	std::vector<t_rt_node*>& get_reached_sinks() {return reached_sinks;}

	void convert_sink_nodes_to_net_pins(unsigned inet, vector<int>& rr_sink_nodes) const;

	void put_sink_rt_nodes_in_net_pins_lookup(unsigned inet, const vector<t_rt_node*>& sink_rt_nodes,
	 t_rt_node** rt_node_of_sink) const;

	bool sanity_check_lookup() const;
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


void alloc_timing_driven_route_structs(float **pin_criticality_ptr,
		int **sink_order_ptr, t_rt_node *** rt_node_of_sink_ptr);
void free_timing_driven_route_structs(float *pin_criticality, int *sink_order,
		t_rt_node ** rt_node_of_sink);

struct timing_driven_route_structs {
	// data while timing driven route is active 
	float* pin_criticality; /* [1..max_pins_per_net-1] */
	int* sink_order; /* [1..max_pins_per_net-1] */
	t_rt_node** rt_node_of_sink; /* [1..max_pins_per_net-1] */

	timing_driven_route_structs();
	~timing_driven_route_structs();
};

#include <vector>
#include "globals.h"
#include "vpr_types.h"
#include "route_profiling.h"

namespace profiling {

using namespace std;

#ifdef PROFILE
constexpr unsigned int fanout_per_bin = 4;
constexpr float criticality_per_bin = 0.05;

// data structures indexed by fanout bin (ex. fanout of x is in bin x/fanout_per_bin)
static vector<float> time_on_fanout;
static vector<float> time_on_fanout_rebuild;
static vector<float> time_on_criticality;
static vector<int> itry_on_fanout;
static vector<int> itry_on_criticality;
static vector<int> rerouted_sinks;
static vector<int> finished_sinks;
#endif

// action counters for what setup routing resources did
#ifdef PROFILE
static int entire_net_rerouted;
void net_rerouted() {++entire_net_rerouted;}

static int entire_tree_pruned;
void route_tree_pruned() {++entire_tree_pruned;}

static int part_tree_preserved;
void route_tree_preserved() {++part_tree_preserved;}

static int connections_forced_to_reroute;
void mark_for_forced_reroute() {++connections_forced_to_reroute;}
static int connections_rerouted_due_to_forcing;
void perform_forced_reroute() {++connections_rerouted_due_to_forcing;}

#else
void net_rerouted() {}
void route_tree_pruned() {}
void route_tree_preserved() {}
void mark_for_forced_reroute() {}
void perform_forced_reroute() {}
#endif


// timing functions where *_start starts a clock and *_end terminates the clock
#ifdef PROFILE
static clock_t sink_criticality_clock;
void sink_criticality_start() {sink_criticality_clock = clock();}

void sink_criticality_end(float target_criticality) {
	if (!time_on_criticality.empty()) {
		time_on_criticality[target_criticality / criticality_per_bin] += static_cast<float>(clock() - sink_criticality_clock) / CLOCKS_PER_SEC;
		++itry_on_criticality[target_criticality / criticality_per_bin];
	}	
}

static clock_t net_rebuild_clock;
void net_rebuild_start() {net_rebuild_clock = clock();}

void net_rebuild_end(unsigned net_fanout, unsigned sinks_left_to_route) {
	unsigned int bin {net_fanout / fanout_per_bin};
	unsigned int sinks_already_routed = net_fanout - sinks_left_to_route;
	float rebuild_time {static_cast<float>(clock() - net_rebuild_clock) / CLOCKS_PER_SEC};

	finished_sinks[bin] += sinks_already_routed;
	rerouted_sinks[bin] += sinks_left_to_route;
	time_on_fanout_rebuild[bin] += rebuild_time;
}

static clock_t net_fanout_clock;
void net_fanout_start() {
	net_fanout_clock = clock();
}

void net_fanout_end(unsigned net_fanout) {
	float time_for_net = static_cast<float>(clock() - net_fanout_clock) / CLOCKS_PER_SEC;
	time_on_fanout[net_fanout / fanout_per_bin] += time_for_net;
	itry_on_fanout[net_fanout / fanout_per_bin] += 1;
}
#else
void sink_criticality_start() {}
void sink_criticality_end(float) {}

void net_rebuild_start() {}
void net_rebuild_end(unsigned, unsigned) {}

void net_fanout_start() {}
void net_fanout_end(unsigned) {}
#endif

void time_on_fanout_analysis() {
#ifdef PROFILE
	vpr_printf_info("%d entire net rerouted, %d entire trees pruned (route to each sink from scratch), %d partially rerouted\n", 
		entire_net_rerouted, entire_tree_pruned, part_tree_preserved);
	vpr_printf_info("%d connections marked for forced reroute, %d forced reroutes performed\n", connections_forced_to_reroute, connections_rerouted_due_to_forcing);
	// using the global time_on_fanout and itry_on_fanout
	vpr_printf_info("fanout low      time (s)        attemps  rebuild tree time (s)   finished sinks   rerouted sinks\n");
	for (size_t bin = 0; bin < time_on_fanout.size(); ++bin) {
		if (itry_on_fanout[bin]) {	// avoid printing the many 0 bins
			vpr_printf_info("%4d      %14.3f   %12d     %14.3f   %12d  %12d\n",
				bin*fanout_per_bin, 
				time_on_fanout[bin], 
				itry_on_fanout[bin], 
				time_on_fanout_rebuild[bin],
				finished_sinks[bin],
				rerouted_sinks[bin]);
		}
		// clear the non-cumulative values
		finished_sinks[bin] = 0;
		rerouted_sinks[bin] = 0;
	}

	connections_forced_to_reroute = connections_rerouted_due_to_forcing = 0;
#endif
	return;
}

void time_on_criticality_analysis() {
#ifdef PROFILE
	vpr_printf_info("criticality low           time (s)        attemps\n");
	for (size_t bin = 0; bin < time_on_criticality.size(); ++bin) {
		if (itry_on_criticality[bin]) {	// avoid printing the many 0 bins
			vpr_printf_info("%4f           %14.3f   %12d\n",bin*criticality_per_bin, time_on_criticality[bin], itry_on_criticality[bin]);
		}
	}	
#endif
	return;
}

// at the end of a routing iteration, profile how much congestion is taken up by each type of rr_node
#ifdef PROFILE
// efficient bit array for checking against congested type
struct Congested_node_types {
	uint32_t mask;
	Congested_node_types() : mask{0} {}
	void set_congested(int rr_node_type) {mask |= (1 << rr_node_type);}
	void clear_congested(int rr_node_type) {mask &= ~(1 << rr_node_type);}
	bool is_congested(int rr_node_type) const {return mask & (1 << rr_node_type);}
	bool empty() const {return mask == 0;}
};
#endif

void congestion_analysis() {
#ifdef PROFILE
	// each type indexes into array which holds the congestion for that type
	std::vector<int> congestion_per_type((size_t) NUM_RR_TYPES, 0);
	// print out specific node information if congestion for type is low enough

	int total_congestion = 0;
	for (int inode = 0; inode < num_rr_nodes; ++inode) {
		const t_rr_node& node = rr_node[inode];
		int congestion = node.get_occ() - node.get_capacity();

		if (congestion > 0) {
			total_congestion += congestion;
			congestion_per_type[node.type] += congestion;
		}
	}

	constexpr int specific_node_print_threshold = 5;
	Congested_node_types congested;
	for (int type = SOURCE; type < NUM_RR_TYPES; ++type) {
		float congestion_percentage = (float)congestion_per_type[type] / (float) total_congestion * 100;
		vpr_printf_info(" %6s: %10.6f %\n", node_typename[type], congestion_percentage); 
		// nodes of that type need specific printing
		if (congestion_per_type[type] > 0 &&
			congestion_per_type[type] < specific_node_print_threshold) congested.set_congested(type);
	}

	// specific print out each congested node
	if (!congested.empty()) {
		vpr_printf_info("Specific congested nodes\nxlow ylow   type\n");
		for (int inode = 0; inode < num_rr_nodes; ++inode) {
			const t_rr_node& node = rr_node[inode];
			if (congested.is_congested(node.type) && (node.get_occ() - node.get_capacity()) > 0) {
				vpr_printf_info("(%3d,%3d) %6s\n", node.get_xlow(), node.get_ylow(), node_typename[node.type]);
			}
		}
	}
#endif
	return;
}


void profiling_initialization(unsigned max_fanout) {
#ifdef PROFILE
	// add 1 so that indexing on the max fanout would still be valid
	time_on_fanout.resize((max_fanout / fanout_per_bin) + 1, 0);
	itry_on_fanout.resize((max_fanout / fanout_per_bin) + 1, 0);
	time_on_fanout_rebuild.resize((max_fanout / fanout_per_bin) + 1, 0);
	rerouted_sinks.resize((max_fanout / fanout_per_bin) + 1, 0);
	finished_sinks.resize((max_fanout / fanout_per_bin) + 1, 0);
	time_on_criticality.resize((1 / criticality_per_bin) + 1, 0);
	itry_on_criticality.resize((1 / criticality_per_bin) + 1, 0);
	entire_net_rerouted = 0;
	entire_tree_pruned = 0;
	part_tree_preserved = 0;
	connections_forced_to_reroute = 0;
	connections_rerouted_due_to_forcing = 0;
#endif
	return;
}

void report_total_route_time() {
#ifdef PROFILE
    // sum up all the time spent on all the different fanouts
    double total_route_time = 0;
	for (size_t bin = 0; bin < time_on_fanout.size(); ++bin) {
	    total_route_time += time_on_fanout[bin];
    }
    vpr_printf_info("Total route time (s): %14.3f\n", total_route_time);
#endif
    return;
}


}	// end namespace profiling

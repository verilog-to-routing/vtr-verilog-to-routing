#include <vector>
#include "globals.h"
#include "vpr_types.h"
#include "route_profiling.h"

namespace profiling {

#ifndef PROFILE
void net_rerouted() {}
void route_tree_pruned() {}
void route_tree_preserved() {}
void mark_for_forced_reroute() {}
void perform_forced_reroute() {}

// timing functions where *_start starts a clock and *_end terminates the clock
void sink_criticality_start() {}
void sink_criticality_end(float /*target_criticality*/) {}

void net_rebuild_start() {}
void net_rebuild_end(unsigned /*net_fanout*/, unsigned /*sinks_left_to_route*/) {}

void net_fanout_start() {}
void net_fanout_end(unsigned /*net_fanout*/) {}

// analysis functions for printing out the profiling data for an iteration
void congestion_analysis() {}
void time_on_criticality_analysis() {}
void time_on_fanout_analysis() {}

void profiling_initialization(unsigned /*max_net_fanout*/) {}

#else
using namespace std;

constexpr unsigned int fanout_per_bin = 1;
constexpr float criticality_per_bin = 0.05;

// data structures indexed by fanout bin (ex. fanout of x is in bin x/fanout_per_bin)
static vector<float> time_on_fanout;
static vector<float> time_on_fanout_rebuild;
static vector<float> time_on_criticality;
static vector<int> itry_on_fanout;
static vector<int> itry_on_criticality;
static vector<int> rerouted_sinks;
static vector<int> finished_sinks;

// action counters for what setup routing resources did
static int entire_net_rerouted;
void net_rerouted() { ++entire_net_rerouted; }

static int entire_tree_pruned;
void route_tree_pruned() { ++entire_tree_pruned; }

static int part_tree_preserved;
void route_tree_preserved() { ++part_tree_preserved; }

static int connections_forced_to_reroute;
void mark_for_forced_reroute() { ++connections_forced_to_reroute; }
static int connections_rerouted_due_to_forcing;
void perform_forced_reroute() { ++connections_rerouted_due_to_forcing; }

// timing functions where *_start starts a clock and *_end terminates the clock
static clock_t sink_criticality_clock;
void sink_criticality_start() { sink_criticality_clock = clock(); }

void sink_criticality_end(float target_criticality) {
    if (!time_on_criticality.empty()) {
        time_on_criticality[target_criticality / criticality_per_bin] += static_cast<float>(clock() - sink_criticality_clock) / CLOCKS_PER_SEC;
        ++itry_on_criticality[target_criticality / criticality_per_bin];
    }
}

static clock_t net_rebuild_clock;
void net_rebuild_start() { net_rebuild_clock = clock(); }

void net_rebuild_end(unsigned net_fanout, unsigned sinks_left_to_route) {
    unsigned int bin{net_fanout / fanout_per_bin};
    unsigned int sinks_already_routed = net_fanout - sinks_left_to_route;
    float rebuild_time{static_cast<float>(clock() - net_rebuild_clock) / CLOCKS_PER_SEC};

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

void time_on_fanout_analysis() {
    VTR_LOG("%d entire net rerouted, %d entire trees pruned (route to each sink from scratch), %d partially rerouted\n",
            entire_net_rerouted, entire_tree_pruned, part_tree_preserved);
    VTR_LOG("%d connections marked for forced reroute, %d forced reroutes performed\n", connections_forced_to_reroute, connections_rerouted_due_to_forcing);
    // using the global time_on_fanout and itry_on_fanout
    VTR_LOG("fanout low      time (s)        attemps  rebuild tree time (s)   finished sinks   rerouted sinks\n");
    for (size_t bin = 0; bin < time_on_fanout.size(); ++bin) {
        if (itry_on_fanout[bin]) { // avoid printing the many 0 bins
            VTR_LOG("%4d      %14.3f   %12d     %14.3f   %12d  %12d\n",
                    bin * fanout_per_bin,
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
    return;
}

void time_on_criticality_analysis() {
    VTR_LOG("criticality low           time (s)        attemps\n");
    for (size_t bin = 0; bin < time_on_criticality.size(); ++bin) {
        if (itry_on_criticality[bin]) { // avoid printing the many 0 bins
            VTR_LOG("%4f           %14.3f   %12d\n", bin * criticality_per_bin, time_on_criticality[bin], itry_on_criticality[bin]);
        }
    }
    return;
}

// at the end of a routing iteration, profile how much congestion is taken up by each type of rr_node
// efficient bit array for checking against congested type
struct Congested_node_types {
    uint32_t mask;
    Congested_node_types()
        : mask{0} {}
    void set_congested(int rr_node_type) { mask |= (1 << rr_node_type); }
    void clear_congested(int rr_node_type) { mask &= ~(1 << rr_node_type); }
    bool is_congested(int rr_node_type) const { return mask & (1 << rr_node_type); }
    bool empty() const { return mask == 0; }
};

void congestion_analysis() {
#    if 0
	// each type indexes into array which holds the congestion for that type
	std::vector<int> congestion_per_type((size_t)NUM_RR_TYPES, 0);
	// print out specific node information if congestion for type is low enough

	int total_congestion = 0;
	for (int inode = 0; inode < device_ctx.rr_nodes.size(); ++inode) {
		const t_rr_node& node = device_ctx.rr_nodes[inode];
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
		VTR_LOG(" %6s: %10.6f %\n", node_typename[type], congestion_percentage);
		// nodes of that type need specific printing
		if (congestion_per_type[type] > 0 &&
			congestion_per_type[type] < specific_node_print_threshold) congested.set_congested(type);
	}

	// specific print out each congested node
	if (!congested.empty()) {
		VTR_LOG("Specific congested nodes\nxlow ylow   type\n");
		for (int inode = 0; inode < device_ctx.rr_nodes.size(); ++inode) {
			const t_rr_node& node = device_ctx.rr_nodes[inode];
			if (congested.is_congested(node.type) && (node.get_occ() - node.get_capacity()) > 0) {
				VTR_LOG("(%3d,%3d) %6s\n", node.get_xlow(), node.get_ylow(), node_typename[node.type]);
			}
		}
	}
	return;
#    endif
}

void profiling_initialization(unsigned max_fanout) {
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
    return;
}
#endif

} // end namespace profiling

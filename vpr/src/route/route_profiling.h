#pragma once
/* optional profiling for developers on the breakdown of time spent and other quantities
 * profiling mostly focuses on per-fanout, but also has per-type (SINK, IPIN, ...) and
 * per-criticality */

//Uncomment to enable profiling
//#define PROFILE

namespace profiling {

// action counters for what actions setting up routing resources took to build targets list
void net_rerouted();
void route_tree_pruned();
void route_tree_preserved();
void mark_for_forced_reroute();
void perform_forced_reroute();

// timing functions where *_start starts a clock and *_end terminates the clock
void sink_criticality_start();
void sink_criticality_end(float target_criticality);

void net_rebuild_start();
void net_rebuild_end(unsigned net_fanout, unsigned sinks_left_to_route);

void net_fanout_start();
void net_fanout_end(unsigned net_fanout);

// analysis functions for printing out the profiling data for an iteration
void congestion_analysis();
void time_on_criticality_analysis();
void time_on_fanout_analysis();

void profiling_initialization(unsigned max_net_fanout);

} // end namespace profiling

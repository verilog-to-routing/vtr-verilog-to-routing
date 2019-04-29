/************ Defines and types shared by all route files ********************/
/* IMPORTANT:
 * The following preprocessing flags are added to 
 * avoid compilation error when this headers are included in more than 1 times 
 */
#ifndef ROUTER_PROFILING_H
#define ROUTER_PROFILING_H

/* optional profiling for developers on the breakdown of time spent and other quantities
   profiling mostly focuses on per-fanout, but also has per-type (SINK, IPIN, ...) and
   per-criticality */

//Uncomment to enable profiling
//#define PROFILE
//
/*
 * Notes in include header files in a head file 
 * Only include the neccessary header files 
 * that is required by the data types in the function/class declarations!
 */
/* Header files should be included in a sequence */
/* Standard header files required go first */

/* EXTERNAL library header files go second*/

/* VPR header files go second*/
/* use a namespace for the functions */
/* Namespace declaration */
/* All the routers should be placed in the namespace of router
 * A specific router may have it own namespace under the router namespace
 * To call a function in a router, function need a prefix of router::<your_router_namespace>::
 * This will ease the development in modularization.
 * The concern is to minimize/avoid conflicts between data type names as much as possible
 * Also, this will keep function names as short as possible.
 */
namespace router { 


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

} /* end of namespace router */

#endif 

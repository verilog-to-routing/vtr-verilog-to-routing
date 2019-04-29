/* IMPORTANT:
 * The following preprocessing flags are added to 
 * avoid compilation error when this headers are included in more than 1 times 
 */
#ifndef TIMING_DRIVEN_ROUTE_TREE_TIMING_H
#define TIMING_DRIVEN_ROUTE_TREE_TIMING_H

/*
 * Notes in include header files in a head file 
 * Only include the neccessary header files 
 * that is required by the data types in the function/class declarations!
 */
/* Header files should be included in a sequence */
/* Standard header files required go first */
#include <vector>

#include "timing_driven_route_tree_type.h"
#include "timing_driven_connection_based_routing.h"
#include "router_common.h"
#include "timing_driven_spatial_route_tree_lookup.h"

/* Namespace declaration */
/* All the routers should be placed in the namespace of router
 * A specific router may have it own namespace under the router namespace
 * To call a function in a router, function need a prefix of router::<your_router_namespace>::
 * This will ease the development in modularization.
 * The concern is to minimize/avoid conflicts between data type names as much as possible
 * Also, this will keep function names as short as possible.
 */
namespace router { 

namespace timing_driven { 

/**************** Subroutines exported by route_tree_timing.c ***************/
//Returns true if allocated
bool alloc_route_tree_timing_structs(bool exists_ok = false);

void free_route_tree_timing_structs();

t_rt_node *init_route_tree_to_source(ClusterNetId inet);

void free_route_tree(t_rt_node * rt_node);
void print_route_tree(const t_rt_node* rt_node, int depth=0);

t_rt_node *update_route_tree(t_heap *hptr, SpatialRouteTreeLookup* spatial_rt_lookup);

void update_net_delays_from_route_tree(float *net_delay,
		const t_rt_node* const * rt_node_of_sink, ClusterNetId inet);
void update_remaining_net_delays_from_route_tree(float* net_delay,
		const t_rt_node* const * rt_node_of_sink, const std::vector<int>& remaining_sinks);

void load_route_tree_Tdel(t_rt_node* rt_root, float Tarrival);
void load_route_tree_rr_route_inf(t_rt_node* root);

t_rt_node* init_route_tree_to_source_no_net(int inode);

void add_route_tree_to_rr_node_lookup(t_rt_node* node);

bool verify_route_tree(t_rt_node* root);
bool verify_traceback_route_tree_equivalent(const t_trace* trace_head, const t_rt_node* rt_root);

/********** Incremental reroute ***********/
// instead of ripping up a net that has some congestion, cut the branches
// that don't legally lead to a sink and start routing with that partial route tree

void print_edge(const t_linked_rt_edge* edge);
void print_route_tree(const t_rt_node* rt_root);
void print_route_tree_inf(const t_rt_node* rt_root);
void print_route_tree_congestion(const t_rt_node* rt_root);

t_rt_node* traceback_to_route_tree(ClusterNetId inet);
t_trace* traceback_from_route_tree(ClusterNetId inet, const t_rt_node* root, int num_remaining_sinks);

t_rt_node* prune_route_tree(t_rt_node* rt_root, CBRR& connections_inf);

void pathfinder_update_cost_from_route_tree(const t_rt_node* rt_root, int add_or_sub, float pres_fac);

bool is_equivalent_route_tree(const t_rt_node* rt_root, const t_rt_node* cloned_rt_root);
bool is_valid_skeleton_tree(const t_rt_node* rt_root);
bool is_valid_route_tree(const t_rt_node* rt_root);
bool is_uncongested_route_tree(const t_rt_node* root);
float load_new_subtree_C_downstream(t_rt_node* rt_node);
void load_new_subtree_R_upstream(t_rt_node* rt_node);

} /* end of namespace timing_driven */

} /* end of namespace router */

#endif

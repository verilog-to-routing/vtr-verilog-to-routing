#pragma once
#include <vector>
#include "route_tree_type.h"
#include "connection_based_routing.h"


/**************** Subroutines exported by route_tree_timing.c ***************/

void alloc_route_tree_timing_structs(void);

void free_route_tree_timing_structs(void);

t_rt_node *init_route_tree_to_source(int inet);

void free_route_tree(t_rt_node * rt_node);

t_rt_node *update_route_tree(struct s_heap *hptr);

void update_net_delays_from_route_tree(float *net_delay,
		const t_rt_node* const * rt_node_of_sink, int inet);
void update_remaining_net_delays_from_route_tree(float* net_delay, 
		const t_rt_node* const * rt_node_of_sink, const std::vector<int>& remaining_sinks);

void load_route_tree_Tdel(t_rt_node* rt_root, float Tarrival);
void load_route_tree_rr_route_inf(t_rt_node* root);	


/********** Incremental reroute ***********/
// instead of ripping up a net that has some congestion, cut the branches
// that don't legally lead to a sink and start routing with that partial route tree

void print_edge(const t_linked_rt_edge* edge);
void print_route_tree(const t_rt_node* rt_root);
void print_route_tree_inf(const t_rt_node* rt_root);
void print_route_tree_congestion(const t_rt_node* rt_root);

t_rt_node* traceback_to_route_tree(int inet);
t_trace* traceback_from_route_tree(int inet, const t_rt_node* root, int num_remaining_sinks);

bool prune_route_tree(t_rt_node* rt_root, float pres_fac, CBRR& connections_inf); 	// 0 R_upstream for SOURCE

void pathfinder_update_cost_from_route_tree(const t_rt_node* rt_root, int add_or_sub, float pres_fac);

bool is_equivalent_route_tree(const t_rt_node* rt_root, const t_rt_node* cloned_rt_root);
bool is_valid_skeleton_tree(const t_rt_node* rt_root);
bool is_valid_route_tree(const t_rt_node* rt_root);
bool is_uncongested_route_tree(const t_rt_node* root);
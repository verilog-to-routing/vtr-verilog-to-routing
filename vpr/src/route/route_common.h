/************ Defines and types shared by all route files ********************/
#pragma once
#include <vector>
#include "clustered_netlist.h"
#include "vtr_vector.h"
#include "bucket.h"

/******* Subroutines in route_common used only by other router modules ******/

vtr::vector<ClusterNetId, t_bb> load_route_bb(int bb_factor);

t_bb load_net_route_bb(ClusterNetId net_id, int bb_factor);

void pathfinder_update_path_cost(t_trace* route_segment_start,
                                 int add_or_sub,
                                 float pres_fac);
void pathfinder_update_single_node_cost(int inode, int add_or_sub, float pres_fac);

void pathfinder_update_cost(float pres_fac, float acc_fac);

t_trace* update_traceback(t_heap* hptr, ClusterNetId net_id);

void reset_path_costs(const std::vector<int>& visited_rr_nodes);

float get_rr_cong_cost(int inode);

void mark_ends(ClusterNetId net_id);
void mark_remaining_ends(const std::vector<int>& remaining_sinks);

void node_to_heap(int inode, float cost, int prev_node, int prev_edge, float backward_path_cost, float R_upstream);

void free_traceback(ClusterNetId net_id);
void free_traceback(t_trace* tptr);

void add_to_mod_list(int inode, std::vector<int>& modified_rr_node_inf);

void invalidate_heap_entries(int sink_node, int ipin_node);

void init_route_structs(int bb_factor);

void alloc_and_load_rr_node_route_structs();

void reset_rr_node_route_structs();

void free_trace_structs();

void reserve_locally_used_opins(float pres_fac, float acc_fac, bool rip_up_local_opins);

void free_chunk_memory_trace();

bool validate_traceback(t_trace* trace);
void print_traceback(ClusterNetId net_id);
void print_traceback(const t_trace* trace);

void print_rr_node_route_inf();
void print_rr_node_route_inf_dot();
void print_invalid_routing_info();

t_trace* alloc_trace_data();
void free_trace_data(t_trace* trace);

bool router_needs_lookahead(enum e_router_algorithm router_algorithm);

/************ Defines and types shared by all route files ********************/
#pragma once
#include <vector>
#include "clustered_netlist.h"
#include "vtr_vector.h"
#include "heap_type.h"

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

void free_traceback(ClusterNetId net_id);
void drop_traceback_tail(ClusterNetId net_id);
void free_traceback(t_trace* tptr);

void add_to_mod_list(int inode, std::vector<int>& modified_rr_node_inf);

void init_route_structs(int bb_factor);

void alloc_and_load_rr_node_route_structs();

void reset_rr_node_route_structs();

void free_trace_structs();

void reserve_locally_used_opins(HeapInterface* heap, float pres_fac, float acc_fac, bool rip_up_local_opins);

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

std::string describe_unrouteable_connection(const int source_node, const int sink_node);

/* Creates a new t_heap object to be placed on the heap, if the new cost    *
 * given is lower than the current path_cost to this channel segment.  The  *
 * index of its predecessor is stored to make traceback easy.  The index of *
 * the edge used to get from its predecessor to it is also stored to make   *
 * timing analysis, etc.  The backward_path_cost and R_upstream values are  *
 * used only by the timing-driven router -- the breadth-first router        *
 * ignores them.                                                            *
 *                                                                          *
 * Returns t_heap suitable for adding to heap or nullptr if node is more    *
 * expensive than previously explored path.                                 */
template<typename T, typename RouteInf>
t_heap* prepare_to_add_node_to_heap(
    T* heap,
    const RouteInf& rr_node_route_inf,
    int inode,
    float total_cost,
    int prev_node,
    int prev_edge,
    float backward_path_cost,
    float R_upstream) {
    if (total_cost >= rr_node_route_inf[inode].path_cost)
        return nullptr;

    t_heap* hptr = heap->alloc();

    hptr->index = inode;
    hptr->cost = total_cost;
    hptr->u.prev.node = prev_node;
    hptr->u.prev.edge = prev_edge;
    hptr->backward_path_cost = backward_path_cost;
    hptr->R_upstream = R_upstream;
    return hptr;
}

/* Puts an rr_node on the heap if it is the cheapest path.    */
template<typename T, typename RouteInf>
void add_node_to_heap(
    T* heap,
    const RouteInf& rr_node_route_inf,
    int inode,
    float total_cost,
    int prev_node,
    int prev_edge,
    float backward_path_cost,
    float R_upstream) {
    t_heap* hptr = prepare_to_add_node_to_heap(
        heap,
        rr_node_route_inf, inode, total_cost, prev_node,
        prev_edge, backward_path_cost, R_upstream);
    if (hptr) {
        heap->add_to_heap(hptr);
    }
}

/* Puts an rr_node on the heap with the same condition as add_node_to_heap,
 * but do not fix heap property yet as that is more efficiently done from
 * bottom up with build_heap    */
template<typename T, typename RouteInf>
void push_back_node(
    T* heap,
    const RouteInf& rr_node_route_inf,
    int inode,
    float total_cost,
    int prev_node,
    int prev_edge,
    float backward_path_cost,
    float R_upstream) {
    t_heap* hptr = prepare_to_add_node_to_heap(
        heap,
        rr_node_route_inf, inode, total_cost, prev_node, prev_edge,
        backward_path_cost, R_upstream);
    if (hptr) {
        heap->push_back(hptr);
    }
}

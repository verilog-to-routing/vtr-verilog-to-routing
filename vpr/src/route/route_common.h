/************ Defines and types shared by all route files ********************/
#pragma once
#include <vector>
#include "clustered_netlist.h"
#include "vtr_vector.h"
#include "heap_type.h"
#include "rr_node_fwd.h"
#include "router_stats.h"
#include "globals.h"

/******* Subroutines in route_common used only by other router modules ******/

vtr::vector<ClusterNetId, t_bb> load_route_bb(int bb_factor);

t_bb load_net_route_bb(ClusterNetId net_id, int bb_factor);

void pathfinder_update_path_occupancy(t_trace* route_segment_start, int add_or_sub);

void pathfinder_update_single_node_occupancy(int inode, int add_or_sub);

void pathfinder_update_acc_cost_and_overuse_info(float acc_fac, OveruseInfo& overuse_info);

float update_pres_fac(float new_pres_fac);

/* Pass in the hptr starting at a SINK with target_net_pin_index, which is the net pin index corresonding *
 * to the sink (ranging from 1 to fanout). Returns a pointer to the first "new" node in the traceback     *
 * (node not previously in trace).                                                                        */
t_trace* update_traceback(t_heap* hptr, int target_net_pin_index, ClusterNetId net_id);

void reset_path_costs(const std::vector<int>& visited_rr_nodes);

float get_rr_cong_cost(int inode, float pres_fac);

/* Returns the base cost of using this rr_node */
inline float get_single_rr_cong_base_cost(int inode) {
    auto& device_ctx = g_vpr_ctx.device();
    auto cost_index = device_ctx.rr_graph.node_cost_index(RRNodeId(inode));

    return device_ctx.rr_indexed_data[cost_index].base_cost;
}

/* Returns the accumulated congestion cost of using this rr_node */
inline float get_single_rr_cong_acc_cost(int inode) {
    auto& route_ctx = g_vpr_ctx.routing();

    return route_ctx.rr_node_route_inf[inode].acc_cost;
}

/* Returns the present congestion cost of using this rr_node */
inline float get_single_rr_cong_pres_cost(int inode, float pres_fac) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();

    int occ = route_ctx.rr_node_route_inf[inode].occ();
    int capacity = rr_graph.node_capacity(RRNodeId(inode));

    if (occ >= capacity) {
        return (1. + pres_fac * (occ + 1 - capacity));
    } else {
        return 1.;
    }
}

/* Returns the congestion cost of using this rr_node,
 * *ignoring* non-configurable edges */
inline float get_single_rr_cong_cost(int inode, float pres_fac) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();

    float pres_cost;
    int overuse = route_ctx.rr_node_route_inf[inode].occ() - rr_graph.node_capacity(RRNodeId(inode));

    if (overuse >= 0) {
        pres_cost = (1. + pres_fac * (overuse + 1));
    } else {
        pres_cost = 1.;
    }

    auto cost_index = rr_graph.node_cost_index(RRNodeId(inode));

    float cost = device_ctx.rr_indexed_data[cost_index].base_cost * route_ctx.rr_node_route_inf[inode].acc_cost * pres_cost;

    VTR_ASSERT_DEBUG_MSG(
        cost == get_single_rr_cong_base_cost(inode) * get_single_rr_cong_acc_cost(inode) * get_single_rr_cong_pres_cost(inode, pres_fac),
        "Single rr node congestion cost is inaccurate");

    return cost;
}

void mark_ends(ClusterNetId net_id);
void mark_remaining_ends(ClusterNetId net_id, const std::vector<int>& remaining_sinks);

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
    RREdgeId prev_edge,
    float backward_path_cost,
    float R_upstream) {
    if (total_cost >= rr_node_route_inf[inode].path_cost)
        return nullptr;

    t_heap* hptr = heap->alloc();

    hptr->index = inode;
    hptr->cost = total_cost;
    hptr->set_prev_node(prev_node);
    hptr->set_prev_edge(prev_edge);
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
    RREdgeId prev_edge,
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
    RREdgeId prev_edge,
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

/* Puts an rr_node on the heap with the same condition as node_to_heap,
 * but do not fix heap property yet as that is more efficiently done from
 * bottom up with build_heap. Certain information is also added     */
template<typename T>
void push_back_node_with_info(
    T* heap,
    int inode,
    float total_cost,
    float backward_path_cost,
    float R_upstream,
    float backward_path_delay,
    PathManager* rcv_path_manager) {
    t_heap* hptr = heap->alloc();
    rcv_path_manager->alloc_path_struct(hptr->path_data);

    hptr->index = inode;
    hptr->cost = total_cost;
    hptr->backward_path_cost = backward_path_cost;
    hptr->R_upstream = R_upstream;

    hptr->path_data->backward_delay = backward_path_delay;

    heap->push_back(hptr);
}
